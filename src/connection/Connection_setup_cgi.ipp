#pragma once
#include "Connection.hpp"

ATTR(static_inl)
void s_exec_script(char* const argv[3], char** envp, int fdIn[2], int fdOut[2], char* cwdPath) {
	bool fail = dup2(fdOut[1], STDOUT_FILENO) == -1;
	fail = fail || dup2(fdIn[0], STDIN_FILENO) == -1;

	close(fdOut[0]), close(fdOut[1]);
	close(fdIn[0]), close(fdIn[1]);
	if (fail || chdir(*cwdPath == '\0' ? "/" : cwdPath) == -1) {	// HOTFIX
		close(STDOUT_FILENO), close(STDIN_FILENO);
		_exit(1);
	}

	execve(argv[0], argv, envp);
	if (errno != ENOENT && errno != ENOTDIR)
		_exit(126);
	_exit(127);
}

ATTR(static_inl)
char* s_split_filename(char* cwdPath, usize length) {
	char* slashPtr = NULL;
	char* end = cwdPath + length;

	*end = '/';
	while (true) {
		while (*cwdPath != '/')
			cwdPath++;
		if (cwdPath >= end)
			break;
		slashPtr = cwdPath++;
	}
	ASSERT(slashPtr != NULL, "Script path did not contain a slash");
	*slashPtr = 0;
	*end = 0;
	return slashPtr + 1;
}

/*
	Doing this before forking avoids Copy on Write. Fake env is good for that!
	REQUEST_METHOD=POST, SCRIPT_NAME=/cgi/test.py
	QUERY_STRING=a=1, CONTENT_LENGTH=42, CONTENT_TYPE=application/x-www-form-urlencoded
	HTTP_HOST=example.com:8080, HTTP_COOKIE=session=xyz
*/
// TODO: Review and write what it is supposed to do
CONNECTION_INL
(char*) append_env(Buffer64& buffer, char* argv[3]) {
	static const char requestMethod[3][24] =
		{"REQUEST_METHOD=GET", "REQUEST_METHOD=POST", "REQUEST_METHOD=DELETE"};
	const usize methodIndex = (options & 7) / 2;

	Environment::reset();
	Environment::append((char*)requestMethod[methodIndex]);
	Environment::append(buffer.append("SCRIPT_NAME=").ptr);		// SCRIPTNAME=
	buffer.append(req.target.ptr, req.target.size + 1);	// SCRIPTNAME=/images/cgi/process.py
	const Span scriptPath = buffer.append_path_resolved(req.root, req.relativeTarget);	// /home/webserv/www/images/cgi/process.py
	char* cwdPath = buffer.append(scriptPath.ptr, scriptPath.size + 1).ptr;			// /home/webserv/www/images/cgi
	argv[0] = buffer.append(req.interpreter.ptr, req.interpreter.size + 1).ptr;		// /bin/python3
	argv[1] = s_split_filename(cwdPath, scriptPath.size);							// process.py
	argv[2] = NULL;
	if (options & Options::FIXED_LENGTH) {
		Environment::append(buffer.append("CONTENT_LENGTH=").ptr);
		buffer.append_digit10(bodySize);	// Review: if null terminator doesnt exist
		buffer.append("\0");
	}
	if (req.contentTypeHeader.size != 0) {
		Environment::append(buffer.append("CONTENT_TYPE=").ptr);
		buffer.append(req.contentTypeHeader);
		buffer.append("\0");
	}
	Environment::append(buffer.append("HTTP_HOST=").ptr);
	buffer.append(req.host.ptr, req.host.size + 1);
	Environment::append(LITPREPEND(req.query.ptr, "QUERY_STRING="));
	if (req.cookies.size != 0)
		Environment::append(LITPREPEND(req.cookies.ptr, "HTTP_COOKIE="));
	return cwdPath;
}

CONNECTION_INL
(isize) cgi_setup(Epoll& epoll) {
	Buffer64 pathBuffer = {};
	char* chdirPath;
	char* argv[3];
	int fdIn[2], fdOut[2];
	Mode nextMode = Mode::CGI;
	pid_t pid;
	if (options & Options::POST)
		nextMode = (options & Options::FIXED_LENGTH) ? Mode::CGI_FIXED : Mode::CGI_CHUNKED;

	chdirPath = append_env(pathBuffer, argv);
	if (pipe2(fdIn, O_CLOEXEC) == -1)
		goto Error;
	if (pipe2(fdOut, O_CLOEXEC) == -1)
		goto ErrorCloseInput;
	if (fcntl(fdIn[1], F_SETFL, O_NONBLOCK) == -1 || fcntl(fdOut[0], F_SETFL, O_NONBLOCK) == -1)
		goto ErrorCloseOutput;
	pid = fork();
	if (pid < 0)
		goto ErrorCloseOutput;
	if (pid == 0)
		s_exec_script(argv, Environment::envp, fdIn, fdOut, chdirPath);
	close(fdIn[0]);
	close(fdOut[1]);
	readFd = fdOut[0];
	writeFd = fdIn[1];
	processId = pid;
	activate_streaming(nextMode);
	sendBuffer.init(256, 256, 256);	// Leave room for the HTTP status and connection headers
	if (nextMode == Mode::CGI_FIXED)
		return cgi_fixed(epoll);
	if (nextMode == Mode::CGI_CHUNKED)
		return cgi_chunked(epoll);
	return switch_to_cgi(epoll);

	ErrorCloseOutput:	close(fdOut[0]), close(fdOut[1]);
	ErrorCloseInput:	close(fdIn[0]), close(fdIn[1]);
	Error:				return flush_setup_close(epoll, s_get_status());
}

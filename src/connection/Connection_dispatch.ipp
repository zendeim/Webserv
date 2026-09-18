#pragma once
#include "Connection.hpp"

/*
	A mode is the state that the connection is in. It's an exclusive variable, not a bitfield
	The connection starts in parse mode. When it is done parsing, it calls:

	Setup configures each mode and calls the execution of a method.
	When a method is done (GET, POST, CGI, AUTOINDEX) or when a non fatal error happens, it enters Flush mode

	Flush mode writes all the remaining bytes in sendBuffer, then either closes or goes back to parsing mode
	Whether it closes or not depends on if there was an error, or if it specified a keep-alive option
*/

CONNECTION_INL
(isize) dispatch(Epoll& epoll) {
	if (Clock::time_elapsed() - startTime > HTTP_TIMEOUT)
		return flush_setup_close(epoll, Status::i504);

	switch (mode) {
		case Mode::PARSE_FIRST:		return parse_first(epoll);
		case Mode::PARSE:			return parse(epoll);
		case Mode::GET:				return upload_file(epoll);
		case Mode::POST_FIXED:		return download_file_fixed(epoll);
		case Mode::POST_CHUNKED:	return download_file_chunked(epoll);
		case Mode::FLUSH:			return flush(epoll);
		case Mode::CGI:				return cgi(epoll);
		case Mode::CGI_FIXED:		return cgi_fixed(epoll);
		case Mode::CGI_CHUNKED:		return cgi_chunked(epoll);
		case Mode::CGI_PARSED:		return cgi_parsed(epoll);
		case Mode::AUTOINDEX:		return upload_directory(epoll);
		default: return -1;
	}
}

CONNECTION_INL
(isize) parse_first(Epoll& epoll) {
	if (epoll.request_read() && parseBuffer.read(clientFd, ATOMIC_IOSIZE) <= 0)
		return -1;	// REVIEW

	Span line = parseBuffer.find_line_end();
	if (line == NULL) {
		if (parseBuffer.size() > 8000)
			return flush_setup_close(epoll, Status::i431);
		return 0;
	}

	Status::Code code = parse_first_line(line);
	if (code != Status::unset)
		return flush_setup_close(epoll, code);
	mode = Mode::PARSE;
	return parse(epoll);
}

CONNECTION_INL
(isize) parse(Epoll& epoll) {
	if (parseBuffer.writePos >= 16000)
		return flush_setup_close(epoll, Status::i431);
	if (epoll.request_read() && parseBuffer.read(clientFd, ATOMIC_IOSIZE) <= 0)
		return -1;	// REVIEW

	Span line;
	while ((line = parseBuffer.find_line_end()) != NULL) {
		if (line.size == 0) {
			parseBuffer.readPos = parseBuffer.scanPos;
			return setup_dispatch(epoll);
		}
		Status::Code code = parse_line(line);
		if (code != Status::unset)
			return flush_setup_close(epoll, code);
	}
	return 0;
}

CONNECTION_INL
(isize) setup_dispatch(Epoll& epoll) {
	const bool isBodyMethod = options & Options::POST;
	const bool encodingSet = options & (Options::CHUNKED_LENGTH | Options::FIXED_LENGTH);

	if ((options & Options::HOST) == 0)
		return flush_setup_close(epoll, Status::i400);	// Host not set
	if (!isBodyMethod && encodingSet)
		return flush_setup_close(epoll, Status::i400);	// Encoding set for non-body methods
	if (isBodyMethod && !encodingSet)
		return flush_setup_close(epoll, Status::i411);	// Transfer encoding not set

	if (options & Options::CHUNKED_LENGTH)
		bodySize = cfg->maxBodySize;

	startTime = Clock::time_elapsed();	// Resets the clock on a valid response header
	if (Status::is_redirect(req.location->redirectStatus))
		return redirect_setup(epoll, req.location->redirectStatus);
	if (options & Options::CGI)
		return cgi_setup(epoll);
	if (options & Options::GET)
		return get_setup(epoll);
	if (options & Options::POST)
		return post_setup(epoll);
	ASSERT(options & Options::DELETE, "Invalid request method");
	return del_setup(epoll);
}

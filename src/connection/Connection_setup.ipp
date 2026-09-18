#pragma once
#include "Connection.hpp"

/*
	The IO state upon entering setup is EPOLLIN
	* del/post don't need state changes because if necessary, flush handles it
	* cgi gets changed to EPOLLOUT when changing to cgi_parsed
	* get changes to EPOLLOUT
*/

CONNECTION_INL
(isize) del_setup(Epoll& epoll) {
	Buffer64 pathBuffer = {};
	char* path = pathBuffer.append_path_resolved(req.root, req.relativeTarget).ptr;
	if (unlink(path) == -1)
		return flush_setup_close(epoll, s_get_status());
	activate_streaming(Mode::FLUSH);
	build_header(Status::i204);
	return flush_setup(epoll);
}

CONNECTION_INL
(isize) post_setup(Epoll& epoll) {
	Buffer64 pathBuffer = {};
	const Span uploadStore = req.location->get_upload_store();
	pathBuffer.append(uploadStore);
	pathBuffer.append(req.relativeTarget);
	*pathBuffer = 0;

	writeFd = open(pathBuffer, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NONBLOCK, 0644);
	if (writeFd == -1)
		return flush_setup_close(epoll, s_get_status());
	const bool fixedLength = options & Options::FIXED_LENGTH;
	activate_streaming(fixedLength ? Mode::POST_FIXED : Mode::POST_CHUNKED);
	if (fixedLength)
		return download_file_fixed(epoll);
	return download_file_chunked(epoll);
}

CONNECTION_INL
(isize) redirect_setup(Epoll& epoll, Status::Code code) {
	const Span statusStr = Status::get_status_str(code);
	const Span target = req.location->get_redirect_target();
	bodySize = 0;
	options &= ~(u16)Options::KEEP_ALIVE;
	activate_streaming(Mode::FLUSH);
	sendBuffer.append("HTTP/1.1 ");
	sendBuffer.append(statusStr);
	sendBuffer.append("\r\nLocation: ");
	sendBuffer.append(target);
	sendBuffer.append("\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
	return flush_setup(epoll);
}

CONNECTION_INL
(isize) parse_setup(Epoll& epoll) {
	activate_parsing();
	options = 0;
	contentType = Mime::OCTET_STREAM;
	bodySize = 0;
	chunkSize = 0;
	startTime = Clock::time_elapsed();
	if (epoll.modify(clientFd, EPOLLIN, epollState))
		return -1;
	return parse_first(epoll);		// Keep the connection alive until header is flushed
}

CONNECTION_INL
(isize) flush_setup(Epoll& epoll) {
	readFd = fn::close_noerr(readFd);
	writeFd = fn::close_noerr(writeFd);
	mode = Mode::FLUSH;
	isize bytesWritten = write_to_client(epoll);
	if (sendBuffer.size() > 0 && epoll.modify(clientFd, EPOLLOUT, epollState))
		return -1;	// TODO: See if i can't just stream the output then close
	return bytesWritten;
}

// Flush_close only needs to know the Status
CONNECTION_INL
(isize) flush_setup_close(Epoll& epoll, Status::Code code) {
	readFd = fn::close_noerr(readFd);
	writeFd = fn::close_noerr(writeFd);
	options &= ~(u16)Options::KEEP_ALIVE;
	mode = Mode::FLUSH;
	build_error_header(code);	// Already calls sendBuffer.clear()
	if (epoll.modify(clientFd, EPOLLOUT, epollState))
		return -1;
	return write_to_client(epoll);
}

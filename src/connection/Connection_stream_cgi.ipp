#pragma once
#include "Connection.hpp"

CONNECTION_INL
(isize) switch_to_cgi(Epoll& epoll) {
	writeFd = fn::close_noerr(writeFd);
	mode = Mode::CGI;
	if (epoll.modify(clientFd, EPOLLOUT, epollState))
		return -1;
	return 0;
}

CONNECTION_INL
(isize) cgi_chunked(Epoll& epoll) {
	const isize code = read_chunked(epoll);
	if (code < 0)
		return -1;
	if (Status::is_error((Status::Code)code))
		return flush_setup_close(epoll, (Status::Code)code);
	const usize bytesToWrite = recvBuffer.scanPos - recvBuffer.readPos;
	if (bytesToWrite != 0 && recvBuffer.write_all(writeFd, bytesToWrite) < 0)
		return flush_setup_close(epoll, Status::i500);
	if (code == Status::ok) {
		bodySize = 0;
		return switch_to_cgi(epoll);
	}
	return 0;
}

CONNECTION_INL
(isize) cgi_fixed(Epoll& epoll) {
	if (read_from_client(epoll) < 0)
		return -1;
	isize bytesWritten = 0;
	if (bodySize != 0 && recvBuffer.size() != 0) {
		bytesWritten = recvBuffer.write_all(writeFd, bodySize);
		if (bytesWritten < 0)
			return flush_setup_close(epoll, Status::i500);
		bodySize -= (usize)bytesWritten;
	}
	if (bodySize == 0)
		return switch_to_cgi(epoll);
	return 0;
}

CONNECTION_INL
(isize) cgi(Epoll& epoll) {
	isize bytesRead = sendBuffer.read(readFd, ATOMIC_IOSIZE);
	if (bytesRead == 0)
		readFd = fn::close_noerr(readFd);
	Span header = sendBuffer.find_cgi_header_end();
	if (header.ptr == NULL) {
		if (sendBuffer.size() > 7500)
			return flush_setup_close(epoll, Status::i500);
		if (bytesRead == -2 || readFd == -1)
			return flush_setup_close(epoll, Status::i500);
		return 0;	// Still no CGI Header
	}
	Status::Code code = build_cgi_header(Status::i200);
	if (code == Status::invalid)
		return flush_setup_close(epoll, Status::i500);
	if (readFd == -1)
		return flush_setup(epoll);
	mode = Mode::CGI_PARSED;
	if (epoll.modify(clientFd, EPOLLOUT, epollState))
		return -1;
	return write_to_client(epoll);
}

CONNECTION_INL
(isize) cgi_parsed(Epoll& epoll) {
	isize bytesRead = sendBuffer.read_compact(readFd, ATOMIC_IOSIZE);
	if (bytesRead == 0)
		return flush_setup(epoll);
	return write_to_client(epoll);
}

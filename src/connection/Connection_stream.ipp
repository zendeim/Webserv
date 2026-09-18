#pragma once
#include "Connection.hpp"

CONNECTION_INL
(isize) write_to_client(Epoll& epoll) {
	if (epoll.request_write())
		return sendBuffer.write(clientFd, ATOMIC_IOSIZE);
	return 0;
}

CONNECTION_INL
(isize) read_from_client(Epoll& epoll) {
	if (!epoll.request_read())
		return 0;
	const isize bytesRead = recvBuffer.read_compact(clientFd, ATOMIC_IOSIZE);
	if (bytesRead == -2)
		return flush_setup_close(epoll, Status::i413);
	return bytesRead == 0 ? -1 : bytesRead;
}

CONNECTION_INL
(isize) read_chunked(Epoll& epoll) {
	HTTP_Buffer tmp = {};
	tmp.append(recvBuffer.sptr(), recvBuffer.writePos - recvBuffer.scanPos);

	const usize maxBytesToRead = recvBuffer.capacity() - recvBuffer.size();
	if (maxBytesToRead != 0 && epoll.request_read()) {
		const isize bytesRead = tmp.read(clientFd, MIN(maxBytesToRead, (usize)ATOMIC_IOSIZE));
		if (bytesRead <= 0)
			return -1;
	}
	recvBuffer.writePos = recvBuffer.scanPos;
	if (recvBuffer.capacity() - recvBuffer.writePos < tmp.size())
		recvBuffer.compact();
	return tmp.dechunk(recvBuffer, chunkSize, bodySize);
}

CONNECTION_INL
(isize) flush(Epoll& epoll) {
	isize bytesWritten = write_to_client(epoll);
	if (sendBuffer.size() > 0)
		return bytesWritten;
	if (options & Options::KEEP_ALIVE)
		return parse_setup(epoll);
	return -1;	// Close the connection
}

CONNECTION_INL
(isize) download_file_fixed(Epoll& epoll) {
	if (read_from_client(epoll) < 0)
		return -1;
	isize bytesWritten = 0;
	if (bodySize != 0 && recvBuffer.size() != 0) {
		bytesWritten = recvBuffer.write_all(writeFd, bodySize);
		if (bytesWritten < 0)
			return flush_setup_close(epoll, Status::i500);
		bodySize -= (usize)bytesWritten;
	}
	if (bodySize == 0) {
		build_header(Status::i201);
		return flush_setup(epoll);
	}
	return 0;
}

CONNECTION_INL
(isize) download_file_chunked(Epoll& epoll) {
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
		build_header(Status::i201);
		return flush_setup(epoll);
	}
	return 0;
}

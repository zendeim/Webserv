#pragma once
#include "Connection.hpp"

/*	CGI output is server-controlled, so this path performs only the inexpensive
	structural checks needed before forwarding the line
*/
CONNECTION_INL
(Status::Code) parse_cgi_line(Buffer64& dst) {
	const char* lineEnd = (char*)sendBuffer.sptr() - 1;
	lineEnd -= lineEnd[-1] == '\r';
	const usize totalLength = (usize)(lineEnd - (char*)sendBuffer.rptr());

	const usize readEnd = sendBuffer.readPos + totalLength;
	Span field = sendBuffer.find_char(':');
	if (field.ptr == NULL)
		return Status::ixxx;

	const Field fieldIndex = fn::match_field(field);
	if (fieldIndex == Field::CONNECTION)
		return Status::ok;
	if (fieldIndex != Field::STATUS) {
		dst.append(field.ptr, totalLength);
		dst.append("\r\n");
		return Status::ok;
	}

	Span value = sendBuffer.get_field_value(readEnd);
	if (value.ptr == NULL)
		return Status::ixxx;	// Rejects empty values

	Status::Code code = Status::s_str_to_code(value.ptr);	// TODO: change the check to be if OK not if error
	return code;
}

CONNECTION_INL
(Status::Code) build_cgi_header(Status::Code code) {
	Buffer64 tmpBuffer = {};
	tmpBuffer.writePos += 256;
	tmpBuffer.readPos += 256;
	tmpBuffer.scanPos += 256;

	const usize headerEnd = sendBuffer.scanPos;
	sendBuffer.scanPos = sendBuffer.readPos;

	while (sendBuffer.readPos < headerEnd) {
		const Span line = sendBuffer.find_cgi_line_end();
		ASSERT(line.ptr != NULL, "Complete CGI header contained an incomplete line");
		if (line.size == 0) {
			sendBuffer.readPos = sendBuffer.scanPos;
			break;
		}
		Status::Code lineCode = parse_cgi_line(tmpBuffer);
		if (lineCode == Status::ixxx)
			return Status::ixxx;
		if (lineCode != Status::ok)
			code = lineCode;
		sendBuffer.readPos = sendBuffer.scanPos;
	}
	ASSERT(sendBuffer.readPos == headerEnd, "CGI header ended at an unexpected offset");

	Span statusStr = Status::get_status_str(code);

	tmpBuffer.append("Connection: close\r\n\r\n");
	tmpBuffer.append(sendBuffer.get_span());
	tmpBuffer.prepend("\r\n");
	tmpBuffer.prepend(statusStr);
	tmpBuffer.prepend("HTTP/1.1 ");
	if (tmpBuffer.size() > sendBuffer.capacity())
		return Status::ixxx;
	sendBuffer.clear();
	sendBuffer.append(tmpBuffer.get_span());
	options &= ~(u16)Options::KEEP_ALIVE;
	return code;
}

CONNECTION_INL
(void) build_header(Status::Code code) {
	Span statusStr = Status::get_status_str(code);

	sendBuffer.append("HTTP/1.1 ");
	sendBuffer.append(statusStr);
	sendBuffer.append("\r\nContent-Type: ");
	sendBuffer.append_mime(contentType);
	sendBuffer.append("\r\nContent-Length: ");
	sendBuffer.append_digit10(bodySize);
	if (options & Options::KEEP_ALIVE)
		sendBuffer.append("\r\nConnection: keep-alive\r\n\r\n");
	else
		sendBuffer.append("\r\nConnection: close\r\n\r\n");
}

// HTTP/1.1 404 Not Found
// Content-Type: text/plain
// Content-Length: 14
// Connection: close
// 404 Not Found

CONNECTION_INL
(void) build_error_header(Status::Code code) {
	Span statusStr = Status::get_status_str(code);
	Span errorPage = cfg->errorPages[Status::s_get_page_index(code)];

	options &= ~(u16)Options::KEEP_ALIVE;
	sendBuffer.clear();
	sendBuffer.append("HTTP/1.1 ");
	sendBuffer.append(statusStr);
	sendBuffer.append("\r\nContent-Type: text/html\r\nContent-Length: ");
	sendBuffer.append_digit10(errorPage.size);
	sendBuffer.append("\r\nConnection: close\r\n\r\n");
	sendBuffer.append(errorPage);
}

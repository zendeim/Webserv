#pragma once
#include "Buffer.hpp"

/*
	chunkSize == 0 means we're reading the chunk header
	bodySize is the remaining allowance specified in the config
	other receives decoded bytes; unprocessed input remains in this buffer
*/

BUFFER_INL
(Status::Code) dechunk(Buffer& other, usize& chunkSize, usize& bodySize) {
	while (writePos - readPos > 2) {
// ==== Reading chunk header ==================================================
		if (chunkSize == 0) {
			if (writePos - readPos < 5)
				break;
			if (LITCMP(data + readPos, "0\r\n\r\n") == 0) {
				readPos += 5;
				scanPos = readPos;
				return Status::ok;
			}
			const Span line = find_line_end();
			if (line.ptr == NULL)
				return (writePos - readPos > 16) ? Status::i400 : Status::unset;
			chunkSize = fn::qstrtol16((char*)data + readPos, line.size);
			if (chunkSize == 0 || chunkSize == SIZE_MAX)
				return Status::i400;
			if (chunkSize > bodySize)
				return Status::i413;
			bodySize -= chunkSize;
			readPos = scanPos;
		}
// ==== Reading chunk body ====================================================
		else {
			const usize appendLength = MIN(chunkSize, writePos - readPos - 2);
			other.append((char*)data + readPos, appendLength);
			readPos += appendLength;
			scanPos = readPos;
			chunkSize -= appendLength;	// Guaranteed to be chunksize or less
			if (chunkSize == 0) {
				if (LITCMP(data + readPos, "\r\n") != 0)
					return Status::i400;
				readPos += 2;
				scanPos = readPos;
			}
		}
	}
	return Status::unset;
}

BUFFER_INL
(Span) get_field_value(usize readEnd) {
	Span result = {};
	while ((data[readPos] == ' ' || data[readPos] == '\t'))
		readPos++;
	if (readPos >= readEnd)
		return result;
	usize valueEnd = readEnd;
	while ((data[valueEnd - 1] == ' ' || data[valueEnd - 1] == '\t'))
		valueEnd--;
	data[valueEnd] = '\0';	//	REVIEW
	result.ptr = (char*)data + readPos;
	result.size = valueEnd - readPos;
	readPos = scanPos;
	return result;
}

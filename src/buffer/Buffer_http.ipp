#pragma once
#include "Buffer.hpp"

/*
	chunkSize == 0 means we're reading the chunk header
	bodySize is the remaining allowance specified in the config
	other receives decoded bytes followed by unprocessed input, with scanPos marking the boundary
*/

BUFFER_INL
(Status::Code) dechunk(Buffer& other, usize& chunkSize, usize& bodySize) {
	ASSERT(other.capacity() - other.writePos >= size(), "Insufficient dechunk destination space");
	Status::Code code = Status::unset;
	while (writePos - readPos > 2) {
// ==== Reading chunk header ==================================================
		if (chunkSize == 0) {
			if (writePos - readPos < 5)
				break;
			if (LITCMP(data + readPos, "0\r\n\r\n") == 0) {
				readPos += 5;
				scanPos = readPos;
				code = Status::ok;
				break;
			}
			const Span line = find_line_end();
			if (line.ptr == NULL) {
				if (writePos - readPos > 16)
					return Status::i400;
				break;
			}
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
	other.scanPos = other.writePos;
	other.append(get_span());
	return code;
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

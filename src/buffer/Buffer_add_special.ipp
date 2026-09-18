#pragma once
#include "Buffer.hpp"

BUFFER_INL
(Span) append_mime(Mime mimeIndex) {
	static const u8 mimeStrings[][32] = MIME_STRINGS;

	const u8* str = mimeStrings[(usize)mimeIndex];
	const usize length = *str;
	char* optr = (char*)data + writePos;
	MEMCPY_INLINE(optr, str + 1, 24);
	writePos += length;
	return {optr, length};
}

BUFFER_INL
(Span) append_digit10(usize number) {
	const usize maxLength = 24;
	char buffer[maxLength * 2];
	Span digit = fn::itoa10(number, buffer, maxLength);

	char* optr = (char*)data + writePos;
	MEMCPY_INLINE(optr, digit.ptr, maxLength);
	writePos += digit.size;
	return {optr, digit.size};
}

BUFFER_INL
(Span) append_digit16(usize number) {
	const usize maxLength = 16;
	char buffer[maxLength * 2];
	Span digit = fn::itoa16(number, buffer, maxLength);

	char* optr = (char*)data + writePos;
	MEMCPY_INLINE(optr, digit.ptr, maxLength);
	writePos += digit.size;
	return {optr, digit.size};
}

BUFFER_INL
(Span) append_path_resolved(Span root, Span relativeTarget) {
	const usize start = writePos;
	append(root);
	if (relativeTarget.size != 0) {
		if (relativeTarget.ptr[0] != '/')
			append("/");
		append(relativeTarget);
	}
	else if (root.size == 0)
		append("/");
	Span fullPath = {(char*)data + start, writePos - start};
	data[writePos++] = 0;
	return fullPath;
}

BUFFER_INL
(Span) append_url_encoded(const char* ptr, usize length) {
	static const u8 hex[] = "0123456789ABCDEF";
	char* optr = (char*)data + writePos;

	for (usize index = 0; index < length; index++) {
		const u8 value = (u8)ptr[index];
		const bool encode = gAsciiLut[value] > ASCII_URL_VALID;
		u8 buffer[4] = {value, '%', hex[value >> 4], hex[value & 15]};
		append_inline<3>((char*)buffer + encode, 1 + 2 * encode);
	}
	return {optr, (usize)((char*)data + writePos - optr)};
}

BUFFER_INL
(Span) append_html_encoded(char* ptr, usize length) {
	static const u8 lengthLut[6] = {5, 5, 6, 4, 4, 1};
	static const char strLut[5][8] = {"&amp;", "&#39;", "&quot;", "&lt;", "&gt;"};
	char* optr = (char*)data + writePos;

	for (usize index = 0; index < length; index++) {
		u8 asciiLutIndex = gAsciiLut[(u8)ptr[index]] - ASCII_HTML_ESCAPE_START;
		u8 strLutIndex = MIN(5, asciiLutIndex);
		const char* src = (strLutIndex == 5) ? ptr + index : strLut[strLutIndex];
		append_inline<6>(src, lengthLut[strLutIndex]);	// Up to 8 bytes overflow is safe
	}
	return {optr, (usize)((char*)data + writePos - optr)};
}

// <a href="filename[256]">filename[64]</a>    02-Dec-2004 18:46    241476

BUFFER_INL
(Span) append_entry(int directoryFd, char* name) {
	char* optr = (char*)data + writePos;
	Span entry = {name, STRLEN(name)};

	struct stat st;
	if (LITCMP(entry.ptr, ".\0") == 0 || LITCMP(entry.ptr, "..\0") == 0)
		return {optr, 0};
	if (fstatat(directoryFd, name, &st, 0))
		return append("<a href=\"\">--- Privileged access ---</a>\n");

	usize fileSize = S_ISDIR(st.st_mode) ? 0 : (usize)st.st_size;
	char buf[32];
	Clock::format_time(&st.st_mtim, buf);

	// 0 visible, 776 bytes (11 + 3 * 255)
	append("<a href=\"");
	append_url_encoded(entry.ptr, entry.size);
	append("\">");

	// 52 visible, 297 bytes (3 + (52 - 3) * 6 bytes) (52 = HTTP_INDEX_NAME_LENGTH)
	if (entry.size >= HTTP_INDEX_NAME_LENGTH) {
		append_html_encoded(entry.ptr, HTTP_INDEX_NAME_LENGTH - 3);
		append("...</a>");
	}
	else {
		append_html_encoded(entry.ptr, entry.size);
		append("</a>");
		memset(' ', HTTP_INDEX_NAME_LENGTH - entry.size);
	}

	// 48 visible, 53 bytes
	memset(' ', 8);
	append_inline<17>(buf, 17);
	memset(' ', 4);
	append_digit10(fileSize);
	append("\n");
	return {optr, (usize)((char*)data + writePos - optr)};
}

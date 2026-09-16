#pragma once
#include "core.hpp"
#include "webserv.hpp"
#include "Span.hpp"

namespace fn {
//

ATTR(inl)	// REVIEW : should be static_inl no?
Span itoa10(usize number, char* buffer, usize bufferSize) {
	ASSERT(bufferSize >= 20, "Buffer isn't big enough for itoa");
	char* ptr = buffer + bufferSize;
	*ptr = 0;
	char* const optr = ptr;
	do {
		*--ptr = (char)((number % 10) + '0');
		number /= 10;
	} while (number != 0);
	Span result = {ptr, (usize)(optr - ptr)};
	return result;
}

ATTR(inl)
Span itoa16(usize number, char* buffer, usize bufferSize) {
	static const char digits[16] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	ASSERT(bufferSize >= 16, "Buffer isn't big enough for itoa");
	char* ptr = buffer + bufferSize;
	*ptr = 0;
	char* const optr = ptr;
	do {
		*--ptr = digits[(number % 16)];
		number /= 16;
	} while (number != 0);
	Span result = {ptr, (usize)(optr - ptr)};
	return result;
}

/*
	These two functions only check overflow through the range checks
	15 and 19 correspond to the maximum safely representable digit length
	for base10 and 16 respectively. If exact length strtol is required,
	just use minlength = maxlength
*/
ATTR(static_inl, pure)
usize qstrtol10(const char* src, usize minLength = 1, usize maxLength = 19) {
	char buffer[32] = {};
	MEMCPY_INLINE(buffer, src, 24);
	usize value = 0;
	usize digit;

	char* ptr = buffer;
	while ((digit = (usize)(*ptr - '0')) <= 9) {
		value = value * 10 + digit;
		ptr++;
	}
	const usize length = (usize)(ptr - buffer);
	return length >= minLength && length <= maxLength ? value : SIZE_MAX;
}

ATTR(static_inl, pure)
usize qstrtol16(const char* src, usize minLength = 1, usize maxLength = 15) {
	char buffer[32] = {};
	MEMCPY_INLINE(buffer, src, 24);
	usize value = 0;
	usize digit;

	char* ptr = buffer;
	while ((digit = (usize)gAsciiLut[(u8)*ptr]) <= ASCII_HEX) {
		value = value * 16 + digit;
		ptr++;
	}
	const usize length = (usize)(ptr - buffer);
	return length >= minLength && length <= maxLength ? value : SIZE_MAX;
}
}

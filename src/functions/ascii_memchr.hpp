#pragma once
#include <emmintrin.h>
#include <immintrin.h>
#include <unistd.h>

#include "core.hpp"

namespace fn {

// Overreads at most 15 bytes
ATTR(static_inl)
void* q16memchr(void* vstr, u8 c, usize length) {
	u8* str = (u8*)vstr;
	u8* end = (u8*)vstr + length;
	usize matchIndex;
	u8 tmp = str[length];

	const __m128i needle = _mm_set1_epi8((char)c);
	*end = c;
	while (true) {
		__m128i block = _mm_loadu_si128((const __m128i*)str);	// Loads 16 bytes
		__m128i eq = _mm_cmpeq_epi8(block, needle);				// Compares 16 bytes (0xFF for match)
		usize mask = (u32)_mm_movemask_epi8(eq);				// Extracts top bit from each byte
		matchIndex = (mask == 0) ? WORD_BITS : (usize)CTZ(mask);
		if (matchIndex != WORD_BITS)
			break;
		str += 16;
	}
	*end = tmp;
	if (str + matchIndex >= end)
		return NULL;
	return str + matchIndex;
}

// Overreads at most 31 bytes
ATTR(static_inl)
void* q32memchr(void* vstr, u8 c, usize length) {
	u8* str = (u8*)vstr;
	u8* end = str + length;
	usize matchIndex;
	u8 tmp = str[length];

	const __m256i needle = _mm256_set1_epi8((char)c);
	*end = c;
	while (true) {
		__m256i block = _mm256_loadu_si256((const __m256i*)str);
		__m256i eq = _mm256_cmpeq_epi8(block, needle);
		usize mask = (u32)_mm256_movemask_epi8(eq);
		matchIndex = TZCNT(mask);
		if (matchIndex != WORD_BITS)
			break;
		str += 32;
	}
	*end = tmp;
	if (str + matchIndex >= end)
		return NULL;
	return str + matchIndex;
}

ATTR(static_inl)
void* q16memrchr(void* vstr, u8 c, usize length) {
	ENFORCE(length > 0, "Empty range in q16memrchr");
	u8* str = (u8*)vstr + length - 16;
	u8* end = (u8*)vstr;
	usize matchIndex;
	u8 tmp = *end;

	*end = c;
	const __m128i needle = _mm_set1_epi8((char)c);
	while (true) {
		__m128i block = _mm_loadu_si128((const __m128i*)str);	// Loads 16 bytes
		__m128i eq = _mm_cmpeq_epi8(block, needle);				// Compares 16 bytes (0xFF for match)
		usize mask = (u32)_mm_movemask_epi8(eq);				// Extracts top bit from each byte
		matchIndex = (mask == 0) ? WORD_BITS : (WORD_BITS - 1 - (usize)CLZ(mask));
		if (matchIndex != WORD_BITS)
			break;
		str -= 16;
	}
	u8* match = str + matchIndex;
	*end = tmp;
	if (match == end)
		return (tmp == c) ? end : NULL;
	return match;
}

ATTR(static_inl)
void* q32memrchr(void* vstr, u8 c, usize length) {
	ENFORCE(length > 0, "Empty range in q32memrchr");
	u8* str = (u8*)vstr + length - 32;
	u8* end = (u8*)vstr;
	usize matchIndex;
	u8 tmp = *end;

	*end = c;
	const __m256i needle = _mm256_set1_epi8((char)c);
	while (true) {
		__m256i block = _mm256_loadu_si256((const __m256i*)str);	// Loads 32 bytes
		__m256i eq = _mm256_cmpeq_epi8(block, needle);			// Compares 32 bytes (0xFF for match)
		usize mask = (u32)_mm256_movemask_epi8(eq);				// Extracts top bit from each byte
		matchIndex = (mask == 0) ? WORD_BITS : (WORD_BITS - 1 - (usize)CLZ(mask));
		if (matchIndex != WORD_BITS)
			break;
		str -= 32;
	}
	u8* match = str + matchIndex;
	*end = tmp;
	if (match == end)
		return (tmp == c) ? end : NULL;
	return match;
}

}

#ifdef __AVX2__
	#define QMEMCHR(vstr, c, length) fn::q32memchr(vstr, c, length)
	#define QMEMRCHR(vstr, c, length) fn::q32memrchr(vstr, c, length)
#else
	#define QMEMCHR(vstr, c, length) fn::q16memchr(vstr, c, length)
	#define QMEMRCHR(vstr, c, length) fn::q16memrchr(vstr, c, length)
#endif

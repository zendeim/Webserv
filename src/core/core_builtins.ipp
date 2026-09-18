#pragma once
#include "core.hpp"

#define MEMCPY(dst, src, n)		__builtin_memcpy(dst, src, n)
#define MEMMOVE(dst, src, n)	__builtin_memmove(dst, src, n)
#define MEMSET(dst, val, n)		__builtin_memset(dst, val, n)
#define MEMCHR(src, val, n)		__builtin_memchr(src, val, n)
#define MEMCMP(s1, s2, n)		__builtin_memcmp(s1, s2, n)
#define STRLEN(str) 			__builtin_strlen(str)

#if defined(__clang__) && __clang_major__ >= 15 && __has_builtin(__builtin_memcpy_inline)
	#define MEMCPY_INLINE(dst, src, n)	__builtin_memcpy_inline(dst, src, n)
#else
	#define MEMCPY_INLINE(dst, src, n)	__builtin_memcpy(dst, src, n)
#endif

#if defined(__clang__) && __clang_major__ >= 15 && __has_builtin(__builtin_memset_inline)
	#define MEMSET_INLINE(dst, val, n)	__builtin_memset_inline(dst, val, n)
#else
	#define MEMSET_INLINE(dst, val, n)	__builtin_memset(dst, val, n)
#endif

#define CLZ(x)			__builtin_clzll(x)
#define LZCNT(x)		x == 0 ? WORD_BITS : (usize)__builtin_clzll(x)
#define CTZ(x)			__builtin_ctzll(x)
#define TZCNT(x)		x == 0 ? WORD_BITS : (usize)__builtin_ctzll(x)
#define POPCOUNT(x)		__builtin_popcountll(x)
#define FFS(x)			__builtin_ffsll(x)
#define PARITY(x)		__builtin_parityll(x)
#define BSWAP16(x)		__builtin_bswap16(x)
#define BSWAP32(x)		__builtin_bswap32(x)
#define BSWAP64(x)		__builtin_bswap64(x)
#define BITREVERSE(x)	__builtin_bitreverse64(x)	// This is clang specific

// To add:
// TODO: find better names, Bitcasts
// === Builtin extensions =====================================
#define LITCMP(s1, s2)		__builtin_memcmp(s1, s2, sizeof(s2) - 1)
#define LITCPY(dst, src)	__builtin_memcpy(dst, src, sizeof(src) - 1)
#define LITPREPEND(s1, s2)		((char*)__builtin_memcpy(s1 - (sizeof(s2) - 1), s2, (sizeof(s2) - 1)))
#define LITAPPEND(s1, s2)		(char*)__builtin_memcpy(s1, s2, (sizeof(s2) - 1)) + ((sizeof(s2) - 1))

#define MEMFIND(dst, str, dstSize) \
({ \
	const u8* mf_dst = (const u8*)(dst); \
	const usize mf_dstSize = (usize)(dstSize); \
	const usize mf_strSize = sizeof(str) - 1; \
	usize mf_result = SIZE_MAX; \
	for (usize mf_i = 0; mf_i <= mf_dstSize - mf_strSize; mf_i++) { \
		if (MEMCMP(mf_dst + mf_i, (str), mf_strSize) == 0) { \
			mf_result = mf_i; \
			break; \
		} \
	}\
	mf_result; \
})

#define MEMCHR_INDEX(src, val, n)\
({\
	const unsigned char* _memchr_src = (const unsigned char*)(src);\
	const unsigned char* _memchr_result = (const unsigned char*)MEMCHR(_memchr_src, val, n);\
	_memchr_result ? (size_t)(_memchr_result - _memchr_src) : SIZE_MAX;\
})

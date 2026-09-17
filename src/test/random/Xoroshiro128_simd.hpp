#pragma once
#include "core.hpp"
#include "Span.hpp"
#include "x86intrin.h"
#include "Random.hpp"

/*
	Uses 

	https://prng.di.unimi.it/xoroshiro128plus.c

	TODO: 
		1) See if mixing + and ++ is bad
		2) Create SIMD RNG generation
*/
struct Xoroshiro128 {
	static_inl u64x4 stateLow;
	static_inl u64x4 stateHigh;

	ATTR(static_inl, constructor)
	void init() {
		u64 seed = __rdtsc();
		for (usize i = 0; i < 4; i++) {
			stateLow[i] = Random::splitmix64(seed);
			stateHigh[i]= Random::splitmix64(stateLow[i]);
			seed = stateHigh[i];
		}
	}

	ATTR(static_inl)
	u64 next() {
		const u64 s0 = stateLow[0];
		u64 s1 = stateHigh[0];
		const u64 result = Random::rotl(s0 + s1, 17) + s0;

		s1 ^= s0;
		stateLow[0] = Random::rotl(s0, 49) ^ s1 ^ (s1 << 21);
		stateHigh[0] = Random::rotl(s1, 28);
		return result;
	}

	ATTR(static_inl)
	u64x4 next4() {
		const u64x4 s0 = stateLow;
		u64x4 s1 = stateHigh;
		const u64x4 result = Random::vec_rotl(s0 + s1, 17) + s0;

		s1 ^= s0;
		stateLow = Random::vec_rotl(s0, 49) ^ s1 ^ (s1 << 21);
		stateHigh = Random::vec_rotl(s1, 28);
		return result;
	}

	ATTR(static_inl)
	void random_range(u8* dst, usize length) {
		const usize bodyLength = length / sizeof(u64x4);
		const usize tailLength = length % sizeof(u64x4);

		for (usize i = 0; i < bodyLength; i += sizeof(u64x4)) {
			u64x4 randomValues = next4();
			MEMCPY_INLINE(dst + i, &randomValues, sizeof(u64x4));
		}

		if (tailLength) {
			u64x4 randomValues = next4();
			u8* ptr = (u8*) &randomValues;
			dst += bodyLength;
			for (usize i = 0; i < tailLength; i++)
				dst[i] = ptr[i];
		}
	}

	ATTR(static_inl)	// xoroshiro128++ jump
	void jump() {
		static const u64 jumpTable[2] = {0x2bd7a6a6e99c2ddc, 0x0992ccaf6a6fca05};

		u64 s0 = 0;
		u64 s1 = 0;
		for (usize i = 0; i < 2; i++) {
			for (usize bitIndex = 0; bitIndex < 64; bitIndex++) {
				if (jumpTable[i] & 1ull << bitIndex) {
					s0 ^= stateLow[0];
					s1 ^= stateHigh[0];
				}
				next();
			}
		}
		stateLow[0] = s0;
		stateHigh[0] = s1;
	}
};

// u64x4 next4(u64x4& s0, u64x4& s1) {
// 	const u64x4 a = s0;
// 	u64x4 b = s1;
// 	const u64x4 result = a + b;

// 	b ^= a;
// 	s0 = vec_rotl(a, 24) ^ b ^ (b << 16);
// 	s1 = vec_rotl(b, 37);

// 	return result;
// }

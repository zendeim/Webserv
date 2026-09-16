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
	static_inl u64 stateLow = 9223372036854775783;
	static_inl u64 stateHigh = 6364136223846793005;

	ATTR(static_inl, constructor)
	void init() {
		const u64 seed = __rdtsc();
		stateLow = Random::splitmix64(seed);
		stateHigh = Random::splitmix64(stateLow);
	}

	ATTR(static_inl)	// xoroshiro128++
	u64 next() {
		const u64 s0 = stateLow;
		u64 s1 = stateHigh;
		const u64 result = Random::rotl(s0 + s1, 17) + s0;

		s1 ^= s0;
		stateLow = Random::rotl(s0, 49) ^ s1 ^ (s1 << 21);
		stateHigh = Random::rotl(s1, 28);
		return result;
	}

	ATTR(static_inl)	// xoroshiro128+
	u64 qnext() {
		const u64 s0 = stateLow;
		u64 s1 = stateHigh;
		const u64 result = s0 + s1;

		s1 ^= s0;
		stateLow = Random::rotl(s0, 24) ^ s1 ^ (s1 << 16);
		stateHigh = Random::rotl(s1, 37);
		return result;
	}

	ATTR(static_inl)
	float nextf() {
		u32 intResult = 0x3F800000u | qnext() >> 41;	// Sets exponent to 127, so range: 1.0f ~ 2.0f
		float floatResult;

		MEMCPY_INLINE(&floatResult, &intResult, sizeof(float));
		return floatResult - 1.0f;
	}

	ATTR(static_inl)
	f32x2 next2f() {
		u64 randomValue = qnext();
		u32x2 intResult = {0x3F800000, 0x3F800000};
		intResult[0] |= randomValue >> 41;
		intResult[1] |= (randomValue << 23) >> 41;	// Sets exponent to 127, so range: 1.0f ~ 2.0f

		f32x2 floatResult;
		MEMCPY_INLINE(&floatResult, &intResult, sizeof(floatResult));
		return floatResult - 1.0f;
	}

	ATTR(static_inl)	// xoroshiro128++ jump
	void jump() {
		static const u64 jumpTable[2] = {0x2bd7a6a6e99c2ddc, 0x0992ccaf6a6fca05};

		u64 s0 = 0;
		u64 s1 = 0;
		for (usize i = 0; i < 2; i++) {
			for (usize bitIndex = 0; bitIndex < 64; bitIndex++) {
				if (jumpTable[i] & 1ull << bitIndex) {
					s0 ^= stateLow;
					s1 ^= stateHigh;
				}
				next();
			}
		}
		stateLow = s0;
		stateHigh = s1;
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

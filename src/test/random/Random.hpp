#pragma once
#include "core.hpp"

// global / local, constant / mutable, inline / 

struct Random {

	ATTR(static_inl, const)
	u64 rotl(u64 value, u64 shiftWidth) {
		return (value << shiftWidth) | (value >> (64 - shiftWidth));
	}

	template <typename Vec> ATTR(static_inl, const)
	Vec vec_rotl(Vec value, usize shiftWidth) {
		return (value << shiftWidth) | (value >> (64 - shiftWidth));
	}

	ATTR(static_inl, const)
	u64 splitmix64(u64 seed) {
		u64 result = seed + 0x9E3779B97f4A7C15;
		result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9;
		result = (result ^ (result >> 27)) * 0x94D049BB133111EB;
		return result ^ (result >> 31);
	}

	ATTR(static_inl)
	f32 random_float(u64 randomValue) {
		u32 intResult = 0x3F800000u | randomValue >> 41;	// Sets exponent to 127, so range: 1.0f ~ 2.0f
		f32 floatResult;

		MEMCPY_INLINE(&floatResult, &intResult, sizeof(f32));
		return floatResult - 1.0f;
	}

	ATTR(static_inl)
	f32x2 random_float2(u64 randomValue) {
		u32x2 intResult = {0x3F800000, 0x3F800000};
		intResult[0] |= randomValue >> 41;
		intResult[1] |= (randomValue << 23) >> 41;	// Sets exponent to 127, so range: 1.0f ~ 2.0f

		f32x2 floatResult;
		MEMCPY_INLINE(&floatResult, &intResult, sizeof(floatResult));
		return floatResult - 1.0f;
	}

	ATTR(static_inl, const)
	u64 create_random_range(u64 randomValue, usize min, usize max) {

	}
};

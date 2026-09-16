#pragma once
#include "core.hpp"
#include "Span.hpp"
#include "x86intrin.h"

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

	ATTR(static_inl, const)
	u64 create_random_range(u64 randomValue, usize min, usize max) {
		
	}
};

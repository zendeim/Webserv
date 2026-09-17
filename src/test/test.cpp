#include <cpuid.h>
#include <ctime>
#include <x86intrin.h>
#include <unistd.h>
#include <iostream>

#include "core.hpp"
#include "HWTimer.hpp"
#include "Random.hpp"
#include "Xoroshiro128.hpp"
#include "pure_functions.hpp"

ATTR(none, noinline, flatten)
void* memchr_libc(void* vstr, u8 c, usize length) {	return MEMCHR(vstr, (int)c, length); }

ATTR(none, noinline, flatten)
void* memchr_q32(void* vstr, u8 c, usize length) {	return fn::q32memchr(vstr, c, length); }

struct TestRange {
	u32 start;
	u32 size;
	u8 value;
};

struct Sample {
	usize timeA;
	usize timeB;
	u32 size;
	u32 iteration;
	u16 sizeClass;
	u16 throwaway;
};

static inline
TestRange s_create_random_range(u8* buffer) {
	static u32 sizes[] = {
		4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
		32, 36, 40, 44, 48, 52, 56, 60,
		64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240,
		256, 512, 768, 1024, 1280, 1536, 1792, 2048, 2304, 2560, 2816, 3072, 3328, 3584, 3840,
		4_KB, 8_KB, 16_KB, 32_KB, 64_KB, 128_KB, 256_KB, 512_KB, 1_MB, 1_MB, 2_MB, 2_MB, 4_MB, 4_MB, 4_MB};
	TestRange range = {};

	u64 randomValue = Xoroshiro128::next();
	MEMCPY_INLINE(&range, &randomValue, sizeof(u64));
	float randomFloat = Random::random_float(randomValue);

	range.start = CLAMP(range.start % 4_MB, 128, 4_MB - 128);
	range.size = sizes[range.size % ARRAY_SIZE(sizes)];
	range.value = (u8)(randomValue >> 41);
	range.value = MAX(range.value + 64u, 255);	// Biases it so that 75% of sentinels are valid

	usize index = range.start + (usize)(range.size * randomFloat);
	buffer[index] = range.value;
	return range;
}

static inline
Sample s_test_functions(u8* str, usize iteration, TestRange range) {
	Sample sample = {};
	bool libcFirst = iteration & 1;
	str += range.start;
	sample.iteration = iteration;
	sample.size = range.size;
	sample.sizeClass = WORD_BITS - (u32)CLZ(range.size);
	if (libcFirst) {
		u64 t0 = HWTimer::get_tsc_gated();
		void* first = memchr_libc(str, range.value, range.size);
		u64 t1 = HWTimer::get_tsc_gated();
		void* second = memchr_q32(str, range.value, range.size);
		u64 t2 = HWTimer::get_tsc_gated();
		sample.timeA = HWTimer::tsc_to_ns(t1 - t0);
		sample.timeB = HWTimer::tsc_to_ns(t2 - t1);
		sample.throwaway = ((usize)first * (usize)second) % 64_KB;
		return sample;
	}
	u64 t0 = HWTimer::get_tsc_gated();
	void* second = memchr_q32(str, range.value, range.size);
	u64 t1 = HWTimer::get_tsc_gated();
	void* first = memchr_libc(str, range.value, range.size);
	u64 t2 = HWTimer::get_tsc_gated();
	sample.timeB = HWTimer::tsc_to_ns(t1 - t0);
	sample.timeA = HWTimer::tsc_to_ns(t2 - t1);
	sample.throwaway = ((usize)first * (usize)second) % 64_KB;
	return sample;
}

#define NUM_SAMPLES 8192
#define NUM_EPOCHS 4096

int main() {
	static u8 buffer[8_MB];
	static TestRange ranges[NUM_SAMPLES];
	static Sample samples[NUM_SAMPLES];
	static u128 libcTotalTime[64] = {};
	static u128 q32TotalTime[64] = {};

	for (usize epoch = 0; epoch < NUM_EPOCHS; epoch++) {
		for (usize i = 0; i < sizeof(buffer); i += 8) {
			u64 randomValue = Xoroshiro128::next();
			randomValue &= 0x7F7F7F7F7F7F7F7F;
			MEMCPY_INLINE(buffer + i, &randomValue, sizeof(randomValue));
		}
		for (usize i = 0; i < NUM_SAMPLES; i++)
			ranges[i] = s_create_random_range(buffer);
		for (usize i = 0; i < NUM_SAMPLES; i++)
			samples[i] = s_test_functions(buffer, i, ranges[i]);
		for (usize i = 0; i < NUM_SAMPLES; i++) {
			const Sample& sample = samples[i];
			const u16 sizeClass = sample.sizeClass;
			libcTotalTime[sizeClass] += sample.timeA;
			q32TotalTime[sizeClass] += sample.timeB;
		}
	}

	for (usize i = 0; i < 64; i++) {
		if (libcTotalTime[i] == 0 || q32TotalTime[i] == 0)
			continue;
		u128 result = (q32TotalTime[i] << 32) / libcTotalTime[i];
		double dblResult = (double) result / (double)(1ull << 32);
		std::cout << dblResult << ", ";
	}
}

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
TestRange s_create_random_range() {
	TestRange range = {};

	u64 randomValue = Xoroshiro128::next();
	MEMCPY_INLINE(&range, &randomValue, sizeof(u64));
	range.start = CLAMP(range.start % 4_MB, 128, 4_MB - 128);
	range.size = CLAMP(range.size % 4_MB, 1, 4_MB - 128);
	range.value = (u8)(randomValue >> 41);
	return range;
}

static inline
Sample s_test_functions(char* str, usize iteration, TestRange range) {
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

static inline
void s_print_average_by_size_class(const Sample* samples, usize count) {
	struct Average {
		u64 libc;
		u64 q32;
		u64 count;
	};

	Average averages[WORD_BITS + 1] = {};

	for (usize i = 0; i < count; i++) {
		const Sample& sample = samples[i];
		Average& average = averages[sample.sizeClass];

		average.libc += sample.timeA;
		average.q32 += sample.timeB;
		average.count++;
	}

	std::cout << "size\tlibc\tq32\tq32/libc\n";

	for (usize i = 0; i <= WORD_BITS; i++) {
		const Average& average = averages[i];
		if (!average.count)
			continue;

		double libc = (double)average.libc / average.count;
		double q32 = (double)average.q32 / average.count;

		std::cout << i << '\t'
			<< libc << '\t'
			<< q32 << '\t'
			<< q32 / libc << '\n';
	}
}

#define NUM_SAMPLES 256_KB

int main() {
	static char buffer[8_MB];
	static TestRange ranges[NUM_SAMPLES];
	static Sample samples[NUM_SAMPLES];

	for (usize i = 0; i < sizeof(buffer); i += 8) {
		u64 randomValue = Xoroshiro128::next();
		MEMCPY_INLINE(buffer + i, &randomValue, sizeof(randomValue));
	}

	for (usize i = 0; i < NUM_SAMPLES; i++)
		ranges[i] = s_create_random_range();
	for (usize i = 0; i < NUM_SAMPLES; i++)
		samples[i] = s_test_functions(buffer, i, ranges[i]);
	s_print_average_by_size_class(samples, NUM_SAMPLES);
}

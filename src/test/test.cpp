#include <cpuid.h>
#include <ctime>
#include <x86intrin.h>
#include <unistd.h>
#include <iostream>

#include "core.hpp"
#include "HWTimer.hpp"
#include "Random.hpp"
#include "Xoroshiro128_simd.hpp"
#include "pure_functions.hpp"

ATTR(none, noinline, flatten)
void* memchr_libc(void* vstr, u8 c, usize length) {	return MEMCHR(vstr, (int)c, length); }

ATTR(none, noinline, flatten)
void* memchr_q32(void* vstr, u8 c, usize length) {	return fn::q32memchr(vstr, c, length); }

static u64 throwaway = 0;

struct TestRange {
	u32 start;
	u32 size;
	u8 value;
	u8* ptr;
	u8 old;
};

#define BUFFERSIZE 128_MB

static inline
TestRange s_create_random_range(u8* buffer) {
	static u32 sizes[] = {
		4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
		32, 36, 40, 44, 48, 52, 56, 60,
		64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240,
		256, 512, 768, 1024, 1280, 1536, 1792, 2048, 2304, 2560, 2816, 3072, 3328, 3584, 3840,
		4_KB, 8_KB, 16_KB, 32_KB, 64_KB, 128_KB, 256_KB, 512_KB, 1_MB
	};

	TestRange range = {};
	u64 randomValue = Xoroshiro128::next();
	MEMCPY_INLINE(&range, &randomValue, sizeof(u64));
	const float randomFloat = Random::random_float(randomValue);
	range.size = sizes[range.size % ARRAY_SIZE(sizes)];
	range.start = CLAMP(range.start % BUFFERSIZE, 128, BUFFERSIZE - 4_MB);
	range.value = (u8)(randomValue >> 41);
	range.value = MIN(range.value + 64u, 255);
	range.ptr = buffer + range.start + (u32)(range.size * randomFloat);
	range.old = *range.ptr;
	*range.ptr = range.value;
	return range;
}

static inline
u64 s_test_functions(u8* buffer, usize iteration, u128 libcTotalTime[64], u128 q32TotalTime[64]) {
	TestRange range = s_create_random_range(buffer);
	usize sizeClass = WORD_BITS - (u32)CLZ(range.size);
	u8* ptr = buffer + range.start;
	if (iteration & 1) {
		u64 t0 = HWTimer::get_tsc_gated();
		void* first = memchr_libc(ptr, range.value, range.size);
		u64 t1 = HWTimer::get_tsc_gated();
		void* second = memchr_q32(ptr, range.value, range.size);
		u64 t2 = HWTimer::get_tsc_gated();
		libcTotalTime[sizeClass] += t1 - t0;
		q32TotalTime[sizeClass] += t2 - t1;
		*range.ptr = range.old;
		return ((usize)first * (usize)second);
	}
	u64 t0 = HWTimer::get_tsc_gated();
	void* second = memchr_q32(ptr, range.value, range.size);
	u64 t1 = HWTimer::get_tsc_gated();
	void* first = memchr_libc(ptr, range.value, range.size);
	u64 t2 = HWTimer::get_tsc_gated();
	q32TotalTime[sizeClass] += t1 - t0;
	libcTotalTime[sizeClass] += t2 - t1;
	*range.ptr = range.old;
	return ((usize)first * (usize)second);
}

#define NUM_EPOCHS 4_MB
// #include <sys/mman.h>
// u8* buffer = (u8*)mmap(nullptr, 2_GB, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

int main() {
	static u8 buffer[BUFFERSIZE];
	static u128 libcTotalTime[64] = {};
	static u128 q32TotalTime[64] = {};
	Xoroshiro128::random_range(buffer, sizeof(buffer), 0x7F);

	for (usize epoch = 1; epoch <= NUM_EPOCHS; epoch++) {
		throwaway += s_test_functions(buffer, epoch, libcTotalTime, q32TotalTime);
	}
	for (usize i = 0; i < 64; i++) {
		if (libcTotalTime[i] == 0 || q32TotalTime[i] == 0)
			continue;
		u128 result = (q32TotalTime[i] << 32) / libcTotalTime[i];
		double dblResult = (double) result / (double)(1ull << 32);
		std::cout << dblResult << ", ";
	}
	std::cout << "\n";
	return (int)throwaway % 64;
}

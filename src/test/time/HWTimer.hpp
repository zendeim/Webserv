#pragma once
#include <cpuid.h>
#include <ctime>
#include <x86intrin.h>
#include <unistd.h>

#include "core.hpp"

// If 0, it will run CPUID, and if that fails, calibration
#define TSC_FREQUENCY 0
#define TSC_FACTOR 3932232912438853884
STATIC_ASSERT(TSC_FREQUENCY == 0 || TSC_FREQUENCY > 1_G);

/*	Hardware Timer using RDTSC:

	Uses 64 bits for the fractional component, and computes the scale to ns
	This can overflow if tsc_freq is lower than GIGA. Very few architectures
	qualify, and by shifting it with 64 clang compiles this to a perfect and beautiful:
		mov (rdx, rsi);    mulx (rax, rax, rdi);    ret

	TODO: 
		1) Make the timer thread friendly
		2) Improve calibration (initial calibration is not very good, with 1ms calibration 
			deviates around 1000us every second. I don't really want to increase the budget
			because it's a forced calibration that delays other tasks)
		3) Get TSC_FACTOR as constexpr from TSC_FREQUENCY (move to class)
============================================================================= */

struct HWTimer {
	struct TimeSample {
		u64 tick;
		u64 time;
	};

	static_inl u64 tscFactor = 0;
	static_inl TimeSample unixBase = {}, calBase = {};

	ATTR(static_inl) // Get unix timestamp in nanoseconds
	u64 get_time() {
		u64 tscTick = get_tsc();
		u64 timeNow = unixBase.time + tsc_to_ns(tscTick - unixBase.tick);
		return timeNow;
	}

	ATTR(static_inl) // Get elapsed time in nanoseconds
	u64 get_time_elapsed() {
		u64 tscTick = get_tsc();
		u64 timeNow = tsc_to_ns(tscTick - unixBase.tick);
		return timeNow;
	}

// ==== RDTSC Calibration =====================================================
	ATTR(static_inl, constructor)
	void init() {
		unixBase = measure_clock(64, CLOCK_REALTIME);	// 1024 iterations takes around 50 microseconds
		if (TSC_FACTOR != 0) {
			tscFactor = TSC_FACTOR;
			calBase = measure_clock();
			return;
		}
		u64 tscFreq = TSC_FREQUENCY == 0 ? identify_tsc_freq() : TSC_FREQUENCY;
		if (tscFreq != 0) {
			tscFactor = freq_to_ns_cycles(tscFreq);
			calBase = measure_clock();
			return;
		}
		calBase = measure_clock(256);		// Important for the first measurement to be accurate
		soft_calibrate(100'000, 64);	// Intentionally short to not take up precious init time
	}

	ATTR(static_inl, flatten)
	TimeSample measure_clock(usize numSamples = 8, clockid_t clockType = CLOCK_MONOTONIC_RAW) {
		u64 bestWidth = UINT64_MAX;
		u64 bestTick, bestTime;
		timespec timeNow;
		clock_gettime(clockType, &timeNow); // VDSO Warmup
		for (usize i = 0; i < numSamples; i++) {
			u64 t1 = get_tsc_gated();
			clock_gettime(clockType, &timeNow);
			u64 t2 = get_tsc_gated();
			u64 width = t2 - t1;

			if (width < bestWidth) {
				bestWidth = width;
				bestTick = t1 + width / 2;
				bestTime = (u64)timeNow.tv_sec * 1_G + (u64)timeNow.tv_nsec;
			}
		}
		return {bestTick, bestTime};
	}

	ATTR(static_inl, flatten)
	isize measure_current_error(usize numSamples = 8) {
		TimeSample cur = measure_clock(numSamples);
		u64 tscTimeElapsed = tsc_to_ns(cur.tick - calBase.tick);
		u64 unixTimeElapsed = cur.time - calBase.time;
		return (isize)tscTimeElapsed - (isize)unixTimeElapsed;
	}

	ATTR(static_inl, flatten)	// Progressively gets better as elapsed grows
	void soft_calibrate(usize nsDelay = 0, usize numSamples = 8) {
		if (nsDelay > 0) {
			timespec wait{.tv_sec = 0, .tv_nsec = (long)nsDelay};
			nanosleep(&wait, nullptr);
		}
		TimeSample cur = measure_clock(numSamples);
		TimeSample delta = {cur.tick - calBase.tick, cur.time - calBase.time};
		tscFactor = (u64)(((u128)delta.time << 64) / delta.tick);
	}

	ATTR(static_inl, flatten)	// Resets base measurement
	u64 hard_calibrate(usize secDelay, usize nsecDelay, usize numSamples) {
		calBase = measure_clock(numSamples);
		timespec wait{.tv_sec = (long)secDelay, .tv_nsec = (long)nsecDelay};
		nanosleep(&wait, nullptr);
		TimeSample cur = measure_clock(numSamples);
		TimeSample delta = {cur.tick - calBase.tick, cur.time - calBase.time};
		tscFactor = (u64)(((u128)delta.time << 64) / delta.tick);
		return tscFactor;
	}

	ATTR(static_inl)
	u64 identify_tsc_freq() {
		uint eax, ebx;		// EAX, EBX define the ratio between TSC and crystal
		uint ecx, edx;		// TSC = EBX / EAX * ECX, where ECX is the crystal frequency
		uint maxLeaf = __get_cpuid_max(0, NULL);
		u64 tscFreq = 0;	// 10^9 hz order of magnitude, so GHz

		// leaf 0x15: TSC/core-crystal clock ratio
		if (maxLeaf >= 0x15 && __get_cpuid_count(0x15, 0, &eax, &ebx, &ecx, &edx)) {
			if (eax != 0 && ebx != 0 && ecx != 0)
				tscFreq = ((u64)ecx * ebx) / eax;
		}
		return tscFreq;
	}

// ==== RDTSC Handling ========================================================
	ATTR(static_inl, const) constexpr
	u64 freq_to_ns_cycles(u64 tscFreq) {
		return (u64)(((u128)1_G << 64) / tscFreq);
	}

	ATTR(static_inl)
	u64 tsc_to_ns(u64 tscTicks) {
		return (u64)((u128)tscTicks * tscFactor >> 64);
	}

	ATTR(static_inl)
	u64 get_tsc() {
		return __rdtsc();
	}

	ATTR(static_inl)
	u64 get_tsc_gated() {
		_mm_lfence();
		u64 tscTicks = __rdtsc();
		_mm_lfence();
		return tscTicks;
	}
};	// ==== HWTimer End =======================================================

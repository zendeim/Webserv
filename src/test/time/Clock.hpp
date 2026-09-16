#pragma once
#include <cpuid.h>
#include <ctime>
#include <x86intrin.h>
#include <unistd.h>

#include "core.hpp"
#include "pure_functions.hpp"
#include "HWTimer.hpp"

/*	Clock:

	TODO: 
		1) Separate formatting from date collection
		2) Add WEEK to full date
============================================================================= */
struct Clock {
	static inline const u8 months[12][10] = {	// TODO: USE
		"January", "February", "March", "April", "May", "June", "July",
		"August", "September", "October", "November","December"
	};
	u64 timeNow = 0;

	ATTR(inl)
	u64 update() {
		timeNow = HWTimer::get_time();
		return timeNow;
	}

	ATTR(inl)
	u64 time_elapsed() {
		return timeNow - HWTimer::unixBase.time;
	}

	struct Date {
		u16 year;
		u8 month, day;
	};

	// Ben Joffe's FastDate algorithm adapted https://www.benjoffe.com/fast-date-64
	ATTR(static_inl, const)
	Date get_calendar_time(u64 nanoseconds) {
		Date output = {};
		const u64 totalSeconds = nanoseconds / 1000000000UL;
		const u64 days = totalSeconds / 86400UL;
		const u32 revDays = 303210U - (u32)days;
		const u32 revCenturies = (u32)(((u64)revDays * 470369U) >> 34U);
		const u32 revJulianDays = revDays + revCenturies - (revCenturies >> 2U);
		const u64 yearProduct = (u64)revJulianDays * 11758980U;
		const u32 marchYear = 2799U - (u32)(yearProduct >> 32U);
		const u32 revYearPos = (u32)(((u64)(u32)yearProduct * 782432U) >> 32U);
		const u32 isJanOrFeb = revYearPos < 126464U;
		const u32 monthDay = (marchYear & 3U) * 512U + (isJanOrFeb ? 191360U : 977792U) - revYearPos;
		output.year = marchYear + isJanOrFeb;
		output.month = (u8)(monthDay >> 16U);
		output.day = (u8)((((u64)(u16)monthDay * 2006994U) >> 32U) + 1U);
		return output;
	}

	// Maybe good idea to define everything up to week as seconds?
	// Like % (7 * 24 * 60 * 60 = 604800 secs)
	// Then we have year, week and weeksecond
	struct DateFull {
		u32 year;
		u8 month, week, day, hour, minute, second;
		u16 millisecond, microsecond, nanosecond;
	};

	ATTR(static_inl, const)
	DateFull get_calendar_time_full(u64 nanoseconds) {
		DateFull output = {};
		const Date date = get_calendar_time(nanoseconds);
		const u64 totalSeconds = nanoseconds / 1000000000ULL;
		const u32 secondsInDay = (u32)(totalSeconds % 86400ULL);
		const u32 nanosecondsInSecond = (u32)(nanoseconds % 1000000000ULL);
		const u32 hoursInDay = (u32)(((u64)secondsInDay * 1193047U) >> 32U);
		const u32 minutesInDay = (u32)(((u64)secondsInDay * 71582789U) >> 32U);
		const u32 milliseconds = nanosecondsInSecond / 1000000U;
		const u32 microsecondsInSecond = nanosecondsInSecond / 1000U;
		output.year = date.year;
		output.month = date.month;
		output.day = date.day;
		output.hour = (u8)hoursInDay;
		output.minute = (u8)((minutesInDay + hoursInDay * 4U) & 63U);
		output.second = (u8)((secondsInDay + minutesInDay * 4U) & 63U);
		output.millisecond = (u16)milliseconds;
		output.microsecond = (u16)(microsecondsInSecond - milliseconds * 1000U);
		output.nanosecond = (u16)(nanosecondsInSecond - microsecondsInSecond * 1000U);
		return output;
	}
};	// ==== Clock End =======================================================

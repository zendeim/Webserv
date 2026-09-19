#pragma once
#include <cstddef>
#include <stdint.h>
#include <climits>
#include "core_types.hpp"

// New Keywords
#define restrict			__restrict__
#define static_inl			static inline
#define offsetof(t, d)		__builtin_offsetof(t, d)

#define ATTR(kind, ...) ATTR_##kind __attribute__((__VA_ARGS__))
#define ATTR_none
#define ATTR_noinl __attribute__((noinline))
#define ATTR_inl inline __attribute__((always_inline))
#define ATTR_static static
#define ATTR_static_inl static inline __attribute__((always_inline))

// Wrapped keywords
#if defined(__cplusplus) && __cplusplus >= 201103L
	#define STATIC_ASSERT(expr) static_assert((expr), #expr)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
	#define STATIC_ASSERT(expr) _Static_assert((expr), #expr)
#else
	#define STATIC_ASSERT(expr) typedef char JOIN_MACROS(static_assert_failed_, __LINE__)[(expr) ? 1 : -1]
#endif

#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]))
#define ARRAY_END(arr)		(&(arr)[ARRAY_SIZE(arr)])
// always_inline, noinline, packed, aligned(n), cold, hot
// const: Function depends only on its arguments (doesn't read from memory)
// pure: Function produces no observable side effects (may read from memory)
// flatten: Function calls inside this function are aggressively inlined
// __attribute__((section("compressed")))

#define UNREACHABLE()	__builtin_unreachable()
#define LIKELY(x)		__builtin_expect(!!(x), 1)
#define UNLIKELY(x)		__builtin_expect(!!(x), 0)

#if defined(__clang__)
	#define ASSUME(x)	__builtin_assume(x)
#elif defined(__GNUC__)
	#define ASSUME(x) ((x) ? (void)0 : __builtin_unreachable())
#endif

// Defines
#define WORD_SIZE	sizeof(size_t)
#define WORD_BITS	(WORD_SIZE * CHAR_BIT)

// Macro Helpers
#define JOIN_MACROS_(a, b) a##b
#define JOIN_MACROS(a, b) JOIN_MACROS_(a, b)
#define STRINGIFY_(x)		#x
#define STRINGIFY(x)		STRINGIFY_(x)

#define PRINT_LN(fd, str)		((void)!::write(fd, str "\n", sizeof(str)))
#define PERR_RETURN(value, str)	return (PRINT_LN(2, str), (value))
#define PERR_EXIT(value, str)	_exit((PRINT_LN(2, str), (value)))

#ifdef DEBUG_MODE
	#define ON_DEBUG(x) (x)
	#define ASSERT(x, str) ((x) != 0 ? (void)0 : (PRINT_LN(2, str), __builtin_trap()))
#else
	#define ON_DEBUG(x) ((void)0)
	#define ASSERT(x, str) ((void)0)
#endif

#define ENFORCE(x, str) (ASSERT(x, str), ASSUME(x))

#include "core_builtins.ipp"
#include "core_macros.ipp"
#include "core_info.ipp"

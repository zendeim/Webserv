#pragma once
#include "core.hpp"

// === Generic Helpers =====================================
#define SWAP(a, b) 			({__typeof__(a) _swap_tmp_ = (a); (a) = (b); (b) = _swap_tmp_; (void)0;})
#define ALIGN_UP(x, a)		(((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a)	((x) & ~((a) - 1))
#define IS_POW2(x)			((x) != 0 && ((x) & ((x) - 1)) == 0)
#define NEXT_POW2(x)		((__typeof__(x))((usize)1 << ((sizeof(x) * 8) - (usize)CLZ(x))))

// === Bit Helpers =========================================
// TODO: Get ranged versions, like start, end
#define BITREAD(word, index)	(((word) >> (index)) & 1)
#define BITSET(word, index)		((word) |= ((__typeof__(word))1 << (index)))
#define BITFLIP(word, index)	((word) ^= ((__typeof__(word))1 << (index)))
#define BITCLR(word, index)		((word) &= ~((__typeof__(word))1 << (index)))

// === MINMAX Helpers ========================================
#define MIN(x, y)			((x) < (y) ? (x) : (y))
#define MAX(x, y)			((x) > (y) ? (x) : (y))
#define ABS(x)				((x) > 0 ? (x) : -(x))
#define CLAMP(x, low, high)	MAX(low, MIN(x, high))

#define ABSMAX(x, y)		MAX(ABS(x), ABS(y))
#define ABSMIN(x, y)		MIN(ABS(x), ABS(y))
#define ABSDIFF(x, y)		(MAX(x, y) - MIN(x, y))

#define MIN3(x, y, z)		MIN(x, MIN(y, z))
#define MAX3(x, y, z)		MAX(x, MAX(y, z))
#define MIN4(x, y, z, w)	MIN(MIN(x, y), MIN(z, w))
#define MAX4(x, y, z, w)	MAX(MAX(x, y), MAX(z, w))

#define ABSMIN3(x, y, z)	MIN3(ABS(x), ABS(y), ABS(z))
#define ABSMAX3(x, y, z)	MAX3(ABS(x), ABS(y), ABS(z))
#define ABSMIN4(x, y, z, w)	MIN4(ABS(x), ABS(y), ABS(z), ABS(w))
#define ABSMAX4(x, y, z, w)	MAX4(ABS(x), ABS(y), ABS(z), ABS(w))

// === ASCII Helpers =======================================
#define IS_ASCII(c) ((c) >= 0 && (c) < 128)
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_UPPER(c) ((c) >= 'A' && (c) <= 'Z')
#define IS_LOWER(c) ((c) >= 'a' && (c) <= 'z')
#define IS_ALPHA(c) (IS_LOWER(c | 32))
// #define IS_SPACE(c) (((c) == ' ') || ((unsigned char)(c) - (unsigned char)'\t' < 5u))	// LUTLESS version
#define IS_SPACE(c)	(gAsciiLut[((unsigned char)(c))] == ASCII_SPACE)
#define IS_HEX(c)	(gAsciiLut[((unsigned char)(c))] <= ASCII_HEX)
#define IS_ALNUM(c) (gAsciiLut[((unsigned char)(c))] <= ASCII_LETTERS)
#define IS_IDENT(c)	(gAsciiLut[((unsigned char)(c))] <= ASCII_IDENT)

enum e_ascii {
	ASCII_DIGITS      = 9u,   // 0-9 map to 0-9
	ASCII_HEX         = 15u,  // A-F / a-f map to 10-16
	ASCII_LETTERS     = 35u,  // A-Z / a-z map to 10-35
	ASCII_IDENT       = 36u,  // 0-36: (_)
	ASCII_URL_VALID   = 39u,  // 0-39: (-) (.) (~)
	ASCII_RFC_SYMBOLS = 54u,  // 0-54: (!) ($) (%) (() ()) (*) (+) (,) (/) (:) (;) (=) (@) (&) (')
	ASCII_HTML_ESCAPE = 57u,  // 53-57: (&) (') (") (<) (>)
	ASCII_SYMBOLS     = 67u,  // 36-67  (#) (?) ([) (\) (]) (^) (`) ({) (|) (})
	ASCII_SPACE       = 98u,
	ASCII_CONTROL     = 99u,
	ASCII_INVALID     = 255u,
	ASCII_HTML_ESCAPE_START = 53u	// Used in the HTML_ESCAPE lut
};

// 36 (_)  37 (-)  38 (.)  39 (~)  40 (!)  41 ($)  42 (%)  43 (()
// 44 ())  45 (*)  46 (+)  47 (,)  48 (/)  49 (:)  50 (;)  51 (=)
// 52 (@)  53 (&)  54 (')  55 (")  56 (<)  57 (>)  58 (#)  59 (?)
// 60 ([)  61 (\)  62 (])  63 (^)  64 (`)  65 ({)  66 (|)  67 (})

// Tables
#ifdef MAIN_FILE
	const u8 gAsciiLut[256] = {
		255, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 99, 99, // 0x00-0x0F
		99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, // 0x10-0x1F
		//     SP   !   "   #   $   %   &   '   (   )   *   +   ,   -   .   /
		/*20*/ 98, 40, 55, 58, 41, 42, 53, 54, 43, 44, 45, 46, 47, 37, 38, 48,
		//      0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?
		/*30*/  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 49, 50, 56, 51, 57, 59,
		//      @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O
		/*99*/ 52, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
		//      P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _
		/*50*/ 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 60, 61, 62, 63, 36,
		//      `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
		/*60*/ 64, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
		//      p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~  DEL
		/*70*/ 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 65, 66, 67, 39, 99,

		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	};
#else
	extern const u8 gAsciiLut[256];
#endif

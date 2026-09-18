#pragma once

#include "core.hpp"
#include "Span.hpp"
#include "Status_tables.inl"

/*
	Status stores a u16 offset into its static string catalog.  Every
	status record is length-prefixed and NUL-terminated.  Error records append
	a length-prefixed, NUL-terminated default page immediately after the status.

	Index 1 is unset and index 2 is invalid
*/
struct Status {
	static const usize errorPageCount = 32 + 12;
	static inline char strings[] = HTTP_STATUS_STRINGS;
	u16 index;

	#pragma push_macro("SUBS")
	#pragma push_macro("SUBP")
	#undef SUBS
	#undef SUBP

	#define SUBS(code, next) (i##next - sizeof(HTTP_STATUS(code)) - 1)
	#define SUBP(code, next) (i##next - sizeof(HTTP_STATUS(code)) \
		- sizeof(HTTP_STATUS_DEFAULT_PAGE(code)) - 2)

	enum Code {
		ok = 0, unset = 1, ixxx = 2, i511 = sizeof(HTTP_STATUS_STRINGS) - sizeof(HTTP_STATUS(511))
			- sizeof(HTTP_STATUS_DEFAULT_PAGE(511)) - 2,
		i510 = SUBP(510, 511), i508 = SUBP(508, 510), i507 = SUBP(507, 508), i506 = SUBP(506, 507),
		i505 = SUBP(505, 506), i504 = SUBP(504, 505), i503 = SUBP(503, 504), i502 = SUBP(502, 503),
		i501 = SUBP(501, 502), i500 = SUBP(500, 501), i431 = SUBP(431, 500), i429 = SUBP(429, 431),
		i428 = SUBP(428, 429), i426 = SUBP(426, 428), i425 = SUBP(425, 426), i424 = SUBP(424, 425),
		i423 = SUBP(423, 424), i422 = SUBP(422, 423), i421 = SUBP(421, 422), i418 = SUBP(418, 421),
		i417 = SUBP(417, 418), i416 = SUBP(416, 417), i415 = SUBP(415, 416), i414 = SUBP(414, 415),
		i413 = SUBP(413, 414), i412 = SUBP(412, 413), i411 = SUBP(411, 412), i410 = SUBP(410, 411),
		i409 = SUBP(409, 410), i408 = SUBP(408, 409), i407 = SUBP(407, 408), i406 = SUBP(406, 407),
		i405 = SUBP(405, 406), i404 = SUBP(404, 405), i403 = SUBP(403, 404), i402 = SUBP(402, 403),
		i401 = SUBP(401, 402), i400 = SUBP(400, 401), i308 = SUBS(308, 400), i307 = SUBS(307, 308),
		i306 = SUBS(306, 307), i305 = SUBS(305, 306), i304 = SUBS(304, 305), i303 = SUBS(303, 304),
		i302 = SUBS(302, 303), i301 = SUBS(301, 302), i300 = SUBS(300, 301), i226 = SUBS(226, 300),
		i208 = SUBS(208, 226), i207 = SUBS(207, 208), i206 = SUBS(206, 207), i205 = SUBS(205, 206),
		i204 = SUBS(204, 205), i203 = SUBS(203, 204), i202 = SUBS(202, 203), i201 = SUBS(201, 202),
		i200 = SUBS(200, 201), i104 = SUBS(104, 200), i103 = SUBS(103, 104), i102 = SUBS(102, 103),
		i101 = SUBS(101, 102), i100 = SUBS(100, 101)
	};

	// enum Test {
	// 	aaa = ((void)"asdasdsa", 1),
	// };
	#pragma pop_macro("SUBP")
	#pragma pop_macro("SUBS")

	ATTR(static_inl, pure) u16 s_index(usize div, usize rem) {
		static const u16 offsets[160] = {
			i100, i101, i102, i103, i104, ixxx, ixxx, ixxx,
			ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx,
			ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx,
			ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx,
			i200, i201, i202, i203, i204, i205, i206, i207,
			i208, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx,
			ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx,
			ixxx, ixxx, i226, ixxx, ixxx, ixxx, ixxx, ixxx,
			i300, i301, i302, i303, i304, i305, i306, i307,
			i308, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx,
			ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx,
			ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx,
			i400, i401, i402, i403, i404, i405, i406, i407,
			i408, i409, i410, i411, i412, i413, i414, i415,
			i416, i417, i418, ixxx, ixxx, i421, i422, i423,
			i424, i425, i426, ixxx, i428, i429, ixxx, i431,
			i500, i501, i502, i503, i504, i505, i506, i507,
			i508, ixxx, i510, i511, ixxx, ixxx, ixxx, ixxx,
			ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx,
			ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx, ixxx
		};
		if (rem >= 32)
			return (u16)ixxx;
		return offsets[div * 32 + rem];
	}

	ATTR(static_inl, const) u16 s_num_to_code(usize number) {
		if (number - 100 >= 500)
			return ixxx;
		const usize div = number / 100;
		return s_index(div - 1, number - div * 100);
	}

	ATTR(static_inl, pure) Code s_str_to_code(const char* str) {
		if (str[0] < '1' || str[0] > '5' ||
			str[1] < '0' || str[1] > '9' ||
			str[2] < '0' || str[2] > '9')
				return ixxx;

		const usize div = (usize)(str[0] - '1');
		const usize rem = 10 * (usize)(str[1] - '0') + (usize)(str[2] - '0');
		return (Code)s_index(div, rem);
	}

	ATTR(static_inl, pure) usize s_code_to_index(Code code) {
		char* ptr = strings + (usize)code;
		usize first = (u8)(ptr[0] - '0');
		usize second = (u8)(ptr[1] - '0');
		usize third = (u8)(ptr[2] - '0');
		return first * 32 + second * 10 + third;
	}

	ATTR(inl, pure) usize get_page_index() const {
		if (index < Status::i400)
			return SIZE_MAX;
		return s_code_to_index((Code)index) - (32ul * 4);
	}

	ATTR(static_inl, pure) usize s_get_page_index(Code code) {
		if (code < Status::i400)
			return SIZE_MAX;
		return s_code_to_index(code) - (32ul * 4);
	}

	ATTR(inl, pure) Span status_str() const {
		Span result;
		result.ptr = strings + (usize)index;
		result.size = (u8)result.ptr[-1];
		return result;
	}

	ATTR(static_inl, pure) Span s_status_str(Status::Code code) {
		Span result;
		result.ptr = strings + (u16)code;
		result.size = (u8)result.ptr[-1];
		return result;
	}

	ATTR(inl, pure) Span error_page() const {
		Span result;
		result.ptr = strings + (usize)index;
		result.size = (u8)result.ptr[-1];

		result.ptr += result.size + 2;
		result.size = (u8)result.ptr[-1];
		return result;
	}

	ATTR(static_inl, pure) Span s_error_page(usize number) {
		const usize offset = s_index(3 + (number >= 32), number - (number >= 32 ? 32 : 0));

		Span tmp;
		tmp.ptr = strings + offset;
		tmp.size = (u8)tmp.ptr[-1];
		tmp.ptr += tmp.size + 2;
		tmp.size = (u8)tmp.ptr[-1];
		return tmp;
	}

	ATTR(static_inl, pure) Span s_error_page(Status::Code code) {
		Span result;
		result.ptr = strings + (u16)code;
		result.size = (u8)result.ptr[-1];

		result.ptr += result.size + 2;
		result.size = (u8)result.ptr[-1];
		return result;
	}

	ATTR(inl) void clear() {
		index = unset;
	}

	// Utilities
	ATTR(inl, pure) usize number() const {
		const char* str = status_str().ptr;
		usize number = 100 * (usize)(str[0] - '0');
		number += 10 * (usize)(str[1] - '0');
		number += (usize)(str[2] - '0');
		return number;
	}

	ATTR(inl, pure) bool is_valid() const { return index > ixxx; }
	ATTR(inl, pure) bool is_informational() const { return index >= i100 && index < i200; }
	ATTR(inl, pure) bool is_success() const { return index >= i200 && index < i300; }
	ATTR(inl, pure) bool is_redirect() const { return index >= i300 && index < i400; }
	ATTR(inl, pure) bool is_client_error() const { return index >= i400 && index < i500; }
	ATTR(inl, pure) bool is_server_error() const { return index >= i500; }
	ATTR(inl, pure) bool is_error() const { return index >= i400; }
	ATTR(inl, pure) bool is_set() const { return index != unset; }

	ATTR(inl, pure) bool operator==(Code code) const { return index == (u16)code; }
	ATTR(inl, pure) bool operator!=(Code code) const { return index != (u16)code; }
};

STATIC_ASSERT(sizeof(HTTP_STATUS_STRINGS) <= UINT16_MAX);
STATIC_ASSERT(sizeof(Status) == sizeof(u16));

#pragma once

#include "core.hpp"
#include "Span.hpp"
/*
	My magnum opus, the most pointless optimization of all

	It converts any status to a linear index in just 18 lines of branchless assembly, and
	the lookup table is sized exactly to a cache line (64 bytes). It does so by interpreting each number
	between 100-512 as a bit, then to see if a number is valid, check against the LUT bitmap. To get its
	linear index, sum with the sizes lut (that precomputes the POPCOUNT for all previous entries), mask
	the bits higher than it, and get POPCOUNT of the current entry
*/
struct Status {
	enum Code : u8;	// Defined after the implementations

	ATTR(static_inl, pure)
	Code num_to_status(usize number) {
		alignas(64) struct {
			u64 bitmap[7] = {
				0b0000000000000000000000000000000000000000000000000000000000011111, // 100-163
				0b0100000000000000000111111111000000000000000000000000000000000000, // 164-227
				0b0000000000000000000000000000000000000000000000000000000000000000, // 228-291
				0b0000000000000000000000000000000000000000000000011111111100000000, // 292-355
				0b0111111111111111111100000000000000000000000000000000000000000000, // 356-419
				0b0000000000000000000000000000000010000000000000000000101101111110, // 420-483
				0b0000000000000000000000000000000000001101111111110000000000000000, // 484-547
			};
			u8 prevPopSum[8] = {1, 6, 16, 16, 25, 44, 54, 0}; // Cumulative popcount previous of LUT entries
		}	static const lut;

		number -= 100;
		usize div = (number < 412) ? number / 64 : 2;
		usize rem = number % 64;		// rem is the bit index within that LUT segment
		u64 word = lut.bitmap[div];
		u64 bit = 1ull << rem;
		u8 result = POPCOUNT(word & (bit - 1)) + lut.prevPopSum[div];
		return (word & bit) ? (Code)result : invalid;
	}

	ATTR(static_inl, pure)
	Code str_to_status(const char* str) {
		if (str[0] < '1' || str[0] > '5' ||
			str[1] < '0' || str[1] > '5' ||
			str[2] < '0' || str[2] > '9')
				return invalid;

		usize number = (usize)(str[0] - '0') * 100;
		number += 10 * (usize)(str[1] - '0') + (usize)(str[2] - '0');
		return num_to_status(number);
	}

	ATTR(static_inl, pure)
	Span get_status_str(Code code) {
		ASSERT(is_valid(code), "Invalid status code");
		const usize index = code - i100;
		return {(char*)(strings[index] + 1), (u8)strings[index][0]};
	}

	ATTR(static_inl, const)
	usize get_page_index(Code code) {
		ASSERT(is_error(code), "Status code is not an error");
		return code - i400;
	}

	ATTR(static_inl, pure)
	Span get_status_page(Code code) {
		const usize index = get_page_index(code);
		return {(char*)(pages[index] + 1), (u8)pages[index][0]};
	}

	ATTR(static_inl, const) bool is_valid(Code idx) { return idx >= i100 && idx <= i511; }
	ATTR(static_inl, const) bool is_informational(Code idx) { return idx >= i100 && idx < i200; }
	ATTR(static_inl, const) bool is_success(Code idx) { return idx >= i200 && idx < i300; }
	ATTR(static_inl, const) bool is_redirect(Code idx) { return idx >= i300 && idx < i400; }
	ATTR(static_inl, const) bool is_client_error(Code idx) { return idx >= i400 && idx < i500; }
	ATTR(static_inl, const) bool is_server_error(Code idx) { return idx >= i500 && idx <= i511; }
	ATTR(static_inl, const) bool is_error(Code idx) { return idx >= i400 && idx <= i511; }
	ATTR(static_inl, const) bool is_set(Code idx) { return idx != unset; }

	// The (void) strs are just so that upon hovering on the enum, it shows you the associated string
	enum Code : u8 {
		unset = 0, ok = 127, invalid = 255,
		i100 = ((void)"100 Continue", 1),
		i101 = ((void)"101 Switching Protocols", 2),
		i102 = ((void)"102 Processing", 3),
		i103 = ((void)"103 Early Hints", 4),
		i104 = ((void)"104 Upload Resumption Supported", 5),
		i200 = ((void)"200 OK", 6),
		i201 = ((void)"201 Created", 7),
		i202 = ((void)"202 Accepted", 8),
		i203 = ((void)"203 Non-Authoritative Information", 9),
		i204 = ((void)"204 No Content", 10),
		i205 = ((void)"205 Reset Content", 11),
		i206 = ((void)"206 Partial Content", 12),
		i207 = ((void)"207 Multi-Status", 13),
		i208 = ((void)"208 Already Reported", 14),
		i226 = ((void)"226 IM Used", 15),
		i300 = ((void)"300 Multiple Choices", 16),
		i301 = ((void)"301 Moved Permanently", 17),
		i302 = ((void)"302 Found", 18),
		i303 = ((void)"303 See Other", 19),
		i304 = ((void)"304 Not Modified", 20),
		i305 = ((void)"305 Use Proxy", 21),
		i306 = ((void)"306 (Unused)", 22),
		i307 = ((void)"307 Temporary Redirect", 23),
		i308 = ((void)"308 Permanent Redirect", 24),
		i400 = ((void)"400 Bad Request", 25),
		i401 = ((void)"401 Unauthorized", 26),
		i402 = ((void)"402 Payment Required", 27),
		i403 = ((void)"403 Forbidden", 28),
		i404 = ((void)"404 Not Found", 29),
		i405 = ((void)"405 Method Not Allowed", 30),
		i406 = ((void)"406 Not Acceptable", 31),
		i407 = ((void)"407 Proxy Authentication Required", 32),
		i408 = ((void)"408 Request Timeout", 33),
		i409 = ((void)"409 Conflict", 34),
		i410 = ((void)"410 Gone", 35),
		i411 = ((void)"411 Length Required", 36),
		i412 = ((void)"412 Precondition Failed", 37),
		i413 = ((void)"413 Content Too Large", 38),
		i414 = ((void)"414 URI Too Long", 39),
		i415 = ((void)"415 Unsupported Media Type", 40),
		i416 = ((void)"416 Range Not Satisfiable", 41),
		i417 = ((void)"417 Expectation Failed", 42),
		i418 = ((void)"418 (Unused)", 43),
		i421 = ((void)"421 Misdirected Request", 44),
		i422 = ((void)"422 Unprocessable Content", 45),
		i423 = ((void)"423 Locked", 46),
		i424 = ((void)"424 Failed Dependency", 47),
		i425 = ((void)"425 Too Early", 48),
		i426 = ((void)"426 Upgrade Required", 49),
		i428 = ((void)"428 Precondition Required", 50),
		i429 = ((void)"429 Too Many Requests", 51),
		i431 = ((void)"431 Request Header Fields Too Large", 52),
		i451 = ((void)"451 Unavailable For Legal Reasons", 53),
		i500 = ((void)"500 Internal Server Error", 54),
		i501 = ((void)"501 Not Implemented", 55),
		i502 = ((void)"502 Bad Gateway", 56),
		i503 = ((void)"503 Service Unavailable", 57),
		i504 = ((void)"504 Gateway Timeout", 58),
		i505 = ((void)"505 HTTP Version Not Supported", 59),
		i506 = ((void)"506 Variant Also Negotiates", 60),
		i507 = ((void)"507 Insufficient Storage", 61),
		i508 = ((void)"508 Loop Detected", 62),
		i510 = ((void)"510 Not Extended", 63),
		i511 = ((void)"511 Network Authentication Required", 64),
	};

	static inline const char strings[64][37] = {
		"\x0C" "100 Continue",
		"\x17" "101 Switching Protocols",
		"\x0E" "102 Processing",
		"\x0F" "103 Early Hints",
		"\x1F" "104 Upload Resumption Supported",
		"\x06" "200 OK",
		"\x0B" "201 Created",
		"\x0C" "202 Accepted",
		"\x21" "203 Non-Authoritative Information",
		"\x0E" "204 No Content",
		"\x11" "205 Reset Content",
		"\x13" "206 Partial Content",
		"\x10" "207 Multi-Status",
		"\x14" "208 Already Reported",
		"\x0B" "226 IM Used",
		"\x14" "300 Multiple Choices",
		"\x15" "301 Moved Permanently",
		"\x09" "302 Found",
		"\x0D" "303 See Other",
		"\x10" "304 Not Modified",
		"\x0D" "305 Use Proxy",
		"\x0C" "306 (Unused)",
		"\x16" "307 Temporary Redirect",
		"\x16" "308 Permanent Redirect",
		"\x0F" "400 Bad Request",
		"\x10" "401 Unauthorized",
		"\x14" "402 Payment Required",
		"\x0D" "403 Forbidden",
		"\x0D" "404 Not Found",
		"\x16" "405 Method Not Allowed",
		"\x12" "406 Not Acceptable",
		"\x21" "407 Proxy Authentication Required",
		"\x13" "408 Request Timeout",
		"\x0C" "409 Conflict",
		"\x08" "410 Gone",
		"\x13" "411 Length Required",
		"\x17" "412 Precondition Failed",
		"\x15" "413 Content Too Large",
		"\x10" "414 URI Too Long",
		"\x1A" "415 Unsupported Media Type",
		"\x19" "416 Range Not Satisfiable",
		"\x16" "417 Expectation Failed",
		"\x0C" "418 (Unused)",
		"\x17" "421 Misdirected Request",
		"\x19" "422 Unprocessable Content",
		"\x0A" "423 Locked",
		"\x15" "424 Failed Dependency",
		"\x0D" "425 Too Early",
		"\x14" "426 Upgrade Required",
		"\x19" "428 Precondition Required",
		"\x15" "429 Too Many Requests",
		"\x23" "431 Request Header Fields Too Large",
		"\x21" "451 Unavailable For Legal Reasons",
		"\x19" "500 Internal Server Error",
		"\x13" "501 Not Implemented",
		"\x0F" "502 Bad Gateway",
		"\x17" "503 Service Unavailable",
		"\x13" "504 Gateway Timeout",
		"\x1E" "505 HTTP Version Not Supported",
		"\x1B" "506 Variant Also Negotiates",
		"\x18" "507 Insufficient Storage",
		"\x11" "508 Loop Detected",
		"\x10" "510 Not Extended",
		"\x23" "511 Network Authentication Required",
	};

	#define HTTP_PAGE(code, reason) \
		"<title>" code "</title>" \
		"<h1>" code " " reason "</h1>"

	static inline const char pages[40][64] = {
		"\x2A" HTTP_PAGE("400", "Bad Request"),
		"\x2B" HTTP_PAGE("401", "Unauthorized"),
		"\x2F" HTTP_PAGE("402", "Payment Required"),
		"\x28" HTTP_PAGE("403", "Forbidden"),
		"\x28" HTTP_PAGE("404", "Not Found"),
		"\x31" HTTP_PAGE("405", "Method Not Allowed"),
		"\x2D" HTTP_PAGE("406", "Not Acceptable"),
		"\x3C" HTTP_PAGE("407", "Proxy Authentication Required"),
		"\x2E" HTTP_PAGE("408", "Request Timeout"),
		"\x27" HTTP_PAGE("409", "Conflict"),
		"\x23" HTTP_PAGE("410", "Gone"),
		"\x2E" HTTP_PAGE("411", "Length Required"),
		"\x32" HTTP_PAGE("412", "Precondition Failed"),
		"\x30" HTTP_PAGE("413", "Content Too Large"),
		"\x2B" HTTP_PAGE("414", "URI Too Long"),
		"\x35" HTTP_PAGE("415", "Unsupported Media Type"),
		"\x34" HTTP_PAGE("416", "Range Not Satisfiable"),
		"\x31" HTTP_PAGE("417", "Expectation Failed"),
		"\x27" HTTP_PAGE("418", "(Unused)"),
		"\x32" HTTP_PAGE("421", "Misdirected Request"),
		"\x34" HTTP_PAGE("422", "Unprocessable Content"),
		"\x25" HTTP_PAGE("423", "Locked"),
		"\x30" HTTP_PAGE("424", "Failed Dependency"),
		"\x28" HTTP_PAGE("425", "Too Early"),
		"\x2F" HTTP_PAGE("426", "Upgrade Required"),
		"\x34" HTTP_PAGE("428", "Precondition Required"),
		"\x30" HTTP_PAGE("429", "Too Many Requests"),
		"\x3E" HTTP_PAGE("431", "Request Header Fields Too Large"),
		"\x3C" HTTP_PAGE("451", "Unavailable For Legal Reasons"),
		"\x34" HTTP_PAGE("500", "Internal Server Error"),
		"\x2E" HTTP_PAGE("501", "Not Implemented"),
		"\x2A" HTTP_PAGE("502", "Bad Gateway"),
		"\x32" HTTP_PAGE("503", "Service Unavailable"),
		"\x2E" HTTP_PAGE("504", "Gateway Timeout"),
		"\x39" HTTP_PAGE("505", "HTTP Version Not Supported"),
		"\x36" HTTP_PAGE("506", "Variant Also Negotiates"),
		"\x33" HTTP_PAGE("507", "Insufficient Storage"),
		"\x2C" HTTP_PAGE("508", "Loop Detected"),
		"\x2B" HTTP_PAGE("510", "Not Extended"),
		"\x3E" HTTP_PAGE("511", "Network Authentication Required"),
	};
	#undef HTTP_PAGE
	static const usize errorPageCount = ARRAY_SIZE(pages);
};	// ====\ Status End =======================================================

#pragma once
#include <unistd.h>
#include <dirent.h>

#include "core.hpp"
#include "webserv.hpp"
#include "Span.hpp"
#include "pure_functions.hpp"
#include "Clock.hpp"
#include <errno.h>

#define BUFFER_INL(retType) \
	template <usize bufferSize> inline retType Buffer<bufferSize>::

#define BUFFER_INL_T(tmplParam, retType) \
	template <usize bufferSize> template <tmplParam> inline retType Buffer<bufferSize>::

template <usize bufferSize>
struct Buffer {
	u64 clobberPre;
	u8 data[bufferSize - 16 - (3 * sizeof(usize))];	// Trailing storage pads unbounded memory loads
	u64 clobberPost;
	usize writePos, readPos, scanPos;

	ATTR(inl, pure) Span get_span() { return {(char*)data + readPos, writePos - readPos}; }
	ATTR(inl, pure) usize size() const { return writePos - readPos; }
	ATTR(inl, pure) usize capacity() const { return sizeof(data); }
	ATTR(inl, pure) char* rptr() { return (char*)(data + readPos); }
	ATTR(inl, pure) char* wptr() { return (char*)(data + writePos); }
	ATTR(inl, pure) char* sptr() { return (char*)(data + scanPos); }
	ATTR(inl, pure) char* eptr() { return (char*)(data + sizeof(data)); }
	ATTR(inl, pure) operator char*() { return (char*)(data); }
	ATTR(inl, pure) u8& operator*() { return data[writePos]; }

	ATTR(inl) usize reserve(usize bytes) {
		usize bytesFree = sizeof(data) - writePos;
		if (bytes >= bytesFree)
			bytesFree = compact();
		return bytesFree;
	}

	ATTR(inl) void clear() {
		writePos = 0;
		readPos = 0;
		scanPos = 0;
	}

	ATTR(inl) void init(usize newWritePos, usize newReadPos, usize newScanPos) {
		writePos = newWritePos;
		readPos = newReadPos;
		scanPos = newScanPos;
	}

	// IO
	usize compact();
	isize read_compact(int fd, usize bytes);
	isize read(int fd, usize bytes);
	isize write(int fd, usize bytes);
	isize write_all(int fd, usize bytes);
	char* readdir(int fd);

	// HTTP
	Status::Code dechunk(Buffer& tmp, usize& chunkSize, usize& bodySize);

	// Search
	Span find_line_end();
	Span find_header_end();
	Span find_cgi_line_end();
	Span find_cgi_header_end();
	Span find_char(u8 c);

	bool skip_spaces();
	Span get_field_value(usize readEnd);

	// Appends and Prepends
	template <usize N> Span append(const char (&string)[N]);				// Implicit
	template <usize N> Span append_inline(const char* ptr, usize length);	// Explicit
	Span append(const char* ptr, usize length);
	Span append(const Span& span);

	template <usize N> Span prepend(const char (&string)[N]);
	template <usize N> Span prepend_inline(const char* ptr, usize length);
	Span prepend(const char* ptr, usize length);
	Span prepend(const Span& span);

	// Append Special
	Span append_mime(Mime mimeIndex);
	Span append_digit10(usize number);
	Span append_digit16(usize number);
	Span append_url_encoded(const char* ptr, usize length);
	Span append_html_encoded(char* ptr, usize length);
	Span append_entry(int directoryFd, char* name);
	Span append_path_resolved(Span root, Span relativeTarget);
	Span memset(u8 byte, usize length);
	template <usize N> Span memset_inline(u8 byte, usize length);
};

typedef Buffer<8 * 1024ul> Buffer8;
typedef Buffer<16 * 1024ul> Buffer16;
typedef Buffer<32 * 1024ul> Buffer32;
typedef Buffer<64 * 1024ul> Buffer64;
typedef Buffer<HTTP_BUFFERSIZE> HTTP_Buffer;
typedef Buffer<2 * HTTP_BUFFERSIZE - 256> HTTP_PBuffer;

#include "Buffer_io.ipp"
#include "Buffer_add.ipp"
#include "Buffer_add_special.ipp"
#include "Buffer_search.ipp"
#include "Buffer_http.ipp"

#pragma once
#include <unistd.h>

#include "core.hpp"

struct Span {
	char* ptr;
	usize size;

	ATTR(inl, pure) char* end() const { return ptr + size; }

	template <usize length>
	ATTR(inl, pure) bool operator==(const char (&literal)[length]) const {
		return size == length - 1 && LITCMP(ptr, literal) == 0;
	}

	template <usize length>
	ATTR(inl, pure) bool strcasecmp(const char (&string)[length]) const {
		u8 buffer[length];
		const usize strLength = length - 1;
		if (size != strLength)	// TODO: remove this
			return false;

		MEMCPY_INLINE(buffer, ptr, strLength);
		for (usize i = 0; i < strLength; i++)
			buffer[i] |= 32;
		return MEMCMP(buffer, string, strLength) == 0;
	}

	template <usize length>
	ATTR(static_inl, const) Span create(const char (&literal)[length]) {
		Span newSpan = {(char*)literal, length - 1};
		return newSpan;
	}

	ATTR(static_inl, const) Span create(char* srcPtr, usize srcSize) {
		Span newSpan = {srcPtr, srcSize};
		return newSpan;
	}

	ATTR(inl, pure) operator char*() const { return ptr; }
};

struct Span32 {
	u32 index;
	u32 length;
};

// A ZPtr could be an object that only contains its index
// The length is encoded in the location, so extracting is ptr = *zptr + 8, length = *zptr[8]
// But this doesn't save memory. It just shifts the size to the arena rather than the array

struct Span16 {
	u16 index;
	u16 size;
};

// Span extract(char* ptr) const {
// 	Span result;
// 	result.ptr = ptr + index;
// 	result.size = length;
// 	return result;
// }

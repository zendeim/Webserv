#pragma once
#include <unistd.h>

#include "core.hpp"
#include "Array.hpp"
#include "Span.hpp"

struct Arena {
	u8* ptr;
	usize size, capacity;

	ATTR(inl, pure) usize free_space() const { return capacity - size; }
	ATTR(inl, pure) u8* mptr(usize fileOffset) const { return ptr + fileOffset; }
	ATTR(inl, pure) const u8* kptr(usize fileOffset) const { return ptr + fileOffset; }

	ATTR(inl) void clear() { size = 0; }

	u32 alloc(usize bytes, usize padding = 0, usize alignment = 64) {
		ASSERT(IS_POW2(alignment), "Alignment needs to be power of two");
		bytes = ALIGN_UP(bytes + padding, alignment);
		usize newSize = ALIGN_UP(size, alignment);
		if (bytes + newSize > capacity) {
			PRINT_LN(2, "Error: Out of memory");
			return UINT32_MAX;
		}
		u32 index = newSize;
		size = newSize + bytes;
		return index;
	}

	template <typename Type>
	ArrayView<Type> alloc_array(usize numElements, usize alignSize = 16) {
		ASSERT(numElements <= LONG_MAX / sizeof(Type), "Array size exceeds LONG_MAX");
		const usize bytes = numElements * sizeof(Type);
		alignSize = MAX(alignSize, alignof(Type));
		const u32 index = alloc(bytes, 0, alignSize);
		if (index == UINT32_MAX)
			return {};
		return {(Type*)mptr(index), numElements};
	}

	Span alloc_span(usize length) {
		const u32 offset = alloc(length, 1);
		if (offset == UINT32_MAX)
			_exit(1);
		Span result = {(char*)mptr(offset), length};
		result.ptr[result.size] = '\0';
		return result;
	}

	Span copy_span(const Span& source) {
		Span result = alloc_span(source.size);
		MEMCPY(result.ptr, source.ptr, source.size);
		return result;
	}
};

#ifndef ALLOC_HPP
#define ALLOC_HPP

#include "stdlib.h"
#include "types.hpp"
#include "math.hpp"
#include "string.h"

static inline size_t align_forward(size_t i, size_t align) {
    return i + align - 1 & ~(align - 1);
}

static inline bool is_aligned(size_t i, size_t align) {
	return align_forward(i, align) == i;
}

//a simple zero-initialized bump-the-pointer allocator
//which maintains a singly linked list of previous memory blocks
//TODO: allow custom minimum block sizes
struct ArenaListAlloc {
	u8 * block;
	u8 * head;
	u8 * end;

	void * alloc(size_t size, size_t align) {
        assert(!(align & align - 1)); //align is power of two
        assert(!(size & align - 1)); //size is multiple of align

        u8 * ret = (u8 *) align_forward((size_t) head, align);

        if (ret + size > end) {
        	size_t prefix = imax(align, sizeof(u8 *));
        	size_t allocSize = imax(size + prefix, 1024*1024 * 1); //1 MB blocks for now

        	u8 * newBlock = (u8 *) malloc(allocSize);
        	*((u8 **) newBlock) = block; //store pointer to the old block

        	block = newBlock;
        	end = block + allocSize;
        	ret = block + prefix;
        }

        head = ret + size;
        return ret;
	}

	void * realloc(void * old, size_t oldSize, size_t newSize, size_t align) {
        assert(!(align & align - 1)); //align is power of two
        assert(!(newSize & align - 1)); //size is multiple of align

		//NOTE: we always reallocate if `old` is misaligned, otherwise only if newSize > oldSize
		//		this allows reallocating an existing allocation with a higher alignment
		//		I don't know when you'd need to do that, but hey, it's there
		if (oldSize >= newSize && is_aligned((size_t) old, align)) return old;

		void * ret = alloc(newSize, align);
		memcpy(ret, old, oldSize);
		return ret;
	}

	void finalize() {
		while (block) {
			u8 * prevBlock = *((u8 **) block);
			free(block);
			block = prevBlock;
		}
		*this = {};
	}
};

#endif //ALLOC_HPP

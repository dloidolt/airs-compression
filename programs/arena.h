/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief A simple, single-threaded, linear memory arena (bump allocator)
 *
 * @see for more details https://nullprogram.com/blog/2023/09/27/
 */

#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <assert.h>

#define ARENA_NEW(a, t)          ((t *)arena_alloc(a, 1, sizeof(t), __alignof__(t)))
#define ARENA_NEW_ARRAY(a, n, t) ((t *)arena_alloc(a, n, sizeof(t), __alignof__(t)))

struct arena {
	uint8_t *beg;
	uint8_t *end;
};

#include <stdlib.h>
static void __attribute__((noreturn)) oom(void)
{
	exit(3);
}


/**
 * @brief allocates a zero-initialized block of memory from the arena with specified alignment
 *
 * @param a	pointer to the arena
 * @param count	number of elements to allocate
 * @param size	size of each element
 * @param align	the desired alignment, must be a power of two
 *
 * @returns a pointer to the allocated memory; calls oom() on failure.
 */

static __inline void *arena_alloc(struct arena *a, ptrdiff_t count, ptrdiff_t size, ptrdiff_t align)
{
	ptrdiff_t padding, available;
	uint8_t *r;

	assert(count >= 0);
	assert(size > 0);
	assert(align > 0 && (align & (align - 1)) == 0 && "Alignment must be a power of two");

	padding = (ptrdiff_t)-(size_t)a->beg & (align - 1);
	available = a->end - a->beg - padding;
	if (available < 0 || count > available / size)
		oom();

	r = a->beg + padding;
	a->beg += padding + count * size;
	memset(r, 0, (size_t)(count * size));
	return r;
}


/**
 * @brief checks if a memory block can be resized
 *
 * @param a	arena to check against
 * @param ptr	pointer to the memory block to check
 * @param size	the current size of the memory block at ptr
 *
 * @returns non-zero if the block can be possible resized, 0 otherwise
 */

static __inline int arena_is_resize_possible(struct arena a, const void *ptr, ptrdiff_t size)
{
	assert(a.beg && a.end);
	assert(size >= 0);

	/* In a bump allocator, only the last allocation can be extended in-place */
	return ptr && ((const uint8_t *)ptr + size) == a.beg;
}

#endif /* ARENA_H */

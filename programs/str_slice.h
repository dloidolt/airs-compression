/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Lightweight string slice library for substring operations.
 *
 * @warning A slice is a borrowed view and is only valid as long as the
 *          underlying buffer is valid. Do not return slices to stack-allocated
 *          memory from a function.
 *
 * @warning Encoding: this library is byte-oriented. Case-insensitive and
 *          whitespace helpers are ASCII-only (not Unicode/UTF-8 aware).
 *
 * @details To use this library, place this header file in your project.
 *          To include the implementation, define STR_SLICE_IMPLEMENTATION in
 *          file before including the header:
 *
 *          #define STR_SLICE_IMPLEMENTATION
 *          #include "str_slice.h"
 *
 *          You can optionally define STR_SLICE_API to control API visibility
 *          and linkage (e.g., static, extern).
 *          By default, functions are static.
 *
 * This library is highly inspired by the string handling done in u-config by
 * @author Christopher Wellons (skeeto)
 */

#ifndef STR_SLICE_H_MIKG4GPN
#define STR_SLICE_H_MIKG4GPN

#include <stddef.h>
#include <stdint.h>

#ifndef STR_SLICE_API
#  define STR_SLICE_API static
#endif


/** String slice type (8-bit chars) */
struct s8 {
	const unsigned char *s; /**< not null terminated string */
	ptrdiff_t len;          /**< length of string s */
};

/* Utility macros */
#define S8_COUNTOF(a) ((ptrdiff_t)(sizeof(a) / sizeof(*(a))))
/** Creates string slice from string literal @warning: do not use it with pointers! */
#define S8(s) { (const unsigned char *)s, S8_COUNTOF(s) - 1 }

/**
 * printf format specifier for a string slice
 * Example: printf("my_s8: %" PRIs8 "\n", S8_PARG(my_s8));
 */
#define PRIs8 "%.*s"
/** Expands a string slice into the arguments required by PRIs8 */
#define S8_PARG(x) (int)(x).len, (const char *)(x).s

/** Create s8 string from C string */
STR_SLICE_API struct s8 s8_from_cstr(const char *z);
/** Create s8 string from C string with known length */
STR_SLICE_API struct s8 s8_make(const char *z, ptrdiff_t len);
/** Create s8 string from a pointer range [beg, end) */
STR_SLICE_API struct s8 s8_span(const unsigned char *beg, const unsigned char *end);


/** Check if two s8 strings are equal */
STR_SLICE_API int s8_equals(struct s8 s1, struct s8 s2);
/** Check if two s8 strings are equal, differences in case are ignored, ASCII-only (not UTF-8) */
STR_SLICE_API int s8_equals_ignore_case(struct s8 s1, struct s8 s2);
/** Check if string starts with specified substring */
STR_SLICE_API int s8_starts_with(struct s8 s, struct s8 pre);
STR_SLICE_API int s8_starts_with_ignore_case(struct s8 s, struct s8 pre);

/** Take head portion of string */
STR_SLICE_API struct s8 s8_prefix(struct s8 s, ptrdiff_t len);
/** Remove head portion of string */
STR_SLICE_API struct s8 s8_skip(struct s8 s, ptrdiff_t off);

/** Remove whitespace from the begin and the end */
STR_SLICE_API struct s8 s8_trim(struct s8 s);
/** Remove prefix if present */
STR_SLICE_API struct s8 s8_strip_prefix(struct s8 s, struct s8 pre);
/** Remove prefix if present, differences in case are ignored, ASCII-only (not UTF-8) */
STR_SLICE_API struct s8 s8_strip_prefix_ignore_case(struct s8 s, struct s8 pre);

/** split_at() parsing result */
struct s8_split_result {
	struct s8 head;
	struct s8 tail;
	int ok;
};
/** Split string at delimiter */
STR_SLICE_API struct s8_split_result s8_split_at(struct s8 s, unsigned char delim);

/** s8_to_u32() parsing result */
struct s8_u32_result {
	uint32_t value;
	int ok;
};
/** Parse unsigned 32-bit integer from string */
STR_SLICE_API struct s8_u32_result s8_to_u32(struct s8 s);

#endif /* STR_SLICE_H_MIKG4GPN */


/* Implementation */
#ifdef STR_SLICE_IMPLEMENTATION

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>


STR_SLICE_API struct s8 s8_from_cstr(const char *z)
{
	struct s8 r = { 0 };

	if (z) {
		size_t l = strlen(z);

		assert(l <= PTRDIFF_MAX);
		r.len = (ptrdiff_t)l;
		r.s = (const unsigned char *)z;
	}
	return r;
}

STR_SLICE_API struct s8 s8_make(const char *z, ptrdiff_t len)
{
	struct s8 r = { 0 };

	if (len < 0)
		return r;

	r.s = (const unsigned char *)z;
	r.len = len;
	return r;
}

STR_SLICE_API struct s8 s8_span(const unsigned char *beg, const unsigned char *end)
{
	struct s8 r = { 0 };

	assert(beg);
	assert(end);
	assert(end >= beg);
	r.s = (const unsigned char *)beg;
	r.len = end - beg;
	return r;
}

/* Compare two byte arrays */
static int u8_compare(const unsigned char *s1, const unsigned char *s2, ptrdiff_t n)
{
	assert(n >= 0);
	return memcmp(s1, s2, (size_t)n);
}

STR_SLICE_API int s8_equals(struct s8 s1, struct s8 s2)
{
	return s1.len == s2.len && !u8_compare(s1.s, s2.s, s1.len);
}

/* ASCII-only case conversion (not UTF-8) */
static unsigned char u8_to_upper(unsigned char c)
{
	return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}

/* Compare two byte arrays, differences in case are ignored, ASCII-only (not UTF-8) */
static int u8_compare_ignore_case(const unsigned char *a, const unsigned char *b, ptrdiff_t n)
{
	for (; n; n--) {
		int d = u8_to_upper(*a++) - u8_to_upper(*b++);

		if (d)
			return d;
	}
	return 0;
}

STR_SLICE_API int s8_equals_ignore_case(struct s8 a, struct s8 b)
{
	return a.len == b.len && !u8_compare_ignore_case(a.s, b.s, a.len);
}

STR_SLICE_API int s8_starts_with(struct s8 s, struct s8 pre)
{
	return (s.len >= pre.len) && s8_equals(s8_prefix(s, pre.len), pre);
}

STR_SLICE_API int s8_starts_with_ignore_case(struct s8 s, struct s8 pre)
{
	return (s.len >= pre.len) && s8_equals_ignore_case(s8_prefix(s, pre.len), pre);
}

STR_SLICE_API struct s8 s8_prefix(struct s8 s, ptrdiff_t len)
{
	assert(len >= 0);
	assert(len <= s.len);
	s.len = len;
	return s;
}

STR_SLICE_API struct s8 s8_skip(struct s8 s, ptrdiff_t off)
{
	assert(off >= 0);
	assert(off <= s.len);
	s.s += off;
	s.len -= off;
	return s;
}

static int u8_is_whitespace(unsigned char c)
{
	return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == '\f';
}

STR_SLICE_API struct s8 s8_trim(struct s8 s)
{
	const unsigned char *p, *e;

	if (!s.len)
		return s;

	assert(s.s);
	p = s.s;
	e = s.s + s.len;

	while (p < e && u8_is_whitespace(*p))
		p++;
	while (e > p && u8_is_whitespace(*(e - 1)))
		e--;
	return s8_span(p, e);
}

STR_SLICE_API struct s8 s8_strip_prefix(struct s8 s, struct s8 pre)
{
	if (s8_starts_with(s, pre))
		return s8_skip(s, pre.len);
	return s;
}

STR_SLICE_API struct s8 s8_strip_prefix_ignore_case(struct s8 s, struct s8 pre)
{
	if (s8_starts_with_ignore_case(s, pre))
		return s8_skip(s, pre.len);
	return s;
}

STR_SLICE_API struct s8_split_result s8_split_at(struct s8 s, unsigned char delim)
{
	struct s8_split_result r = { 0 };
	ptrdiff_t len = 0;

	for (len = 0; len < s.len; len++)
		if (s.s[len] == delim)
			break;

	if (len == s.len) {
		r.head = s;
		r.tail = s8_skip(s, s.len);
		return r;
	}
	r.head = s8_prefix(s, len);
	r.tail = s8_skip(s, len + 1);
	r.ok = 1;
	return r;
}

STR_SLICE_API struct s8_u32_result s8_to_u32(struct s8 s)
{
	struct s8_u32_result r = { 0 };
	uint32_t value = 0;
	ptrdiff_t i = 0;

	if (s.len == 0)
		return r;
	if (s.len > 1 && s.s[0] == '0') /* reject leading 0 */
		return r;

	for (; i < s.len; i++) {
		uint32_t const d = s.s[i] - '0';

		if (d > 9)
			return r; /* Not a digit */
		if (value > (UINT32_MAX - d) / 10)
			return r;
		value = value * 10 + d;
	}

	r.value = value;
	r.ok = 1;
	return r;
}

#endif /* STR_SLICE_IMPLEMENTATION */

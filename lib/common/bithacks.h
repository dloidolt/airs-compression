/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Collection of some Bit Twiddling Hacks
 *
 * form:
 * @see https://graphics.stanford.edu/~seander/bithacks.html by
 *	@author Sean Eron Anderson et al. and
 * @see linux/tools/include/linux by @author Torvalds et al.
 */

#ifndef BITHACKS_H
#define BITHACKS_H

#include <stdint.h>
#include <limits.h>

#include "compiler.h"


/** @brief Returns the maximum of two unsigned 32-bit */
static __inline uint32_t max_u32(uint32_t a, uint32_t b)
{
	return a > b ? a : b;
}


/** @brief Returns the minimum of two unsigned 32-bit  */
static __inline uint32_t min_u32(uint32_t a, uint32_t b)
{
	return a < b ? a : b;
}


/** @brief Returns the minimum of two unsigned 64-bit  */
static __inline uint64_t min_u64(uint64_t a, uint64_t b)
{
	return a < b ? a : b;
}


/**
 * @brief Divides two numbers rounding up the result
 * @param n	Numerator
 * @param d	Denominator
 */
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))


/** @brief Returns floor(log2(x)) or UINT_MAX if x = 0 */
static __inline unsigned int ilog2(uint32_t x)
{
	compile_time_assert(sizeof(unsigned int) >= sizeof(uint32_t),
			    _expect_unsigned_int_to_be_at_least_32_bit);

	if (x == 0)
		return UINT_MAX;

	return bitsizeof(x) - 1 - (unsigned int)__builtin_clz(x);
}


/** @brief Calculates the floor of division by 2 */
static __inline int32_t floor_division_by_2(int32_t numerator)
{
	compile_time_assert((-1 >> 1) == -1, require_arithmetic_right_shift);
	return numerator >> 1;
}


/** @brief Calculates the floor of division by 4 */
static __inline int32_t floor_division_by_4(int32_t numerator)
{
	compile_time_assert((-1 >> 2) == -1, require_arithmetic_right_shift);
	return numerator >> 2;
}

#endif /*  BITHACKS_H */

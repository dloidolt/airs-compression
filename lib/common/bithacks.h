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


/**
 * @brief Returns the maximum of two values
 * @param a	First value to compare
 * @param b	Second value to compare
 */
static __inline uint32_t max_u32(uint32_t a, uint32_t b)
{
	return a > b ? a : b;
}


/**
 * @brief Returns the minimum of two values
 * @param a	First value to compare
 * @param b	Second value to compare
 */
static __inline uint32_t min_u32(uint32_t a, uint32_t b)
{
	return a < b ? a : b;
}


/**
 * @brief Divides two numbers rounding up the result
 * @param n	Numerator
 * @param d	Denominator
 */
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))


#endif /*  BITHACKS_H */

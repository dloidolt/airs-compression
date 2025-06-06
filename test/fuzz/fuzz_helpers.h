/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Helper functions for fuzzing
 */

#ifndef FUZZ_HELPERS_H
#define FUZZ_HELPERS_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define FUZZ_QUOTE_IMPL(str) #str
#define FUZZ_QUOTE(str) FUZZ_QUOTE_IMPL(str)

/**
 * Asserts for fuzzing that are always enabled.
 */
#define FUZZ_ASSERT_MSG(cond, msg)                                                             \
	((cond) ? (void)0 :                                                                    \
		  (fprintf(stderr, "%s: %u: Assertion: `%s' failed. %s\n", __FILE__, __LINE__, \
			   FUZZ_QUOTE(cond), (msg)),                                           \
		   abort()))
#define FUZZ_ASSERT(cond) FUZZ_ASSERT_MSG((cond), "");

void *FUZZ_malloc(size_t size);

#ifdef __cplusplus
}
#endif

#endif

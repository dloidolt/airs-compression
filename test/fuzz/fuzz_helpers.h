/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2026
 * @copyright GPL-2.0
 *
 * @brief Fuzzing helper functions and utilities
 *
 * @warning This code is platform-specific and dependent on the system endianness.
 */

#ifndef FUZZ_HELPERS_H
#define FUZZ_HELPERS_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../../lib/cmp.h"


#define FUZZ_QUOTE_IMPL(str) #str
#define FUZZ_QUOTE(str)      FUZZ_QUOTE_IMPL(str)

/* Asserts for fuzzing that are always enabled  */
#define FUZZ_ASSERT_MSG(cond, msg)                                                             \
	((cond) ? (void)0 :                                                                    \
		  (fprintf(stderr, "%s: %d: Assertion: `%s' failed. %s\n", __FILE__, __LINE__, \
			   FUZZ_QUOTE(cond), (msg)),                                           \
		   abort()))
#define FUZZ_ASSERT(cond) FUZZ_ASSERT_MSG((cond), "");


struct fuzz_consume {
	const uint8_t *beg;
	const uint8_t *end;
};

/**
 * @brief Allocate memory for fuzzing, aborting on failure
 * @param size	number of bytes to allocate
 * @returns pointer to allocated memory, or NULL if size is 0
 */
void *fuzz_malloc(size_t size);

/**
 * @brief Initialise a fuzz_consume structure
 * @param data		pointer to fuzz input data
 * @param size		size of fuzz input data in bytes
 * @returns initialised fuzz_consume structure
 */
struct fuzz_consume fuzz_consume_init(const uint8_t *data, size_t size);

/**
 * Copy n bytes from fuzz data to destination, if not enough bytes are
 * available, the remaining destination bytes are zeroed.
 */
void fuzz_consume_n_bytes(struct fuzz_consume *f, void *dst, size_t n);

/** Return a uint32_t from fuzz data or 0 if all data consumed */
uint32_t fuzz_consume_u32(struct fuzz_consume *f);

/** Return a uint8_t from fuzz data or 0 if all data consumed */
uint8_t fuzz_consume_u8(struct fuzz_consume *f);

/** Return 1 or 0 from fuzz data or 0 if all data consumed */
uint8_t fuzz_consume_bool(struct fuzz_consume *f);

/** Return value in inclusive range [min, max] from fuzz data 0 if all data consumed */
uint32_t fuzz_consume_range(struct fuzz_consume *f, uint32_t min, uint32_t max);

/** Return a struct cmp_params from fuzz data with 0 fields if all data consumed */
struct cmp_params *fuzz_consume_alloc_cmp_params(struct fuzz_consume *f);
#endif /* FUZZ_HELPERS_H */

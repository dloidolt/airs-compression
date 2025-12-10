/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data decompression functionality
 */

#ifndef CMP_DECMP_H
#define CMP_DECMP_H

#include <stdint.h>

#include "arena.h"

/**
 * @brief Result structure for batch decompression operations
 */
struct decmp_result {
	uint16_t **decmp;      /**< Array of pointers to decompressed data buffers */
	uint32_t *decmp_size;  /**< Array of sizes for each decompressed buffer */
	uint32_t count;        /**< Number of decompressed buffers */
};


/**
 * @brief Decompresses a batch of compressed 16-bit unsigned integer data buffers
 *
 * This function takes an array of compressed data buffers and decompresses them
 * using the compression parameters stored in the compressed data headers.
 *
 * @param a		Arena allocator for memory management of decompressed data
 *			and result structures
 * @param srcs		Array of pointers to compressed data buffers
 * @param src_sizes	Array of sizes for each compressed data buffer in bytes
 * @param src_count	Number of compressed data buffers to process
 *
 * @returns pointer to decmp_result structure containing decompressed data for
 *	each provided source buffer in the same order as provided.
 *	result->decmp_size[i] contains the decompressed size of the compressed
 *	data srcs[i] or an error, which can be checked using
 *	cmp_is_error(result->decmp_size[i])
 *
 * @warning The returned decmp_result structure and all decompressed data buffers
 *	are allocated from the provided arena and remain valid only as long as
 *	the provided arena is not reset or destroyed.
 */

struct decmp_result *decompress_batch_u16(struct arena *a, const void *srcs[],
					  uint32_t src_sizes[], uint32_t src_count);

#endif /* CMP_DECMP_H */

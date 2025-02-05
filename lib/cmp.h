/**
 * @file   cmp.h
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 *
 * @copyright GPLv2
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 *
 * @brief AIRS data compression
 *
 * This library provides two different APIs for data compression:
 *
 * 1. Single-pass Compression API:
 *    - Compresses multiple data buffers in a single function call
 *    - Use cmp_compress16() for compressing multiple 16-bit data buffers
 *    - Ideal for when all data is available at once
 *    - Simple to use, requires minimal setup
 *
 * 2. Multi-pass (Feeding) Compression API:
 *    - Allows compression of data buffers one at a time
 *    - Setup: Initialise context with cmp_initialise()
 *    - Process: Feed data buffers using cmp_feed16()
 *    - Can be reset using cmp_reset() to reuse the context
 *    - Clean-up: Optionally destroy context with cmp_deinitialise()
 *    - Ideal when memory is limited or you do not want to buffer the data
 *
 * Both APIs support the same compression modes and parameters, differing only
 * in how data is provided to the compressor.
 *
 * @see @dir examples directory for usage examples of both APIs
 *
 * @warning The interface is not frozen yet and may change in future versions.
 *          Changes may include:
 *          - Additional compression modes
 *          - New compression parameters
 *          - Change/Extended API functionality
 */

#ifndef CMP_H
#define CMP_H

/* ======   Dependency   ====== */
#include <stdint.h>
#include "common/header.h"

/* ======   Version Information  ====== */
/** major part of the version ID */
#define CMP_VERSION_MAJOR    0
/** minor part of the version ID */
#define CMP_VERSION_MINOR    1
/** release part of the version ID */
#define CMP_VERSION_RELEASE  0

/**
 * @brief complete version number
 */
#define CMP_VERSION_NUMBER  (CMP_VERSION_MAJOR *100*100 + CMP_VERSION_MINOR *100 + CMP_VERSION_RELEASE)


/* ======   Parameter Selection   ====== */
/**
 * @brief available compression modes
 * @note additional compression modes will follow
 */
enum cmp_mode {
	CMP_MODE_UNCOMPRESSED	/**< Uncompressed mode */
};


/**
 * @brief contains the parameters used for compression.
 *
 * @warning number and names of the compression parameters are TBD
 */

struct cmp_params {
	enum cmp_mode mode;		/**< Compression mode */
	uint32_t compression_par;	/**< Compression parameter */
};


/* ======   Helper Functions   ====== */
/**
 * @brief tells if a result is an error code
 *
 * @param code	return value to check
 *
 * @returns non-zero if the code is an error
 */

unsigned int cmp_is_error(uint32_t code);


/**
 * @brief get the maximum compressed size in a worst-case scenario
 *
 * In this scenario the input data are not compressible. This function is
 * primarily useful for memory allocation purposes (destination buffer size).
 *
 * @param num_bufs	number of data buffers you plan to compress
 * @param buf_size	size of each data buffer in bytes
 *
 * @returns the compressed size in the worst-case scenario or an error, which
 *	can be checked using cmp_is_error()

 */

uint32_t cmp_compress_bound(uint32_t num_bufs, uint32_t buf_size);


/**
 * @brief calculates the size needed for the compression working buffer
 *
 * @param buf_size	size of a source data buffer in bytes
 *
 * @returns the minimum size needed for a compression working buffer or an
 *	error, which can be checked using cmp_is_error()

 */

uint32_t cmp_cal_work_buf_size(uint32_t buf_size);


/* ======   Single-pass Compression API  ====== */
/**
 * @brief compress multiple buffers containing unsigned 16-bit data
 *
 * @param dst		the buffer to compress the src buffers into
 * @param dst_capacity	size of the dst buffer; may be any size, but
 *			cmp_compress_bound(num_src_bufs, src_buf_size) is
 *			guaranteed to be large enough
 * @param src_bufs	array containing the different data buffers to compress
 * @param num_src_bufs	number of data buffers in the array to compress
 * @param src_buf_size	size of each data buffer to compress in bytes
 * @param params	pointer to a compression parameters struct used to
 *			compress the data
 * @param work_buf	pointer to a working buffer (can be NULL if not needed)
 * @param work_buf_size	size of the working buffer in bytes; needed size can be
 *			calculated with cmp_cal_work_buf_size(src_buf_size)
 *
 * @returns the compressed size or an error, which can be checked using
 *	cmp_is_error()
 */

uint32_t cmp_compress16(void *dst, uint32_t dst_capacity,
			const uint16_t *src_bufs[],
			uint32_t num_src_bufs, uint32_t src_buf_size,
			const struct cmp_params *params,
			void *work_buf, uint32_t work_buf_size);


/* ======   Multi-pass Compression API   ====== */
/**
 * @brief compression context for multi-pass compression
 *
 * This structure maintains the state of an ongoing compression process,
 * allowing incremental data compression through multiple function calls.
 *
 * @warning This structure MUST NOT be directly manipulated by external code.
 *	Always use the provided API functions to interact with the compression
 *	context.
 */

struct cmp_context {
	uint32_t *dst;		/**< pointer to where the compressed data are written */
	struct cmp_hdr hdr;	/**< compression header information */
};


/**
 * @brief initialises a compression context
 *
 * @param ctx		pointer to a compression context struct to initialise
 * @param dst		the buffer to compress the data into
 * @param dst_capacity	size of the dst buffer; may be any size, but
 *			cmp_compress_bound(<number of feeds>, src_size) is
 *			guaranteed to be large enough
 * @param params	pointer to a compression parameters struct used to
 *			compress the data
 * @param work_buf	pointer to a working buffer (can be NULL if not needed)
 * @param work_buf_size	size of the working buffer in bytes; needed size can be
 *			calculated with cmp_cal_work_buf_size(src_size)
 *
 * @returns the compression header size or an error, which can be checked using
 *	cmp_is_error()
 */

uint32_t cmp_initialise(struct cmp_context *ctx,
			void *dst, uint32_t dst_capacity,
			const struct cmp_params *params,
			void *work_buf, uint32_t work_buf_size);


/**
 * @brief compresses an unsigned 16-bit data buffer
 *
 * You can use this function repeatedly to compress more data of the same size.
 * Once the data processing is complete, you can read the compressed data from
 * the destination buffer.
 *
 * @param ctx		pointer to a compression context; must have been
 *			initialised once with cmp_initialise()
 * @param src		pointer to the data to compress
 * @param src_size	size of the data to compress, must be the same for every
 *			source buffer until the context is reset
 *
 * @returns the compressed size or an error, which can be checked using
 *	cmp_is_error()
 */

uint32_t cmp_feed16(struct cmp_context *ctx, const uint16_t *src, uint32_t src_size);


/**
 * @brief resets the compression context
 *
 * All previously compressed data will be lost after the reset. Newly compressed
 * data will start again at the beginning of the destination buffer.
 *
 * @param ctx	pointer to a compression context to reset
 *
 * @returns the compression header size or an error, which can be checked using
 *	cmp_is_error()
 */

uint32_t cmp_reset(struct cmp_context *ctx);


/**
 * @brief destroys a compression context
 *
 * This function is optional and can be used after the compression process is
 * complete and the context is no longer needed.
 *
 * @param ctx	pointer to the compression context to be destroyed
 */

void cmp_deinitialise(struct cmp_context *ctx);


#endif /* CMP_H */

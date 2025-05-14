/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 *
 * @copyright GPL-2.0
 *
 * @brief Compression header structure and functions
 */

#ifndef CMP_HEADER_H
#define CMP_HEADER_H

#include <stdint.h>

#include "bitstream_writer.h"

/** Size of the compression header in bytes TBD */
#define CMP_HDR_SIZE                                                                       \
	((CMP_HDR_BITS_VERSION + CMP_HDR_BITS_CMP_SIZE + CMP_HDR_BITS_ORIGINAL_SIZE +      \
	  CMP_HDR_BITS_MODE + CMP_HDR_BITS_PREPROCESS + CMP_HDR_BITS_MODEL_RATE +          \
	  CMP_HDR_BITS_MODEL_ID + CMP_HDR_BITS_PASS_COUNT + CMP_HDR_BITS_COMPRESSION_PAR)  \
	 / 8)

/* Bit length of the different header fields */
#define CMP_HDR_BITS_VERSION 16
#define CMP_HDR_BITS_CMP_SIZE 24
#define CMP_HDR_BITS_ORIGINAL_SIZE 24
#define CMP_HDR_BITS_MODE 8
#define CMP_HDR_BITS_PREPROCESS 8
#define CMP_HDR_BITS_MODEL_RATE 8
#define CMP_HDR_BITS_MODEL_ID 48
#define CMP_HDR_BITS_PASS_COUNT 8
#define CMP_HDR_BITS_COMPRESSION_PAR 16


/**
 * @brief compression header structure
 *
 * Stores essential metadata for the compressed data.
 */

struct cmp_hdr {
	/* minimum header fields, sufficient for uncompressed data*/
	uint32_t version;	/**< Compression library version identifier */
	uint32_t cmp_size;	/**< Size of the compressed data including header in bytes */
	uint32_t original_size; /**< Size of the original uncompressed data in bytes */
	uint32_t mode;		/**< Compression mode applied */
	uint32_t preprocess;	/**< Preprocessing technique applied */
	/* additional fields needed for decompression */
	uint32_t model_rate; /**< Rate at which the model adapts during model-based preprocessing */
	uint64_t model_id;   /**< Unique identifier for the baseline model */
	uint32_t pass_count; /**< Number of compression passes performed since the last reset */
	uint32_t compression_par; /**< Compression parameter */
};


/**
 * @brief serialize compression header to a byte buffer
 *
 * @param bs		Pointer to a initialized bitstream writer structure
 * @param hdr		Pointer to header structure to serialize
 *
 * @returns the compression header size or an error, which can be checked using
 *	cmp_is_error()
 */

uint32_t cmp_hdr_serialize(struct bitstream_writer *bs, const struct cmp_hdr *hdr);


/**
 * @brief deserialize compression header from a byte buffer
 *
 * @param src		buffer containing serialized header
 * @param src_size	size of source buffer
 * @param hdr		pointer to header structure to fill
 *
 * @returns the compression header size or an error, which can be checked using
 *	cmp_is_error()
 */

uint32_t cmp_hdr_deserialize(const void *src, uint32_t src_size, struct cmp_hdr *hdr);


/**
 * @brief retrieve pointer to compressed data following the header
 *
 * @warning Assumes the compressed data block starts with a valid header.
 *
 * @param cmp_data	pointer to the start of the compressed data (header)
 *
 * @returns a pointer to the first byte of compressed data after the header
 *
 */

void *cmp_hdr_get_cmp_data(void *cmp_data);


#endif /* CMP_HEADER_H */

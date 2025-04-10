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

/** maximum allowed compressed data size in bytes */
#define CMP_MAX_CMP_SIZE 0xFFFFFF

/** maximum allowed size to compress in bytes */
#define CMP_MAX_ORIGINAL_SIZE 0xFFFFFF

/** size of the compression header in bytes TBD */
#define CMP_HDR_SIZE 10


/**
 * @brief compression header structure
 *
 * Stores essential metadata for the compressed data.
 */

struct cmp_hdr {
	uint32_t version;	/**< compression library version identifier */
	uint32_t cmp_size;	/**< size of the compressed data including header in bytes */
	uint32_t original_size;	/**< size of the original uncompressed data in bytes */
	uint32_t mode;		/**< Compression mode applied */
	uint32_t preprocess;	/**< Preprocessing technique applied */
};


/**
 * @brief serialize compression header to a byte buffer
 *
 * Data are written in big-endian byte order.
 *
 * @param dst		destination buffer to write serialized header
 * @param dst_size	size of destination buffer
 * @param hdr		Pointer to header structure to serialize
 *
 * @returns the compression header size or an error, which can be checked using
 *	cmp_is_error()
 */

uint32_t cmp_hdr_serialize(void *dst, uint32_t dst_size, const struct cmp_hdr *hdr);


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

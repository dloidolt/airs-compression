/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 *
 * @copyright GPL-2.0
 *
 * @brief Extended compression header structure and header functions
 */

#ifndef CMP_HEADER_PRIVATE_H
#define CMP_HEADER_PRIVATE_H

#include <stdint.h>

#include "bitstream_writer.h"
#include "../cmp.h"
#include "../cmp_header.h"


/* Bit length of the different extended header fields */
#define CMP_EXT_HDR_BITS_MODEL_ADAPTATION 8
#define CMP_EXT_HDR_BITS_ENCODER_PARAM    16
#define CMP_EXT_HDR_BITS_ENCODER_OUTLIER  24


/* Extended header offsets */
#define CMP_EXT_HDR_OFFSET_MODEL_RATE    16
#define CMP_EXT_HDR_OFFSET_ENCODER_PARAM 17
#define CMP_EXT_HDR_OFFSET_OUTLIER_PARAM 19


/** Size of the compression extension headers in bytes TBC */
#define CMP_EXT_HDR_SIZE                                                       \
	((CMP_EXT_HDR_BITS_MODEL_ADAPTATION + CMP_EXT_HDR_BITS_ENCODER_PARAM + \
	  CMP_EXT_HDR_BITS_ENCODER_OUTLIER) /                                  \
	 8)


/** Size of the basic compression plus the extension headers in bytes TBC */
#define CMP_HDR_MAX_SIZE (CMP_HDR_SIZE + CMP_EXT_HDR_SIZE)


/** Seed value used for initializing the checksum computation, arbitrarily chosen*/
#define CHECKSUM_SEED 419764627


/**
 * @brief compression header structure
 *
 * Contains metadata describing compressed data
 *
 * @note this is not the on-disk format - use cmp_hdr_serialize() and
 * cmp_hdr_deserialize() for conversion to/from the actual header format.
 */

struct cmp_hdr {
	/* Core header fields */
	uint8_t version_flag;
	uint16_t version_id;
	uint32_t compressed_size;
	uint32_t original_size;
	uint64_t identifier;
	uint8_t sequence_number;

	/* Compression method */
	enum cmp_preprocessing preprocessing;
	uint8_t checksum_enabled;
	enum cmp_encoder_type encoder_type;

	/* Extended compression parameters (optional) */
	uint32_t model_rate;
	uint32_t encoder_param;
	uint32_t encoder_outlier;
};


/**
 * @brief serialize compression header to a byte buffer
 *
 * @param bs	Pointer to a initialized bitstream writer structure
 * @param hdr	Pointer to header structure to serialize
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
 * @brief Calculates a checksum for an array of 16-bit values
 *
 * @param data	Pointer a data buffer (array of 16-bit values)
 * @param size	Size of the data buffer in bytes.
 *
 * @returns a 32-bit checksum of the data buffer
 */

uint32_t cmp_checksum(const uint16_t *data, uint32_t size);

#endif /* CMP_HEADER_PRIVATE_H */

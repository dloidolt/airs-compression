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
#include "../cmp.h"

/*
 * Bit length of the different header fields
 */
#define CMP_HDR_BITS_VERSION (CMP_HDR_BITS_VERSION_FLAG + CMP_HDR_BITS_VERSION_ID)
#define CMP_HDR_BITS_VERSION_FLAG 1
#define CMP_HDR_BITS_VERSION_ID 15
#define CMP_HDR_BITS_COMPRESSED_SIZE 24
#define CMP_HDR_BITS_ORIGINAL_SIZE 24

#define CMP_HDR_BITS_IDENTIFIER 48
#define CMP_HDR_BITS_SEQUENCE_NUMBER 8

#define CMP_HDR_BITS_METHOD                                                         \
	(CMP_HDR_BITS_METHOD_PREPROCESSING + CMP_HDR_BITS_METHOD_CHECKSUM_ENABLED + \
	 CMP_HDR_BITS_METHOD_ENCODER_TYPE)
#define CMP_HDR_BITS_METHOD_PREPROCESSING 4
#define CMP_HDR_BITS_METHOD_CHECKSUM_ENABLED 1
#define CMP_HDR_BITS_METHOD_ENCODER_TYPE 3

#define CMP_EXT_HDR_BITS_MODEL_ADAPTATION 8
#define CMP_EXT_HDR_BITS_ENCODER_PARAM 16
#define CMP_EXT_HDR_BITS_ENCODER_OUTLIER 24

/*
 * Byte offsets of the different header fields
 */
#define CMP_HDR_OFFSET_VERSION 0
#define CMP_HDR_OFFSET_COMPRESSED_SIZE 2
#define CMP_HDR_OFFSET_ORIGINAL_SIZE 5
#define CMP_HDR_OFFSET_IDENTIFIER 8
#define CMP_HDR_OFFSET_SEQUENCE_NUMBER 14
#define CMP_HDR_OFFSET_METHOD 15

/* Extended header offsets */
#define CMP_EXT_HDR_OFFSET_MODEL_RATE 16
#define CMP_EXT_HDR_OFFSET_ENCODER_PARAM 17
#define CMP_EXT_HDR_OFFSET_OUTLIER_PARAM 19

/** Size of the compression header in bytes TBC */
#define CMP_HDR_SIZE                                                                         \
	((CMP_HDR_BITS_VERSION + CMP_HDR_BITS_COMPRESSED_SIZE + CMP_HDR_BITS_ORIGINAL_SIZE + \
	  CMP_HDR_BITS_IDENTIFIER + CMP_HDR_BITS_SEQUENCE_NUMBER + CMP_HDR_BITS_METHOD) /    \
	 8)

/** Size of the compression extension headers in bytes TBC */
#define CMP_EXT_HDR_SIZE                                                       \
	((CMP_EXT_HDR_BITS_MODEL_ADAPTATION + CMP_EXT_HDR_BITS_ENCODER_PARAM + \
	  CMP_EXT_HDR_BITS_ENCODER_OUTLIER) /                                  \
	 8)

/** Size of the basic compression plus the extension headers in bytes TBC */
#define CMP_HDR_MAX_SIZE (CMP_HDR_SIZE + CMP_EXT_HDR_SIZE)


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


#endif /* CMP_HEADER_H */

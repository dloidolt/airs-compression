/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 *
 * @copyright GPL-2.0
 *
 * @brief Main compression header definitions
 *
 * Based on PLATO-UVIE-PL-UM-0001 Issue 0.2
 */

#ifndef CMP_HEADER_H
#define CMP_HEADER_H

/*
 * Maximum values that can be stored in the size fields
 */
#define CMP_HDR_MAX_COMPRESSED_SIZE ((1ULL << CMP_HDR_BITS_COMPRESSED_SIZE) - 1)
#define CMP_HDR_MAX_ORIGINAL_SIZE   ((1ULL << CMP_HDR_BITS_ORIGINAL_SIZE) - 1)


/*
 * Bit length of the different header fields
 */
#define CMP_HDR_BITS_VERSION         (CMP_HDR_BITS_VERSION_FLAG + CMP_HDR_BITS_VERSION_ID)
#define CMP_HDR_BITS_VERSION_FLAG    1
#define CMP_HDR_BITS_VERSION_ID      15
#define CMP_HDR_BITS_COMPRESSED_SIZE 24
#define CMP_HDR_BITS_ORIGINAL_SIZE   24

#define CMP_HDR_BITS_IDENTIFIER      48
#define CMP_HDR_BITS_SEQUENCE_NUMBER 8

#define CMP_HDR_BITS_METHOD                                                         \
	(CMP_HDR_BITS_METHOD_PREPROCESSING + CMP_HDR_BITS_METHOD_CHECKSUM_ENABLED + \
	 CMP_HDR_BITS_METHOD_ENCODER_TYPE)
#define CMP_HDR_BITS_METHOD_PREPROCESSING    4
#define CMP_HDR_BITS_METHOD_CHECKSUM_ENABLED 1
#define CMP_HDR_BITS_METHOD_ENCODER_TYPE     3


/*
 * Byte offsets of the different header fields
 */
#define CMP_HDR_OFFSET_VERSION         0
#define CMP_HDR_OFFSET_COMPRESSED_SIZE 2
#define CMP_HDR_OFFSET_ORIGINAL_SIZE   5
#define CMP_HDR_OFFSET_IDENTIFIER      8
#define CMP_HDR_OFFSET_SEQUENCE_NUMBER 14
#define CMP_HDR_OFFSET_METHOD          15


/** Size of the compression header in bytes */
#define CMP_HDR_SIZE                                                                         \
	((CMP_HDR_BITS_VERSION + CMP_HDR_BITS_COMPRESSED_SIZE + CMP_HDR_BITS_ORIGINAL_SIZE + \
	  CMP_HDR_BITS_IDENTIFIER + CMP_HDR_BITS_SEQUENCE_NUMBER + CMP_HDR_BITS_METHOD) /    \
	 8)


/** Size of the optional trailing checksum in byes */
#define CMP_CHECKSUM_SIZE sizeof(uint32_t)

#endif /* CMP_HEADER_H */

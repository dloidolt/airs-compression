/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Compression error codes definitions
 *
 * @warning The interface is not frozen yet and may change in future versions.
 * Changes may include:
 *    - Change error names
 *    - Change error values
 *    - Change/Extended API functionality
 */

#ifndef CMP_ERRORS_H
#define CMP_ERRORS_H

#include <stdint.h>


/**
 * @brief enumeration of all error codes
 *
 * @warning error name and value are TBC
 */

enum cmp_error {
	CMP_ERR_NO_ERROR = 0, /**< Operation completed successfully */

	/* General errors (1-9) */
	CMP_ERR_GENERIC = 1, /**< Generic error occurred */

	/* Parameter validation errors (10-29) */
	CMP_ERR_CONTEXT_INVALID = 10,	 /**< Invalid compression context */
	CMP_ERR_PARAMS_INVALID = 11,	 /**< Invalid compression parameters */
	CMP_ERR_WORK_BUF_NULL = 12,	 /**< Work buffer is NULL but required */
	CMP_ERR_WORK_BUF_TOO_SMALL = 13, /**< Work buffer is too small */
	CMP_ERR_DST_NULL = 14,		 /**< Destination buffer pointer is NULL */
	CMP_ERR_DST_UNALIGNED = 15,	 /**< Destination buffer not correct aligned */
	CMP_ERR_SRC_NULL = 16,		 /**< Source buffer pointer is NULL */
	CMP_ERR_SRC_SIZE_WRONG = 17,	 /**< Source buffer size doesn't match expected size */
	CMP_ERR_WORK_BUF_UNALIGNED = 18, /**< Work buffer is unaligned */

	/* Runtime errors (30-39) */
	CMP_ERR_DST_TOO_SMALL = 30,	/**< Destination buffer is too small */
	CMP_ERR_SRC_SIZE_MISMATCH = 31, /**< Source data size changed with model preprocessing */
	CMP_ERR_TIMESTAMP_INVALID = 32, /**< Invalid timestamp provided */

	/* Internal errors (100-109) */
	CMP_ERR_INT_HDR = 100,	     /**< Internal header processing error */
	CMP_ERR_INT_ENCODER = 101,   /**< Internal data encoder error */
	CMP_ERR_INT_BITSTREAM = 102, /**< Internal bitstream error */
	/*
	 * Limit marker - not an actual error code
	 * Do not use this value directly. Prefer cmp_is_error() for error checking.
	 */
	CMP_ERR_MAX_CODE = 128 /**< Maximum error code value. Do not use this */
};


/**
 * @brief convert a function result into an error code
 *
 * Can handle the same return values with are also suitable for cmp_is_error()
 * function.
 *
 * @param code	return value to get the error code
 *
 * @returns a unique error code for this error
 */

enum cmp_error cmp_get_error_code(uint32_t code);


/**
 * @brief get a human-readable error message from a return value
 *
 * Useful for debugging and logging purposes.
 *
 * @param code	compression return value to describe
 *
 * @returns a pointer to a string literal that describes the error code.
 */

const char *cmp_get_error_message(uint32_t code);


/**
 * @brief get a human-readable error message from an error code
 *
 * Returns the same as cmp_get_error_message() function but for the error code
 * instead of a return value code.
 *
 * @param code	the error code to describe, obtain with cmp_get_error_code()
 *
 * @returns a pointer to a string literal that describes the error code.
 */

const char *cmp_get_error_string(enum cmp_error code);


#endif /* CMP_ERRORS_H */

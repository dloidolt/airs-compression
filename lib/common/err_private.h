/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Internal error handling
 *
 * This header provides macro utilities for creating and managing error codes
 * within the compression library's internal implementation
 *
 * @warning This is a private header and should NOT be included directly by
 *	external code.
 */


#ifndef CMP_ERR_PRIVATE_H
#define CMP_ERR_PRIVATE_H

#include "../cmp_errors.h"


/** prepends "CMP_ERR_" to the given name */
#define CMP_ERROR_ENUM_PREFIX(name) CMP_ERR_##name


/**
 * Macro to convert error code enum (without CMP_ERR prefix) to a negative
 * unsigned 32-bit integer error code
 */
#define CMP_ERROR(name) ((uint32_t)-CMP_ERROR_ENUM_PREFIX(name))


/**
 * @brief tells if a internal result is an error code
 *
 * Intended for internal use only.
 *
 * @param code	return value to check
 *
 * @returns non-zero if the code is an error
 */

static __inline unsigned int cmp_is_error_int(uint32_t code)
{
	return code > CMP_ERROR(MAX_CODE);
}


#endif /* CMP_ERR_PRIVATE_H */

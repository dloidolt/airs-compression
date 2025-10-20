/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 *
 * @copyright GPL-2.0
 *
 * @brief Data compression error handling implementation
 */

#include <stdint.h>

#include "../cmp_errors.h"
#include "err_private.h"


enum cmp_error cmp_get_error_code(uint32_t code)
{
	if (!cmp_is_error_int(code))
		return CMP_ERR_NO_ERROR;
	return (enum cmp_error)(0 - code);
}


const char *cmp_get_error_string(enum cmp_error code)
{
#ifdef CMP_STRIP_ERROR_STRINGS
	(void)code;
	return "Error strings stripped";
#else
	static const char *const unspecified_error_str = "Unspecified error code";

	switch (code) {
	case CMP_ERR_NO_ERROR:
		return "No error detected";

	case CMP_ERR_GENERIC:
		return "Error (generic)";

	case CMP_ERR_PARAMS_INVALID:
		return "Invalid compression parameters";

	case CMP_ERR_DST_TOO_SMALL:
		return "Destination buffer is too small to hold the content";
	case CMP_ERR_DST_NULL:
		return "Destination buffer pointer is NULL";
	case CMP_ERR_DST_UNALIGNED:
		return "Destination buffer pointer is unaligned";

	case CMP_ERR_SRC_SIZE_WRONG:
		return "Source buffer size is invalid";
	case CMP_ERR_SRC_NULL:
		return "Source buffer pointer is NULL";
	case CMP_ERR_SRC_SIZE_MISMATCH:
		return "Source data size changed using model preprocessing; not allowed until reset";

	case CMP_ERR_WORK_BUF_TOO_SMALL:
		return "Work buffer is too small";
	case CMP_ERR_WORK_BUF_NULL:
		return "Work buffer is NULL but required";
	case CMP_ERR_WORK_BUF_UNALIGNED:
		return "Work buffer is unaligned";

	case CMP_ERR_HDR_CMP_SIZE_TOO_LARGE:
		return "Compressed size exceeds header field limit";
	case CMP_ERR_HDR_ORIGINAL_TOO_LARGE:
		return "Original size exceeds header field limit";

	case CMP_ERR_CONTEXT_INVALID:
		return "Compression context uninitialised or corrupted";

	case CMP_ERR_INT_HDR:
		return "Internal header processing error";
	case CMP_ERR_INT_ENCODER:
		return "Internal data encoder error";
	case CMP_ERR_INT_BITSTREAM:
		return "Internal bitstream writer error";

	case CMP_ERR_MAX_CODE:
	default:
		return unspecified_error_str;
	}
#endif
}


const char *cmp_get_error_message(uint32_t code)
{
	return cmp_get_error_string(cmp_get_error_code(code));
}

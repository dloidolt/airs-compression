/**
 * @file   cmp_errors.c
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
 * @brief AIRS data compression error code handling
 */

#include <stdint.h>

#include "../cmp_errors.h"
#include "err_private.h"


enum cmp_error cmp_get_error_code(uint32_t code)
{
	if (!cmp_is_error_int(code))
		return CMP_ERR_NO_ERROR;
	return (enum cmp_error)(0-code);
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

	case CMP_ERR_CONTEXT_INVALID:
		return "Invalid compression context";
	case CMP_ERR_PARAMS_INVALID:
		return "Invalid compression parameters";
	case CMP_ERR_WORK_BUF_NULL:
		return "Work buffer is NULL but required";
	case CMP_ERR_WORK_BUF_TOO_SMALL:
		return "Work buffer is too small";
	case CMP_ERR_DST_NULL:
		return "Destination buffer pointer is NULL";
	case CMP_ERR_SRC_NULL:
		return "Source buffer pointer is NULL";
	case CMP_ERR_SRC_SIZE_WRONG:
		return "Source buffer size is invalid";

	case CMP_ERR_DST_TOO_SMALL:
		return "Destination buffer is too small to hold the content";

	case CMP_ERR_INT_HDR:
		return "Internal header processing error";

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

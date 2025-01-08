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


unsigned int cmp_is_error(uint32_t code)
{
	return code > CMP_ERROR(MAX_CODE);
}


enum cmp_error cmp_get_error_code(uint32_t code)
{
	if (!cmp_is_error(code))
		return CMP_ERR_NO_ERROR;
	return (enum cmp_error)(0-code);
}


const char* cmp_get_error_string(enum cmp_error code)
{
#ifdef CMP_STRIP_ERROR_STRINGS
	(void)code;
	return "Error strings stripped";
#else
	static const char* const notErrorCode = "Unspecified error code";
	switch (code) {
	case CMP_ERR_NO_ERROR:
		return "No error detected";
	case CMP_ERR_GENERIC:
		return "Error (generic)";


	case CMP_ERR_NO_PARAMS:
		return "Pointer to the compression parameters structure is NULL";
	case CMP_ERR_NO_CONTEXT:
		return "Pointer to the compression context structure is NULL";
	case CMP_ERR_NO_SRC_DATA:
		return "Pointer to the source data is NULL. No data, to process";
	case CMP_ERR_INVALID_SRC_SIZE:
		return "The source size is invalid";


	case CMP_ERR_INT_HDR:
		return "Internal header construction error occurred";


	case CMP_ERR_MAX_CODE:
	default:
		return notErrorCode;
    }
#endif
}


const char* cmp_get_error_message(uint32_t code)
{
	return cmp_get_error_string(cmp_get_error_code(code));
}


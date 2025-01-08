/**
 * @file   cmp_errors.h
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
 * @brief AIRS data compression error code definition
 */


#ifndef CMP_ERRORS_H
#define CMP_ERRORS_H


/**
 * @brief enumeration of all error codes that can be returned by the compression functions
 *
 * @warning error name and value are TBC
 */

enum cmp_error {
	CMP_ERR_NO_ERROR = 0,
	CMP_ERR_GENERIC = 1,
	CMP_ERR_NO_PARAMS,
	CMP_ERR_NO_CONTEXT,
	CMP_ERR_NO_SRC_DATA,
	CMP_ERR_INVALID_SRC_SIZE,
	CMP_ERR_INT_HDR,
	CMP_ERR_MAX_CODE = 128 /* Do not use this value directly, prefer cmp_is_error() for error checking. */
};


#endif /* CMP_ERRORS_H */

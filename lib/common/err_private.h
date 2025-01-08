/**
 * @file   err_private.h
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
 * @brief Internal Error Handling Macros
 *
 * This header provides macro utilities for creating and managing error codes
 * within the compression library's internal implementation
 *
 * @warning This is a private header and should NOT be included directly by external code.
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


#endif /* CMP_ERR_PRIVATE_H */

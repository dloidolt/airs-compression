/*
 * @file   file.h
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
 * @brief file handling code
 */

#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include <stddef.h>

#define STD_OUT_MARK "//*-stdout-*//"
#define STD_IN_MARK "//*-stdin-*//"
#define NULL_MARK "/dev/null"

int file_get_size_u32(const char *filename, uint32_t *file_size32);
int file_load_be16(const char *filename, uint16_t *buffer, size_t buffer_size);
int file_save(const char *filename, const void *buffer, size_t size);

#endif /* FILE_H */

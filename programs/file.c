/*
 * @file   file.c
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
 */

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>

#include "file.h"
#include "log.h"
#include "byteorder.h"


/**
 * @brief open a file
 *
 * @param filename	name of file to open or special marker for standard streams
 * @param mode		file open mode (e.g., "rb", "wb")
 *
 * @returns file handle or NULL on error
 */

static FILE *file_open(const char *filename, const char *mode)
{
	FILE *fp;

	assert(filename);
	assert(mode);

	if (!strcmp(filename, STD_IN_MARK))
		return stdin;

	if (!strcmp(filename, STD_OUT_MARK))
		return stdout;

	fp = fopen(filename, mode);
	if (!fp)
		LOG_ERROR_WITH_ERRNO("Can't open '%s'", filename);

	return fp;
}


/**
 * @brief close a file
 *
 * @param fp		File pointer to close; standard streams will not be closed
 * @param filename	Original filename (to identify standard streams)
 *
 * @return 0 on success, -1 on error
 */

static int file_close(FILE *fp, const char *filename)
{
	assert(fp);
	assert(filename);

	if (fp == stdin || fp == stdout || fp == stderr)
		return 0;

	if (fclose(fp)) {
		LOG_ERROR_WITH_ERRNO("Can't close '%s'", filename);
		return -1;
	}
	return 0;
}


/**
 * @brief get the size of a file
 *
 * @param filename	name of a) file to check
 * @param file_size	pointer to a variable where the file size will be stored
 *
 *
 * @returns size of file or -1 on error; empty file is an error
 */

static int file_get_size(const char *filename, size_t *file_size)
{
	FILE *fp;
	int error;
	struct stat st;

	assert(filename);
	assert(file_size);

	*file_size = 0;

	fp = file_open(filename, "rb");
	if (!fp)
		return -1;

	error = fstat(fileno(fp), &st);
	if (file_close(fp, filename))
		return -1;

	if (error) {
		LOG_ERROR_WITH_ERRNO("Can't get size of '%s'", filename);
		return -1;
	}

	/* 1. st_size should be non-negative,
	 * 2. if off_t -> size_t type conversion results in a discrepancy, the
	 *    file size is too large for type size_t.
	 */
	*file_size = (size_t)st.st_size;
	if ((st.st_size < 0) || (st.st_size != (off_t)*file_size)) {
		LOG_ERROR("'%s' is too large", filename);
		return -1;
	}

	if (*file_size == 0) {
		LOG_ERROR("'%s' is empty.", filename);
		return -1;
	}

	return 0;
}


/**
 * Get the size of a file and store it in a uint32_t variable.
 *
 * @param filename     Name of the file to get the size of.
 * @param file_size32  Pointer to store the size of the file (uint32_t).
 * @return             0 on success, -1 on failure.
 */

int file_get_size_u32(const char *filename, uint32_t *file_size32)
{
	size_t file_size;
	int error;

	assert(filename);
	assert(file_size32);

	error = file_get_size(filename, &file_size);
	if (error)
		return error;

	/* Check if the file size fits into a uint32_t */
	if (file_size > UINT32_MAX) {
		LOG_ERROR("File '%s' is too large to read in (size: %llu bytes)",
			  filename, (unsigned long long)file_size);
		return -1;
	}

	*file_size32 = (uint32_t)file_size;
	return 0;
}


/**
 * @brief load a file into memory
 *
 * @param filename	name of a file to load
 * @param buffer	buffer to load the file into
 * @param buffer_size	size of the buffer in bytes
 *
 * @returns 0 on success or -1 on error
 */

static int file_load(const char *filename, void *buffer, size_t buffer_size)
{
	FILE *fp;
	size_t read_size;
	int close_error;

	assert(filename);
	assert(buffer);

	fp = file_open(filename, "rb");
	if (!fp)
		return -1;

	read_size = fread(buffer, 1, buffer_size, fp);
	close_error = file_close(fp, filename);
	if (read_size != buffer_size) {
		LOG_ERROR_WITH_ERRNO("Can't read '%s'", filename);
		return -1;
	}
	if (close_error) {
		LOG_ERROR("File '%s' read successfully but close failed", filename);
		return -1;
	}

	return 0;
}


/**
 * @brief Load a file of 16-bit big-endian values into memory and convert to
 *	host endianness
 *
 * @param filename	name of a file to load
 * @param buffer	buffer of uint16_t to load file into
 * @param buffer_size	size of the buffer in bytes
 *
 * @returns 0 on success or negative on error
 */

int file_load_be16(const char *filename, uint16_t *buffer, size_t buffer_size)
{
	size_t i;
	int error;

	assert(filename);
	assert(buffer);
	assert(buffer_size);

	error = file_load(filename, buffer, buffer_size);
	if (!error) {
		if (buffer_size % sizeof(*buffer)) {
			LOG_ERROR("%s: file size not a multiple of %lu",
				  filename, sizeof(*buffer));
			return -1;
		}

		for (i = 0; i < (size_t)buffer_size / sizeof(*buffer); i++)
			be16_to_cpus(&buffer[i]);
	}

	return error;
}


/**
 * @brief save memory contents to a file
 *
 * @param filename	name of file to save to
 * @param buffer	buffer containing data to save
 * @param size		number of bytes to save
 *
 * @return 0 on success, -1 on error
 */

int file_save(const char *filename, const void *buffer, size_t size)
{
	FILE *fp;
	size_t written;
	int error = 0;

	assert(filename);
	assert(buffer);

	fp = file_open(filename, "wb");
	if (fp == NULL)
		return -1;
	written = fwrite(buffer, 1, size, fp);
	if (written != size) {
		LOG_ERROR_WITH_ERRNO("Error writing '%s':", filename);
		error = -1;
	}

	if (file_close(fp, filename) && !error)
		LOG_WARNING("File '%s' saved successfully but close failed", filename);

	return error;
}

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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>

#include "file.h"
#include "log.h"
#include "byteorder.h"

#define UNUSED(arg) ((void)(arg))

#if defined(MSDOS) || defined(OS2) || defined(_WIN32)
#  include <fcntl.h>   /* _O_BINARY */
#  include <io.h>      /* _setmode, _fileno, _get_osfhandle */
#  if !defined(__DJGPP__)
#    include <windows.h> /* DeviceIoControl, HANDLE, FSCTL_SET_SPARSE */
#    include <winioctl.h> /* FSCTL_SET_SPARSE */
#    define SET_BINARY_MODE(file) { int const unused = _setmode(_fileno(file), _O_BINARY); (void)unused; }
#  else
#    define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#  endif
#else
#  define SET_BINARY_MODE(file) UNUSED(file)
#endif


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
 * @brief read stdin into a static allocated buffer
 *
 * @param blob		blob to put the stdin content
 * @param blob_size	blob size
 *
 * @returns 0 on success or -1 on error
 */

static int file_read_stdin(void *blob, size_t *blob_size)
{
	static uint8_t *buffer;
	static size_t buffer_size;

	size_t buffer_capacity = 4096;  /* Start with 4KB */

	assert(blob_size);

	if (!buffer) {
		*blob_size = 0;

		buffer = malloc(buffer_capacity);
		if (!buffer) {
			LOG_ERROR_WITH_ERRNO("Failed to allocate memory for stdin buffer");
			return -1;
		}

		/* Read stdin in chunks */
		while (1) {
			size_t bytes_read;

			bytes_read = fread(buffer + buffer_size, 1, buffer_capacity - buffer_size, stdin);
			buffer_size += bytes_read;

			if (buffer_size != buffer_capacity)
				break;

			{ /* Buffer is full, expand it */
				size_t new_capacity = buffer_capacity * 2;
				void *new_buffer = realloc(buffer, new_capacity);

				if (!new_buffer) {
					free(buffer);
					LOG_ERROR_WITH_ERRNO("Failed to reallocate memory for stdin");
					buffer_size = 0;
					return -1;
				}

				buffer = new_buffer;
				buffer_capacity = new_capacity;
			}
		}

		if (ferror(stdin)) {
			free(buffer);
			LOG_ERROR("Error reading from stdin");
			buffer_size = 0;
			return -1;
		}

		/* Trim buffer to exact size if needed */
		if (buffer_size < buffer_capacity) {
			void *trimmed_buffer = realloc(buffer, buffer_size);

			if (trimmed_buffer)  /* It's okay if trimming fails */
				buffer = trimmed_buffer;
		}
	}

	/* Return the results */
	if (blob) {
		if (buffer_size > *blob_size) {
			*blob_size = 0;
			return -1;
		}
		memcpy(blob, buffer, buffer_size);
	}
	*blob_size = buffer_size;
	return buffer_size == 0;
}


/**
 * @brief get the size of a file
 *
 * @param filename	name of a file to check
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

	if (fp == stdin) {
		if (file_read_stdin(NULL, file_size))
			return -1;
	} else {
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

	if (fp == stdin)
		return file_read_stdin(buffer, &buffer_size);

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
			LOG_ERROR("%s: file size not a multiple of %lu", filename, sizeof(*buffer));
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
	int write_err = 0;

	assert(filename);
	assert(buffer);

	if (!strcmp(filename, STD_OUT_MARK)) {
		LOG_DEBUG("Using stdout as output");
		fp = stdout;
		SET_BINARY_MODE(stdout);
	} else {
		if (strcmp(filename, NULL_MARK)) {
			/* Check if destination file already exists */
			fp = fopen(filename, "rb");
			if (fp) {
				fclose(fp);
				LOG_ERROR("'%s' already exists\n", filename);
				return -1;
			}
		}
		fp = file_open(filename, "wb");
		if (!fp)
			return -1;
	}
	written = fwrite(buffer, 1, size, fp);
	if (written != size) {
		LOG_ERROR_WITH_ERRNO("Error writing '%s':", filename);
		write_err = -1;
	}

	if (file_close(fp, filename) && !write_err)
		LOG_WARNING("File '%s' saved successfully but close failed", filename);

	return write_err;
}

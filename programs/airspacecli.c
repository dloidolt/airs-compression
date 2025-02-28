/**
 * @file   airspacecli.c
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
 * @brief AIRSPACE CLI - A tool for (de)compressing AIRS science data
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../lib/cmp.h"
#include "file.h"
#include "log.h"
#include "util.h"

/* Program information */
#define PROGRAM_NAME "AIRSPACE CLI"
#ifndef AIRSPACE_VERSION
#  define AIRSPACE_VERSION "v" CMP_VERSION_STRING
#endif
#define AUTHOR "Dominik Loidolt"
#define AIRSPACE_WELCOME_MESSAGE                                     \
	"*** %s (%d-bit) %s, by %s ***\n", PROGRAM_NAME,             \
		(int)(sizeof(size_t) * 8),  AIRSPACE_VERSION, AUTHOR

/* Operation modes */
enum operation_mode { MODE_COMPRESS, MODE_DECOMPRESS };


/* memory allocation or die */
static void *malloc_safe(size_t size)
{
	void *ptr = malloc(size);

	if (!ptr) {
		LOG_ERROR("Memory allocation failed");
		exit(EXIT_FAILURE);
	}
	return ptr;
}


/**
 * @brief allocates memory for a file list and checks file sizes.
 *
 * This function allocates memory and creates an array of files. It handles
 * special cases like using stdin as input and ensuring that all input files have the
 * same size.
 *
 * @param file_names	array of file names to add to the list
 * @param n_file_names	number of files to add
 * @param file_list_len	pointer to store the number of files in the list
 * @param file_size	pointer to store the size of the files
 *
 * @returns a pointer to an array of input file names, or NULL if an error occurs.
 */

static const char **allocate_and_build_file_list(char **file_names, int n_file_names,
						 int *file_list_len, uint32_t *file_size)
{
	const char **list = NULL;
	int i;

	assert(file_names);
	assert(file_list_len);
	assert(file_size);

	*file_size = 0;
	*file_list_len = 0;

	if (n_file_names == 0) {
		LOG_DEBUG("Using stdin as input");
		if (file_get_size_u32(STD_IN_MARK, file_size))
			return NULL;

		*file_list_len = 1;
		list = malloc_safe(sizeof(*list));
		list[0] = STD_IN_MARK;
		return list;
	}

	*file_list_len = n_file_names;
	list = malloc_safe((size_t)*file_list_len * sizeof(*list));
	for (i = 0; i < n_file_names; i++) {
		assert(file_names[i]);
		list[i] = file_names[i];
		if (!strcmp(list[i], "-")) {
			LOG_DEBUG("Using stdin as input");
			list[i] = STD_IN_MARK;
		}
		if (i == 0) {
			if (file_get_size_u32(list[i], file_size))
				goto fail;
		} else {
			uint32_t size = -1U;

			if (file_get_size_u32(list[i], &size))
				goto fail;

			if (*file_size != size) {
				LOG_ERROR("Expect input files to have the same size. '%s' has %u bytes, '%s' has %u bytes",
					  list[0], *file_size, list[i], size);
				goto fail;
			}
		}
	}

	return list;

fail:
	free(list);
	return NULL;
}


static int compress_files(const char *output_file, char **file_names, int n_file_names,
			  const struct cmp_params *compression_params)
{
	const char **input_files = NULL;
	void *dst_buffer = NULL;
	void *work_buffer = NULL;
	uint16_t *input_buffer = NULL;

	uint32_t file_size;
	uint32_t dst_capacity;
	uint32_t work_size;
	uint32_t compressed_size;

	struct cmp_context ctx;

	int input_file_count;
	int i;
	int ret = EXIT_FAILURE;

	/* Allocate buffers */
	input_files = allocate_and_build_file_list(file_names, n_file_names,
						   &input_file_count, &file_size);
	if (!input_files)
		goto fail;
	/* We expect that all files have the same size */
	input_buffer = malloc_safe(file_size);

	dst_capacity = cmp_compress_bound((uint32_t)input_file_count, file_size);
	if (cmp_is_error(dst_capacity)) {
		LOG_ERROR_CMP(dst_capacity, "Can't calculating compressed data buffer size");
		goto fail;
	}
	dst_buffer = malloc_safe(dst_capacity);

	work_size = cmp_cal_work_buf_size(file_size);
	if (cmp_is_error(work_size)) {
		LOG_ERROR_CMP(work_size, "Error calculating work buffer size");
		goto fail;
	}
	work_buffer = malloc_safe(work_size);

	compressed_size = cmp_initialise(&ctx, dst_buffer, dst_capacity, compression_params,
					 work_buffer, work_size);
	if (cmp_is_error(compressed_size)) {
		LOG_ERROR_CMP(compressed_size, "Compression initialization failed");
		goto fail;
	}

	for (i = 0; i < input_file_count; i++) {
		LOG_PLAIN(LOG_LEVEL_INFO, "Compressing %s", input_files[i]);
		if (file_load_be16(input_files[i], input_buffer, file_size))
			goto fail;

		compressed_size = cmp_feed16(&ctx, input_buffer, file_size);

		if (cmp_is_error(compressed_size)) {
			LOG_ERROR_CMP(compressed_size, "Compression failed for %s", input_files[i]);
			goto fail;
		}
	}

	ret = file_save(output_file, dst_buffer, compressed_size);

fail:
	free(input_files);
	free(dst_buffer);
	free(work_buffer);
	free(input_buffer);

	return ret;
}


static int is_reading_from_console(char **files, int input_file_count)
{
	int i;

	if (input_file_count == 0)
		return 1;

	for (i = 0; i < input_file_count; i++)
		if (!strcmp(files[i], "-"))
			return 1;
	return 0;
}

static void print_usage(FILE *stream, const char *program_name)
{
	LOG_F(stream, "Usage: %s [OPTIONS...] [FILE... | -] [-o OUTPUT]\n", program_name);
	LOG_F(stream, "(De)compress AIRS science data FILE(s).\n\n");
	LOG_F(stream, "With no FILE, or when FILE is -, read standard input.\n");
	LOG_F(stream, "\nOptions:\n");
	LOG_F(stream, "  -c, --compress    Compress input files\n");
	LOG_F(stream, "  -o OUTPUT         Write output to OUTPUT\n");
	LOG_F(stream, "  -q, --quiet       Decrease verbosity\n");
	LOG_F(stream, "  -v, --verbose     Increase verbosity\n");
	LOG_F(stream, "  --[no]color       Print color codes in output\n");
	LOG_F(stream, "  -V, --version     Display version\n");
	LOG_F(stream, "  -h, --help        Display this help\n");
	LOG_F(stream, "\nExamples:\n");
	LOG_F(stream, "# Compressing files1 and files2 to output.air\n");
	LOG_F(stream, "airspace -c file1 file2 -o output.air\n");
	LOG_F(stream, "# Decompressing files (coming soon!)\n");
	LOG_F(stream, "airspace output.air -o file1.dat file2.dat\n");
}


static void print_version(void)
{
	if (log_get_level() < LOG_LEVEL_DEFAULT)
		LOG_STDOUT("%s\n", CMP_VERSION_STRING);
	else
		LOG_STDOUT(AIRSPACE_WELCOME_MESSAGE);
}


int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	int ch;
	/*
	 * For long options that have no equivalent short option, use a
	 * non-character as a pseudo short option, starting with CHAR_MAX + 1.
	 */
	enum {
		COLOR_OPT = CHAR_MAX + 1,
		NO_COLOR_OPT
	};
	static struct option long_options[] = {
		{ "compress", no_argument, NULL, 'c' },
		{ "verbose",  no_argument, NULL, 'v' },
		{ "quiet",    no_argument, NULL, 'q' },
		{ "color",    no_argument, NULL, COLOR_OPT },
		{ "no-color", no_argument, NULL, NO_COLOR_OPT },
		{ "version",  no_argument, NULL, 'V' },
		{ "help",     no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 }
	};

	/* Set defaults */
	enum operation_mode mode = MODE_DECOMPRESS;
	const char *output_file = STD_OUT_MARK;
	struct cmp_params compression_params = { 0 };
	const char *program_name;

	assert(argv);
	assert(argc >= 1);
	program_name = argv[0];
	log_setup_color();

	while ((ch = getopt_long(argc, argv, "Vvqhco:", long_options, NULL)) != -1) {
		switch (ch) {
		case 'c':
			mode = MODE_COMPRESS;
			break;
		case 'o':
			output_file = optarg;
			break;
		case 'v':
			log_increase_verbosity();
			break;
		case 'q':
			log_decrease_verbosity();
			break;
		case COLOR_OPT:
			log_set_color(LOG_COLOR_ENABLED);
			break;
		case NO_COLOR_OPT:
			log_set_color(LOG_COLOR_DISABLED);
			break;
		case 'V':
			print_version();
			return EXIT_SUCCESS;
		case 'h':
			print_usage(stdout, program_name);
			return EXIT_SUCCESS;
		default:
			print_usage(stderr, program_name);
			return EXIT_FAILURE;
		}
	}
	/*
	 * Adjust argc and argv to point to the remaining command-line arguments
	 * after the options have been processed
	 */
	argv += optind;
	argc -= optind;

	LOG_PLAIN(LOG_LEVEL_DEBUG, AIRSPACE_WELCOME_MESSAGE);

	/* Check inputs arguments */
	if (is_reading_from_console(argv, argc) && UTIL_IS_CONSOLE(stdin)) {
		LOG_ERROR("stdin is a terminal, aborting");
		return EXIT_FAILURE;
	}

	if (!strcmp(output_file, STD_OUT_MARK) && UTIL_IS_CONSOLE(stdout)) {
		LOG_ERROR("stdout is a terminal, aborting");
		return EXIT_FAILURE;
	}


	/* Execute requested operation */
	switch (mode) {
	case MODE_COMPRESS:
		ret = compress_files(output_file, argv, argc, &compression_params);
		break;
	case MODE_DECOMPRESS:
		LOG_ERROR("Decompression not implemented yet");
		break;
	default:
		LOG_ERROR("Invalid operation mode");
		break;
	}

	return ret;
}

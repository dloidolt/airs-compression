/**
 * @file   single_pass_compression.c
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
 * @brief simple data compression API example
 * This example shows how to use the single-pass compression step by step.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <cmp.h>
#include <cmp_errors.h>


/**
 * @brief Demonstrates single-pass compression using cmp_compress16()
 *
 * This example shows how to use the single-step compression API
 * for compressing multiple data buffers in one function call.
 */

int example_single_pass_compression(void)
{
	/* Define constants for our example */
	enum {
		NUM_BUFFERS = 2,		/* Number of data buffers to compress */
		SAMPLES_PER_BUFFER = 3,		/* Number of data samples in each buffer */
		DATA_SIZE = sizeof(uint16_t) * SAMPLES_PER_BUFFER
	};

	struct cmp_params params = {0};	/* Parameters for compression configuration */

	uint8_t *dst = NULL;		/* Destination buffer for compressed data */
	uint32_t dst_capacity;		/* Maximum size of destination buffer */

	void *work_buf = NULL;		/* Working buffer for compression algorithm */
	uint32_t work_buf_size;		/* Size of working buffer */

	uint32_t cmp_size;		/* Actual size of compressed data */

	/* Prepare source buffers for compression */
	uint16_t buffer1[SAMPLES_PER_BUFFER] = {0x0000, 0x0001, 0x0002};
	uint16_t buffer2[SAMPLES_PER_BUFFER] = {0xCA75, 0xCAFE, 0xC0DE};

	/* Create an array of buffer pointers */
	const uint16_t *src_buffers[] = {buffer1, buffer2};

	/*
	 * Configure Compression Parameters
	 * For this example, we'll use the uncompressed mode.
	 * In real-world scenarios, you would select an appropriate compression
	 * mode and the additional parameters.
	 */
	params.mode = CMP_MODE_UNCOMPRESSED;
	/* No additional parameters needed for uncompressed mode */


	/*
	 * Allocate Destination Buffer
	 * We calculate maximum possible compressed size to ensure sufficient
	 * buffer space for the compressed data.
	 */
	dst_capacity = cmp_compress_bound(NUM_BUFFERS, DATA_SIZE);
	if (cmp_is_error(dst_capacity)) {
		fprintf(stderr, "Error calculating destination buffer size: %s. (Error Code: %u)\n",
			cmp_get_error_message(dst_capacity), cmp_get_error_code(dst_capacity));
		return -1;
	}

	/* Allocate destination buffer */
	dst = malloc(dst_capacity);
	if (dst == NULL) {
		fprintf(stderr, "Memory allocation failed for destination buffer\n");
		return -1;
	}


	/*
	 * Allocate Working Buffer
	 * Working buffer is needed for compression algorithm. Not strictly
	 * necessary for the uncompressed mode, but here is a demo to show how
	 * it works.
	 */
	work_buf_size = cmp_cal_work_buf_size(DATA_SIZE);
	if (cmp_is_error(work_buf_size)) {
		fprintf(stderr, "Error calculating working buffer size: %s. (Error Code: %u)\n",
			cmp_get_error_message(work_buf_size), cmp_get_error_code(work_buf_size));
		free(dst);
		return -1;
	}

	/* Allocate working buffer */
	work_buf = malloc(work_buf_size);
	if (dst == NULL) {
		fprintf(stderr, "Memory allocation failed for working buffer\n");
		free(dst);
		return -1;
	}


	/*
	 * Perform a Compression of all the data buffers
	 */
	cmp_size = cmp_compress16(dst, dst_capacity,
				  src_buffers, NUM_BUFFERS,  DATA_SIZE,
				  &params, work_buf, work_buf_size);
	if (cmp_is_error(cmp_size)) { /* check compression result */
		fprintf(stderr, "Compression failed: %s. (Error Code: %u)\n",
			cmp_get_error_message(work_buf_size), cmp_get_error_code(work_buf_size));
		free(dst);
		free(work_buf);
		return -1;
	}

	/*
	 * Use the Compressed Data
	 * Lets have a look at the compressed data.
	 */
	{
		uint32_t i;

		printf("Compressed Data (Size: %" PRIu32 "):\n", cmp_size);
		for (i = 0; i < cmp_size; i++)
			printf("%02X%s", dst[i], ((i + 1) % 32 == 0) ? "\n" : " ");
		printf("\n");
	}

	/* Clean-up */
	free(dst);
	free(work_buf);

	return 0;
}


/**
 * Calls the single-pass compression example and returns an error if the example fails
 */

int main(void)
{
	if (example_single_pass_compression())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

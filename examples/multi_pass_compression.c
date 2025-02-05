/**
 * @file   multi_pass_compression.c
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
 * @brief AIRS data compression examples
 * This example shows how to use the compression library step by step.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <cmp.h>
#include <cmp_errors.h>


/**
 * @brief demonstrates multi-pass compression API usage
 */

int example_multi_pass_compression(void)
{
	/* Define constants for our example */
	 enum {
		NUM_DATA_SAMPLES = 3,	/* Number of samples in each data buffer */
		DATA_SIZE = sizeof(uint16_t) * NUM_DATA_SAMPLES,
		MAX_NUM_ADDING = 2	/* Number of data buffers we'll compress */
	};

	struct cmp_params params = {0};	/* Parameters for compression configuration */
	struct cmp_context ctx = {0};	/* Context to maintain compression state */

	uint8_t *dst = NULL;		/* Destination buffer for compressed data */
	uint32_t dst_capacity;		/* Maximum size of destination buffer */

	void *work_buf = NULL;		/* Working buffer for compression algorithm */
	uint32_t work_buf_size;		/* Size of working buffer */

	uint32_t cmp_size;		/* Actual size of compressed data */


	/*
	 * Step 1: Configure Compression Parameters
	 * For this example, we'll use the uncompressed mode.
	 * In real-world scenarios, you would select an appropriate compression
	 * mode and the additional parameters.
	 */
	params.mode = CMP_MODE_UNCOMPRESSED;
	/* No additional parameters needed for uncompressed mode */


	/*
	 * Step 2: Allocate Destination Buffer
	 * We calculate maximum possible compressed size to ensure sufficient
	 * buffer space for the compressed data.
	 */
	dst_capacity = cmp_compress_bound(MAX_NUM_ADDING, DATA_SIZE);
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
	 * Step 3: Allocate Working Buffer
	 * Working buffer is needed for compression algorithm. Not strictly
	 * necessary for the uncompressed mode, but here is a demo to show how it works.
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
	 * Step 4: Initialise Compression Context
	 * This prepares the compression process and writes the initial compression header.
	 */
	cmp_size = cmp_initialise(&ctx, dst, dst_capacity, &params, work_buf, work_buf_size);
	if (cmp_is_error(cmp_size)) {
		fprintf(stderr, "Compression initialisation failed: %s. (Error Code: %u)\n",
			cmp_get_error_message(cmp_size), cmp_get_error_code(cmp_size));
		free(dst);
		free(work_buf);
		return -1;
	}


	/*
	 * Step 5: Compress Multiple Data Buffers
	 * Demonstrating multi-pass compression by feeding different data buffers
	 */
	{
		/* The first data we want to "compress" */
		uint16_t data1[NUM_DATA_SAMPLES] = {0x0000, 0x0001, 0x0002};

		cmp_size = cmp_feed16(&ctx, data1, sizeof(data1));
		if (cmp_is_error(cmp_size)) { /* check compression result */
			fprintf(stderr, "First data compression failed: %s. (Error Code: %u)\n",
				cmp_get_error_message(cmp_size), cmp_get_error_code(cmp_size));
			free(dst);
			free(work_buf);
			return -1;
		}
	}
	{
		/*
		 * The second data buffer we want to "compress".
		 * Note: size should be the same as before
		 */
		uint16_t data2[NUM_DATA_SAMPLES] = {0xCA75, 0xCAFE, 0xC0DE};

		cmp_size = cmp_feed16(&ctx, data2, sizeof(data2));
		if (cmp_is_error(cmp_size)) {
			fprintf(stderr, "Second data compression failed: %s. (Error Code: %u)\n",
				cmp_get_error_message(cmp_size), cmp_get_error_code(cmp_size));
			free(dst);
			free(work_buf);
			return -1;
		}
	}


	/*
	 * Repeat Step 5 until you compressed enough data
	 */


	/*
	 * Step 6: Use the Compressed Data
	 * Lets have a look at the compressed data.
	 */
	{
		uint32_t i;

		printf("Compressed Data (Size: %" PRIu32 "):\n", cmp_size);
		for (i = 0; i < cmp_size; i++)
			printf("%02X%s", dst[i], ((i + 1) % 32 == 0) ? "\n" : " ");
		printf("\n");
	}


	/*
	 * Step 7: Reset Compression Context
	 * Allows reusing the context. Be aware that after reset the compressed
	 * data are lost.
	 */

	cmp_size = cmp_reset(&ctx);
	if (cmp_is_error(cmp_size)) {
		fprintf(stderr, "Context reset failed: %s. (Error Code: %u)\n",
			cmp_get_error_message(cmp_size), cmp_get_error_code(cmp_size));
		free(dst);
		free(work_buf);
		return -1;
	}

	/*
	 * Go to Step 5 until all data are compressed
	 */


	/*
	 * Step 8: Cleanup
	 * Free allocated resources.
	 */
	cmp_deinitialise(&ctx);  /* this is optional */
	free(dst);
	free(work_buf);

	return 0;
}


/**
 * Calls the multi-pass example and returns an error if the example fails
 */

int main(void)
{
	if (example_multi_pass_compression())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief AIRS simple data compression example
 *
 * This example shows how to use the compression library step by step.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <cmp.h>
#include <cmp_errors.h>


/**
 * @brief Dummy timestamp function
 *
 * This function provides a simple dummy implementation for a timestamp provider.
 * It increments a static counter by 2 on each call and returns the result. This
 * function is intended for demonstration or testing purposes only.
 *
 * @returns a dummy 48-bit timestamp value
 */

static uint64_t dummy_timestamp(void)
{
	static uint64_t dummy_time;
	uint64_t const mask48 = (((uint64_t)1 << 48) - 1);

	return (dummy_time += 2) & mask48;
}


/**
 * @brief demonstrate compression API usage
 */

static int simple_compression(void)
{
	/* Define constants for our example */
	enum {
		DATA_SAMPLES = 3, /* We comms 3 uint16_t values */
		DATA_SRC_SIZE = DATA_SAMPLES * sizeof(uint16_t)
	};

	struct cmp_params params = { 0 }; /* Parameters for compression configuration */
	struct cmp_context ctx = { 0 };	  /* Context to maintain compression state */

	uint8_t *dst = NULL;   /* Destination buffer for compressed data */
	uint32_t dst_capacity; /* Maximum size of destination buffer */

	void *work_buf = NULL;	/* Working buffer for compression algorithm */
	uint32_t work_buf_size; /* Size of working buffer */

	uint32_t cmp_size; /* Actual size of compressed data */
	uint32_t return_value;


	/*
	 * Step 0: Setup Timestamp Function
	 * Register the dummy timestamp function to supply timestamps for the
	 * compression library.
	 */
	cmp_set_timestamp_func(dummy_timestamp);


	/*
	 * Step 1: Configure Compression Parameters
	 * For this example, we'll use the uncompressed mode.
	 * In real-world scenarios, you would select an appropriate compression
	 * mode and the additional parameters.
	 */
	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	/* No additional parameters needed for uncompressed mode */


	/*
	 * Step 2: Allocate Working Buffer
	 * Working buffer is needed for compression algorithm. Not strictly
	 * necessary for the uncompressed mode, but here is a demo to show how it works.
	 */
	work_buf_size = cmp_cal_work_buf_size(&params, DATA_SRC_SIZE);
	/* NOTE: Most return values of compression functions must be checked
	 * with cmp_is_error() to check if they were successful.
	 */
	if (cmp_is_error(work_buf_size)) {
		fprintf(stderr, "Error calculating working buffer size: %s. (Error Code: %u)\n",
			cmp_get_error_message(work_buf_size), cmp_get_error_code(work_buf_size));
		free(dst);
		return -1;
	}

	/* Allocate working buffer if need */
	if (work_buf_size > 0) {
		work_buf = malloc(work_buf_size);
		if (work_buf == NULL) {
			fprintf(stderr, "Memory allocation failed for working buffer\n");
			free(dst);
			return -1;
		}
	}


	/*
	 * Step 3: Initialise Compression Context
	 * We need the compression context later in order to compress data.
	 */
	return_value = cmp_initialise(&ctx, &params, work_buf, work_buf_size);
	if (cmp_is_error(return_value)) {
		fprintf(stderr, "Compression initialisation failed: %s. (Error Code: %u)\n",
			cmp_get_error_message(return_value), cmp_get_error_code(return_value));
		free(dst);
		free(work_buf);
		return -1;
	}


	/*
	 * Step 4: Allocate Destination Buffer
	 * We calculate maximum possible compressed size to ensure sufficient
	 * buffer space for the compressed data.
	 */
	dst_capacity = cmp_compress_bound(DATA_SRC_SIZE);
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
	 * Step 5: Compress Data
	 */
	{ /* The first data we want to "compress" */
		uint16_t data1[DATA_SAMPLES] = { 0x0000, 0x0001, 0x0002 };

		cmp_size = cmp_compress_u16(&ctx, dst, dst_capacity, data1, sizeof(data1));
		if (cmp_is_error(cmp_size)) { /* check compression result */
			fprintf(stderr, "First data compression failed: %s. (Error Code: %u)\n",
				cmp_get_error_message(cmp_size), cmp_get_error_code(cmp_size));
			free(dst);
			free(work_buf);
			return -1;
		}
	}


	/*
	 * Step 6: Use the Compressed Data
	 * Lets have a look at the compressed data.
	 */
	{
		uint32_t i;

		printf(" First Compressed Data (Size: %" PRIu32 "):\n", cmp_size);
		for (i = 0; i < cmp_size; i++)
			printf("%02X%s", dst[i], ((i + 1) % 32 == 0) ? "\n" : " ");
		printf("\n");
	}


	/*
	 * Repeat Step 5 until you compressed enough data
	 */
	{ /* The second data buffer we want to "compress". */
		/* NOTE: Data size should be the same as before */
		uint16_t data2[DATA_SAMPLES] = { 0xCA75, 0xCAFE, 0xC0DE };

		cmp_size = cmp_compress_u16(&ctx, dst, dst_capacity, data2, sizeof(data2));
		if (cmp_is_error(cmp_size)) {
			fprintf(stderr, "Second data compression failed: %s. (Error Code: %u)\n",
				cmp_get_error_message(cmp_size), cmp_get_error_code(cmp_size));
			free(dst);
			free(work_buf);
			return -1;
		}
	}

	{ /* Lets have a look at the compressed data. */
		uint32_t i;

		printf("Second Compressed Data (Size: %" PRIu32 "):\n", cmp_size);
		for (i = 0; i < cmp_size; i++)
			printf("%02X%s", dst[i], ((i + 1) % 32 == 0) ? "\n" : " ");
		printf("\n");
	}


	/*
	 * Step 7: Reset Compression Context
	 * Allows reusing the context. Reset the internal model of the compression.
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
	 * Step 8: Clean-up
	 * Free allocated resources.
	 */
	cmp_deinitialise(&ctx); /* this is optional */
	free(dst);
	free(work_buf);

	return 0;
}


/**
 * Calls the example and returns an error if the example fails
 */

int main(void)
{
	if (simple_compression())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

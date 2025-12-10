/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Decompression Tests
 *
 * This test suite verifies the correctness of the decompression pipeline
 * for various compression scenarios, preprocessing types, and edge cases.
 */

#include <stdint.h>
#include <unity.h>
#include "test_common.h"

#include <cmp.h>
#include "../lib/decompress/decmp.h"


struct cmp_buf {
	uint32_t *cmp_data;
	uint32_t cmp_size;
	struct cmp_context *ctx;
};


static struct cmp_buf create_compressed_data(struct arena *a, const uint16_t *input,
					     uint32_t input_sizes, const struct cmp_params *params,
					     struct cmp_context *existing_ctx)
{
	struct cmp_buf r = { 0 };

	uint32_t dst_capacity; /* Maximum size of the destination buffer */

	void *work_buf = NULL; /* Internal working buffer needed by the compressor */
	uint32_t work_buf_size = cmp_cal_work_buf_size(params, input_sizes);

	TEST_ASSERT_CMP_SUCCESS(work_buf_size);

	if (work_buf_size > 0)
		work_buf = arena_alloc(a, work_buf_size, sizeof(uint8_t), __alignof__(*input));

	dst_capacity = cmp_compress_bound(input_sizes);
	TEST_ASSERT_CMP_SUCCESS(dst_capacity);

	/*
	 * Allocate destination buffer for the compressed output
	 * NOTE: The destination buffer must be 8-byte aligned.
	 */
	r.cmp_data = arena_alloc(a, dst_capacity, sizeof(uint8_t), CMP_DST_ALIGNMENT);

	/* Use existing context or create new one */
	if (existing_ctx == NULL) {
		r.ctx = ARENA_NEW(a, struct cmp_context);
		TEST_ASSERT_CMP_SUCCESS(cmp_initialise(r.ctx, params, work_buf, work_buf_size));
	} else {
		r.ctx = existing_ctx;
	}

	r.cmp_size = cmp_compress_u16(r.ctx, r.cmp_data, dst_capacity, input, input_sizes);
	TEST_ASSERT_CMP_SUCCESS(r.cmp_size);

	return r;
}


/* Test helper functions */

static void assert_decompression_success(struct decmp_result *result, const uint16_t *expected_data,
					 uint32_t expected_size, uint32_t expected_count)
{
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL(expected_count, result->count);
	TEST_ASSERT_CMP_SUCCESS(result->decmp_size[0]);
	TEST_ASSERT_EQUAL(expected_size, result->decmp_size[0]);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(expected_data, result->decmp[0],
				      expected_size / sizeof(uint16_t));
}


static void assert_batch_decompression_success(struct decmp_result *result,
					       const uint16_t *expected_data1,
					       uint32_t expected_size1,
					       const uint16_t *expected_data2,
					       uint32_t expected_size2, uint32_t expected_count)
{
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL(expected_count, result->count);

	/* Check first result */
	TEST_ASSERT_CMP_SUCCESS(result->decmp_size[0]);
	TEST_ASSERT_EQUAL(expected_size1, result->decmp_size[0]);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(expected_data1, result->decmp[0],
				      expected_size1 / sizeof(uint16_t));

	/* Check second result */
	TEST_ASSERT_CMP_SUCCESS(result->decmp_size[1]);
	TEST_ASSERT_EQUAL(expected_size2, result->decmp_size[1]);
	TEST_ASSERT_EQUAL_HEX16_ARRAY(expected_data2, result->decmp[1],
				      expected_size2 / sizeof(uint16_t));
}


/* Test functions */

void test_restores_data_when_none_preprocessing_applied(void)
{
	uint16_t data[] = { 0, 1, UINT16_MAX, INT16_MAX, (uint16_t)INT16_MIN, 0xC0FE };
	struct arena *a = clear_test_arena();
	struct cmp_params params = { 0 };
	struct cmp_buf compressed_data =
		create_compressed_data(a, data, sizeof(data), &params, NULL);
	struct decmp_result *result;
	const void *srcs[1];
	uint32_t sizes[1];

	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.primary_encoder_param = CMP_ENCODER_UNCOMPRESSED;
	srcs[0] = compressed_data.cmp_data;
	sizes[0] = compressed_data.cmp_size;

	result = decompress_batch_u16(a, srcs, sizes, 1);

	assert_decompression_success(result, data, sizeof(data), 1);
}


void test_restores_data_when_difference_preprocessing_applied(void)
{
	/* Arrange */
	uint16_t data[] = { 0, 1, UINT16_MAX, INT16_MAX, (uint16_t)INT16_MIN, 0xC0FE };
	struct arena *a = clear_test_arena();
	struct cmp_params params = { 0 };
	struct cmp_buf compressed_data;
	const void *srcs[1];
	uint32_t sizes[1];
	struct decmp_result *result;

	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_param = CMP_ENCODER_UNCOMPRESSED;
	compressed_data = create_compressed_data(a, data, sizeof(data), &params, NULL);
	srcs[0] = compressed_data.cmp_data;
	sizes[0] = compressed_data.cmp_size;

	/* Act */
	result = decompress_batch_u16(a, srcs, sizes, 1);

	/* Assert */
	assert_decompression_success(result, data, sizeof(data), 1);
}


void test_restores_data_when_iwt_preprocessing_applied(void)
{
	/* Arrange */
	uint16_t data[] = { 0, 1, UINT16_MAX, INT16_MAX, (uint16_t)INT16_MIN, 0xC0FE };
	struct arena *a = clear_test_arena();
	struct cmp_params params = { 0 };
	struct cmp_buf compressed_data;
	const void *srcs[1];
	uint32_t sizes[1];
	struct decmp_result *result;

	params.primary_preprocessing = CMP_PREPROCESS_IWT;
	params.primary_encoder_param = CMP_ENCODER_UNCOMPRESSED;
	compressed_data = create_compressed_data(a, data, sizeof(data), &params, NULL);
	srcs[0] = compressed_data.cmp_data;
	sizes[0] = compressed_data.cmp_size;

	/* Act */
	result = decompress_batch_u16(a, srcs, sizes, 1);

	/* Assert */
	TEST_IGNORE_MESSAGE("IWT decompression not yet implemented");
	assert_decompression_success(result, data, sizeof(data), 1);
}


void test_restores_data_when_model_preprocessing_applied(void)
{
	/* Arrange */
	uint16_t data1[] = { 1, 2, 3 };
	uint16_t data2[] = { 4, 5, 0 };
	struct cmp_params params = { 0 };
	struct arena *a = clear_test_arena();

	struct cmp_buf compressed1;
	struct cmp_buf compressed2;
	const void *srcs[2];
	uint32_t sizes[2];
	struct decmp_result *result;

	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.primary_encoder_param = CMP_ENCODER_UNCOMPRESSED;
	params.secondary_iterations = 1;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_encoder_param = CMP_ENCODER_UNCOMPRESSED;
	compressed1 = create_compressed_data(a, data1, sizeof(data1), &params, NULL);
	compressed2 = create_compressed_data(a, data2, sizeof(data2), &params, compressed1.ctx);
	srcs[0] = compressed1.cmp_data;
	srcs[1] = compressed2.cmp_data;
	sizes[0] = compressed1.cmp_size;
	sizes[1] = compressed2.cmp_size;

	/* Act */
	result = decompress_batch_u16(a, srcs, sizes, 2);

	/* Assert */
	TEST_IGNORE_MESSAGE("Model preprocessing not supported yet!");
	assert_batch_decompression_success(result, data1, sizeof(data1), data2, sizeof(data2), 2);
}


void test_decompress_handles_empty_input_gracefully(void)
{
	/* Test decompression error handling with empty/NULL input */
	struct arena *a = clear_test_arena();
	const void *srcs[1];
	uint32_t sizes[1];
	struct decmp_result *result;

	srcs[0] = NULL;
	sizes[0] = 0;

	/* Act */
	result = decompress_batch_u16(a, srcs, sizes, 1);

	/* Assert */
	TEST_IGNORE_MESSAGE("Empty data compression not supported");
	TEST_ASSERT_NOT_NULL(result);
	TEST_ASSERT_EQUAL(1, result->count);
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_NULL, result->decmp_size[0]);
	TEST_ASSERT_NULL(result->decmp[0]);
}


void test_decompress_handles_single_element_arrays(void)
{
	/* Arrange */
	uint16_t data[] = { 42 };
	struct cmp_params params = { 0 };
	struct arena *a = clear_test_arena();
	struct cmp_buf compressed_data;
	const void *srcs[1];
	uint32_t sizes[1];
	struct decmp_result *result;

	/* TODO: completed parameters */
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.primary_encoder_param = CMP_ENCODER_UNCOMPRESSED;
	compressed_data = create_compressed_data(a, data, sizeof(data), &params, NULL);
	srcs[0] = compressed_data.cmp_data;
	sizes[0] = compressed_data.cmp_size;

	/* Act */
	result = decompress_batch_u16(a, srcs, sizes, 1);

	/* Assert */
	assert_decompression_success(result, data, sizeof(data), 1);
}

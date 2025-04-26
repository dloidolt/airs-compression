/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Compression Tests
 */

#include <stdint.h>
#include <string.h>

#include <unity.h>
#include "test_common.h"

#include "../lib/cmp.h"
#include "../lib/cmp_errors.h"
#include "../lib/common/header.h"


static struct cmp_context create_uncompressed_context(void)
{
	struct cmp_context ctx;
	struct cmp_params par_uncompressed = { 0 };
	uint32_t return_val;

	par_uncompressed.mode = CMP_MODE_UNCOMPRESSED;
	par_uncompressed.primary_preprocessing = CMP_PREPROCESS_NONE;
	/* we do not need a working buffer for CMP_MODE_UNCOMPRESSED */
	return_val = cmp_initialise(&ctx, &par_uncompressed, NULL, 0);
	TEST_ASSERT_CMP_SUCCESS(return_val);

	return ctx;
}


void test_no_work_buf_needed_for_none_preprocessing(void)
{
	struct cmp_params par_uncompressed = { 0 };
	uint32_t work_buf_size;

	par_uncompressed.primary_preprocessing = CMP_PREPROCESS_NONE;

	work_buf_size = cmp_cal_work_buf_size(&par_uncompressed, 42);

	TEST_ASSERT_EQUAL(0, work_buf_size);
}


void test_calculate_work_buf_size_for_iwt_correctly(void)
{
	struct cmp_params par = { 0 };
	uint32_t work_buf_size;

	par.primary_preprocessing = CMP_PREPROCESS_IWT;

	work_buf_size = cmp_cal_work_buf_size(&par, 41);

	TEST_ASSERT_EQUAL(42, work_buf_size);
}


void test_calculate_work_buf_size_for_model_preprocess_correctly(void)
{
	struct cmp_params par = { 0 };
	uint32_t work_buf_size;

	par.primary_preprocessing = CMP_PREPROCESS_NONE;
	par.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	par.max_secondary_passes = 1;

	work_buf_size = cmp_cal_work_buf_size(&par, 41);

	TEST_ASSERT_CMP_SUCCESS(work_buf_size);
	TEST_ASSERT_EQUAL(42, work_buf_size);
}


void test_calculate_work_buf_size_ignore_secondary_preprocessing_if_disabled(void)
{
	struct cmp_params par = { 0 };
	uint32_t work_buf_size;

	par.primary_preprocessing = CMP_PREPROCESS_NONE;
	par.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	par.max_secondary_passes = 0;

	work_buf_size = cmp_cal_work_buf_size(&par, 41);

	TEST_ASSERT_CMP_SUCCESS(work_buf_size);
	TEST_ASSERT_EQUAL(0, work_buf_size);
}


void test_work_buf_size_calculation_detects_missing_parameters_struct(void)
{
	uint32_t work_buf_size;

	work_buf_size = cmp_cal_work_buf_size(NULL, 42);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, work_buf_size);
}


void test_work_buf_size_calculation_detects_invalid_primary_preprocessing(void)
{
	struct cmp_params par_uncompressed = { 0 };
	uint32_t work_buf_size;

	par_uncompressed.primary_preprocessing = -1U;

	work_buf_size = cmp_cal_work_buf_size(&par_uncompressed, 42);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, work_buf_size);
}


void test_work_buf_size_calculation_detects_invalid_secondary_preprocessing(void)
{
	struct cmp_params par_uncompressed = { 0 };
	uint32_t work_buf_size;

	par_uncompressed.secondary_preprocessing = -1U;
	par_uncompressed.max_secondary_passes = 1;

	work_buf_size = cmp_cal_work_buf_size(&par_uncompressed, 42);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, work_buf_size);
}


void test_successful_compression_initialisation_with_work_buf(void)
{
	struct cmp_context ctx;
	struct cmp_params par = { 0 };
	uint16_t work_buf[2];

	uint32_t return_val = cmp_initialise(&ctx, &par, work_buf, sizeof(work_buf));

	TEST_ASSERT_CMP_SUCCESS(return_val);
}


void test_invalid_compression_initialisation_no_context(void)
{
	struct cmp_params const par = { 0 };
	uint32_t return_val;

	return_val = cmp_initialise(NULL, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_CONTEXT_INVALID, return_val);
}


void test_invalid_compression_initialisation_no_parameters(void)
{
	struct cmp_context const ctx_all_zero = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	memset(&ctx, 0xFF, sizeof(ctx));

	return_val = cmp_initialise(&ctx, NULL, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
	TEST_ASSERT_EQUAL_MEMORY(&ctx_all_zero, &ctx, sizeof(ctx));
}


void test_invalid_preprocess_initialization(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.mode = CMP_MODE_UNCOMPRESSED;
	par.primary_preprocessing = 0xFFFF;

	memset(&ctx, 0xFF, sizeof(ctx));
	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_compression_in_uncompressed_mode(void)
{
	const uint16_t data[2] = { 0x0001, 0x0203 };
	uint8_t dst[CMP_HDR_SIZE + sizeof(data)];
	/* uncompressed data should be in big endian */
	const uint8_t cmp_data_exp[sizeof(data)] = { 0x00, 0x01, 0x02, 0x03 };
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	struct cmp_hdr hdr;

	uint32_t const cmp_size =
		cmp_compress_u16(&ctx_uncompressed, dst, sizeof(dst), data, sizeof(data));

	TEST_ASSERT_CMP_SUCCESS(cmp_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + 4, cmp_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(cmp_data_exp, cmp_hdr_get_cmp_data(dst), sizeof(data));
	cmp_hdr_deserialize(dst, cmp_size, &hdr);
	TEST_ASSERT_EQUAL(CMP_VERSION_NUMBER, hdr.version);
	TEST_ASSERT_EQUAL(cmp_size, hdr.cmp_size);
	TEST_ASSERT_EQUAL(sizeof(data), hdr.original_size);
	TEST_ASSERT_EQUAL(CMP_MODE_UNCOMPRESSED, hdr.mode);
	TEST_ASSERT_EQUAL(CMP_PREPROCESS_NONE, hdr.preprocess);
}


void test_compression_detects_too_small_dst_buffer(void)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	const uint16_t src[2] = { 0x0001, 0x0203 };
	uint8_t dst[CMP_HDR_SIZE + sizeof(src) - 1];

	uint32_t const cmp_size =
		cmp_compress_u16(&ctx_uncompressed, dst, sizeof(dst), src, sizeof(src));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_TOO_SMALL, cmp_size);
}


void test_compression_detects_missing_context(void)
{
	const uint16_t src[2] = { 0x0001, 0x0203 };
	uint8_t dst[CMP_HDR_SIZE + sizeof(src)];

	uint32_t const cmp_size = cmp_compress_u16(NULL, dst, sizeof(dst), src, sizeof(src));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_CONTEXT_INVALID, cmp_size);
}


void test_compression_detects_missing_dst_buffer(void)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	const uint16_t src[2] = { 0x0001, 0x0203 };

	uint32_t const size = cmp_compress_u16(&ctx_uncompressed, NULL, 0, src, sizeof(src));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_NULL, size);
}


void test_compression_detects_missing_src_data(void)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	uint8_t dst[CMP_HDR_SIZE + sizeof(uint16_t)];

	uint32_t const size =
		cmp_compress_u16(&ctx_uncompressed, dst, sizeof(dst), NULL, sizeof(uint16_t));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_NULL, size);
}


void test_compression_detects_src_size_is_0(void)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	const uint16_t src[2] = { 0x0001, 0x0203 };
	uint8_t dst[CMP_HDR_SIZE + sizeof(src)];

	uint32_t const cmp_size = cmp_compress_u16(&ctx_uncompressed, dst, sizeof(dst), src, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_WRONG, cmp_size);
}


void test_compression_detects_src_size_is_not_multiple_of_2(void)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	const uint16_t src[2] = { 0 };
	uint32_t const src_size = 3;
	uint8_t dst[CMP_HDR_SIZE + sizeof(src)];

	uint32_t const cmp_size =
		cmp_compress_u16(&ctx_uncompressed, dst, sizeof(dst), src, src_size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_WRONG, cmp_size);
}


void test_compression_detects_src_size_too_large(void)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	uint8_t dst[CMP_HDR_SIZE + sizeof(uint16_t)];
	const uint16_t src[2] = { 0x0001, 0x0203 };
	uint32_t const src_size_too_large = CMP_MAX_ORIGINAL_SIZE + 1;

	uint32_t const cmp_size =
		cmp_compress_u16(&ctx_uncompressed, dst, sizeof(dst), src, src_size_too_large);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_WRONG, cmp_size);
}


void test_successful_reset_of_compressed_data(void)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();

	uint32_t const return_val = cmp_reset(&ctx_uncompressed);

	TEST_ASSERT_CMP_SUCCESS(return_val);
}


void test_compression_reset_detect_missing_context(void)
{
	uint32_t const return_val = cmp_reset(NULL);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_CONTEXT_INVALID, return_val);
}


void test_deinitialise_a_compression_context(void)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	struct cmp_context const zero_ctx = { 0 };

	cmp_deinitialise(&ctx_uncompressed);

	TEST_ASSERT_EQUAL_MEMORY(&ctx_uncompressed, &zero_ctx, sizeof(ctx_uncompressed));
}


void test_detect_missing_work_buffer(void)
{
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	uint32_t return_value;

	params.primary_preprocessing = CMP_PREPROCESS_IWT;

	return_value = cmp_initialise(&ctx, &params, NULL, 0);

	TEST_ASSERT_CMP_FAILURE(return_value);
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_NULL, return_value);
}


void test_detect_0_size_work_buffer(void)
{
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	uint32_t work_buf[1];
	uint32_t return_value;

	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.max_secondary_passes = 1;

	return_value = cmp_initialise(&ctx, &params, work_buf, 0);

	TEST_ASSERT_CMP_FAILURE(return_value);
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_TOO_SMALL, return_value);
}


void test_compression_detects_too_small_work_buffer(void)
{
	struct cmp_params params = { 0 };
	const uint16_t data[] = { 0, 0, 0 };
	uint8_t work_buf[sizeof(data) - 1];
	struct cmp_context ctx;
	uint32_t dst_size, work_buf_size;
	uint8_t dst[CMP_HDR_SIZE + sizeof(data)];

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_IWT;
	work_buf_size = cmp_cal_work_buf_size(&params, sizeof(data));
	TEST_ASSERT_LESS_THAN(work_buf_size, sizeof(work_buf));
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	dst_size = cmp_compress_u16(&ctx, dst, sizeof(dst), data, sizeof(data));

	TEST_ASSERT_CMP_FAILURE(dst_size);
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_TOO_SMALL, dst_size);
}


void test_non_model_preprocessing_src_size_change_allowed(void)
{
	const uint16_t data1[] = { 0, 0, 0, 0 };
	const uint16_t data2[] = { 0, 0, 0 };
	uint8_t work_buf[sizeof(data1)];
	uint8_t dst[CMP_HDR_SIZE + sizeof(data1)];
	uint32_t return_code;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_IWT;
	params.max_secondary_passes = 10;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));
	TEST_ASSERT_CMP_SUCCESS(cmp_compress_u16(&ctx, dst, sizeof(dst), data1, sizeof(data1)));

	return_code = cmp_compress_u16(&ctx, dst, sizeof(dst), data2, sizeof(data2));

	TEST_ASSERT_CMP_SUCCESS(return_code);
}


void test_deinitialise_NULL_context_gracefully(void)
{
	cmp_deinitialise(NULL);
}


void test_bound_size_is_enough_for_uncompressed_mode(void)
{
	uint32_t const bound = cmp_compress_bound(3);

	TEST_ASSERT_CMP_SUCCESS(bound);
	TEST_ASSERT_GREATER_OR_EQUAL_UINT32(CMP_HDR_SIZE + 4, bound); /* round size up to next multiple of 2 */
}


void test_bound_size_calculation_detects_to_large_src_size(void)
{
	uint32_t const bound = cmp_compress_bound(UINT32_MAX);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_WRONG, bound);
}


static uint64_t g_timestamp;
static uint64_t return_timestamp_stub(void)
{
	return g_timestamp;
}


void test_detect_too_large_timestamp_during_initialisation(void)
{
	uint32_t return_code;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	g_timestamp = (uint64_t)1 << 48;
	cmp_set_timestamp_func(return_timestamp_stub);
	return_code = cmp_initialise(&ctx, &params, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_TIMESTAMP_INVALID, return_code);
	g_timestamp = 0;
}


void test_detect_too_large_timestamp_during_during_compression(void)
{
	const uint16_t data[] = { 0, 0 };
	uint8_t dst[CMP_HDR_SIZE + sizeof(data)];
	uint32_t return_code;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	cmp_set_timestamp_func(return_timestamp_stub);
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));
	TEST_ASSERT_CMP_SUCCESS(cmp_compress_u16(&ctx, dst, sizeof(dst), data, sizeof(data)));

	g_timestamp = (uint64_t)1 << 48;
	return_code = cmp_compress_u16(&ctx, dst, sizeof(dst), data, sizeof(data));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_TIMESTAMP_INVALID, return_code);
	g_timestamp = 0;
}

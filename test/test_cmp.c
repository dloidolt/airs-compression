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

	par_uncompressed.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	par_uncompressed.primary_preprocessing = CMP_PREPROCESS_NONE;
	/* we do not need a working buffer for CMP_ENCODER_UNCOMPRESSED */
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
	par.secondary_iterations = 1;

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
	par.secondary_iterations = 0;

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
	par_uncompressed.secondary_iterations = 1;

	work_buf_size = cmp_cal_work_buf_size(&par_uncompressed, 42);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, work_buf_size);
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
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(dst, cmp_size, &hdr));
	TEST_ASSERT_EQUAL(CMP_VERSION_NUMBER, hdr.version_id);
	TEST_ASSERT_EQUAL(cmp_size, hdr.compressed_size);
	TEST_ASSERT_EQUAL(sizeof(data), hdr.original_size);
	TEST_ASSERT_EQUAL(CMP_ENCODER_UNCOMPRESSED, hdr.encoder_type);
	TEST_ASSERT_EQUAL(CMP_PREPROCESS_NONE, hdr.preprocessing);
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

	uint32_t const size = cmp_compress_u16(&ctx_uncompressed, NULL, 100, src, sizeof(src));

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
	uint32_t const src_size_too_large = 1 << CMP_HDR_BITS_ORIGINAL_SIZE;

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


void test_compression_detects_too_small_work_buffer(void)
{
	struct cmp_params params = { 0 };
	const uint16_t data[] = { 0, 0, 0 };
	uint8_t work_buf[sizeof(data) - 1];
	struct cmp_context ctx;
	uint32_t dst_size, work_buf_size;
	uint8_t dst[CMP_HDR_MAX_SIZE + sizeof(data)];

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
	uint8_t dst[CMP_HDR_MAX_SIZE + sizeof(data1)];
	uint32_t return_code;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.secondary_preprocessing = CMP_PREPROCESS_IWT;
	params.secondary_iterations = 10;
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
	/* round size up to next multiple of 2 */
	TEST_ASSERT_GREATER_OR_EQUAL_UINT32(CMP_HDR_SIZE + 4, bound);
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


uint32_t get_checksum(void *compressed_data, uint32_t size)
{
	uint8_t *buf = (uint8_t *)compressed_data + size - CMP_CHECKSUM_SIZE;

	TEST_ASSERT(size > 4);
	return (uint32_t)buf[0] << 24 | (uint32_t)buf[1] << 16 | (uint32_t)buf[2] << 8 |
	       (uint32_t)buf[3];
}


void test_checksum_appended_to_compressed_data(void)
{
	const uint16_t input_data[] = { 0xCA, 0xFF, 0xEE };
	uint8_t output_buf[CMP_HDR_SIZE + sizeof(input_data) + CMP_CHECKSUM_SIZE] = { 0 };
	uint32_t const expected_checksum = cmp_checksum(input_data, sizeof(input_data));
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.checksum_enabled = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_data,
				       sizeof(input_data));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(input_data) + CMP_CHECKSUM_SIZE, output_size);
	expected_hdr.compressed_size = output_size;
	expected_hdr.original_size = sizeof(input_data);
	expected_hdr.checksum_enabled = 1;
	TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
	TEST_ASSERT_EQUAL_HEX32(expected_checksum, get_checksum(output_buf, output_size));
}


void test_checksum_is_different_for_different_inputs(void)
{
	const uint16_t input_data1[] = { 0xC0, 0xFF, 0xEE };
	const uint16_t input_data2[] = { 0xC0, 0xFF, 0xEF };
	uint8_t output_buf1[CMP_HDR_SIZE + sizeof(input_data2) + CMP_CHECKSUM_SIZE] = { 0 };
	uint8_t output_buf2[CMP_HDR_SIZE + sizeof(input_data1) + CMP_CHECKSUM_SIZE] = { 0 };
	uint32_t output_size1, output_size2;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.checksum_enabled = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	output_size1 = cmp_compress_u16(&ctx, output_buf1, sizeof(output_buf1), input_data1,
					sizeof(input_data1));
	output_size2 = cmp_compress_u16(&ctx, output_buf2, sizeof(output_buf2), input_data2,
					sizeof(input_data2));

	TEST_ASSERT_CMP_SUCCESS(output_size1);
	TEST_ASSERT_CMP_SUCCESS(output_size2);
	TEST_ASSERT_NOT_EQUAL_HEX32(get_checksum(output_buf1, output_size1),
				    get_checksum(output_buf2, output_size2));
}


void test_checksum_is_same_for_same_inputs(void)
{
	const uint16_t input_data[] = { 0xC0, 0xFF, 0xEE };
	uint8_t output_buf1[40] = { 0 };
	uint8_t output_buf2[40] = { 0 };
	uint32_t output_size1, output_size2;
	struct cmp_context ctx1, ctx2;
	struct cmp_params params = { 0 };

	params.checksum_enabled = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx1, &params, NULL, 0));
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 42;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx2, &params, NULL, 0));


	output_size1 = cmp_compress_u16(&ctx1, output_buf1, sizeof(output_buf1), input_data,
					sizeof(input_data));
	output_size2 = cmp_compress_u16(&ctx2, output_buf2, sizeof(output_buf2), input_data,
					sizeof(input_data));

	TEST_ASSERT_CMP_SUCCESS(output_size1);
	TEST_ASSERT_CMP_SUCCESS(output_size2);
	TEST_ASSERT_EQUAL_HEX32(get_checksum(output_buf1, output_size1),
				get_checksum(output_buf2, output_size2));
}


void test_primary_compression_fallback_for_incompressible_data(void)
{
	const uint16_t input_incompressible[] = { 0xAAAA, 0xBBBB, 0xCCCC };
	const uint8_t expected_incompressible[] = { 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC };
	const uint16_t input_compressible[] = { 0, 0, 0, 0 };
	const uint8_t expected_compressible[] = { 0xAA };
	uint8_t output_buf[40] = { 0 };
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.uncompressed_fallback_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_incompressible,
				       sizeof(input_incompressible));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(input_incompressible), output_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_incompressible, cmp_hdr_get_cmp_data(output_buf),
				     sizeof(expected_incompressible));
	expected_hdr.compressed_size = output_size;
	expected_hdr.original_size = sizeof(input_incompressible);
	TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);

	/* this one is compressible */
	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_compressible,
				       sizeof(input_compressible));
	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_MAX_SIZE + sizeof(expected_compressible), output_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_compressible, cmp_hdr_get_cmp_data(output_buf),
				     sizeof(expected_compressible));
	expected_hdr.compressed_size = output_size;
	expected_hdr.original_size = sizeof(input_compressible);
	expected_hdr.preprocessing = CMP_PREPROCESS_DIFF;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 1;
	expected_hdr.encoder_outlier = 16;
	TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
}

void test_secondary_compression_fallback_for_incompressible_data(void)
{
	const uint16_t input_1[] = { 0, 0, 0, 0 };
	const uint16_t input_2[] = { 0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD };
	const uint8_t expected_2_uncompressed[] = { 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC, 0xDD, 0xDD };
	const uint8_t expected_2_compressed[] = { 0xAA };
	uint8_t work_buf[sizeof(input_2)];
	uint8_t output_buf[40] = { 0 };
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.uncompressed_fallback_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_MULTI;
	params.primary_encoder_param = 1;
	params.primary_encoder_outlier = 16;
	params.secondary_iterations = 3;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.secondary_encoder_param = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	/* normal primary compression, no fallback */
	output_size =
		cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_1, sizeof(input_1));
	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT(CMP_HDR_SIZE + sizeof(input_2) > output_size);

	/* secondary compression with uncompressed fallback */
	output_size =
		cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_2, sizeof(input_2));
	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_2_uncompressed), output_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_2_uncompressed, cmp_hdr_get_cmp_data(output_buf),
				     sizeof(expected_2_uncompressed));
	expected_hdr.compressed_size = output_size;
	expected_hdr.original_size = sizeof(input_2);
	TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);

	/* this one is compressible */
	output_size =
		cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_2, sizeof(input_2));
	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_MAX_SIZE + sizeof(expected_2_compressed), output_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_2_compressed, cmp_hdr_get_cmp_data(output_buf),
				     sizeof(expected_2_compressed));
	expected_hdr.compressed_size = output_size;
	expected_hdr.original_size = sizeof(input_1);
	expected_hdr.preprocessing = CMP_PREPROCESS_MODEL;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 1;
	expected_hdr.encoder_outlier = 16;
	expected_hdr.sequence_number = 1;
	TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
}


void test_fallback_works_with_checksum_enabled(void)
{
	const uint16_t input_incompressible[] = { 0xAAAA, 0xBBBB, 0xCCCC };
	const uint8_t expected_incompressible[] = { 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC };
	const uint16_t input_compressible[] = { 0, 0, 0, 0 };
	const uint8_t expected_compressible[] = { 0xAA };
	uint8_t output_buf[40] = { 0 };
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.uncompressed_fallback_enabled = 1;
	params.checksum_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_incompressible,
				       sizeof(input_incompressible));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + CMP_CHECKSUM_SIZE + sizeof(input_incompressible),
			  output_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_incompressible, cmp_hdr_get_cmp_data(output_buf),
				     sizeof(expected_incompressible));
	expected_hdr.checksum_enabled = 1;
	expected_hdr.compressed_size = output_size;
	expected_hdr.original_size = sizeof(input_incompressible);
	TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);

	/* this one is compressible */
	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_compressible,
				       sizeof(input_compressible));
	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_MAX_SIZE + CMP_CHECKSUM_SIZE + sizeof(expected_compressible),
			  output_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_compressible, cmp_hdr_get_cmp_data(output_buf),
				     sizeof(expected_compressible));
	expected_hdr.compressed_size = output_size;
	expected_hdr.original_size = sizeof(input_compressible);
	expected_hdr.preprocessing = CMP_PREPROCESS_DIFF;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 1;
	expected_hdr.encoder_outlier = 16;
	TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
}

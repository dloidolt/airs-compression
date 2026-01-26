/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Compression Tests
 */

#include "common/compiler.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <unity.h>
#include "test_common.h"

#include "../lib/cmp.h"
#include "../lib/cmp_errors.h"
#include "../lib/common/header_private.h"


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

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_GENERIC, work_buf_size);
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


TEST_CASE(compress_u16_wrapper, ARRAY_AND_SIZE(test_dummy_u16))
TEST_CASE(compress_i16_wrapper, ARRAY_AND_SIZE(test_dummy_i16))
TEST_CASE(compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(test_dummy_i16_in_i32))
void test_compression_in_uncompressed_mode(compress_func_t compress_func, const void *src,
					   uint32_t src_size)
{
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(test_dummy_u16))];
	/* uncompressed data should be in big endian */
	const uint8_t cmp_data_exp[] = { 0x00, 0x01, 0x02, 0x03 };
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	struct cmp_hdr hdr;

	uint32_t const cmp_size = compress_func(&ctx_uncompressed, dst, sizeof(dst), src, src_size);

	TEST_ASSERT_CMP_SUCCESS(cmp_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(cmp_data_exp), cmp_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(cmp_data_exp, cmp_hdr_get_cmp_data(dst), sizeof(cmp_data_exp));
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(dst, cmp_size, &hdr));
	TEST_ASSERT_EQUAL(CMP_VERSION_NUMBER, hdr.version_id);
	TEST_ASSERT_EQUAL(cmp_size, hdr.compressed_size);
	TEST_ASSERT_EQUAL(sizeof(cmp_data_exp), hdr.original_size);
	TEST_ASSERT_EQUAL(CMP_ENCODER_UNCOMPRESSED, hdr.encoder_type);
	TEST_ASSERT_EQUAL(CMP_PREPROCESS_NONE, hdr.preprocessing);
}


TEST_CASE(compress_u16_wrapper, ARRAY_AND_SIZE(test_dummy_u16),
	  CMP_UNCOMPRESSED_BOUND(sizeof(test_dummy_u16)) - CMP_CHECKSUM_SIZE - 1)
TEST_CASE(compress_i16_wrapper, ARRAY_AND_SIZE(test_dummy_i16),
	  CMP_UNCOMPRESSED_BOUND(sizeof(test_dummy_i16)) - CMP_CHECKSUM_SIZE - 1)
TEST_CASE(compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(test_dummy_i16_in_i32),
	  CMP_UNCOMPRESSED_BOUND(ARRAY_SIZE(test_dummy_i16_in_i32) * sizeof(int16_t)) - CMP_CHECKSUM_SIZE - 1)
void test_compression_detects_too_small_dst_buffer(compress_func_t compress_func, const void *src,
						   uint32_t src_size, uint32_t dst_size)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	void *dst = t_malloc(dst_size);

	uint32_t const cmp_size = compress_func(&ctx_uncompressed, dst, dst_size, src, src_size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_TOO_SMALL, cmp_size);

	free(dst);
}


TEST_CASE(compress_u16_wrapper, ARRAY_AND_SIZE(test_dummy_u16),
	  CMP_UNCOMPRESSED_BOUND(sizeof(test_dummy_u16)))
TEST_CASE(compress_i16_wrapper, ARRAY_AND_SIZE(test_dummy_i16),
	  CMP_UNCOMPRESSED_BOUND(sizeof(test_dummy_i16)))
TEST_CASE(compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(test_dummy_i16_in_i32),
	  CMP_UNCOMPRESSED_BOUND(ARRAY_SIZE(test_dummy_i16_in_i32) * sizeof(int16_t)))
void test_compression_detects_missing_context(compress_func_t compress_func, const void *src,
					      uint32_t src_size, uint32_t dst_size)
{
	void *dst = t_malloc(dst_size);

	uint32_t const cmp_size = compress_func(NULL, dst, dst_size, src, src_size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_GENERIC, cmp_size);

	free(dst);
}


TEST_CASE(compress_u16_wrapper, ARRAY_AND_SIZE(test_dummy_u16))
TEST_CASE(compress_i16_wrapper, ARRAY_AND_SIZE(test_dummy_i16))
TEST_CASE(compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(test_dummy_i16_in_i32))
void test_compression_detects_missing_dst_buffer(compress_func_t compress_func, const void *src,
						 uint32_t src_size)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();

	uint32_t const size = compress_func(&ctx_uncompressed, NULL, 100, src, src_size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_NULL, size);
}


TEST_MATRIX([compress_u16_wrapper, compress_i16_wrapper, compress_i16_in_i32_wrapper])
void test_compression_detects_missing_src_data(compress_func_t compress_func)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(4)];
	uint32_t random_src_size = 8;

	uint32_t const size =
		compress_func(&ctx_uncompressed, dst, sizeof(dst), NULL, random_src_size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_NULL, size);
}


TEST_MATRIX([compress_u16_wrapper, compress_i16_wrapper, compress_i16_in_i32_wrapper])
void test_compression_detects_src_size_is_0(compress_func_t compress_func)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	const uint16_t src[2] = { 0x0001, 0x0203 };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src))];

	uint32_t const cmp_size = compress_func(&ctx_uncompressed, dst, sizeof(dst), src, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_WRONG, cmp_size);
}


TEST_MATRIX([compress_u16_wrapper, compress_i16_wrapper, compress_i16_in_i32_wrapper])
void test_compression_detects_src_size_is_not_multiple_of_2(compress_func_t compress_func)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	const uint16_t src[2] = { 0 };
	uint32_t const src_size = 3;
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src))];

	uint32_t const cmp_size = compress_func(&ctx_uncompressed, dst, sizeof(dst), src, src_size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_WRONG, cmp_size);
}


TEST_MATRIX([compress_u16_wrapper, compress_i16_wrapper])
void test_compression_detects_src_size_too_large_for_header(compress_func_t compress_func)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	uint64_t dst[5];
	const uint16_t src[2] = { 0x0001, 0x0203 };
	uint32_t const src_size_too_large = 1 << CMP_HDR_BITS_ORIGINAL_SIZE;

	uint32_t const cmp_size =
		compress_func(&ctx_uncompressed, dst, sizeof(dst), src, src_size_too_large);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_HDR_ORIGINAL_TOO_LARGE, cmp_size);
}


void test_compression_detects_src_size_too_large_for_header_i16_in_i32(void)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	uint64_t dst[5];
	const int32_t src[2] = { 0x0001, 0x0203 };
	uint32_t const src_size_too_large = (1 << CMP_HDR_BITS_ORIGINAL_SIZE) * sizeof(int16_t);

	uint32_t const cmp_size = compress_i16_in_i32_wrapper(&ctx_uncompressed, dst, sizeof(dst),
							      src, src_size_too_large);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_HDR_ORIGINAL_TOO_LARGE, cmp_size);
}


TEST_MATRIX([compress_u16_wrapper, compress_i16_wrapper])
void test_compression_detects_dst_size_too_large_for_header(compress_func_t compress_func)
{
	uint32_t const src_size = CMP_HDR_MAX_COMPRESSED_SIZE & ~1UL; /* must be a multiple 2 */
	uint32_t const dst_cap = CMP_HDR_SIZE + src_size; /* to large for the cmp size field */
	uint16_t *src = calloc(src_size, 1);
	uint8_t *dst = calloc(dst_cap, 1);
	uint32_t cmp_size;
	struct cmp_context ctx_uncompressed = create_uncompressed_context();

	TEST_ASSERT_NOT_NULL(src);
	TEST_ASSERT_NOT_NULL(dst);

	cmp_size = compress_func(&ctx_uncompressed, dst, dst_cap, src, src_size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_HDR_CMP_SIZE_TOO_LARGE, cmp_size);

	free(src);
	free(dst);
}


void test_compression_detects_dst_size_too_large_for_header_i16_in_i32(void)
{
	uint32_t const packed_size = CMP_HDR_MAX_COMPRESSED_SIZE & ~1UL; /* must be a multiple 2 */
	uint32_t const src_size = sizeof(int16_t) * packed_size;
	uint32_t const dst_cap = CMP_HDR_SIZE + packed_size; /* to large for the cmp size field */
	int32_t *src = calloc(src_size, 1);
	uint8_t *dst = calloc(dst_cap, 1);
	uint32_t cmp_size;
	struct cmp_context ctx_uncompressed = create_uncompressed_context();

	TEST_ASSERT_NOT_NULL(src);
	TEST_ASSERT_NOT_NULL(dst);

	cmp_size = cmp_compress_i16_in_i32(&ctx_uncompressed, dst, dst_cap, src, src_size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_HDR_CMP_SIZE_TOO_LARGE, cmp_size);

	free(src);
	free(dst);
}


TEST_CASE(compress_u16_wrapper, ARRAY_AND_SIZE(test_dummy_u16))
TEST_CASE(compress_i16_wrapper, ARRAY_AND_SIZE(test_dummy_i16))
TEST_CASE(compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(test_dummy_i16_in_i32))
void test_compression_detects_unaligned_dst(compress_func_t compress_func, const void *src,
					    uint32_t src_size)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(8)];
	uint8_t *dst_wrong_aligned = dst + 4;
	uint32_t dst_wrong_aligned_size = sizeof(dst) - 4;

	uint32_t const cmp_size = compress_func(&ctx_uncompressed, dst_wrong_aligned,
						dst_wrong_aligned_size, src, src_size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_UNALIGNED, cmp_size);
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

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_GENERIC, return_val);
}


void test_deinitialise_a_compression_context(void)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	struct cmp_context const zero_ctx = { 0 };

	cmp_deinitialise(&ctx_uncompressed);

	TEST_ASSERT_EQUAL_MEMORY(&ctx_uncompressed, &zero_ctx, sizeof(ctx_uncompressed));
}


TEST_CASE(compress_u16_wrapper, ARRAY_AND_SIZE(test_dummy_u16))
TEST_CASE(compress_i16_wrapper, ARRAY_AND_SIZE(test_dummy_i16))
TEST_CASE(compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(test_dummy_i16_in_i32))
void test_compression_detects_too_small_work_buffer(compress_func_t compress_func, const void *src,
						    uint32_t src_size)
{
	struct cmp_params params = { 0 };
	uint32_t dst_cap = cmp_compress_bound(sizeof(test_dummy_u16));
	void *dst = t_malloc(dst_cap);
	struct cmp_context ctx;
	uint32_t work_buf_size, small_work_buf_size;
	void *small_work_buf;
	uint32_t dst_size;

	params.primary_preprocessing = CMP_PREPROCESS_IWT;
	work_buf_size = cmp_cal_work_buf_size(&params, sizeof(test_dummy_u16));
	TEST_ASSERT(work_buf_size > 0);
	small_work_buf_size = work_buf_size - 1;
	small_work_buf = t_malloc(small_work_buf_size);
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, small_work_buf, small_work_buf_size));

	dst_size = compress_func(&ctx, dst, dst_cap, src, src_size);

	TEST_ASSERT_CMP_FAILURE(dst_size);
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_TOO_SMALL, dst_size);

	free(small_work_buf);
	free(dst);
}


TEST_MATRIX([compress_u16_wrapper, compress_i16_wrapper, compress_i16_in_i32_wrapper])
void test_non_model_preprocessing_src_size_change_allowed(compress_func_t compress_func)

{
	const int32_t src1[4] = { 0 };
	const int32_t src2[3] = { 0 };
	DST_ALIGNED_U8 dst[CMP_HDR_MAX_SIZE + sizeof(src1)];
	uint16_t work_buf[ARRAY_SIZE(src1)*sizeof(int16_t)];
	uint32_t dst_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.secondary_preprocessing = CMP_PREPROCESS_IWT;
	params.secondary_iterations = 10;

	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	TEST_ASSERT_CMP_SUCCESS(compress_func(&ctx, dst, sizeof(dst), src1, sizeof(src1)));
	dst_size = compress_func(&ctx, dst, sizeof(dst), src2, sizeof(src2));

	TEST_ASSERT_CMP_SUCCESS(dst_size);
}


void test_deinitialise_NULL_context_gracefully(void)
{
	cmp_deinitialise(NULL);
}


void test_bound_size_is_enough_for_uncompressed_mode_with_checksum(void)
{
	uint32_t const bound = cmp_compress_bound(3);

	TEST_ASSERT_CMP_SUCCESS(bound);
	/* round size up to next multiple of 2 */
	TEST_ASSERT_GREATER_OR_EQUAL_UINT32(CMP_HDR_MAX_SIZE + CMP_CHECKSUM_SIZE + 4, bound);
}


void test_compress_bound_provides_sufficient_buffer_size(void)
{
	uint64_t dst[5];
	const uint16_t worst_case_data[2] = { 0xAAAA, 0xBBBB };
	struct cmp_context ctx;
	struct cmp_params worst_case_params = { 0 };
	uint32_t bound;

	worst_case_params.primary_encoder_type = CMP_ENCODER_GOLOMB_MULTI;
	worst_case_params.primary_encoder_param = 1;
	worst_case_params.primary_encoder_outlier = 32;
	worst_case_params.checksum_enabled = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &worst_case_params, NULL, 0));

	bound = cmp_compress_bound(sizeof(worst_case_data) - 1);

	TEST_ASSERT_CMP_SUCCESS(bound);
	TEST_ASSERT_LESS_OR_EQUAL(sizeof(dst), bound);
	TEST_ASSERT_CMP_SUCCESS(
		cmp_compress_u16(&ctx, dst, bound, worst_case_data, sizeof(worst_case_data)));
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_TOO_SMALL,
				    cmp_compress_u16(&ctx, dst, bound - 1, worst_case_data,
						     sizeof(worst_case_data)));
}


void test_bound_size_calculation_detects_too_large_src_size(void)
{
	uint32_t const bound = cmp_compress_bound(CMP_HDR_MAX_ORIGINAL_SIZE + 1);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_HDR_ORIGINAL_TOO_LARGE, bound);
}


static uint32_t g_coarse_time;
static uint16_t g_fine_time;
static void timestamp_stub(uint32_t *coarse, uint16_t *fine)
{
	*coarse = g_coarse_time;
	*fine = g_fine_time;
}


TEST_MATRIX([compress_u16_wrapper, compress_i16_wrapper, compress_i16_in_i32_wrapper])
void test_use_provided_timestamp_as_hdr_identifier(compress_func_t compress_func)
{
	uint16_t src[2] = { 0 };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src))];
	uint32_t cmp_size;
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	struct cmp_hdr hdr;

	cmp_set_timestamp_func(timestamp_stub);
	g_coarse_time = 0x12345678;
	g_fine_time = 0xABCD;
	cmp_size = compress_func(&ctx_uncompressed, dst, sizeof(dst), src, sizeof(src));

	TEST_ASSERT_CMP_SUCCESS(cmp_size);
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(dst, cmp_size, &hdr));
	TEST_ASSERT_EQUAL_HEX64(0x12345678ABCD, hdr.identifier);

	g_coarse_time = 0;
	g_fine_time = 0;
	cmp_set_timestamp_func(NULL);
}


uint32_t get_checksum(const void *compressed_data, uint32_t size)
{
	const uint8_t *buf = (const uint8_t *)compressed_data + size - CMP_CHECKSUM_SIZE;

	TEST_ASSERT(size > 4);
	return (uint32_t)buf[0] << 24 | (uint32_t)buf[1] << 16 | (uint32_t)buf[2] << 8 |
	       (uint32_t)buf[3];
}


void test_checksum_appended_to_compressed_data(void)
{
	const uint16_t src[] = { 0xCA, 0xFF, 0xEE };
	struct sample_desc desc;
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src))];
	uint32_t expected_checksum;
	uint32_t dst_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	TEST_ASSERT_CMP_SUCCESS(sample_read_src_init(&desc, src, sizeof(src), CMP_U16));
	expected_checksum = cmp_checksum(&desc);
	params.checksum_enabled = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	dst_size = cmp_compress_u16(&ctx, dst, sizeof(dst), src, sizeof(src));

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL((uint32_t)CMP_UNCOMPRESSED_BOUND(sizeof(src)), dst_size);
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = sizeof(src);
	expected_hdr.checksum_enabled = 1;
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
	TEST_ASSERT_EQUAL_HEX32(expected_checksum, get_checksum(dst, dst_size));
}


void test_checksum_is_different_for_different_inputs(void)
{
	const uint16_t src1[] = { 0xC0, 0xFF, 0xEE };
	const uint16_t src2[] = { 0xC0, 0xFF, 0xEF };
	DST_ALIGNED_U8 dst1[CMP_UNCOMPRESSED_BOUND(sizeof(src1))];
	DST_ALIGNED_U8 dst2[CMP_UNCOMPRESSED_BOUND(sizeof(src2))];
	uint32_t dst_size1, dst_size2;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.checksum_enabled = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	dst_size1 = cmp_compress_u16(&ctx, dst1, sizeof(dst1), src1, sizeof(src1));
	dst_size2 = cmp_compress_u16(&ctx, dst2, sizeof(dst2), src2, sizeof(src2));

	TEST_ASSERT_CMP_SUCCESS(dst_size1);
	TEST_ASSERT_CMP_SUCCESS(dst_size2);
	TEST_ASSERT_NOT_EQUAL_HEX32(get_checksum(dst1, dst_size1), get_checksum(dst2, dst_size2));
}


void test_checksum_is_same_for_same_inputs(void)
{
	const uint16_t src[] = { 0xC0, 0xFF, 0xEE };
	DST_ALIGNED_U8 dst1[CMP_UNCOMPRESSED_BOUND(sizeof(src))];
	/* dst2 needs larger buffer as compressed output exceed uncompressed bound */
	DST_ALIGNED_U8 dst2[CMP_UNCOMPRESSED_BOUND(sizeof(src)) + 16];
	uint32_t dst_size1, dst_size2;
	struct cmp_context ctx1, ctx2;
	struct cmp_params params = { 0 };

	params.checksum_enabled = 0xFF;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx1, &params, NULL, 0));
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 42;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx2, &params, NULL, 0));

	dst_size1 = cmp_compress_u16(&ctx1, dst1, sizeof(dst1), src, sizeof(src));
	dst_size2 = cmp_compress_u16(&ctx2, dst2, sizeof(dst2), src, sizeof(src));

	TEST_ASSERT_CMP_SUCCESS(dst_size1);
	TEST_ASSERT_CMP_SUCCESS(dst_size2);
	TEST_ASSERT_EQUAL_HEX32(get_checksum(dst1, dst_size1), get_checksum(dst2, dst_size2));
}


void test_checksum_is_same_for_same_inputs_of_every_compression_function(void)
{
	struct {
		compress_func_t compress_func;
		const void *src;
		uint32_t src_size;
	} test_cases[] = {
		{ compress_u16_wrapper,        ARRAY_AND_SIZE(test_dummy_u16)        },
		{ compress_i16_wrapper,        ARRAY_AND_SIZE(test_dummy_i16)        },
		{ compress_i16_in_i32_wrapper, ARRAY_AND_SIZE(test_dummy_i16_in_i32) }
	};
	uint32_t dst_size;
	struct test_env *e;
	struct cmp_params params = { 0 };
	size_t i;
	uint32_t checksum;

	params.checksum_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 42;
	e = make_env(&params, test_cases[0].src_size);
	for (i = 0; i < ARRAY_SIZE(test_cases); i++) {
		dst_size = test_cases[i].compress_func(&e->ctx, e->dst, e->dst_cap,
						       test_cases[i].src, test_cases[i].src_size);

		TEST_ASSERT_CMP_SUCCESS(dst_size);
		if (i == 0)
			checksum = get_checksum(e->dst, dst_size);
		else
			TEST_ASSERT_EQUAL_HEX32(checksum, get_checksum(e->dst, dst_size));
	}
	free_env(e);
}


TEST_MATRIX([compress_u16_wrapper, compress_i16_wrapper])
void test_primary_compression_fallback_for_incompressible_data(compress_func_t compress_func)
{
	const uint16_t src_incompressible[] = { 0xAAAA, 0xBBBB, 0xCCCC };
	const uint8_t expected_incompressible[] = { 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC };
	const uint16_t src_compressible[] = { 0, 0, 0, 0 };
	const uint8_t expected_compressible[] = { 0xAA };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src_incompressible))];
	uint32_t dst_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.uncompressed_fallback_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	dst_size = compress_func(&ctx, dst, sizeof(dst), src_incompressible,
				 sizeof(src_incompressible));

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(src_incompressible), dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_incompressible, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_incompressible));
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = sizeof(src_incompressible);
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);

	/* this one is compressible */
	dst_size =
		compress_func(&ctx, dst, sizeof(dst), src_compressible, sizeof(src_compressible));
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_MAX_SIZE + sizeof(expected_compressible), dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_compressible, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_compressible));
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = sizeof(src_compressible);
	expected_hdr.preprocessing = CMP_PREPROCESS_DIFF;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 1;
	expected_hdr.encoder_outlier = 16;
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
}


void test_primary_compression_fallback_for_incompressible_data_i16_in_i32(void)
{
	const int32_t src_incompressible[] = { 0xAAAA, 0xBBBB, 0xCCCC };
	const uint8_t expected_incompressible[] = { 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC };
	const int32_t src_compressible[] = { 0, 0, 0, 0 };
	const uint8_t expected_compressible[] = { 0xAA };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(ARRAY_SIZE(src_incompressible) * sizeof(int16_t))];
	uint32_t dst_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.uncompressed_fallback_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	dst_size = cmp_compress_i16_in_i32(&ctx, dst, sizeof(dst), src_incompressible,
					   sizeof(src_incompressible));

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_incompressible), dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_incompressible, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_incompressible));
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = sizeof(expected_incompressible);
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);

	/* this one is compressible */
	dst_size = cmp_compress_i16_in_i32(&ctx, dst, sizeof(dst), src_compressible,
					   sizeof(src_compressible));
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_MAX_SIZE + sizeof(expected_compressible), dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_compressible, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_compressible));
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = ARRAY_SIZE(src_compressible)*sizeof(int16_t);
	expected_hdr.preprocessing = CMP_PREPROCESS_DIFF;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 1;
	expected_hdr.encoder_outlier = 16;
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
}


TEST_MATRIX([compress_u16_wrapper, compress_i16_wrapper])
void test_secondary_compression_fallback_for_incompressible_data(compress_func_t compress_func)
{
	const uint16_t src_1[] = { 0, 0, 0, 0 };
	const uint16_t src_2[ARRAY_SIZE(src_1)] = { 0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD };
	const uint8_t expected_2_uncompressed[] = { 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC, 0xDD, 0xDD };
	const uint8_t expected_2_compressed[] = { 0xAA };
	uint16_t work_buf[ARRAY_SIZE(src_1)];
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src_1))];
	uint32_t dst_size;
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
	dst_size = compress_func(&ctx, dst, sizeof(dst), src_1, sizeof(src_1));
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT(CMP_HDR_SIZE + sizeof(src_2) > dst_size);

	/* secondary compression with uncompressed fallback */
	dst_size = compress_func(&ctx, dst, sizeof(dst), src_2, sizeof(src_2));
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_2_uncompressed), dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_2_uncompressed, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_2_uncompressed));
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = sizeof(src_2);
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);

	/* this one is compressible */
	dst_size = compress_func(&ctx, dst, sizeof(dst), src_2, sizeof(src_2));
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_MAX_SIZE + sizeof(expected_2_compressed), dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_2_compressed, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_2_compressed));
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = sizeof(src_1);
	expected_hdr.preprocessing = CMP_PREPROCESS_MODEL;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 1;
	expected_hdr.encoder_outlier = 16;
	expected_hdr.sequence_number = 1;
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
}


void test_secondary_compression_fallback_for_incompressible_data_i16_i32(void)
{
	const int32_t src_1[] = { 0, 0, 0, 0 };
	const int32_t src_2[ARRAY_SIZE(src_1)] = { 0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD };
	const uint8_t expected_2_uncompressed[] = { 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC, 0xDD, 0xDD };
	const uint8_t expected_2_compressed[] = { 0xAA };
	uint16_t work_buf[ARRAY_SIZE(src_1)];
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(ARRAY_SIZE(src_1) * sizeof(int16_t))];
	uint32_t dst_size;
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
	dst_size = cmp_compress_i16_in_i32(&ctx, dst, sizeof(dst), src_1, sizeof(src_1));
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT(CMP_HDR_SIZE + sizeof(src_2) > dst_size);

	/* secondary compression with uncompressed fallback */
	dst_size = cmp_compress_i16_in_i32(&ctx, dst, sizeof(dst), src_2, sizeof(src_2));
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_2_uncompressed), dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_2_uncompressed, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_2_uncompressed));
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = ARRAY_SIZE(src_2) * sizeof(int16_t);
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);

	/* this one is compressible */
	dst_size = cmp_compress_i16_in_i32(&ctx, dst, sizeof(dst), src_2, sizeof(src_2));
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_MAX_SIZE + sizeof(expected_2_compressed), dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_2_compressed, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_2_compressed));
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = ARRAY_SIZE(src_2) * sizeof(int16_t);
	expected_hdr.preprocessing = CMP_PREPROCESS_MODEL;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 1;
	expected_hdr.encoder_outlier = 16;
	expected_hdr.sequence_number = 1;
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
}


TEST_MATRIX([compress_u16_wrapper, compress_i16_wrapper])
void test_fallback_to_uncompressed_works_with_checksum_enabled(compress_func_t compress_func)
{
	const uint16_t src_incompressible[] = { 0xAAAA, 0xBBBB, 0xCCCC };
	const uint8_t expected_incompressible[] = { 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src_incompressible))];
	uint32_t dst_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.uncompressed_fallback_enabled = 1;
	params.checksum_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	dst_size = compress_func(&ctx, dst, sizeof(dst), src_incompressible,
				 sizeof(src_incompressible));

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(sizeof(src_incompressible)), dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_incompressible, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_incompressible));
	expected_hdr.checksum_enabled = 1;
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = sizeof(src_incompressible);
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
}


void test_fallback_to_uncompressed_works_with_checksum_enabled_i32_in_16(void)
{
	const int32_t src_incompressible[] = { 0xAAAA, 0xBBBB, 0xCCCC };
	const uint8_t expected_incompressible[] = { 0xAA, 0xAA, 0xBB, 0xBB, 0xCC, 0xCC };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(ARRAY_SIZE(src_incompressible) * sizeof(int16_t))];
	uint32_t dst_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.uncompressed_fallback_enabled = 1;
	params.checksum_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	dst_size = cmp_compress_i16_in_i32(&ctx, dst, sizeof(dst), src_incompressible,
					   sizeof(src_incompressible));

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(ARRAY_SIZE(src_incompressible)*sizeof(int16_t)), dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_incompressible, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_incompressible));
	expected_hdr.checksum_enabled = 1;
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = ARRAY_SIZE(src_incompressible)*sizeof(int16_t);
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
}


TEST_MATRIX([compress_u16_wrapper, compress_i16_wrapper])
void test_compression_works_with_checksum_enabled(compress_func_t compress_func)
{
	const uint16_t src_compressible[] = { 0, 0, 0, 0 };
	const uint8_t expected_compressible[] = { 0xAA };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src_compressible))];
	uint32_t dst_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.uncompressed_fallback_enabled = 1;
	params.checksum_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	dst_size =
		compress_func(&ctx, dst, sizeof(dst), src_compressible, sizeof(src_compressible));
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_MAX_SIZE + CMP_CHECKSUM_SIZE + sizeof(expected_compressible),
			  dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_compressible, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_compressible));
	expected_hdr.checksum_enabled = 1;
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = sizeof(src_compressible);
	expected_hdr.preprocessing = CMP_PREPROCESS_DIFF;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 1;
	expected_hdr.encoder_outlier = 16;
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
}


void test_compression_works_with_checksum_enabled_i16_in_i32(void)
{
	const int32_t src_compressible[] = { 0, 0, 0, 0 };
	const uint8_t expected_compressible[] = { 0xAA };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(ARRAY_SIZE(src_compressible) * sizeof(int16_t))];
	uint32_t dst_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.uncompressed_fallback_enabled = 1;
	params.checksum_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	dst_size = cmp_compress_i16_in_i32(&ctx, dst, sizeof(dst), src_compressible,
					   sizeof(src_compressible));
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_MAX_SIZE + CMP_CHECKSUM_SIZE + sizeof(expected_compressible),
			  dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_compressible, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_compressible));
	expected_hdr.checksum_enabled = 1;
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = ARRAY_SIZE(src_compressible)*sizeof(int16_t);
	expected_hdr.preprocessing = CMP_PREPROCESS_DIFF;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 1;
	expected_hdr.encoder_outlier = 16;
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
}


TEST_MATRIX([compress_u16_wrapper, compress_i16_wrapper, compress_i16_in_i32_wrapper])
void test_compression_fails_when_capacity_is_an_error(compress_func_t compress_func)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	int32_t src[2] = { 0 };
	DST_ALIGNED_U8 dst_dummy[CMP_UNCOMPRESSED_BOUND(sizeof(src))] = { 0 };

	uint32_t const bound_error = cmp_compress_bound(CMP_HDR_MAX_ORIGINAL_SIZE + 1);
	uint32_t const return_val =
		compress_func(&ctx_uncompressed, dst_dummy, bound_error, src, sizeof(src));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_GENERIC, return_val);
}


TEST_MATRIX([compress_u16_wrapper, compress_i16_wrapper, compress_i16_in_i32_wrapper])
void test_detect_uninitialise_context_in_compression(compress_func_t compress_func)
{
	const int32_t src[] = { 0 };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src))];
	struct cmp_context ctx;
	uint32_t return_val;

	cmp_deinitialise(&ctx);
	return_val = compress_func(&ctx, dst, sizeof(dst), src, sizeof(src));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_CONTEXT_INVALID, return_val);
}


void test_detect_uninitialise_context_in_reset(void)
{
	uint32_t return_val;
	struct cmp_context ctx = { 0 };

	cmp_deinitialise(&ctx);
	return_val = cmp_reset(&ctx);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_CONTEXT_INVALID, return_val);
}

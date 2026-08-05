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
#include "../lib/cmp_header.h"
#include "../lib/cmp_errors.h"


static const int32_t dummy_samples[2] = { 1, 2 };


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


TEST_MATRIX([CMP_U16, CMP_I16, CMP_I16_IN_I32])
void test_no_work_buf_needed_for_none_preprocessing(enum cmp_type src_type)
{
	struct cmp_params par_uncompressed = { 0 };
	uint32_t work_buf_size;

	par_uncompressed.primary_preprocessing = CMP_PREPROCESS_NONE;

	work_buf_size = cmp_cal_work_buf_size(&par_uncompressed, 42, src_type);

	TEST_ASSERT_EQUAL(0, work_buf_size);
}


TEST_MATRIX([CMP_U16, CMP_I16, CMP_I16_IN_I32])
void test_calculate_work_buf_size_for_iwt_correctly(enum cmp_type src_type)
{
	struct arena *a = clear_test_arena();
	const int32_t samples[] = { 1, 2, 3, 4 };
	struct test_src const src = make_test_src(a, src_type, samples, ARRAY_SIZE(samples));
	struct cmp_params par = { 0 };
	uint32_t work_buf_size;

	par.primary_preprocessing = CMP_PREPROCESS_IWT;

	work_buf_size = cmp_cal_work_buf_size(&par, src.size, src_type);

	TEST_ASSERT_EQUAL(8, work_buf_size);
}


TEST_MATRIX([CMP_U16, CMP_I16, CMP_I16_IN_I32])
void test_calculate_work_buf_size_rounds_up_even(enum cmp_type src_type)
{
	struct arena *a = clear_test_arena();
	const int32_t samples[] = { 1, 2, 3, 4 };
	struct test_src const src3 = make_test_src(a, src_type, samples, ARRAY_SIZE(samples) - 1);
	struct test_src const src4 = make_test_src(a, src_type, samples, ARRAY_SIZE(samples));
	struct cmp_params par = { 0 };
	uint32_t s;
	uint32_t work_buf_size;

	par.primary_preprocessing = CMP_PREPROCESS_IWT;

	for (s = src3.size + 1; s <= src4.size; s++) {
		work_buf_size = cmp_cal_work_buf_size(&par, s, src_type);
		TEST_ASSERT_EQUAL(8, work_buf_size);
	}
}


TEST_MATRIX([CMP_U16, CMP_I16, CMP_I16_IN_I32])
void test_calculate_work_buf_size_rounds_up_uneven(enum cmp_type src_type)
{
	struct arena *a = clear_test_arena();
	const int32_t samples[] = { 1, 2, 3, 4, 5 };
	struct test_src const src4 = make_test_src(a, src_type, samples, ARRAY_SIZE(samples) - 1);
	struct test_src const src5 = make_test_src(a, src_type, samples, ARRAY_SIZE(samples));
	struct cmp_params par = { 0 };
	uint32_t s;
	uint32_t work_buf_size;

	par.primary_preprocessing = CMP_PREPROCESS_IWT;

	for (s = src4.size + 1; s <= src5.size; s++) {
		work_buf_size = cmp_cal_work_buf_size(&par, s, src_type);
		TEST_ASSERT_EQUAL(10, work_buf_size);
	}
}


TEST_CASE(CMP_U16, CMP_HDR_MAX_ORIGINAL_SIZE - 1, CMP_HDR_MAX_ORIGINAL_SIZE - 1)
TEST_CASE(CMP_I16, CMP_HDR_MAX_ORIGINAL_SIZE - 1, CMP_HDR_MAX_ORIGINAL_SIZE - 1)
TEST_CASE(CMP_I16_IN_I32, (CMP_HDR_MAX_ORIGINAL_SIZE - 1) * 2, CMP_HDR_MAX_ORIGINAL_SIZE - 1)
void test_calculate_work_buf_size_rounds_up_maximum_allowed_size(enum cmp_type src_type,
								 uint32_t max_size,
								 uint32_t exp_size)
{
	/* In this case we have a large work_buf as ever needed */
	struct cmp_params par = { 0 };
	uint32_t work_buf_size;

	par.primary_preprocessing = CMP_PREPROCESS_IWT;

	work_buf_size = cmp_cal_work_buf_size(&par, max_size, src_type);

	TEST_ASSERT_CMP_SUCCESS(work_buf_size);
	TEST_ASSERT_EQUAL(exp_size, work_buf_size);
}


TEST_MATRIX([CMP_U16, CMP_I16, CMP_I16_IN_I32])
void test_calculate_work_buf_size_ignore_secondary_preprocessing_if_disabled(enum cmp_type src_type)
{
	struct cmp_params par = { 0 };
	uint32_t work_buf_size;

	par.primary_preprocessing = CMP_PREPROCESS_NONE;
	par.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	par.secondary_iterations = 0;

	work_buf_size = cmp_cal_work_buf_size(&par, 41, src_type);

	TEST_ASSERT_CMP_SUCCESS(work_buf_size);
	TEST_ASSERT_EQUAL(0, work_buf_size);
}


void test_work_buf_size_calculation_detects_missing_parameters_struct(void)
{
	uint32_t work_buf_size;

	work_buf_size = cmp_cal_work_buf_size(NULL, 42, CMP_I16);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_GENERIC, work_buf_size);
}


void test_work_buf_size_calculation_detects_invalid_primary_preprocessing(void)
{
	struct cmp_params par_uncompressed = { 0 };
	uint32_t work_buf_size;

	par_uncompressed.primary_preprocessing = -1U;

	work_buf_size = cmp_cal_work_buf_size(&par_uncompressed, 42, CMP_I16);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, work_buf_size);
}


void test_work_buf_size_calculation_detects_invalid_secondary_preprocessing(void)
{
	struct cmp_params par_uncompressed = { 0 };
	uint32_t work_buf_size;

	par_uncompressed.secondary_preprocessing = -1U;
	par_uncompressed.secondary_iterations = 1;

	work_buf_size = cmp_cal_work_buf_size(&par_uncompressed, 42, CMP_I16);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, work_buf_size);
}


void test_work_buf_size_calculation_detects_invalid_source_type(void)
{
	struct cmp_params params = { 0 };
	uint32_t work_buf_size;

	params.primary_preprocessing = CMP_PREPROCESS_IWT;

	work_buf_size = cmp_cal_work_buf_size(&params, 42, (enum cmp_type) - 1);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, work_buf_size);
}


TEST_CASE(CMP_U16, CMP_HDR_MAX_ORIGINAL_SIZE)
TEST_CASE(CMP_I16, CMP_HDR_MAX_ORIGINAL_SIZE)
TEST_CASE(CMP_I16_IN_I32, CMP_HDR_MAX_ORIGINAL_SIZE * 2)
void test_work_buf_size_calculation_detects_too_large_src_size(enum cmp_type src_type,
							       uint32_t too_large_src_size)
{
	struct cmp_params params = { 0 };
	uint32_t work_buf_size;

	params.primary_preprocessing = CMP_PREPROCESS_IWT;

	work_buf_size = cmp_cal_work_buf_size(&params, too_large_src_size, src_type);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_HDR_ORIGINAL_TOO_LARGE, work_buf_size);
}


TEST_MATRIX([CMP_U16, CMP_I16, CMP_I16_IN_I32])
void test_work_buf_size_calculation_rejects_max_src_size(enum cmp_type src_type)
{
	struct cmp_params params = { 0 };
	uint32_t work_buf_size;

	params.primary_preprocessing = CMP_PREPROCESS_IWT;

	work_buf_size = cmp_cal_work_buf_size(&params, UINT32_MAX, src_type);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_HDR_ORIGINAL_TOO_LARGE, work_buf_size);
}


/* uncompressed 16-bit data should be in big endian */
const uint8_t expected_uncompressed_16bit[12] = { 0x00, 0x00, 0x0A, 0xBC, 0x0D, 0xEF,
						  0x01, 0x23, 0x04, 0x56, 0x0F, 0xF0 };

TEST_CASE(&t_fix_u16, ARRAY_AND_SIZE(expected_uncompressed_16bit))
TEST_CASE(&t_fix_i16, ARRAY_AND_SIZE(expected_uncompressed_16bit))
TEST_CASE(&t_fix_i16_in_i32, ARRAY_AND_SIZE(expected_uncompressed_16bit))
void test_compression_in_uncompressed_mode(const struct t_fixture *fix,
					   const uint8_t *exp_uncompressed, uint32_t expected_size)
{
	struct arena *a = clear_test_arena();
	const int32_t samples[] = { 0, 0xABC, 0xDEF, 0x123, 0x456, 0xFF0 };
	struct test_src src = make_test_src(a, fix->dtype, samples, ARRAY_SIZE(samples));
	uint32_t const dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src.packed_size);
	uint8_t *dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	struct cmp_hdr expected_hdr = { 0 };

	uint32_t const dst_size =
		fix->compress(&ctx_uncompressed, dst, dst_cap, src.data, src.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + expected_size, dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(exp_uncompressed, cmp_hdr_get_cmp_data(dst), expected_size);
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = src.packed_size;
	expected_hdr.original_dtype = fix->dtype;
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
}


TEST_CASE(&t_fix_u16, expected_uncompressed_16bit,
	  sizeof(expected_uncompressed_16bit) - sizeof(uint16_t))
TEST_CASE(&t_fix_i16, expected_uncompressed_16bit,
	  sizeof(expected_uncompressed_16bit) - sizeof(int16_t))
TEST_CASE(&t_fix_i16_in_i32, expected_uncompressed_16bit,
	  sizeof(expected_uncompressed_16bit) - sizeof(int16_t))
void test_compression_in_uncompressed_mode_with_uneven_samples(const struct t_fixture *fix,
							       const uint8_t *exp_uncompressed,
							       uint32_t expected_size)
{
	const int32_t samples[] = { 0, 0xABC, 0xDEF, 0x123, 0x456 };
	struct arena *a = clear_test_arena();
	struct test_src src = make_test_src(a, fix->dtype, samples, ARRAY_SIZE(samples));
	uint32_t const dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src.packed_size);
	uint8_t *dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	struct cmp_hdr expected_hdr = { 0 };

	uint32_t const dst_size =
		fix->compress(&ctx_uncompressed, dst, dst_cap, src.data, src.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + expected_size, dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(exp_uncompressed, cmp_hdr_get_cmp_data(dst), expected_size);
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = src.packed_size;
	expected_hdr.original_dtype = fix->dtype;
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_compression_detects_too_small_dst_buffer(const struct t_fixture *fix)
{
	struct arena *a = clear_test_arena();
	struct test_src src =
		make_test_src(a, fix->dtype, dummy_samples, ARRAY_SIZE(dummy_samples));
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	uint32_t const dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src.packed_size) - 1;
	uint8_t *dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);

	uint32_t const cmp_size =
		fix->compress(&ctx_uncompressed, dst, dst_cap, src.data, src.size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_TOO_SMALL, cmp_size);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_compression_detects_missing_context(const struct t_fixture *fix)
{
	struct arena *a = clear_test_arena();
	struct test_src src =
		make_test_src(a, fix->dtype, dummy_samples, ARRAY_SIZE(dummy_samples));
	uint32_t const dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src.packed_size);
	uint8_t *dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);

	uint32_t const cmp_size = fix->compress(NULL, dst, dst_cap, src.data, src.size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_GENERIC, cmp_size);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_compression_detects_missing_dst_buffer(const struct t_fixture *fix)
{
	struct arena *a = clear_test_arena();
	struct test_src src =
		make_test_src(a, fix->dtype, dummy_samples, ARRAY_SIZE(dummy_samples));
	struct cmp_context ctx_uncompressed = create_uncompressed_context();

	uint32_t const size = fix->compress(&ctx_uncompressed, NULL, 100, src.data, src.size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_NULL, size);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_compression_detects_missing_src_data(const struct t_fixture *fix)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(4)];
	uint32_t random_src_size = 8;

	uint32_t const size =
		fix->compress(&ctx_uncompressed, dst, sizeof(dst), NULL, random_src_size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_NULL, size);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_compression_detects_src_size_is_0(const struct t_fixture *fix)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	const uint16_t src[2] = { 0x0001, 0x0203 };
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src))];

	uint32_t const cmp_size = fix->compress(&ctx_uncompressed, dst, sizeof(dst), src, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_WRONG, cmp_size);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_compression_detects_invalid_src_size(const struct t_fixture *fix)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	const uint16_t src[4] = { 0 };
	uint32_t const src_size = 7;
	DST_ALIGNED_U8 dst[CMP_UNCOMPRESSED_BOUND(sizeof(src))];

	uint32_t const cmp_size = fix->compress(&ctx_uncompressed, dst, sizeof(dst), src, src_size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_WRONG, cmp_size);
}


TEST_CASE(&t_fix_u16, 1 << CMP_HDR_BITS_ORIGINAL_SIZE)
TEST_CASE(&t_fix_i16, 1 << CMP_HDR_BITS_ORIGINAL_SIZE)
TEST_CASE(&t_fix_i16_in_i32, (1U << CMP_HDR_BITS_ORIGINAL_SIZE) * sizeof(int16_t))
void test_compression_detects_src_size_too_large_for_header(const struct t_fixture *fix,
							    uint32_t src_size_too_large)
{
	struct arena *a = clear_test_arena();
	/* we use dummy data and use a too long src size, to save some memory */
	struct test_src src =
		make_test_src(a, fix->dtype, dummy_samples, ARRAY_SIZE(dummy_samples));
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	uint64_t dst[5]; /* chose randomly size */

	uint32_t const cmp_size =
		fix->compress(&ctx_uncompressed, dst, sizeof(dst), src.data, src_size_too_large);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_HDR_ORIGINAL_TOO_LARGE, cmp_size);
}


TEST_CASE(&t_fix_u16, CMP_HDR_MAX_COMPRESSED_SIZE & ~1UL) /* must be a multiple of 2 */
TEST_CASE(&t_fix_i16, CMP_HDR_MAX_COMPRESSED_SIZE & ~1UL)
TEST_CASE(&t_fix_i16_in_i32,
	  (CMP_HDR_MAX_COMPRESSED_SIZE & ~1UL) * (sizeof(int32_t) / sizeof(int16_t)))
void test_compression_detects_dst_size_too_large_for_header(const struct t_fixture *fix,
							    uint32_t src_size)
{
	uint32_t const dst_cap = CMP_HDR_MAX_COMPRESSED_SIZE + 100;
	void *src = calloc(src_size, 1);
	uint8_t *dst = calloc(dst_cap, 1);
	uint32_t cmp_size;
	struct cmp_context ctx_uncompressed = create_uncompressed_context();

	if (!src || !dst) {
		free(src);
		free(dst);
		TEST_IGNORE_MESSAGE("Memory allocation failed; ignoring this test.");
	}

	cmp_size = fix->compress(&ctx_uncompressed, dst, dst_cap, src, src_size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_HDR_CMP_SIZE_TOO_LARGE, cmp_size);

	free(src);
	free(dst);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_compression_detects_unaligned_dst(const struct t_fixture *fix)
{
	struct arena *a = clear_test_arena();
	struct test_src src =
		make_test_src(a, fix->dtype, dummy_samples, ARRAY_SIZE(dummy_samples));
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	uint32_t const dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src.packed_size) + 4;
	uint8_t *dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	uint8_t *dst_wrong_aligned = dst + 4;
	uint32_t dst_wrong_aligned_size = dst_cap - 4;

	uint32_t const cmp_size = fix->compress(&ctx_uncompressed, dst_wrong_aligned,
						dst_wrong_aligned_size, src.data, src.size);

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


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_compression_detects_too_small_work_buffer(const struct t_fixture *fix)
{
	struct cmp_params params = { 0 };
	struct arena *a = clear_test_arena();
	struct test_src src =
		make_test_src(a, fix->dtype, dummy_samples, ARRAY_SIZE(dummy_samples));
	uint32_t dst_cap = cmp_compress_bound(src.size, fix->dtype);
	void *dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	struct cmp_context ctx;
	uint32_t work_buf_size, small_work_buf_size;
	void *small_work_buf;
	uint32_t dst_size;

	params.primary_preprocessing = CMP_PREPROCESS_IWT;
	work_buf_size = cmp_cal_work_buf_size(&params, src.size, fix->dtype);
	TEST_ASSERT(work_buf_size > 0);
	small_work_buf_size = work_buf_size - 1;
	small_work_buf = arena_alloc(a, (ptrdiff_t)small_work_buf_size, 1, sizeof(uint16_t));
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, small_work_buf, small_work_buf_size));

	dst_size = fix->compress(&ctx, dst, dst_cap, src.data, src.size);

	TEST_ASSERT_CMP_FAILURE(dst_size);
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_TOO_SMALL, dst_size);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_non_model_preprocessing_src_size_change_allowed(const struct t_fixture *fix)
{
	const int32_t samples_4[4] = { 0, 0, 0, 0 };
	const int32_t samples_2[2] = { 0, 0 };
	struct arena *a = clear_test_arena();
	struct test_src src1 = make_test_src(a, fix->dtype, samples_4, ARRAY_SIZE(samples_4));
	struct test_src src2 = make_test_src(a, fix->dtype, samples_2, ARRAY_SIZE(samples_2));
	uint8_t *dst = arena_alloc(a, (ptrdiff_t)cmp_compress_bound(src1.size, fix->dtype), 1,
				   CMP_DST_ALIGNMENT);
	uint16_t work_buf[ARRAY_SIZE(samples_4) * sizeof(int16_t)];
	uint32_t dst_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.secondary_preprocessing = CMP_PREPROCESS_IWT;
	params.secondary_iterations = 10;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	TEST_ASSERT_CMP_SUCCESS(fix->compress(&ctx, dst, cmp_compress_bound(src1.size, fix->dtype),
					      src1.data, src1.size));
	dst_size = fix->compress(&ctx, dst, cmp_compress_bound(src1.size, fix->dtype), src2.data,
				 src2.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
}


void test_deinitialise_NULL_context_gracefully(void)
{
	cmp_deinitialise(NULL);
}


TEST_MATRIX([CMP_U16, CMP_I16, CMP_I16_IN_I32])
void test_compress_bound_rounds_up_partial_even_samples(enum cmp_type src_type)
{
	struct arena *a = clear_test_arena();
	const int32_t samples[] = { 1, 2, 3, 4 };
	struct test_src const src3 = make_test_src(a, src_type, samples, 3);
	struct test_src const src4 = make_test_src(a, src_type, samples, 4);
	uint32_t s;

	uint32_t const bound_unround = cmp_compress_bound(src4.size, src_type);
	for (s = src3.size + 1; s <= src4.size; s++) {
		uint32_t const bound = cmp_compress_bound(s, src_type);

		TEST_ASSERT_CMP_SUCCESS(bound);
		TEST_ASSERT_EQUAL(bound_unround, bound);
	}
}


TEST_MATRIX([CMP_U16, CMP_I16, CMP_I16_IN_I32])
void test_compress_bound_rounds_up_partial_uneven_samples(enum cmp_type src_type)
{
	struct arena *a = clear_test_arena();
	const int32_t samples[] = { 1, 2, 3, 4, 5 };
	struct test_src const src4 = make_test_src(a, src_type, samples, 4);
	struct test_src const src5 = make_test_src(a, src_type, samples, 5);
	uint32_t s;

	uint32_t const bound_unround = cmp_compress_bound(src5.size, src_type);
	for (s = src4.size + 1; s <= src5.size; s++) {
		uint32_t const bound = cmp_compress_bound(s, src_type);

		TEST_ASSERT_CMP_SUCCESS(bound);
		TEST_ASSERT_EQUAL(bound_unround, bound);
	}
}


void test_compress_bound_provides_sufficient_buffer_size(void)
{
	const uint16_t worst_case_src[2] = { 0xAAAA, 0xBBBB };
	DST_ALIGNED_U8 dst[CMP_HDR_SIZE + (2 * (4 + 2))];
	struct cmp_context ctx;
	struct cmp_params worst_case_params = { 0 };
	uint32_t bound;

	worst_case_params.primary_encoder_type = CMP_ENCODER_GOLOMB_MULTI;
	worst_case_params.primary_encoder_param = 1;
	worst_case_params.primary_encoder_outlier = 32;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &worst_case_params, NULL, 0));

	bound = cmp_compress_bound(sizeof(worst_case_src), CMP_U16);

	TEST_ASSERT_CMP_SUCCESS(bound);
	TEST_ASSERT_LESS_OR_EQUAL(sizeof(dst), bound);
	TEST_ASSERT_CMP_SUCCESS(
		cmp_compress_u16(&ctx, dst, bound, worst_case_src, sizeof(worst_case_src)));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_TOO_SMALL,
				    cmp_compress_u16(&ctx, dst, bound - 1, worst_case_src,
						     sizeof(worst_case_src)));
}


TEST_CASE(CMP_U16, CMP_HDR_MAX_ORIGINAL_SIZE)
TEST_CASE(CMP_I16, CMP_HDR_MAX_ORIGINAL_SIZE)
TEST_CASE(CMP_I16_IN_I32, CMP_HDR_MAX_ORIGINAL_SIZE * 2)
void test_bound_size_calculation_detects_too_large_src_size(enum cmp_type src_type,
							    uint32_t too_large_src_size)
{
	uint32_t const bound = cmp_compress_bound(too_large_src_size, src_type);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_HDR_ORIGINAL_TOO_LARGE, bound);
}


TEST_MATRIX([CMP_U16, CMP_I16, CMP_I16_IN_I32])
void test_bound_size_calculation_detects_too_large_max_src_size(enum cmp_type src_type)
{
	uint32_t const bound = cmp_compress_bound(UINT32_MAX, src_type);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_HDR_ORIGINAL_TOO_LARGE, bound);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_set_hdr_identifier(const struct t_fixture *fix)
{
	const int32_t samples[2] = { 0 };
	struct arena *a = clear_test_arena();
	struct test_src src = make_test_src(a, fix->dtype, samples, ARRAY_SIZE(samples));
	uint32_t const dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src.packed_size);
	void *dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	uint32_t cmp_size;
	struct cmp_context ctx_uncompressed;
	struct cmp_hdr hdr;

	cmp_hdr_set_identifier(0xDEADCAFE);
	ctx_uncompressed = create_uncompressed_context();
	cmp_size = fix->compress(&ctx_uncompressed, dst, dst_cap, src.data, src.size);

	TEST_ASSERT_CMP_SUCCESS(cmp_size);
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(dst, cmp_size, &hdr));
	TEST_ASSERT_EQUAL_HEX(0xDEADCAFE, hdr.identifier);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_primary_compression_fallback_for_incompressible_data(const struct t_fixture *fix)
{
	static const int32_t samples[] = { 0xAAA, 0xBBB, 0xCCC, 0xDDD };
	struct arena *a = clear_test_arena();
	struct test_src src = make_test_src(a, fix->dtype, samples, ARRAY_SIZE(samples));
	uint32_t const dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src.packed_size);
	uint8_t *expected_dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	uint8_t *fallback_dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	struct cmp_context uncompressed_ctx = create_uncompressed_context();
	struct cmp_context ctx;
	struct cmp_hdr expected_hdr = { 0 };
	uint32_t uncompressed_size;
	uint32_t fallback_size;
	struct cmp_params params = { 0 };

	params.uncompressed_fallback_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	fallback_size = fix->compress(&ctx, fallback_dst, dst_cap, src.data, src.size);

	TEST_ASSERT_CMP_SUCCESS(fallback_size);
	uncompressed_size =
		fix->compress(&uncompressed_ctx, expected_dst, dst_cap, src.data, src.size);
	TEST_ASSERT_EQUAL(uncompressed_size, fallback_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(cmp_hdr_get_cmp_data(expected_dst),
				     cmp_hdr_get_cmp_data(fallback_dst),
				     uncompressed_size - CMP_HDR_SIZE);
	expected_hdr.compressed_size = uncompressed_size;
	expected_hdr.original_size = src.packed_size;
	expected_hdr.original_dtype = fix->dtype;
	TEST_ASSERT_CMP_HDR(fallback_dst, fallback_size, expected_hdr);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_secondary_compression_fallback_for_incompressible_data(const struct t_fixture *fix)
{
	static const int32_t primary_samples[] = { 0, 0, 0, 0 };
	static const int32_t samples[] = { 0xAAA, 0xBBB, 0xCCC, 0xDDD };
	struct arena *a = clear_test_arena();
	struct test_src primary_src =
		make_test_src(a, fix->dtype, primary_samples, ARRAY_SIZE(primary_samples));
	struct test_src fallback_src = make_test_src(a, fix->dtype, samples, ARRAY_SIZE(samples));
	uint16_t work_buf[ARRAY_SIZE(primary_samples)];
	uint32_t const dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(fallback_src.packed_size);
	uint8_t *dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	uint8_t *expected_dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	struct cmp_context uncompressed_ctx = create_uncompressed_context();
	struct cmp_context ctx;
	struct cmp_hdr expected_hdr = { 0 };
	uint32_t uncompressed_size;
	uint32_t dst_size, fallback_size;
	struct cmp_params params = { 0 };

	params.uncompressed_fallback_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_MULTI;
	params.primary_encoder_param = 1;
	params.primary_encoder_outlier = 16;
	params.secondary_iterations = 3;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.secondary_encoder_param = 1;
	params.model_rate = 13;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));
	dst_size = fix->compress(&ctx, dst, dst_cap, primary_src.data, primary_src.size);
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT(dst_size < CMP_UNCOMPRESSED_BOUND(primary_src.packed_size));

	fallback_size = fix->compress(&ctx, dst, dst_cap, fallback_src.data, fallback_src.size);

	TEST_ASSERT_CMP_SUCCESS(fallback_size);
	uncompressed_size = fix->compress(&uncompressed_ctx, expected_dst, dst_cap,
					  fallback_src.data, fallback_src.size);
	TEST_ASSERT_EQUAL(uncompressed_size, fallback_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(cmp_hdr_get_cmp_data(expected_dst), cmp_hdr_get_cmp_data(dst),
				     uncompressed_size - CMP_HDR_SIZE);
	expected_hdr.compressed_size = uncompressed_size;
	expected_hdr.original_size = fallback_src.packed_size;
	expected_hdr.original_dtype = fix->dtype;
	expected_hdr.preprocess_param = 3;
	TEST_ASSERT_CMP_HDR(dst, fallback_size, expected_hdr);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_primary_compression_is_used_for_compressible_data(const struct t_fixture *fix)
{
	static const int32_t samples[] = { 0, 0, 0, 0 };
	static const uint8_t expected_compressed[] = { 0xAA };
	struct arena *a = clear_test_arena();
	struct test_src src = make_test_src(a, fix->dtype, samples, ARRAY_SIZE(samples));
	uint32_t const dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src.packed_size);
	uint8_t *dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };
	uint32_t dst_size;

	params.uncompressed_fallback_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	dst_size = fix->compress(&ctx, dst, dst_cap, src.data, src.size);
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(sizeof(expected_compressed)), dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_compressed, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_compressed));
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = src.packed_size;
	expected_hdr.original_dtype = fix->dtype;
	expected_hdr.preprocessing = CMP_PREPROCESS_DIFF;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 1;
	expected_hdr.encoder_outlier = 16;
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_secondary_compression_is_used_for_compressible_data(const struct t_fixture *fix)
{
	static const int32_t primary_samples[] = { 0, 0, 0, 0 };
	static const int32_t samples[] = { 0xAAA, 0xBBB, 0xCCC, 0xDDD };
	static const uint8_t expected_compressed[] = { 0xAA };
	struct arena *a = clear_test_arena();
	struct test_src primary_src =
		make_test_src(a, fix->dtype, primary_samples, ARRAY_SIZE(primary_samples));
	struct test_src secondary_src = make_test_src(a, fix->dtype, samples, ARRAY_SIZE(samples));
	uint16_t work_buf[ARRAY_SIZE(primary_samples)];
	uint32_t const dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(secondary_src.packed_size);
	uint8_t *dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	struct cmp_context ctx;
	struct cmp_hdr expected_hdr = { 0 };
	uint32_t dst_size;
	struct cmp_params params = { 0 };

	params.uncompressed_fallback_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_MULTI;
	params.primary_encoder_param = 1;
	params.primary_encoder_outlier = 16;
	params.secondary_iterations = 3;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.secondary_encoder_param = 1;
	params.model_rate = 13;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	dst_size = fix->compress(&ctx, dst, dst_cap, primary_src.data, primary_src.size);
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	dst_size = fix->compress(&ctx, dst, dst_cap, secondary_src.data, secondary_src.size);
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	dst_size = fix->compress(&ctx, dst, dst_cap, secondary_src.data, secondary_src.size);
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_compressed), dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_compressed, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_compressed));
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = secondary_src.packed_size;
	expected_hdr.original_dtype = fix->dtype;
	expected_hdr.preprocessing = CMP_PREPROCESS_MODEL;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 1;
	expected_hdr.encoder_outlier = 16;
	expected_hdr.sequence_number = 1;
	expected_hdr.preprocess_param = 13;
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32], [0xBE3FC74C])
void test_write_checksum_into_header_when_enabled(const struct t_fixture *fix,
						  uint32_t exp_checksum)
{
	const int32_t samples[] = { 0xCA, 0xFF, 0xEE, 0x123 };
	struct arena *a = clear_test_arena();
	struct test_src src = make_test_src(a, fix->dtype, samples, ARRAY_SIZE(samples));
	uint32_t const dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src.packed_size);
	uint8_t *dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	uint32_t checksum;
	uint32_t dst_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.checksum_enabled = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_checksum(&checksum, src.data, src.size, fix->dtype));
	dst_size = fix->compress(&ctx, dst, dst_cap, src.data, src.size);

	TEST_ASSERT_EQUAL_HEX(exp_checksum, checksum);
	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL((uint32_t)CMP_UNCOMPRESSED_BOUND(src.packed_size), dst_size);
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = src.packed_size;
	expected_hdr.checksum = exp_checksum;
	expected_hdr.original_dtype = fix->dtype;
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
	TEST_ASSERT_NOT_EQUAL(0, checksum);
}


void test_checksum_is_same_for_same_inputs_of_16_bit_compression_function(void)
{
	struct arena *a = clear_test_arena();
	struct {
		const struct t_fixture *fix;
	} test_cases[] = { { &t_fix_i16 }, { &t_fix_u16 }, { &t_fix_i16_in_i32 } };
	uint32_t checksum_i16;
	uint32_t dst_size;
	struct test_src src;
	size_t i;
	struct cmp_hdr hdr;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	src = make_test_src(a, CMP_I16, dummy_samples, ARRAY_SIZE(dummy_samples));
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_checksum(&checksum_i16, src.data, src.size, CMP_I16));
	params.checksum_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 42;

	for (i = 0; i < ARRAY_SIZE(test_cases); i++) {
		uint32_t dst_cap;
		uint8_t *dst;

		TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));
		src = make_test_src(a, test_cases[i].fix->dtype, dummy_samples,
				    ARRAY_SIZE(dummy_samples));
		dst_cap = cmp_compress_bound(src.size, test_cases[i].fix->dtype);
		dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);

		dst_size = test_cases[i].fix->compress(&ctx, dst, dst_cap, src.data, src.size);

		TEST_ASSERT_CMP_SUCCESS(dst_size);
		TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(dst, dst_size, &hdr));
		TEST_ASSERT_EQUAL_HEX32(checksum_i16, hdr.checksum);
	}
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_checksum_is_different_for_different_inputs(const struct t_fixture *fix)
{
	const int32_t samples1[] = { 0xC0, 0xFF, 0xEE };
	const int32_t samples2[] = { 0xC0, 0xFF, 0xEF };
	struct arena *a = clear_test_arena();
	struct test_src src1 = make_test_src(a, fix->dtype, samples1, ARRAY_SIZE(samples1));
	struct test_src src2 = make_test_src(a, fix->dtype, samples2, ARRAY_SIZE(samples2));
	uint32_t const dst_cap1 = (uint32_t)CMP_UNCOMPRESSED_BOUND(src1.packed_size);
	uint32_t const dst_cap2 = (uint32_t)CMP_UNCOMPRESSED_BOUND(src2.packed_size);
	uint8_t *dst1 = arena_alloc(a, (ptrdiff_t)dst_cap1, 1, CMP_DST_ALIGNMENT);
	uint8_t *dst2 = arena_alloc(a, (ptrdiff_t)dst_cap2, 1, CMP_DST_ALIGNMENT);
	uint32_t dst_size1, dst_size2;
	struct cmp_hdr hdr1, hdr2;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.checksum_enabled = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	dst_size1 = fix->compress(&ctx, dst1, dst_cap1, src1.data, src1.size);
	dst_size2 = fix->compress(&ctx, dst2, dst_cap2, src2.data, src2.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size1);
	TEST_ASSERT_CMP_SUCCESS(dst_size2);
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(dst1, dst_size1, &hdr1));
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(dst2, dst_size2, &hdr2));
	TEST_ASSERT_NOT_EQUAL_HEX32(hdr1.checksum, hdr2.checksum);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_compression_works_with_checksum_enabled(const struct t_fixture *fix)
{
	const int32_t samples[] = { 0, 0, 0, 0 };
	const uint8_t expected_compressible[] = { 0xAA };
	struct arena *a = clear_test_arena();
	struct test_src src = make_test_src(a, fix->dtype, samples, ARRAY_SIZE(samples));
	uint32_t const dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src.packed_size);
	uint8_t *dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	uint32_t dst_size;
	uint32_t checksum;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.uncompressed_fallback_enabled = 1;
	params.checksum_enabled = 1;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	params.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.primary_encoder_param = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	dst_size = fix->compress(&ctx, dst, dst_cap, src.data, src.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_compressible), dst_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_compressible, cmp_hdr_get_cmp_data(dst),
				     sizeof(expected_compressible));
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = src.packed_size;
	expected_hdr.preprocessing = CMP_PREPROCESS_DIFF;
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_checksum(&checksum, src.data, src.size, fix->dtype));
	expected_hdr.checksum = checksum;
	expected_hdr.original_dtype = fix->dtype;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 1;
	expected_hdr.encoder_outlier = 16;
	TEST_ASSERT_CMP_HDR(dst, dst_size, expected_hdr);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_compression_fails_when_capacity_is_an_error(const struct t_fixture *fix)
{
	struct cmp_context ctx_uncompressed = create_uncompressed_context();
	const int32_t samples[2] = { 0, 0 };
	struct test_src src =
		make_test_src(clear_test_arena(), fix->dtype, samples, ARRAY_SIZE(samples));
	DST_ALIGNED_U8 dst_dummy[CMP_UNCOMPRESSED_BOUND(42)] = { 0 };

	uint32_t const bound_error = cmp_compress_bound(CMP_HDR_MAX_ORIGINAL_SIZE + 1, fix->dtype);
	uint32_t const return_val =
		fix->compress(&ctx_uncompressed, dst_dummy, bound_error, src.data, src.size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_GENERIC, return_val);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_detect_uninitialise_context_in_compression(const struct t_fixture *fix)
{
	const int32_t samples[2] = { 0, 0 };
	struct arena *a = clear_test_arena();
	struct test_src src = make_test_src(a, fix->dtype, samples, ARRAY_SIZE(samples));
	uint32_t const dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src.packed_size);
	uint8_t *dst = arena_alloc(a, (ptrdiff_t)dst_cap, 1, CMP_DST_ALIGNMENT);
	struct cmp_context ctx = create_uncompressed_context();
	uint32_t return_val;

	cmp_deinitialise(&ctx);
	return_val = fix->compress(&ctx, dst, dst_cap, src.data, src.size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_CONTEXT_INVALID, return_val);
}


void test_detect_uninitialise_context_in_reset(void)
{
	uint32_t return_val;
	struct cmp_context ctx;

	cmp_deinitialise(&ctx);
	return_val = cmp_reset(&ctx);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_CONTEXT_INVALID, return_val);
}

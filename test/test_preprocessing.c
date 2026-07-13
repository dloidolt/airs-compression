/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Pre-Processing Tests
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <unity.h>
#include "test_common.h"

#include "../lib/cmp.h"
#include "../lib/cmp_header.h"
#include "../lib/common/compiler.h"


/* Global arena pointer to an arena which is cleared before every test */
static struct arena *g_a;

void setUp(void)
{
	g_a = clear_test_arena();
}


struct test_env {
	void *dst;
	struct cmp_context ctx;
	uint32_t dst_cap;
};

/* Create and initialize arena-backed test environment with compression context and buffers */
static struct test_env *make_env(struct arena *a, struct cmp_params *params, enum cmp_type dtype,
				 uint32_t src_size, uint32_t src_samples)
{
	struct test_env *e;
	uint32_t work_len;
	void *work = NULL;

	TEST_ASSERT_NOT_NULL(a);
	TEST_ASSERT_NOT_NULL(params);

	e = ARENA_NEW(a, struct test_env);

	work_len = cmp_cal_work_buf_size(params, src_size, dtype);
	TEST_ASSERT_CMP_SUCCESS(work_len);
	if (work_len)
		work = arena_alloc(a, 1, work_len, sizeof(uint16_t));

	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&e->ctx, params, work, work_len));

	if (params->primary_encoder_type != CMP_ENCODER_UNCOMPRESSED ||
	    (params->secondary_iterations > 0 &&
	     params->secondary_encoder_type != CMP_ENCODER_UNCOMPRESSED)) {
		e->dst_cap = cmp_compress_bound(src_size, dtype);
	} else {
		/* Preprocessing represents each input sample as one 16-bit value. */
		e->dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src_samples * sizeof(int16_t));
	}
	TEST_ASSERT_CMP_SUCCESS(e->dst_cap);
	e->dst = arena_alloc(a, e->dst_cap, 1, CMP_DST_ALIGNMENT);

	return e;
}


static void assert_preprocessing_data(const int16_t *expected_output, uint32_t num_elements,
				      const uint8_t *compressed_data)
{
	int16_t output[12];
	const uint8_t *p = cmp_hdr_get_cmp_data(compressed_data);
	uint32_t i;

	TEST_ASSERT_LESS_OR_EQUAL(ARRAY_SIZE(output), num_elements);
	for (i = 0; i < num_elements; i++) /* convert to system endianness */
		output[i] = (int16_t)(p[i * 2] << 8) | (int16_t)(p[i * 2 + 1]);
	TEST_ASSERT_EQUAL_INT16_ARRAY(expected_output, output, num_elements);
}


const int32_t t_diff[12] = {
	1, 3, 0, 0xFFF, 0, 0x7FF, 0x800, 0xFFB, 0, INT16_MAX, INT16_MIN, -5,
};
const int16_t t_exp_diff[12] = {
	1, 2, -3, 4095, -4095, 2047, 1, 2043, -4091, INT16_MAX, 1, 32763,
};
const uint32_t t_diff_raw12_count = 9; /* number of common raw12 values */

TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32], [t_diff], [t_exp_diff],
	    [ARRAY_SIZE(t_exp_diff)])
TEST_CASE(&t_fix_raw12, t_diff, t_exp_diff, t_diff_raw12_count)
void test_diff_preprocessing_for_multiple_values(const struct t_fixture *fix,
						 const int32_t *samples,
						 const int16_t *expected_diff, uint32_t count)
{
	struct test_src src = make_test_src(g_a, fix->dtype, samples, count);
	struct test_env *e;
	uint32_t dst_size;
	struct cmp_hdr expected_hdr = { 0 };
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	e = make_env(g_a, &params, fix->dtype, src.size, count);

	dst_size = fix->compress(&e->ctx, e->dst, e->dst_cap, src.data, src.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(count * sizeof(int16_t)), dst_size);
	assert_preprocessing_data(expected_diff, count, e->dst);
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = src.packed_size;
	expected_hdr.original_dtype = fix->dtype;
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.primary_preprocessing;
	TEST_ASSERT_CMP_HDR(e->dst, dst_size, expected_hdr);
}


const int32_t t_iwt1[1] = { 42 };
const int16_t t_exp_iwt1[1] = { 42 };

const int32_t t_iwt2[2] = { -23809, 23901 };
const int16_t t_exp_iwt2[2] = { -32722, -17826 };

const int32_t t_iwt5[5] = { -1, 2, -3, 4, -5 };
const int16_t t_exp_iwt5[5] = { 0, 4, 0, 8, -2 };

const int32_t t_iwt7[7] = { 0, 0, 2, 0, 0, 0, 0 };
const int16_t t_exp_iwt7[7] = { -1, -1, 2, -1, -1, 0, 1 };

const int32_t t_iwt8[8] = { -3, 2, -1, 3, -2, 5, 0, 7 };
const int16_t t_exp_iwt8[8] = { 0, 4, 2, 5, 1, 6, 3, 7 };

/* clang-format off */
/* negative input values are out of range for RAW12 */
TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32, &t_fix_raw12], [t_iwt1], [t_exp_iwt1], [ARRAY_SIZE(t_exp_iwt1)])
TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32],               [t_iwt2], [t_exp_iwt2], [ARRAY_SIZE(t_exp_iwt2)])
TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32],               [t_iwt5], [t_exp_iwt5], [ARRAY_SIZE(t_exp_iwt5)])
TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32, &t_fix_raw12], [t_iwt7], [t_exp_iwt7], [ARRAY_SIZE(t_exp_iwt7)])
TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32],               [t_iwt8], [t_exp_iwt8], [ARRAY_SIZE(t_exp_iwt8)])
/* clang-format on */
void test_iwt_transform(const struct t_fixture *fix, const int32_t *samples, const int16_t *exp_iwt,
			uint32_t count)
{
	struct test_src src = make_test_src(g_a, fix->dtype, samples, count);
	uint32_t dst_size;
	struct test_env *e;
	struct cmp_hdr expected_hdr = { 0 };
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_IWT;
	e = make_env(g_a, &params, fix->dtype, src.size, count);

	dst_size = fix->compress(&e->ctx, e->dst, e->dst_cap, src.data, src.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(count * sizeof(int16_t)), dst_size);
	assert_preprocessing_data(exp_iwt, count, e->dst);
	expected_hdr.compressed_size = CMP_HDR_SIZE + (count * (uint32_t)sizeof(int16_t));
	expected_hdr.original_size = src.packed_size;
	expected_hdr.original_dtype = fix->dtype;
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.primary_preprocessing;
	TEST_ASSERT_CMP_HDR(e->dst, dst_size, expected_hdr);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32, &t_fix_raw12])
void test_model_preprocessing_for_multiple_values(const struct t_fixture *fix)
{
	const int32_t start_model_samples[] = { 0, 1, 10 };
	const int32_t data_samples[ARRAY_SIZE(start_model_samples)] = { 1, 3, 5 };
	const int16_t expected_output[ARRAY_SIZE(start_model_samples)] = { 1, 2, -5 };
	struct test_src const start_model = make_test_src(g_a, fix->dtype, start_model_samples,
							  ARRAY_SIZE(start_model_samples));
	struct test_src const data =
		make_test_src(g_a, fix->dtype, data_samples, ARRAY_SIZE(data_samples));
	uint32_t dst_size;
	struct test_env *e;
	struct cmp_hdr expected_hdr = { 0 };
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 1;
	e = make_env(g_a, &params, fix->dtype, start_model.size, ARRAY_SIZE(start_model_samples));

	TEST_ASSERT_CMP_SUCCESS(
		fix->compress(&e->ctx, e->dst, e->dst_cap, start_model.data, start_model.size));
	dst_size = fix->compress(&e->ctx, e->dst, e->dst_cap, data.data, data.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(sizeof(expected_output)), dst_size);
	assert_preprocessing_data(expected_output, ARRAY_SIZE(expected_output), e->dst);
	expected_hdr.compressed_size =
		CMP_HDR_SIZE + (ARRAY_SIZE(expected_output) * sizeof(int16_t));
	expected_hdr.original_size = data.packed_size;
	expected_hdr.original_dtype = fix->dtype;
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.secondary_preprocessing;
	expected_hdr.sequence_number = 1;
	TEST_ASSERT_CMP_HDR(e->dst, dst_size, expected_hdr);
}


TEST_MATRIX([&t_fix_i16, &t_fix_i16_in_i32])
void test_model_preprocessing_for_multiple_signed_values(const struct t_fixture *fix)
{
	const int32_t start_model_samples[] = { 0, 1, 10, -4 };
	const int32_t data_samples[ARRAY_SIZE(start_model_samples)] = { 1, 3, 5, -1 };
	const int16_t expected_output[ARRAY_SIZE(start_model_samples)] = { 1, 2, -5, 3 };
	struct test_src const start_model = make_test_src(g_a, fix->dtype, start_model_samples,
							  ARRAY_SIZE(start_model_samples));
	struct test_src const data =
		make_test_src(g_a, fix->dtype, data_samples, ARRAY_SIZE(data_samples));
	uint32_t dst_size;
	struct test_env *e;
	struct cmp_hdr expected_hdr = { 0 };
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 1;
	e = make_env(g_a, &params, fix->dtype, start_model.size, ARRAY_SIZE(start_model_samples));

	TEST_ASSERT_CMP_SUCCESS(
		fix->compress(&e->ctx, e->dst, e->dst_cap, start_model.data, start_model.size));
	dst_size = fix->compress(&e->ctx, e->dst, e->dst_cap, data.data, data.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(sizeof(expected_output)), dst_size);
	assert_preprocessing_data(expected_output, ARRAY_SIZE(expected_output), e->dst);
	expected_hdr.compressed_size =
		CMP_HDR_SIZE + (ARRAY_SIZE(expected_output) * sizeof(int16_t));
	expected_hdr.original_size = data.packed_size;
	expected_hdr.original_dtype = fix->dtype;
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.secondary_preprocessing;
	expected_hdr.sequence_number = 1;
	TEST_ASSERT_CMP_HDR(e->dst, dst_size, expected_hdr);
}

/* this is the model */
const int32_t t_model1_u16[7] = { 0, 2, 21, 0x800, 0xFFF, 1, UINT16_MAX };
/* this is the value */
const int32_t t_model2_u16[7] = { 1, 3, 5, 0x7FF, 0x000, UINT16_MAX, UINT16_MAX };
const int32_t t_model3_u16[7] = { 0 }; /* when m=0 -> o=-m (o=v-m)  */
const int16_t t_exp_model_u16[7] = {
	0, -2, -6, -2047, -255, (int16_t)-61439, (uint16_t)-UINT16_MAX
};
const uint32_t t_model_raw12_count = 5; /* for raw we only use the first values in raw12 range */

const int32_t t_model1_i16[7] = { 15, 2, 21, 0, 0, INT16_MIN, INT16_MAX };
const int32_t t_model2_i16[7] = { -2, 3, 5, -1, 0, INT16_MIN, INT16_MAX };
const int32_t t_model3_i16[7] = { 0 };
const int16_t t_exp_model_i16[7] = { 1, -2, -6, 1, 0, (int16_t)-INT16_MIN, -INT16_MAX };

TEST_CASE(&t_fix_u16, t_model1_u16, t_model2_u16, t_model3_u16, t_exp_model_u16,
	  ARRAY_SIZE(t_exp_model_u16))
TEST_CASE(&t_fix_raw12, t_model1_u16, t_model2_u16, t_model3_u16, t_exp_model_u16,
	  t_model_raw12_count)
TEST_MATRIX([&t_fix_i16, &t_fix_i16_in_i32], [t_model1_i16], [t_model2_i16], [t_model3_i16],
	    [t_exp_model_i16], [ARRAY_SIZE(t_exp_model_i16)])
void test_model_updates_correctly(const struct t_fixture *fix, const void *src1_samples,
				  const void *src2_samples, const void *src3_samples,
				  const void *pre_data_exp, uint32_t count)
{
	struct test_src src1 = make_test_src(g_a, fix->dtype, src1_samples, count);
	struct test_src src2 = make_test_src(g_a, fix->dtype, src2_samples, count);
	struct test_src src3 = make_test_src(g_a, fix->dtype, src3_samples, count);
	uint32_t dst_size;
	struct test_env *e;
	struct cmp_hdr expected_hdr = { 0 };
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.model_rate = 1;
	params.secondary_iterations = 2;
	e = make_env(g_a, &params, fix->dtype, src1.size, count);

	TEST_ASSERT_CMP_SUCCESS(fix->compress(&e->ctx, e->dst, e->dst_cap, src1.data, src1.size));
	TEST_ASSERT_CMP_SUCCESS(fix->compress(&e->ctx, e->dst, e->dst_cap, src2.data, src2.size));
	dst_size = fix->compress(&e->ctx, e->dst, e->dst_cap, src3.data, src3.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(count * sizeof(int16_t)), dst_size);
	assert_preprocessing_data(pre_data_exp, count, e->dst);
	expected_hdr.compressed_size = CMP_HDR_SIZE + (count * (uint32_t)sizeof(int16_t));
	expected_hdr.original_size = src3.packed_size;
	expected_hdr.original_dtype = fix->dtype;
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.secondary_preprocessing;
	expected_hdr.preprocess_param = 1;
	expected_hdr.sequence_number = 2;
	TEST_ASSERT_CMP_HDR(e->dst, dst_size, expected_hdr);
}


void test_can_mix_i16_in_i32_and_i16_in_model_preprocessing(void)
{
	struct test_src const src1_i16_in_i32 =
		make_test_src(g_a, CMP_I16_IN_I32, t_model1_i16, ARRAY_SIZE(t_model1_i16));
	struct test_src const src2_i16 =
		make_test_src(g_a, CMP_I16, t_model2_i16, ARRAY_SIZE(t_model2_i16));
	struct test_src const src3_i16_in_i32 =
		make_test_src(g_a, CMP_I16_IN_I32, t_model3_i16, ARRAY_SIZE(t_model3_i16));
	uint32_t dst_size;
	struct test_env *e;
	struct cmp_hdr expected_hdr = { 0 };
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.model_rate = 1;
	params.secondary_iterations = 2;
	e = make_env(g_a, &params, CMP_I16_IN_I32, src1_i16_in_i32.size, ARRAY_SIZE(t_model1_i16));

	TEST_ASSERT_CMP_SUCCESS(cmp_compress_i16_in_i32(
		&e->ctx, e->dst, e->dst_cap, src1_i16_in_i32.data, src1_i16_in_i32.size));
	TEST_ASSERT_CMP_SUCCESS(
		cmp_compress_i16(&e->ctx, e->dst, e->dst_cap, src2_i16.data, src2_i16.size));
	dst_size = cmp_compress_i16_in_i32(&e->ctx, e->dst, e->dst_cap, src3_i16_in_i32.data,
					   src3_i16_in_i32.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(sizeof(t_exp_model_i16)), dst_size);
	assert_preprocessing_data(t_exp_model_i16, ARRAY_SIZE(t_exp_model_i16), e->dst);
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = src3_i16_in_i32.packed_size;
	expected_hdr.original_dtype = CMP_I16_IN_I32;
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.secondary_preprocessing;
	expected_hdr.preprocess_param = 1;
	expected_hdr.sequence_number = 2;
	TEST_ASSERT_CMP_HDR(e->dst, dst_size, expected_hdr);
}


void test_can_mix_raw12_and_u16_in_model_preprocessing(void)
{
	struct test_src const src1_raw12 =
		make_test_src(g_a, CMP_RAW12, t_model1_u16, t_model_raw12_count);
	struct test_src const src2_u16 =
		make_test_src(g_a, CMP_U16, t_model2_u16, t_model_raw12_count);
	struct test_src const src3_raw12 =
		make_test_src(g_a, CMP_RAW12, t_model3_u16, t_model_raw12_count);
	uint32_t dst_size;
	struct test_env *e;
	struct cmp_hdr expected_hdr = { 0 };
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.model_rate = 1;
	params.secondary_iterations = 2;
	e = make_env(g_a, &params, CMP_RAW12, src1_raw12.size, t_model_raw12_count);

	TEST_ASSERT_CMP_SUCCESS(
		cmp_compress_raw12(&e->ctx, e->dst, e->dst_cap, src1_raw12.data, src1_raw12.size));
	TEST_ASSERT_CMP_SUCCESS(
		cmp_compress_u16(&e->ctx, e->dst, e->dst_cap, src2_u16.data, src2_u16.size));
	dst_size =
		cmp_compress_raw12(&e->ctx, e->dst, e->dst_cap, src3_raw12.data, src3_raw12.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(t_model_raw12_count * sizeof(uint16_t)), dst_size);
	assert_preprocessing_data(t_exp_model_u16, t_model_raw12_count, e->dst);
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = src3_raw12.packed_size;
	expected_hdr.original_dtype = CMP_RAW12;
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.secondary_preprocessing;
	expected_hdr.preprocess_param = 1;
	expected_hdr.sequence_number = 2;
	TEST_ASSERT_CMP_HDR(e->dst, dst_size, expected_hdr);
}


TEST_MATRIX([&t_fix_i16, &t_fix_i16_in_i32], [&t_fix_u16, &t_fix_raw12])
void test_detect_model_signed_change_using_model_preprocessing(const struct t_fixture *fix_signed,
							       const struct t_fixture *fix_unsigned)
{
	const int32_t samples[3] = { 0 };
	struct test_src const src_i =
		make_test_src(g_a, fix_signed->dtype, samples, ARRAY_SIZE(samples));
	struct test_src const src_u =
		make_test_src(g_a, fix_unsigned->dtype, samples, ARRAY_SIZE(samples));
	uint32_t return_code;
	struct test_env *e;
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 1;
	e = make_env(g_a, &params, fix_signed->dtype, src_i.size, ARRAY_SIZE(samples));

	TEST_ASSERT_CMP_SUCCESS(
		fix_signed->compress(&e->ctx, e->dst, e->dst_cap, src_i.data, src_i.size));
	return_code = fix_unsigned->compress(&e->ctx, e->dst, e->dst_cap, src_u.data, src_u.size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_MISMATCH, return_code);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32, &t_fix_raw12])
void test_primary_preprocessing_after_max_secondary_iterations(const struct t_fixture *fix)
{
	const int32_t samples[4] = { 0, 0, 0, 0 };
	struct test_src const src = make_test_src(g_a, fix->dtype, samples, ARRAY_SIZE(samples));
	uint32_t dst_size;
	struct test_env *e;
	struct cmp_hdr expected_hdr = { 0 };
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 2;
	e = make_env(g_a, &params, fix->dtype, src.size, ARRAY_SIZE(samples));

	TEST_ASSERT_CMP_SUCCESS(fix->compress(&e->ctx, e->dst, e->dst_cap, src.data, src.size));
	TEST_ASSERT_CMP_SUCCESS(fix->compress(&e->ctx, e->dst, e->dst_cap, src.data, src.size));
	TEST_ASSERT_CMP_SUCCESS(fix->compress(&e->ctx, e->dst, e->dst_cap, src.data, src.size));
	dst_size = fix->compress(&e->ctx, e->dst, e->dst_cap, src.data, src.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_UNCOMPRESSED_BOUND(src.packed_size), dst_size);
	TEST_ASSERT_EACH_EQUAL_HEX8(0, cmp_hdr_get_cmp_data(e->dst), src.packed_size);
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = src.packed_size;
	expected_hdr.original_dtype = fix->dtype;
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = CMP_PREPROCESS_NONE;
	expected_hdr.preprocess_param = params.secondary_iterations;
	TEST_ASSERT_CMP_HDR(e->dst, dst_size, expected_hdr);
}


void test_detect_invalid_primary_preprocessing_model_usage(void)
{
	struct cmp_context ctx;
	uint16_t work_buf[4];
	uint32_t return_val;
	struct cmp_params par = { 0 };

	par.primary_preprocessing = CMP_PREPROCESS_MODEL;

	return_val = cmp_initialise(&ctx, &par, work_buf, sizeof(work_buf));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32, &t_fix_raw12])
void test_unrelated_compressions_get_unique_identifiers_in_model_preprocessing(
	const struct t_fixture *fix)
{
	const int32_t samples[4] = { 0 };
	struct test_src const src1 = make_test_src(g_a, fix->dtype, samples, ARRAY_SIZE(samples));
	struct test_src const src2 = make_test_src(g_a, fix->dtype, samples, ARRAY_SIZE(samples));
	uint32_t dst_size1, dst_size2;
	struct test_env *e1, *e2;
	struct cmp_hdr hdr1 = { 0 };
	struct cmp_hdr hdr2 = { 0 };
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 10;
	e1 = make_env(g_a, &params, fix->dtype, src1.size, ARRAY_SIZE(samples));
	e2 = make_env(g_a, &params, fix->dtype, src2.size, ARRAY_SIZE(samples));

	dst_size1 = fix->compress(&e1->ctx, e1->dst, e1->dst_cap, src1.data, src1.size);
	dst_size2 = fix->compress(&e2->ctx, e2->dst, e2->dst_cap, src2.data, src2.size);

	TEST_ASSERT_CMP_SUCCESS(dst_size1);
	TEST_ASSERT_CMP_SUCCESS(dst_size2);
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(e1->dst, dst_size1, &hdr1));
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(e2->dst, dst_size2, &hdr2));
	TEST_ASSERT_NOT_EQUAL(hdr1.identifier, hdr2.identifier);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32, &t_fix_raw12])
void test_detect_too_small_work_buffer_in_model_preprocessing(const struct t_fixture *fix)
{
	const int32_t samples[4] = { 0 };
	struct test_src const src = make_test_src(g_a, fix->dtype, samples, ARRAY_SIZE(samples));
	uint32_t dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src.packed_size);
	void *dst = arena_alloc(g_a, dst_cap, 1, CMP_DST_ALIGNMENT);
	uint16_t work_buf[ARRAY_SIZE(samples) - 1];
	uint32_t work_buf_size, return_code;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 1;
	work_buf_size = cmp_cal_work_buf_size(&params, src.size, fix->dtype);
	TEST_ASSERT_LESS_THAN(work_buf_size, sizeof(work_buf));
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	return_code = fix->compress(&ctx, dst, dst_cap, src.data, src.size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_TOO_SMALL, return_code);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32, &t_fix_raw12])
void test_detect_src_size_change_using_model_preprocessing(const struct t_fixture *fix)
{
	const int32_t samples1[4] = { 0 };
	const int32_t samples2[2] = { 0 };
	struct test_src const src1 = make_test_src(g_a, fix->dtype, samples1, ARRAY_SIZE(samples1));
	struct test_src const src2 = make_test_src(g_a, fix->dtype, samples2, ARRAY_SIZE(samples2));
	uint32_t return_code;
	struct test_env *e;
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 10;
	e = make_env(g_a, &params, fix->dtype, src1.size, ARRAY_SIZE(samples1));
	TEST_ASSERT_CMP_SUCCESS(fix->compress(&e->ctx, e->dst, e->dst_cap, src1.data, src1.size));

	return_code = fix->compress(&e->ctx, e->dst, e->dst_cap, src2.data, src2.size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_MISMATCH, return_code);
}

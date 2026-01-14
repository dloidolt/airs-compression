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
#include "../lib/common/header_private.h"
#include "../lib/common/compiler.h"


struct test_env {
	void *dst;
	void *work;
	struct cmp_context ctx;
	uint32_t dst_cap;
};


/* Create and initialize a test environment with compression context and buffers */
static struct test_env *make_env(struct cmp_params *params, uint32_t src_len)
{
	struct test_env *e = t_malloc(sizeof(*e));
	uint32_t work_len;

	memset(e, 0, sizeof(*e));

	work_len = cmp_cal_work_buf_size(params, src_len);
	TEST_ASSERT_CMP_SUCCESS(work_len);
	if (work_len)
		e->work = t_malloc(work_len);

	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&e->ctx, params, e->work, work_len));

	if (params->primary_preprocessing == CMP_PREPROCESS_NONE &&
	    params->secondary_preprocessing == CMP_PREPROCESS_NONE) {
		if (params->primary_encoder_type == CMP_ENCODER_UNCOMPRESSED &&
		    params->secondary_encoder_type == CMP_ENCODER_UNCOMPRESSED) {
			e->dst_cap = (uint32_t)CMP_UNCOMPRESSED_BOUND(src_len);
		} else {
			e->dst_cap = CMP_HDR_MAX_SIZE + src_len;
		}
	} else {
		e->dst_cap = cmp_compress_bound(src_len);
	}
	TEST_ASSERT_CMP_SUCCESS(e->dst_cap);
	e->dst = t_malloc(e->dst_cap);

	return e;
}


static void free_env(struct test_env *e)
{
	free(e->dst);
	free(e->work);
	free(e);
}


static void assert_preprocessing_data(const int16_t *expected_output, uint32_t num_elements,
				      const uint8_t *compressed_data)
{
	int16_t output[8];
	const uint8_t *p = cmp_hdr_get_cmp_data(compressed_data);
	uint32_t i;

	TEST_ASSERT_LESS_OR_EQUAL(ARRAY_SIZE(output), num_elements);
	for (i = 0; i < num_elements; i++) /* convert to system endianness */
		output[i] = (int16_t)(p[i * 2] << 8) | (int16_t)(p[i * 2 + 1]);
	TEST_ASSERT_EQUAL_INT16_ARRAY(expected_output, output, num_elements);
}


TEST_CASE(compress_u16_wrapper)
TEST_CASE(compress_i16_wrapper)
void test_1d_difference_preprocessing_for_multiple_values(compress_func_t compress_func)
{
	/* clang-format off */
	const uint16_t input_data[] = {
		1, 3,  0, UINT16_MAX, 0, INT16_MAX, (uint16_t)INT16_MIN, (uint16_t)-5
	};
	const int16_t expected_1d_diff[ARRAY_SIZE(input_data)] = {
		1, 2, -3,         -1, 1, INT16_MAX,                   1, 0x7FFB
	};
	/* clang-format on */
	uint32_t dst_size;
	struct test_env *setup;
	struct cmp_hdr expected_hdr = { 0 };
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	setup = make_env(&params, sizeof(input_data));

	dst_size = compress_func(&setup->ctx, setup->dst, setup->dst_cap, input_data,
				 sizeof(input_data));

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_MAX_SIZE + sizeof(expected_1d_diff), dst_size);
	assert_preprocessing_data(expected_1d_diff, ARRAY_SIZE(expected_1d_diff), setup->dst);
	expected_hdr.version_id = CMP_VERSION_NUMBER;
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = sizeof(input_data);
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.primary_preprocessing;
	TEST_ASSERT_CMP_HDR(setup->dst, dst_size, expected_hdr);

	free_env(setup);
}


const int16_t g_iwt_input_1[] = { 42 };
const int16_t g_iwt_exp_out_1[] = { 42 };
TEST_CASE(compress_u16_wrapper, g_iwt_input_1, g_iwt_exp_out_1, sizeof(g_iwt_input_1))
TEST_CASE(compress_i16_wrapper, g_iwt_input_1, g_iwt_exp_out_1, sizeof(g_iwt_input_1))

const int16_t g_iwt_input_2[2] = { -23809, 23901 };
const int16_t g_iwt_exp_out_2[2] = { -32722, -17826 };
TEST_CASE(compress_u16_wrapper, g_iwt_input_2, g_iwt_exp_out_2, sizeof(g_iwt_input_2))
TEST_CASE(compress_i16_wrapper, g_iwt_input_2, g_iwt_exp_out_2, sizeof(g_iwt_input_2))

const int16_t g_iwt_input_5[5] = { -1, 2, -3, 4, -5 };
const int16_t g_iwt_exp_out_5[5] = { 0, 4, 0, 8, -2 };
TEST_CASE(compress_u16_wrapper, g_iwt_input_5, g_iwt_exp_out_5, sizeof(g_iwt_input_5))
TEST_CASE(compress_i16_wrapper, g_iwt_input_5, g_iwt_exp_out_5, sizeof(g_iwt_input_5))

const int16_t g_iwt_input_7[7] = { 0, 0, 2, 0, 0, 0, 0 };
const int16_t g_iwt_exp_out_7[7] = { -1, -1, 2, -1, -1, 0, 1 };
TEST_CASE(compress_u16_wrapper, g_iwt_input_7, g_iwt_exp_out_7, sizeof(g_iwt_input_7))
TEST_CASE(compress_i16_wrapper, g_iwt_input_7, g_iwt_exp_out_7, sizeof(g_iwt_input_7))

const int16_t g_iwt_input_8[8] = { -3, 2, -1, 3, -2, 5, 0, 7 };
const int16_t g_iwt_exp_out_8[8] = { 0, 4, 2, 5, 1, 6, 3, 7 };
TEST_CASE(compress_u16_wrapper, g_iwt_input_8, g_iwt_exp_out_8, sizeof(g_iwt_input_8))
TEST_CASE(compress_i16_wrapper, g_iwt_input_8, g_iwt_exp_out_8, sizeof(g_iwt_input_8))

void test_iwt_transform(compress_func_t compress_func, const int16_t *input_data,
			const int16_t *expected_output, uint32_t size)
{
	uint32_t dst_size;
	struct test_env *e;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_IWT;
	e = make_env(&params, size);

	dst_size = compress_func(&e->ctx, e->dst, e->dst_cap, (const uint16_t *)input_data, size);

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_MAX_SIZE + size, dst_size);
	assert_preprocessing_data(expected_output, size / sizeof(int16_t), e->dst);
	expected_hdr.version_id = CMP_VERSION_NUMBER;
	expected_hdr.compressed_size = CMP_HDR_MAX_SIZE + size;
	expected_hdr.original_size = size;
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.primary_preprocessing;
	TEST_ASSERT_CMP_HDR(e->dst, dst_size, expected_hdr);

	free_env(e);
}


TEST_CASE(compress_u16_wrapper)
TEST_CASE(compress_i16_wrapper)
void test_model_preprocessing_for_multiple_values(compress_func_t compress_func)
{
	const uint16_t start_model[] = { 0, 1, 10 };
	const uint16_t data[ARRAY_SIZE(start_model)] = { 1, 3, 5 };
	const int16_t expected_output[ARRAY_SIZE(start_model)] = { 1, 2, -5 };
	uint32_t dst_size;
	struct test_env *e;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 1;
	e = make_env(&params, sizeof(data));

	TEST_ASSERT_CMP_SUCCESS(
		compress_func(&e->ctx, e->dst, e->dst_cap, start_model, sizeof(start_model)));
	dst_size = compress_func(&e->ctx, e->dst, e->dst_cap, data, sizeof(data));

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_MAX_SIZE + sizeof(expected_output), dst_size);
	assert_preprocessing_data(expected_output, ARRAY_SIZE(expected_output), e->dst);
	expected_hdr.version_id = CMP_VERSION_NUMBER;
	expected_hdr.compressed_size = dst_size;
	expected_hdr.original_size = sizeof(data);
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.secondary_preprocessing;
	expected_hdr.sequence_number = 1;
	TEST_ASSERT_CMP_HDR(e->dst, dst_size, expected_hdr);

	free_env(e);
}


const uint16_t model_input1_u16[5] = { 0, 2, 21, 1, UINT16_MAX };         /* this is the model */
const uint16_t model_input2_u16[5] = { 1, 3, 5, UINT16_MAX, UINT16_MAX }; /* this is the value */
const uint16_t model_input3_u16[5] = { 0, 0, 0, 0, 0 }; /* when m=0 -> o=-m (o=v-m)  */
const int16_t expec_output_u16[5] = { 0, -2, -6, (int16_t)-61439,
				      (uint16_t)-UINT16_MAX }; /* this is -m */
TEST_CASE(compress_u16_wrapper, model_input1_u16, model_input2_u16, model_input3_u16,
	  expec_output_u16, sizeof(expec_output_u16))

/* this is the 1st model */
const int16_t model_input1_i16[7] = { 15, 2, 21, 0, 0, INT16_MIN, INT16_MAX };
/* this is the value */
const int16_t model_input2_i16[7] = { -2, 3, 5, -1, 0, INT16_MIN, INT16_MAX };
/* when m=0 -> o=-m (o=v-m)  */
const int16_t model_input3_i16[7] = { 0, 0, 0, 0, 0, 0, 0 };
/* this is -m_update */
const int16_t expected_out_i16[7] = { 1, -2, -6, 1, 0, (int16_t)-INT16_MIN, -INT16_MAX };
TEST_CASE(compress_i16_wrapper, model_input1_i16, model_input2_i16, model_input3_i16,
	  expected_out_i16, sizeof(expected_out_i16))
void test_model_updates_correctly(compress_func_t compress_func, const void *src1, const void *src2,
				  const void *src3, const void *pre_data_exp, uint32_t src_size)
{
	uint32_t output_size;
	struct test_env *e;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.model_rate = 1;
	params.secondary_iterations = 2;
	e = make_env(&params, src_size);

	TEST_ASSERT_CMP_SUCCESS(compress_func(&e->ctx, e->dst, e->dst_cap, src1, src_size));
	TEST_ASSERT_CMP_SUCCESS(compress_func(&e->ctx, e->dst, e->dst_cap, src2, src_size));
	output_size = compress_func(&e->ctx, e->dst, e->dst_cap, src3, src_size);

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_MAX_SIZE + src_size, output_size);
	assert_preprocessing_data(pre_data_exp, src_size / 2, e->dst);
	expected_hdr.version_id = CMP_VERSION_NUMBER;
	expected_hdr.compressed_size = output_size;
	expected_hdr.original_size = src_size;
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = params.secondary_preprocessing;
	expected_hdr.model_rate = 1;
	expected_hdr.sequence_number = 2;
	TEST_ASSERT_CMP_HDR(e->dst, output_size, expected_hdr);

	free_env(e);
}


TEST_CASE(compress_u16_wrapper)
TEST_CASE(compress_i16_wrapper)
void test_primary_preprocessing_after_max_secondary_iterations(compress_func_t compress_func)
{
	const uint16_t input[3] = { INT16_MAX, UINT16_MAX, 0 };
	const uint16_t input_after_reset[3] = { 1, 2, 3 };
	const int16_t expected_output[3] = { 1, 2, 3 };
	uint32_t output_size;
	struct test_env *e;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 2;
	e = make_env(&params, sizeof(input));

	TEST_ASSERT_CMP_SUCCESS(compress_func(&e->ctx, e->dst, e->dst_cap, input, sizeof(input)));
	TEST_ASSERT_CMP_SUCCESS(compress_func(&e->ctx, e->dst, e->dst_cap, input, sizeof(input)));
	TEST_ASSERT_CMP_SUCCESS(compress_func(&e->ctx, e->dst, e->dst_cap, input, sizeof(input)));
	output_size = compress_func(&e->ctx, e->dst, e->dst_cap, input_after_reset,
				    sizeof(input_after_reset));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_output), output_size);
	assert_preprocessing_data(expected_output, ARRAY_SIZE(expected_output), e->dst);
	expected_hdr.version_id = CMP_VERSION_NUMBER;
	expected_hdr.compressed_size = output_size;
	expected_hdr.original_size = sizeof(input);
	expected_hdr.encoder_type = params.primary_encoder_type;
	expected_hdr.preprocessing = CMP_PREPROCESS_NONE;
	TEST_ASSERT_CMP_HDR(e->dst, output_size, expected_hdr);

	free_env(e);
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


TEST_CASE(compress_u16_wrapper)
TEST_CASE(compress_i16_wrapper)
void test_assigns_unique_model_ids(compress_func_t compress_func)
{
	const uint16_t input1[3] = { 0, 1, 10 };
	const uint16_t input2[3] = { 1, 3, 5 };

	uint32_t dst_size1, dst_size2;
	struct cmp_hdr hdr1, hdr2;
	struct test_env *e1, *e2;
	struct cmp_params params = { 0 };

	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 10;
	e1 = make_env(&params, sizeof(input1));
	e2 = make_env(&params, sizeof(input2));

	dst_size1 = compress_func(&e1->ctx, e1->dst, e1->dst_cap, input1, sizeof(input1));
	dst_size2 = compress_func(&e2->ctx, e2->dst, e2->dst_cap, input2, sizeof(input2));

	TEST_ASSERT_CMP_SUCCESS(dst_size1);
	TEST_ASSERT_CMP_SUCCESS(dst_size2);
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(e1->dst, dst_size1, &hdr1));
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(e2->dst, dst_size2, &hdr2));
	TEST_ASSERT_NOT_EQUAL(hdr1.identifier, hdr2.identifier);

	free_env(e1);
	free_env(e2);
}


TEST_CASE(compress_u16_wrapper)
TEST_CASE(compress_i16_wrapper)
void test_detect_to_small_work_buffer_in_model_preprocessing(compress_func_t compress_func)
{
	const uint16_t data[] = { 0, 0, 0 };
	DST_ALIGNED_U8 dst[CMP_HDR_MAX_SIZE + sizeof(data)];
	uint16_t work_buf[ARRAY_SIZE(data) - 1];
	uint32_t work_buf_size, return_code;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 1;
	work_buf_size = cmp_cal_work_buf_size(&params, sizeof(data));
	TEST_ASSERT_LESS_THAN(work_buf_size, sizeof(work_buf));
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	return_code = compress_func(&ctx, dst, sizeof(dst), data, sizeof(data));

	TEST_ASSERT_CMP_FAILURE(return_code);
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_TOO_SMALL, return_code);
}


TEST_CASE(compress_u16_wrapper)
TEST_CASE(compress_i16_wrapper)
void test_detect_src_size_change_using_model_preprocessing(compress_func_t compress_func)
{
	const uint16_t data1[4] = { 0 };
	const uint16_t data2[3] = { 0 };

	uint32_t return_code;
	struct test_env *e;
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.secondary_iterations = 10;
	e = make_env(&params, sizeof(data1));

	TEST_ASSERT_CMP_SUCCESS(compress_func(&e->ctx, e->dst, e->dst_cap, data1, sizeof(data1)));
	return_code = compress_func(&e->ctx, e->dst, e->dst_cap, data2, sizeof(data2));

	TEST_ASSERT_CMP_FAILURE(return_code);
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_MISMATCH, return_code);

	free_env(e);
}

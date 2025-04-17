/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Pre-Processing Tests
 */

#define UNITY_SUPPORT_TEST_CASES
#include <unity.h>
#include "test_common.h"
#include "iwt_test_data.h"

#include "../lib/cmp.h"
#include "../lib/common/header.h"
#include "../lib/common/compiler.h"
#include "../programs/byteorder.h"


#define TEST_ASSERT_CMP_HDR(compressed_data, size, expeced_hdr)							\
do {															\
	struct cmp_hdr assert_hdr;											\
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(compressed_data, size, &assert_hdr));				\
	TEST_ASSERT_EQUAL_MESSAGE(expeced_hdr.version, assert_hdr.version, "header version mismatch");			\
	TEST_ASSERT_EQUAL_MESSAGE(expeced_hdr.cmp_size, assert_hdr.cmp_size, "header compressed data size mismatch");	\
	TEST_ASSERT_EQUAL_MESSAGE(expeced_hdr.original_size, assert_hdr.original_size, "header original size mismatch");\
	TEST_ASSERT_EQUAL_MESSAGE(expeced_hdr.mode, assert_hdr.mode, "header mode mismatch");				\
	TEST_ASSERT_EQUAL_MESSAGE(expeced_hdr.preprocess, assert_hdr.preprocess, "header preprocessing mismatch");	\
	TEST_ASSERT_EQUAL_MESSAGE(expeced_hdr.model_rate, assert_hdr.model_rate, "model rate mismatch");		\
	TEST_ASSERT_EQUAL_MESSAGE(expeced_hdr.pass_count, assert_hdr.pass_count, "pass counter mismatch");		\
	assert_hdr.model_id = expeced_hdr.model_id; /* ignore to check model id*/					\
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&expeced_hdr, &assert_hdr, sizeof(expeced_hdr), "header mismatch");		\
} while (0)


static void convert_cmp_data_to_system_endianness(void *compressed_data, uint32_t size)
{
	uint32_t const cmp_data_size = size - CMP_HDR_SIZE;
	uint32_t const num_16_samples = cmp_data_size/sizeof(uint16_t);
	uint32_t i;

	TEST_ASSERT_CMP_SUCCESS(size);
	TEST_ASSERT_TRUE(size > CMP_HDR_SIZE);
	TEST_ASSERT_FALSE(cmp_data_size % sizeof(uint16_t));

	for (i = 0; i < num_16_samples; i++) {
		uint16_t *p = cmp_hdr_get_cmp_data(compressed_data);

		be16_to_cpus(&p[i]);
	}
}


void test_1d_difference_preprocessing_for_multiple_values(void)
{
	const uint16_t input_data[] = {
		1, 3,  0, UINT16_MAX, 0, INT16_MAX, (uint16_t)INT16_MIN, (uint16_t)-5 };
	const int16_t expected_1d_diff[ARRAY_SIZE(input_data)] = {
		1, 2, -3, -1, 1, INT16_MAX, 1, 0x7FFB };
	uint8_t output_buf[CMP_HDR_SIZE + sizeof(expected_1d_diff)];
	uint32_t output_size;
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	struct cmp_hdr expected_hdr = { 0 };

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_DIFF;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_data, sizeof(input_data));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_1d_diff), output_size);
	convert_cmp_data_to_system_endianness(output_buf, output_size);
	TEST_ASSERT_EQUAL_INT16_ARRAY(expected_1d_diff, cmp_hdr_get_cmp_data(output_buf), ARRAY_SIZE(expected_1d_diff));
	expected_hdr.version = CMP_VERSION_NUMBER;
	expected_hdr.cmp_size = output_size;
	expected_hdr.original_size = sizeof(input_data);
	expected_hdr.mode = params.mode;
	expected_hdr.preprocess = params.primary_preprocessing;
	TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
}


const int16_t iwt_input_single[1] = {0x0F};
const int16_t iwt_expected_singel[1] = {0x0F};

const int16_t iwt_input_two[2] = { -23809, 23901 };
const int16_t iwt_expected_two[2] = { -32722, -17826 };

const int16_t iwt_input_five[5] = { -1, 2, -3, 4, -5};
const int16_t iwt_expected_five[5] = {0, 4, 0, 8, -2};

const int16_t iwt_input_eight[8] = { -3, 2, -1, 3, -2, 5, 0, 7 };
const int16_t iwt_expected_eight[8] = { 0, 4,  2, 5,  1, 6, 3, 7 };

TEST_CASE(iwt_input_single, iwt_expected_singel, sizeof(iwt_expected_singel))
TEST_CASE(iwt_input_two, iwt_expected_two, sizeof(iwt_input_two))
TEST_CASE(iwt_input_five, iwt_expected_five, sizeof(iwt_input_five))
TEST_CASE(iwt_input_eight, iwt_expected_eight, sizeof(iwt_input_eight))
void test_iwt_transform_correct(const int16_t *input_data,
				const int16_t *expeced_output, uint32_t size)
{
	int16_t work_buf[8];
	uint8_t output_buf[CMP_HDR_SIZE+8*sizeof(int16_t)];
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_IWT;
	TEST_ASSERT_TRUE(sizeof(work_buf) >= size);
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, size));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), (const uint16_t *)input_data, size);

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + size, output_size);
	convert_cmp_data_to_system_endianness(output_buf, output_size);
	TEST_ASSERT_EQUAL_INT16_ARRAY(expeced_output, cmp_hdr_get_cmp_data(output_buf), size/sizeof(int16_t));
	{	struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.version = CMP_VERSION_NUMBER;
		expected_hdr.cmp_size = CMP_HDR_SIZE + size;
		expected_hdr.original_size = size;
		expected_hdr.mode = params.mode;
		expected_hdr.preprocess = params.primary_preprocessing;
		TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
	}
}


void test_model_preprocessing_for_multiple_values(void)
{
	const uint16_t start_model[] = { 0, 1, 10 };
	const uint16_t data[ARRAY_SIZE(start_model)] = { 1, 3, 5 };
	const int16_t expected_output[ARRAY_SIZE(start_model)] = { 1, 2, -5 };
	uint8_t work_buf[sizeof(start_model)];
	uint8_t output_buf[CMP_HDR_SIZE + sizeof(start_model)];
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.max_secondary_passes = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));
	TEST_ASSERT_CMP_SUCCESS(cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), start_model, sizeof(start_model)));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), data, sizeof(data));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_output), output_size);
	convert_cmp_data_to_system_endianness(output_buf, output_size);
	TEST_ASSERT_EQUAL_INT16_ARRAY(expected_output, cmp_hdr_get_cmp_data(output_buf), ARRAY_SIZE(expected_output));
	{	struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.version = CMP_VERSION_NUMBER;
		expected_hdr.cmp_size = output_size;
		expected_hdr.original_size = sizeof(data);
		expected_hdr.mode = params.mode;
		expected_hdr.preprocess = params.secondary_preprocessing;
		expected_hdr.pass_count = 1;
		TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
	}
}


void test_model_updates_correctly(void)
{
	const uint16_t input1[3] = { 0, 1, 10 };
	const uint16_t input2[3] = { 1, 3, 5 };
	const uint16_t input3[3] = { 0, 0, 0 };
	const int16_t expected_output[3] = { 0, -2, -5 };
	uint16_t work_buf[ARRAY_SIZE(input1)];
	uint8_t output_buf[CMP_HDR_SIZE + sizeof(input1)];
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.model_rate = 1;
	params.max_secondary_passes = 2;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));
	TEST_ASSERT_CMP_SUCCESS(cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input1, sizeof(input1)));
	TEST_ASSERT_CMP_SUCCESS(cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input2, sizeof(input2)));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input3, sizeof(input3));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_output), output_size);
	convert_cmp_data_to_system_endianness(output_buf, output_size);
	TEST_ASSERT_EQUAL_INT16_ARRAY(expected_output, cmp_hdr_get_cmp_data(output_buf), ARRAY_SIZE(expected_output));
	{	struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.version = CMP_VERSION_NUMBER;
		expected_hdr.cmp_size = output_size;
		expected_hdr.original_size = sizeof(input2);
		expected_hdr.mode = params.mode;
		expected_hdr.preprocess = params.secondary_preprocessing;
		expected_hdr.model_rate = 1;
		expected_hdr.pass_count = 2;
		TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
	}
}


void test_primary_preprocessing_after_max_secondary_passes(void)
{
	const uint16_t input[3] = { INT16_MAX, UINT16_MAX, 0 };
	const uint16_t input_after_reset[3] = { 1, 2, 3 };
	const int16_t expected_output[3] = { 1, 2, 3 };
	uint8_t work_buf[sizeof(input)];
	uint8_t output_buf[CMP_HDR_SIZE + sizeof(input)];
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.max_secondary_passes = 2;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));
	TEST_ASSERT_CMP_SUCCESS(cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input, sizeof(input)));
	TEST_ASSERT_CMP_SUCCESS(cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input, sizeof(input)));
	TEST_ASSERT_CMP_SUCCESS(cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input, sizeof(input)));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_after_reset, sizeof(input_after_reset));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_output), output_size);
	convert_cmp_data_to_system_endianness(output_buf, output_size);
	TEST_ASSERT_EQUAL_INT16_ARRAY(expected_output, cmp_hdr_get_cmp_data(output_buf), ARRAY_SIZE(expected_output));
	{	struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.version = CMP_VERSION_NUMBER;
		expected_hdr.cmp_size = output_size;
		expected_hdr.original_size = sizeof(input);
		expected_hdr.mode = params.mode;
		expected_hdr.preprocess = CMP_PREPROCESS_NONE;
		TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
	}
}


void test_detects_invalid_model_rate(void)
{
	uint32_t return_value;
	struct cmp_context ctx;
	uint16_t work_buf[4];
	struct cmp_params params = { 0 };

	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.max_secondary_passes = 1;
	params.model_rate = 17;

	return_value = cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_value);
}


void test_detect_invalid_primary_presseing_model_usage(void)
{
	struct cmp_context ctx;
	uint16_t work_buf[3];
	uint32_t return_val;
	struct cmp_params par = { 0 };

	par.primary_preprocessing = CMP_PREPROCESS_MODEL;

	return_val = cmp_initialise(&ctx, &par, work_buf, sizeof(work_buf));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_detects_invalid_max_secondary_passes_value(void)
{
	uint32_t return_value;
	struct cmp_context ctx;
	uint16_t work_buf[4];
	struct cmp_params params = { 0 };

	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.max_secondary_passes = CMP_MAX_SECONDARY_PASSES_MAX + 1;

	return_value = cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_value);
}


void test_assigns_unique_model_ids(void)
{
	const uint16_t input1[3] = { 0, 1, 10 },
		       input2[3] = { 1, 3, 5 };
	uint16_t work_buf1[ARRAY_SIZE(input1)],
		 work_buf2[ARRAY_SIZE(input2)];
	uint8_t output_buf1[CMP_HDR_SIZE + sizeof(input1)],
		output_buf2[CMP_HDR_SIZE + sizeof(input2)];
	uint32_t output_size1, output_size2;
	struct cmp_context ctx1, ctx2;
	struct cmp_params params = { 0 };

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.max_secondary_passes = 10;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx1, &params, work_buf1, sizeof(work_buf1)));
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx2, &params, work_buf2, sizeof(work_buf2)));

	output_size1 = cmp_compress_u16(&ctx1, output_buf1, sizeof(output_buf1), input1, sizeof(input1));
	output_size2 = cmp_compress_u16(&ctx2, output_buf2, sizeof(output_buf2), input2, sizeof(input2));

	TEST_ASSERT_CMP_SUCCESS(output_size1);
	TEST_ASSERT_CMP_SUCCESS(output_size2);
	{	struct cmp_hdr hdr1, hdr2;

		TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(output_buf1, output_size1, &hdr1));
		TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(output_buf2, output_size2, &hdr2));
		TEST_ASSERT_NOT_EQUAL(hdr1.model_id, hdr2.model_id);
	}
}


void test_detect_to_small_work_buffer_in_model_preprocessing(void)
{
	const uint16_t data[] = { 0, 0, 0};
	uint8_t *dst[CMP_HDR_SIZE + sizeof(data)];
	uint8_t work_buf[sizeof(data)-1];
	uint32_t work_buf_size, return_code;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.max_secondary_passes = 1;
	work_buf_size = cmp_cal_work_buf_size(&params, sizeof(data));
	TEST_ASSERT_LESS_THAN(work_buf_size, sizeof(work_buf));
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	return_code = cmp_compress_u16(&ctx, dst, sizeof(dst), data, sizeof(data));

	TEST_ASSERT_CMP_FAILURE(return_code);
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_TOO_SMALL, return_code);
}


void test_detect_scr_size_change_using_model_preprocessing(void)
{
	const uint16_t data1[] = { 0, 0, 0, 0 };
	const uint16_t data2[] = { 0, 0, 0 };
	uint8_t work_buf[sizeof(data1)];
	uint8_t *dst[CMP_HDR_SIZE + sizeof(data1)];
	uint32_t return_code;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.primary_preprocessing = CMP_PREPROCESS_NONE;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.max_secondary_passes = 10;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));
	TEST_ASSERT_CMP_SUCCESS(cmp_compress_u16(&ctx, dst, sizeof(dst), data1, sizeof(data1)));

	return_code = cmp_compress_u16(&ctx, dst, sizeof(dst), data2, sizeof(data2));

	TEST_ASSERT_CMP_FAILURE(return_code);
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_MISMATCH, return_code);
}

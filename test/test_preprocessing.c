/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Preprocessing Tests
 */

/* #include <string.h> */

#include <unity.h>
#include "test_common.h"

#include "../lib/cmp.h"
#include "../lib/common/header.h"
#include "../lib/common/compiler.h"
#include "../programs/byteorder.h"

#define TEST_ASSERT_EQUAL_CMP_HDR(compressed_data, size, expeced_hdr)						\
do {														\
	struct cmp_hdr assert_hdr;										\
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(compressed_data, size, &assert_hdr));			\
	TEST_ASSERT_EQUAL_MESSAGE(expeced_hdr.version, assert_hdr.version, "header version");			\
	TEST_ASSERT_EQUAL_MESSAGE(expeced_hdr.cmp_size, assert_hdr.cmp_size, "header compressed data size");	\
	TEST_ASSERT_EQUAL_MESSAGE(expeced_hdr.original_size, assert_hdr.original_size, "header original size");	\
	TEST_ASSERT_EQUAL_MESSAGE(expeced_hdr.mode, assert_hdr.mode, "header mode");				\
	TEST_ASSERT_EQUAL_MESSAGE(expeced_hdr.preprocess, assert_hdr.preprocess, "header preprocessing");	\
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&expeced_hdr, &assert_hdr, sizeof(expeced_hdr), "header mismatch");	\
} while (0)


void test_1d_difference_preprocessing_for_multiple_values(void)
{
	const uint16_t input_data[] = { 1, 3,  0, UINT16_MAX, 0, INT16_MAX, (uint16_t)INT16_MIN, (uint16_t)-5 };
	const int16_t exp_1d_diff[ARRAY_SIZE(input_data)] = { 1, 2, -3, -1, 1, INT16_MAX, 1, 0x7FFB }; /* in system endianness */
	uint8_t output_buf[CMP_HDR_SIZE + sizeof(exp_1d_diff)];
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	uint32_t output_size, i;
	struct cmp_hdr exp_hdr = { 0 };

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.preprocess = CMP_PREPROCESS_DIFF;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_data, sizeof(input_data));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(exp_1d_diff), output_size);
	/* convert to system endianness */
	for (i = 0; i < ARRAY_SIZE(exp_1d_diff); i++) {
		uint16_t *p = cmp_hdr_get_cmp_data(output_buf);

		be16_to_cpus(&p[i]);
	}
	TEST_ASSERT_EQUAL_INT16_ARRAY(exp_1d_diff, cmp_hdr_get_cmp_data(output_buf), ARRAY_SIZE(exp_1d_diff));
	exp_hdr.version = CMP_VERSION_NUMBER;
	exp_hdr.cmp_size = output_size;
	exp_hdr.original_size = sizeof(input_data);
	exp_hdr.mode = params.mode;
	exp_hdr.preprocess = params.preprocess;
	TEST_ASSERT_EQUAL_CMP_HDR(output_buf, output_size, exp_hdr);
}


void test_iwt_preprocessing_eight_values(void)
{
	const int16_t input_data[] = { -3, 2, -1, 3, -2, 5, 0, 7 };
	const int16_t exp_iwt[] = {  0, 4,  2, 5,  1, 6, 3, 7 };
	uint8_t output_buf[CMP_HDR_SIZE + sizeof(exp_iwt)];
	uint8_t work_buf[sizeof(input_data)];
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	struct cmp_hdr exp_hdr = { 0 };
	uint32_t i, output_size;

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.preprocess = CMP_PREPROCESS_IWT;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), (const uint16_t *)input_data, sizeof(input_data));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(exp_iwt), output_size);
	/* convert to system endianness */
	for (i = 0; i < ARRAY_SIZE(exp_iwt); i++) {
		uint16_t *p = cmp_hdr_get_cmp_data(output_buf);

		be16_to_cpus(&p[i]);
	}
	TEST_ASSERT_EQUAL_INT16_ARRAY(exp_iwt, cmp_hdr_get_cmp_data(output_buf), ARRAY_SIZE(exp_iwt));
	exp_hdr.version = CMP_VERSION_NUMBER;
	exp_hdr.cmp_size = output_size;
	exp_hdr.original_size = sizeof(input_data);
	exp_hdr.mode = params.mode;
	exp_hdr.preprocess = params.preprocess;
	TEST_ASSERT_EQUAL_CMP_HDR(output_buf, output_size, exp_hdr);
}


void test_iwt_preprocessing_single_value(void)
{
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	uint32_t dst_size;
	const uint16_t data = 0x0F;
	const uint8_t iwt_exp[] = { 0x00, 0x0F };
	uint8_t *dst[CMP_HDR_SIZE + sizeof(data)];
	uint8_t work_buf[sizeof(data)];

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.preprocess = CMP_PREPROCESS_IWT;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	dst_size = cmp_compress_u16(&ctx, dst, sizeof(dst), &data, sizeof(data));

	TEST_ASSERT_CMP_SUCCESS(dst_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(iwt_exp), dst_size);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(iwt_exp, cmp_hdr_get_cmp_data(dst), ARRAY_SIZE(iwt_exp));
}


void test_iwt_preprocessing_for_2_samples(void)
{
	const uint16_t input_data[2] = { (uint16_t)-23809, 23901};
	const int16_t exp_iwt[2] = { -32722, -17826};
	uint8_t output_buf[CMP_HDR_SIZE + sizeof(exp_iwt)];
	uint8_t work_buf[sizeof(input_data)];
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	struct cmp_hdr exp_hdr = { 0 };
	uint32_t i, output_size;

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.preprocess = CMP_PREPROCESS_IWT;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), input_data, sizeof(input_data));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(exp_iwt), output_size);
	/* convert to system endianness */
	for (i = 0; i < ARRAY_SIZE(exp_iwt); i++) {
		uint16_t *p = cmp_hdr_get_cmp_data(output_buf);

		be16_to_cpus(&p[i]);
	}
	TEST_ASSERT_EQUAL_INT16_ARRAY(exp_iwt, cmp_hdr_get_cmp_data(output_buf), ARRAY_SIZE(exp_iwt));
	exp_hdr.version = CMP_VERSION_NUMBER;
	exp_hdr.cmp_size = output_size;
	exp_hdr.original_size = sizeof(input_data);
	exp_hdr.mode = params.mode;
	exp_hdr.preprocess = params.preprocess;
	TEST_ASSERT_EQUAL_CMP_HDR(output_buf, output_size, exp_hdr);
}


void test_iwt_preprocessing_five_values(void)
{
	const int16_t input_data[5] = { -1, 2, -3, 4, -5};
	const int16_t exp_iwt[5] = {0, 4, 0, 8, -2};
	uint8_t output_buf[CMP_HDR_SIZE + sizeof(exp_iwt)];
	uint8_t work_buf[sizeof(input_data)];
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	struct cmp_hdr exp_hdr = { 0 };
	uint32_t i, output_size;

	params.mode = CMP_MODE_UNCOMPRESSED;
	params.preprocess = CMP_PREPROCESS_IWT;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf)));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf), (const uint16_t *)input_data, sizeof(input_data));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(exp_iwt), output_size);
	/* convert to system endianness */
	for (i = 0; i < ARRAY_SIZE(exp_iwt); i++) {
		uint16_t *p = cmp_hdr_get_cmp_data(output_buf);

		be16_to_cpus(&p[i]);
	}
	TEST_ASSERT_EQUAL_INT16_ARRAY(exp_iwt, cmp_hdr_get_cmp_data(output_buf), ARRAY_SIZE(exp_iwt));
	exp_hdr.version = CMP_VERSION_NUMBER;
	exp_hdr.cmp_size = output_size;
	exp_hdr.original_size = sizeof(input_data);
	exp_hdr.mode = params.mode;
	exp_hdr.preprocess = params.preprocess;
	TEST_ASSERT_EQUAL_CMP_HDR(output_buf, output_size, exp_hdr);
}

/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Compression Context Initialisation Tests
 */

#include <string.h>

#include <unity.h>
#include "test_common.h"

#include "../lib/cmp.h"
#include "../lib/cmp_errors.h"


void test_successful_compression_initialisation_without_work_buf(void)
{
	struct cmp_context ctx;
	struct cmp_params par = { 0 };

	uint32_t return_val = cmp_initialise(&ctx, &par, NULL, 3);

	TEST_ASSERT_CMP_SUCCESS(return_val);
}


void test_successful_compression_initialisation_with_work_buf(void)
{
	struct cmp_context ctx;
	struct cmp_params par = { 0 };
	uint16_t work_buf[2];

	uint32_t return_val = cmp_initialise(&ctx, &par, work_buf, sizeof(work_buf));

	TEST_ASSERT_CMP_SUCCESS(return_val);
}


void test_detect_null_context_initialisation(void)
{
	struct cmp_params const par = { 0 };
	uint32_t return_val;

	return_val = cmp_initialise(NULL, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_CONTEXT_INVALID, return_val);
}


/*
 * Compression Parameters Tests
 */

void test_detect_null_parameters_initialisation(void)
{
	struct cmp_context const ctx_all_zero = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	memset(&ctx, 0xFF, sizeof(ctx));

	return_val = cmp_initialise(&ctx, NULL, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
	TEST_ASSERT_EQUAL_MEMORY(&ctx_all_zero, &ctx, sizeof(ctx));
}


#define INVALID_PREPROCESSING ((enum cmp_preprocessing)0xFFFF)
void test_detect_invalid_primary_preprocessing_initialization(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.primary_preprocessing = INVALID_PREPROCESSING;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_detect_primary_model_preprocessing_initialization(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.primary_preprocessing = CMP_PREPROCESS_MODEL;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_detect_invalid_secondary_preprocessing_initialization(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.secondary_iterations = 1;
	par.secondary_preprocessing = INVALID_PREPROCESSING;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_ignore_invalid_secondary_preprocessing_when_not_used(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.secondary_iterations = 0;
	par.secondary_preprocessing = INVALID_PREPROCESSING;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_CMP_SUCCESS(return_val);
}


#define INVALID_ENCODER ((enum cmp_encoder_type)0xFFFF)
void test_detect_invalid_primary_encoder_initialisation(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.primary_encoder_type = INVALID_ENCODER;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_detect_invalid_secondary_endoder_initialisation(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.secondary_iterations = 1;
	par.secondary_encoder_type = INVALID_ENCODER;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_ignore_invalid_secondary_encodder_when_not_used(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.secondary_iterations = 0;
	par.secondary_encoder_type = INVALID_ENCODER;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_CMP_SUCCESS(return_val);
}


void test_detects_invalid_secondary_iterations_value(void)
{
	uint32_t return_value;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.secondary_iterations = 256;

	return_value = cmp_initialise(&ctx, &params, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_value);
}


void test_detect_invalid_primary_golomb_encoder_parameter(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	par.primary_encoder_param = UINT16_MAX + 1;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);

	/* zero is also invalid */
	par.primary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	par.primary_encoder_param = 0;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_detect_secondary_primary_golomb_encoder_parameter(void)
{
	struct cmp_params par = { 0 };
	struct cmp_context ctx;
	uint32_t return_val;

	par.secondary_iterations = 1;
	par.secondary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	par.secondary_encoder_param = UINT16_MAX + 1;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);

	/* zero is also invalid */
	par.secondary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	par.secondary_encoder_param = 0;

	return_val = cmp_initialise(&ctx, &par, NULL, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_val);
}


void test_detects_invalid_model_rate(void)
{
	uint32_t return_value;
	struct cmp_context ctx;
	uint16_t work_buf[4];
	struct cmp_params params = { 0 };

	params.secondary_iterations = 1;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;
	params.model_rate = 17;

	return_value = cmp_initialise(&ctx, &params, work_buf, sizeof(work_buf));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_PARAMS_INVALID, return_value);
}


/*
 * Work Buffer Initialisation Tests
 */

void test_detect_missing_iwt_work_buffer(void)
{
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	uint32_t return_value;

	params.primary_preprocessing = CMP_PREPROCESS_IWT;

	return_value = cmp_initialise(&ctx, &params, NULL, 0);

	TEST_ASSERT_CMP_FAILURE(return_value);
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_NULL, return_value);
}


void test_detect_missing_model_work_buffer(void)
{
	struct cmp_params params = { 0 };
	struct cmp_context ctx;
	uint32_t return_value;

	params.secondary_iterations = 1;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;

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

	params.secondary_iterations = 1;
	params.secondary_preprocessing = CMP_PREPROCESS_MODEL;

	return_value = cmp_initialise(&ctx, &params, work_buf, 0);

	TEST_ASSERT_CMP_FAILURE(return_value);
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_WORK_BUF_TOO_SMALL, return_value);
}

/**
 * @file   test_cmp.c
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 *
 * @copyright GPLv2
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @brief Data Compression Tests
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <unity.h>

#include <cmp.h>
#include <cmp_errors.h>
#include <header.h>

/*==== Compression lib test macros/functions ====*/
/* Compression return code assert macros */
#define TEST_ASSERT_EQUAL_CMP_ERROR(expected_CMP_ERROR, cmp_ret_code)                       \
	TEST_ASSERT_EQUAL_INT_MESSAGE(expected_CMP_ERROR, cmp_get_error_code(cmp_ret_code), \
				      gen_cmp_error_message(expected_CMP_ERROR,             \
							    cmp_get_error_code(cmp_ret_code)))

#define TEST_ASSERT_CMP_SUCCESS(cmp_ret_code) \
	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_NO_ERROR, cmp_ret_code)

#define TEST_ASSERT_CMP_FAILURE(cmp_ret_code) TEST_ASSERT_TRUE(cmp_is_error(cmp_ret_code))


static const char *cmp_error_enum_to_str(enum cmp_error error)
{
	switch (error) {
	case CMP_ERR_NO_ERROR:
		return "CMP_ERR_NO_ERROR";
	case CMP_ERR_GENERIC:
		return "CMP_ERR_GENERIC";
	case CMP_ERR_PARAMS_INVALID:
		return "CMP_ERR_PARAMS_INVALID";
	case CMP_ERR_CONTEXT_INVALID:
		return "CMP_ERR_CONTEXT_INVALID";
	case CMP_ERR_WORK_BUF_NULL:
		return "CMP_ERR_WORK_BUF_NULL";
	case CMP_ERR_WORK_BUF_TOO_SMALL:
		return "CMP_ERR_WORK_BUF_TOO_SMALL";
	case CMP_ERR_DST_NULL:
		return "CMP_ERR_DST_NULL";
	case CMP_ERR_SRC_NULL:
		return "CMP_ERR_SRC_NULL";
	case CMP_ERR_SRC_SIZE_WRONG:
		return "CMP_ERR_SRC_SIZE_WRONG";
	case CMP_ERR_DST_TOO_SMALL:
		return "CMP_ERR_DST_TOO_SMALL";
	case CMP_ERR_INT_HDR:
		return "CMP_ERR_INT_HDR";
	case CMP_ERR_MAX_CODE:
	default:
		TEST_FAIL_MESSAGE("Missing error name");
		return "Unknown error";
	}
}


static const char *gen_cmp_error_message(enum cmp_error expected, enum cmp_error actual)
{
	enum{ CMP_TEST_MESSAGE_BUF_SIZE = 128 };
	static char message[CMP_TEST_MESSAGE_BUF_SIZE];

	snprintf(message, sizeof(message), "Expected %s Was %s.", cmp_error_enum_to_str(expected),
		 cmp_error_enum_to_str(actual));
	return message;
}


/*==== Tests ====*/
/* Global variables */
static struct cmp_context g_ctx_uncompressed;
static const uint16_t g_src[2] = { 0x0001, 0x0203 };
static uint8_t g_dst[CMP_HDR_SIZE + sizeof(g_src)];


/* Test setup executed for every test case */
void setUp(void)
{
	struct cmp_params par_uncompressed = { 0 };
	uint32_t return_val;

	par_uncompressed.mode = CMP_MODE_UNCOMPRESSED;
	/* we do not need a working buffer for CMP_MODE_UNCOMPRESSED */
	return_val = cmp_initialise(&g_ctx_uncompressed, &par_uncompressed, NULL, 0);
	TEST_ASSERT_CMP_SUCCESS(return_val);

	memset(g_dst, 0xBE, sizeof(g_dst));
}


void test_no_work_buf_needed_for_uncompressed_mode(void)
{
	struct cmp_params par_uncompressed = { 0 };
	uint32_t work_buf_size;

	par_uncompressed.mode = CMP_MODE_UNCOMPRESSED;

	work_buf_size = cmp_cal_work_buf_size(&par_uncompressed, 42);

	TEST_ASSERT_EQUAL(0, work_buf_size);
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


void test_compression_in_uncompressed_mode(void)
{
	const uint16_t data[2] = { 0x0001, 0x0203 };
	uint8_t *dst[CMP_HDR_SIZE + sizeof(data)];
	/* uncompressed data should be in big endian */
	const uint8_t cmp_data_exp[sizeof(data)] = { 0x00, 0x01, 0x02, 0x03 };
	struct cmp_hdr hdr;

	uint32_t const cmp_size =
		cmp_compress_u16(&g_ctx_uncompressed, dst, sizeof(dst), data, sizeof(data));

	TEST_ASSERT_CMP_SUCCESS(cmp_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + 4, cmp_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(cmp_data_exp, cmp_hdr_get_cmp_data(dst), sizeof(data));
	cmp_hdr_deserialize(dst, cmp_size, &hdr);
	TEST_ASSERT_EQUAL(CMP_VERSION_NUMBER, hdr.version);
	TEST_ASSERT_EQUAL(cmp_size, hdr.cmp_size);
	TEST_ASSERT_EQUAL(sizeof(data), hdr.original_size);
}


void test_compression_detects_too_small_dst_buffer(void)
{
	uint8_t dst[CMP_HDR_SIZE + sizeof(g_src) - 1];

	uint32_t const cmp_size =
		cmp_compress_u16(&g_ctx_uncompressed, dst, sizeof(dst), g_src, sizeof(g_src));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_TOO_SMALL, cmp_size);
}


void test_compression_detects_missing_context(void)
{
	uint32_t const cmp_size =
		cmp_compress_u16(NULL, g_dst, sizeof(g_dst), g_src, sizeof(g_src));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_CONTEXT_INVALID, cmp_size);
}


void test_compression_detects_missing_dst_buffer(void)
{
	uint32_t const size =
		cmp_compress_u16(&g_ctx_uncompressed, NULL, 0, g_src, sizeof(g_src));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_NULL, size);
}


void test_compression_detects_missing_src_data(void)
{
	uint32_t const size = cmp_compress_u16(&g_ctx_uncompressed, g_dst, sizeof(g_dst),
					       NULL, sizeof(uint16_t));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_NULL, size);
}


void test_compression_detects_src_size_is_0(void)
{
	uint32_t const cmp_size =
		cmp_compress_u16(&g_ctx_uncompressed, g_dst, sizeof(g_dst), g_src, 0);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_WRONG, cmp_size);
}


void test_compression_detects_src_size_is_not_multiple_of_2(void)
{
	const uint16_t src[2] = { 0 };
	uint32_t const src_size = 3;

	uint32_t const cmp_size =
		cmp_compress_u16(&g_ctx_uncompressed, g_dst, sizeof(g_dst), src, src_size);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_WRONG, cmp_size);
}


void test_compression_detects_src_size_too_large(void)
{
	uint32_t const src_size_too_large = CMP_MAX_ORIGINAL_SIZE + 1;

	uint32_t const cmp_size
		= cmp_compress_u16(&g_ctx_uncompressed, g_dst, sizeof(g_dst),
				   g_src, src_size_too_large);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_WRONG, cmp_size);
}


void test_successful_reset_of_compressed_data(void)
{
	uint32_t const return_val = cmp_reset(&g_ctx_uncompressed);

	TEST_ASSERT_CMP_SUCCESS(return_val);
}


void test_compression_reset_detect_missing_context(void)
{
	uint32_t const return_val = cmp_reset(NULL);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_CONTEXT_INVALID, return_val);
}


void test_deinitialise_a_compression_context(void)
{
	struct cmp_context const zero_ctx = { 0 };

	cmp_deinitialise(&g_ctx_uncompressed);

	TEST_ASSERT_EQUAL_MEMORY(&g_ctx_uncompressed, &zero_ctx, sizeof(g_ctx_uncompressed));
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

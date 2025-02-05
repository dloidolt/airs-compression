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


/* a global context for uncompressed mode which is fresh created for every test */
struct cmp_context ctx_uncompressed;
uint32_t dst_for_ctx_uncompressed[2];


void setUp(void)
{
	struct cmp_params par_uncompressed = {0};
	uint32_t size;

	memset(dst_for_ctx_uncompressed, 0xCD, sizeof(dst_for_ctx_uncompressed));

	par_uncompressed.mode = CMP_MODE_UNCOMPRESSED;
	size = cmp_initialise(&ctx_uncompressed, dst_for_ctx_uncompressed,
		sizeof(dst_for_ctx_uncompressed), &par_uncompressed, NULL, 0);
	TEST_ASSERT_FALSE(cmp_is_error(size));
}


void test_calculation_of_work_buf_size(void)
{
	uint32_t work_buf_size = cmp_cal_work_buf_size(42);

	TEST_ASSERT_EQUAL(42, work_buf_size);
}


void test_successful_compression_initialisation_with_work_buf(void)
{
	struct cmp_context ctx;
	uint16_t work_buf[2];
	uint32_t work_buf_size = sizeof(work_buf);
	uint32_t dst[2];
	uint32_t dst_capacity = sizeof(dst);
	uint32_t size;
	struct cmp_params par = {0};

	size = cmp_initialise(&ctx, dst, dst_capacity, &par, work_buf, work_buf_size);

	TEST_ASSERT_EQUAL_size_t(CMP_HDR_SIZE, size);
}


void test_invalid_compression_initialisation_no_context(void)
{
	struct cmp_params par = {0};
	uint32_t dst[2];
	uint32_t dst_capacity = sizeof(dst);
	uint32_t size;

	size = cmp_initialise(NULL, dst, dst_capacity, &par, NULL, 0);

	TEST_ASSERT_TRUE(cmp_is_error(size));
	TEST_ASSERT_EQUAL(CMP_ERR_NO_CONTEXT, cmp_get_error_code(size));
}


void test_invalid_compression_initialisation_no_parameters(void)
{
	struct cmp_context ctx_all_zero = {0};
	struct cmp_context ctx;
	uint32_t dst[2];
	uint32_t dst_capacity = sizeof(dst);
	uint32_t size;

	memset(&ctx, 0xFF, sizeof(ctx));

	size = cmp_initialise(&ctx, dst, dst_capacity, NULL, NULL, 0);

	TEST_ASSERT_TRUE(cmp_is_error(size));
	TEST_ASSERT_EQUAL(CMP_ERR_NO_PARAMS, cmp_get_error_code(size));
	TEST_ASSERT_EQUAL_MEMORY(&ctx_all_zero, &ctx, sizeof(ctx));
}


void test_initial_data_processing_in_uncompressed_mode(void)
{
	struct cmp_context ctx;
	struct cmp_params par = {.mode = CMP_MODE_UNCOMPRESSED};
	struct cmp_hdr hdr;
	const uint16_t data[2] = {0x0001, 0x0203};
	/* uncompressed data should be in big endian */
	const uint8_t cmp_data_exp[sizeof(data)] = {0x00, 0x01, 0x02, 0x03};
	uint8_t *dst[CMP_HDR_SIZE + sizeof(data)];
	uint32_t cmp_size = cmp_initialise(&ctx, dst, sizeof(dst), &par, NULL, 0);

	TEST_ASSERT_FALSE(cmp_is_error(cmp_size));

	cmp_size = cmp_feed16(&ctx, data, sizeof(data));

	TEST_ASSERT_FALSE(cmp_is_error(cmp_size));
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(data), cmp_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(cmp_data_exp, cmp_hdr_get_cmp_data(dst), sizeof(data));
	cmp_hdr_deserialize(dst, cmp_size, &hdr);
	TEST_ASSERT_EQUAL(CMP_VERSION_NUMBER, hdr.version);
	TEST_ASSERT_EQUAL(cmp_size, hdr.cmp_size);
	TEST_ASSERT_EQUAL(sizeof(data), hdr.original_size);
}


void test_subsequent_data_processing_in_uncompressed_mode(void)
{
	struct cmp_context ctx;
	struct cmp_params par;
	struct cmp_hdr hdr;
	const uint16_t data1[2] = {0x0001, 0x0203};
	const uint16_t data2[2] = {0x0405, 0x0607};
	const uint8_t cmp_data_exp[sizeof(data1)+ sizeof(data2)] = {0x00, 0x01,
		0x02, 0x03, 0x04, 0x05, 0x06, 0x07}; /* uncompressed data should be in big endian */
	uint8_t dst[CMP_HDR_SIZE + sizeof(cmp_data_exp)];
	uint32_t cmp_size;

	par.mode = CMP_MODE_UNCOMPRESSED;
	cmp_size = cmp_initialise(&ctx, dst, sizeof(dst), &par, NULL, 0);
	TEST_ASSERT_FALSE(cmp_is_error(cmp_size));
	cmp_size = cmp_feed16(&ctx, data1, sizeof(data1));
	TEST_ASSERT_FALSE(cmp_is_error(cmp_size));

	cmp_size = cmp_feed16(&ctx, data2, sizeof(data2));

	TEST_ASSERT_FALSE(cmp_is_error(cmp_size));
	TEST_ASSERT_EQUAL(sizeof(dst), cmp_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(cmp_data_exp, cmp_hdr_get_cmp_data(dst),
				     sizeof(cmp_data_exp));
	cmp_hdr_deserialize(dst, cmp_size, &hdr);
	TEST_ASSERT_EQUAL(CMP_VERSION_NUMBER, hdr.version);
	TEST_ASSERT_EQUAL(cmp_size, hdr.cmp_size);
	TEST_ASSERT_EQUAL(sizeof(cmp_data_exp), hdr.original_size);
}


void test_feed_api_returns_error_when_no_context_provided(void)
{
	const uint16_t src[2] = {0x0001, 0x0203};
	uint32_t src_size = sizeof(src);

	uint32_t cmp_size = cmp_feed16(NULL, src, src_size);

	TEST_ASSERT_TRUE(cmp_is_error(cmp_size));
	TEST_ASSERT_EQUAL(CMP_ERR_NO_CONTEXT, cmp_get_error_code(cmp_size));
}


void test_feed_api_returns_error_when_no_data_provided(void)
{
	uint32_t size = cmp_feed16(&ctx_uncompressed, NULL, sizeof(uint16_t));

	TEST_ASSERT_TRUE(cmp_is_error(size));
	TEST_ASSERT_EQUAL(CMP_ERR_NO_SRC_DATA, cmp_get_error_code(size));
}


void test_feed_api_returns_error_when_data_size_is_zero(void)
{
	const uint16_t src[2] = {0};
	uint32_t src_size = 0;

	uint32_t size = cmp_feed16(&ctx_uncompressed, src, src_size);

	TEST_ASSERT_TRUE(cmp_is_error(size));
	TEST_ASSERT_EQUAL(CMP_ERR_INVALID_SRC_SIZE, cmp_get_error_code(size));
}


void test_feed_api_returns_error_when_data_size_is_not_multiple_of_2(void)
{
	const uint16_t src[2] = {0};
	uint32_t src_size = 3;

	uint32_t size = cmp_feed16(&ctx_uncompressed, src, src_size);

	TEST_ASSERT_TRUE(cmp_is_error(size));
	TEST_ASSERT_EQUAL(CMP_ERR_INVALID_SRC_SIZE, cmp_get_error_code(size));
}


void test_successful_reset_of_compressed_data(void)
{
	struct cmp_hdr hdr;

	uint32_t cmp_size = cmp_reset(&ctx_uncompressed);

	TEST_ASSERT_FALSE(cmp_is_error(cmp_size));
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE, cmp_size);
	cmp_hdr_deserialize(dst_for_ctx_uncompressed, cmp_size, &hdr);
	TEST_ASSERT_EQUAL(CMP_VERSION_NUMBER, hdr.version);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE, hdr.cmp_size);
	TEST_ASSERT_EQUAL(0, hdr.original_size);
}


void test_deinitialise_a_compression_context(void)
{
	struct cmp_context empty_ctx = {0};

	cmp_deinitialise(&ctx_uncompressed);

	TEST_ASSERT_EQUAL_MEMORY(&ctx_uncompressed, &empty_ctx, sizeof(ctx_uncompressed));
}


void test_deinitialise_NULL_context_gracefully(void)
{
	cmp_deinitialise(NULL);
}

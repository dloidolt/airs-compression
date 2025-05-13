/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Encoder Tests
 */

#include <stdint.h>

#include <unity.h>
#include "test_common.h"

#include "../lib/cmp.h"
#include "../lib/cmp_errors.h"
#include "../lib/common/header.h"
#include "../lib/compress/bitstream_write.h"

void test_bitstream_write_nothing(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	uint8_t buffer[1];

	bitstream_write_init(&bsw, buffer, sizeof(buffer));

	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL(0, size);
}

void test_bitstream_write_single_bit_one(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	uint8_t buffer[1];

	bitstream_write_init(&bsw, buffer, sizeof(buffer));

	bitstream_write(&bsw, 1, 1);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL_UINT8(0x80, buffer[0]);
	TEST_ASSERT_EQUAL(1, size);
}


void test_bitstream_write_two_bits_zero_one(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	uint8_t buffer[1];

	bitstream_write_init(&bsw, buffer, sizeof(buffer));

	bitstream_write(&bsw, 0, 1);
	bitstream_write(&bsw, 1, 1);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL_UINT8(0x40, buffer[0]);
	TEST_ASSERT_EQUAL(1, size);
}


void test_bitstream_write_10bytes(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	uint8_t buffer[10];
	uint8_t expected_bs[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };

	bitstream_write_init(&bsw, buffer, sizeof(buffer));

	bitstream_write(&bsw, 0x0001, 16);
	bitstream_write(&bsw, 0x0203, 16);
	bitstream_write(&bsw, 0x0405, 16);
	bitstream_write(&bsw, 0x0607, 16);
	bitstream_write(&bsw, 0x0809, 16);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL(sizeof(expected_bs), size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_bs, buffer, sizeof(expected_bs));
}


void test_detect_bitstream_overflow(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	uint8_t buffer[1];

	bitstream_write_init(&bsw, buffer, sizeof(buffer));

	bitstream_write(&bsw, 0x1F, 9);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_TOO_SMALL, size);
}

void test_bitstream_skip_first_8_bytes(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	uint8_t buffer[10] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
	uint8_t expected_bs[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xAB };

	bitstream_write_init(&bsw, buffer, sizeof(buffer));

	bitstream_skip_bytes(&bsw, 8);
	bitstream_write(&bsw, 0xAB, 8);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL(sizeof(expected_bs), size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_bs, buffer, sizeof(expected_bs));
}

void test_bitstream_skip_first_3_bytes(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	uint8_t buffer[10] = { 0xAB, 0xCD, 0xEF };
	uint8_t expected_bs[] = { 0xAB, 0xCD, 0xEF, 0x12 };

	bitstream_write_init(&bsw, buffer, sizeof(buffer));

	bitstream_skip_bytes(&bsw, 3);
	bitstream_write(&bsw, 0x12, 8);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL(sizeof(expected_bs), size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_bs, buffer, sizeof(expected_bs));
}


void test_bitstream_skip_over_buffer_end(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	uint8_t buffer[4];

	bitstream_write_init(&bsw, buffer, sizeof(buffer));

	bitstream_skip_bytes(&bsw, 7);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_TOO_SMALL, size);
}

void test_encode_values_with_golomb_zero(void)
{
	const int16_t data_to_encode[] = { -1, 1 };
	const uint8_t expected_output[] = { 0xDC };
	uint8_t output_buf[CMP_HDR_SIZE + 4];
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.mode = CMP_MODE_GOLOMB_ZERO;
	params.compression_par = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf),
				       (const uint16_t *)data_to_encode, sizeof(data_to_encode));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_output), output_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_output, cmp_hdr_get_cmp_data(output_buf),
				     sizeof(expected_output));
	{
		struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.version = CMP_VERSION_NUMBER;
		expected_hdr.cmp_size = output_size;
		expected_hdr.original_size = sizeof(data_to_encode);
		expected_hdr.mode = params.mode;
		expected_hdr.preprocess = params.secondary_preprocessing;
		expected_hdr.compression_par = params.compression_par;
		TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
	}
}


void test_encode_values_with_golomb_zero_withoutlier(void)
{
	const int16_t data_to_encode[] = { -9 };
	const uint8_t expected_output[] = { 0x00, 0x08, 0x80 };
	uint8_t output_buf[CMP_HDR_SIZE + 4];
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.mode = CMP_MODE_GOLOMB_ZERO;
	params.compression_par = 1;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf),
				       (const uint16_t *)data_to_encode, sizeof(data_to_encode));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_output), output_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_output, cmp_hdr_get_cmp_data(output_buf),
				     sizeof(expected_output));
	{
		struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.version = CMP_VERSION_NUMBER;
		expected_hdr.cmp_size = output_size;
		expected_hdr.original_size = sizeof(data_to_encode);
		expected_hdr.mode = params.mode;
		expected_hdr.preprocess = params.secondary_preprocessing;
		expected_hdr.compression_par = params.compression_par;
		TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
	}
}


void test_encode_values_with_compression_par(void)
{
	const int16_t data_to_encode[] = { 0, -10 };
	const uint8_t expected_output[] = { 0x1C, 0 };
	uint8_t output_buf[CMP_HDR_SIZE + 4];
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };

	params.mode = CMP_MODE_GOLOMB_ZERO;
	params.compression_par = 10;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	output_size = cmp_compress_u16(&ctx, output_buf, sizeof(output_buf),
				       (const uint16_t *)data_to_encode, sizeof(data_to_encode));

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_output), output_size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_output, cmp_hdr_get_cmp_data(output_buf),
				     sizeof(expected_output));
	{
		struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.version = CMP_VERSION_NUMBER;
		expected_hdr.cmp_size = output_size;
		expected_hdr.original_size = sizeof(data_to_encode);
		expected_hdr.mode = params.mode;
		expected_hdr.preprocess = params.secondary_preprocessing;
		expected_hdr.compression_par = params.compression_par;
		TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
	}
}

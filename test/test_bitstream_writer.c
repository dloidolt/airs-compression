/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2026
 * @copyright GPL-2.0
 *
 * @brief Bitstream Write Tests
 */

#include <stdint.h>

#include <unity.h>
#include "test_common.h"

#include "../lib/cmp_errors.h"
#include "../lib/common/bitstream_writer.h"


void test_bitstream_write_nothing(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	DST_ALIGNED_U8 buffer[1] = { 0xFF };

	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL(0, size);
}


void test_bitstream_write_single_bit_one(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	DST_ALIGNED_U8 buffer[1] = { 0xFF };

	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

	bitstream_add_bits32(&bsw, 1, 1);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL_UINT8(0x80, buffer[0]);
	TEST_ASSERT_EQUAL(1, size);
}


void test_bitstream_write_two_bits_zero_one(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	DST_ALIGNED_U8 buffer[1] = { 0xFF };

	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

	bitstream_add_bits32(&bsw, 0, 1);
	bitstream_add_bits32(&bsw, 1, 1);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL_UINT8(0x40, buffer[0]);
	TEST_ASSERT_EQUAL(1, size);
}


void test_bitstream_write_10bytes(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	DST_ALIGNED_U8 buffer[10];
	uint8_t expected_bs[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };

	memset(buffer, 0xFF, sizeof(buffer));

	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

	bitstream_add_bits32(&bsw, 0x0001, 16);
	bitstream_add_bits32(&bsw, 0x0203, 16);
	bitstream_add_bits32(&bsw, 0x0405, 16);
	bitstream_add_bits32(&bsw, 0x0607, 16);
	bitstream_add_bits32(&bsw, 0x0809, 16);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL(sizeof(expected_bs), size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_bs, buffer, sizeof(expected_bs));
}


void test_detect_bitstream_overflow(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	DST_ALIGNED_U8 buffer[1] = { 0xFF };

	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

	bitstream_add_bits32(&bsw, 0x1F, 9);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_DST_TOO_SMALL, size);
}


void test_bitstream_write_bytes_than_bits(void)
{
	uint8_t expected_bs[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
				  0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
				  0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };
	int16_t src16_input[] = { 0x0001, 0x0203, 0x0405, 0x0607, 0x0809, 0x0A0B,
				  0x0C0D, 0x0E0F, 0x1011, 0x1213, 0x1415, 0x1617 };
	uint32_t o;

	for (o = 0; o < 5; o++) {
		struct bitstream_writer bsw;
		uint32_t size;
		DST_ALIGNED_U8 buffer[sizeof(expected_bs)];
		int16_t src16[ARRAY_SIZE(src16_input) + 4];
		int16_t *src16_start = src16 + o;

		memset(buffer, 0xFF, sizeof(buffer));
		memset(src16, 0xFF, sizeof(src16));
		memcpy(src16_start, src16_input, sizeof(src16_input));

		TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

		bitstream_add_be16_array(&bsw, src16_start, ARRAY_SIZE(src16_input));
		bitstream_add_bits32(&bsw, 0x1819, 16);
		bitstream_add_bits32(&bsw, 0x1A, 8);
		bitstream_add_bits32(&bsw, 0x1B, 8);
		bitstream_add_bits32(&bsw, 0x1C1D1E1F, 32);
		size = bitstream_flush(&bsw);

		TEST_ASSERT_CMP_SUCCESS(size);
		TEST_ASSERT_EQUAL(sizeof(expected_bs), size);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_bs, buffer, sizeof(expected_bs));
	}
}


void test_bitstream_write_16in32_array_than_bits(void)
{
	uint8_t expected_bs[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
				  0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
				  0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
				  0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B };
	int32_t src16_in_32_input[] = {
		0x7FFF0001, 0x7FFF0203, 0x7FFF0405, 0x7FFF0607, 0x7FFF0809, 0x7FFF0A0B,
		0x7FFF0C0D, 0x7FFF0E0F, 0x7FFF1011, 0x7FFF1213, 0x7FFF1415, 0x7FFF1617,
		0x7FFF1819, 0x7FFF1A1B, 0x7FFF1C1D, 0x7FFF1E1F, 0x7FFF2021, 0x7FFF2223,
	};
	uint32_t o;

	for (o = 0; o < 2; o++) {
		uint32_t size;
		struct bitstream_writer bsw;
		DST_ALIGNED_U8 buffer[sizeof(expected_bs)];
		ALIGNED_TYPE(8, int32_t) src16_in_32[ARRAY_SIZE(src16_in_32_input) + 1];
		int32_t *src_start = src16_in_32 + o;

		memset(buffer, 0xFF, sizeof(buffer));
		memset(src16_in_32, 0xFF, sizeof(src16_in_32));
		memcpy(src_start, src16_in_32_input, sizeof(src16_in_32_input));

		TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

		bitstream_add_be16_in_32_array(&bsw, src_start, ARRAY_SIZE(src16_in_32_input));
		bitstream_add_bits32(&bsw, 0x2425, 16);
		bitstream_add_bits32(&bsw, 0x26, 8);
		bitstream_add_bits32(&bsw, 0x27, 8);
		bitstream_add_bits32(&bsw, 0x28292A2B, 32);
		size = bitstream_flush(&bsw);

		TEST_ASSERT_CMP_SUCCESS(size);
		TEST_ASSERT_EQUAL(sizeof(expected_bs), size);
		TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_bs, buffer, sizeof(expected_bs));
	}
}


void test_bitstream_write_16in32_array_requires_64bit_boundary(void)
{
	struct bitstream_writer bsw;
	DST_ALIGNED_U8 buffer[16] = { 0 };
	int32_t src32[2] = { 0x7FFF0001, 0x7FFF0203 };

	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

	bitstream_add_bits32(&bsw, 1, 1);
	bitstream_add_be16_in_32_array(&bsw, src32, ARRAY_SIZE(src32));

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_INT_BITSTREAM, bitstream_error(&bsw));
}


void test_bitstream_write_be16_array_with_cached_bits(void)
{
	uint32_t size;
	struct bitstream_writer bsw;
	uint8_t expected_bs[] = { 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98,
				  0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xD0 };
	DST_ALIGNED_U8 buffer[sizeof(expected_bs)];
	int16_t src16[] = { 0x1234, 0x5678, (int16_t)0x9ABC };

	memset(buffer, 0xFF, sizeof(buffer));

	TEST_ASSERT_CMP_SUCCESS(bitstream_writer_init(&bsw, buffer, sizeof(buffer)));

	bitstream_add_bits32(&bsw, 0x89ABCDEF, 32);
	bitstream_add_bits32(&bsw, 0xFEDCBA98, 32);
	bitstream_add_be16_array(&bsw, src16, ARRAY_SIZE(src16));
	bitstream_add_bits32(&bsw, 0xD, 4);
	size = bitstream_flush(&bsw);

	TEST_ASSERT_CMP_SUCCESS(size);
	TEST_ASSERT_EQUAL(sizeof(expected_bs), size);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_bs, buffer, sizeof(expected_bs));
}

/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief data compression header tests
 */

#include <stdint.h>
#include <string.h>

#include <unity.h>
#include "test_common.h"

#include "../lib/common/header.h"
#include "../lib/cmp_errors.h"
#include "../lib/cmp.h"
#include "../lib/common/bitstream_writer.h"


void test_serialize_header(void)
{
	struct cmp_hdr hdr = { 0 };
	uint8_t buf[CMP_HDR_SIZE];
	uint32_t hdr_size;
	int i;
	struct bitstream_writer bs;

	memset(buf, 0xAB, sizeof(buf));
	bitstream_writer_init(&bs, buf, sizeof(buf));
	hdr.version = 0x0001;
	hdr.cmp_size = 0x020304;
	hdr.original_size = 0x050607;
	hdr.mode = 0x08;
	hdr.preprocess = 0x09;
	hdr.model_rate = 0x0A;
	hdr.model_id = 0x0B0C0D0E0F10;
	hdr.pass_count = 0x11;
	hdr.compression_par = 0x1213;

	hdr_size = cmp_hdr_serialize(&bs, &hdr);

	TEST_ASSERT_FALSE(cmp_is_error(hdr_size));
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE, hdr_size);
	for (i = 0; i < CMP_HDR_SIZE; i++)
		TEST_ASSERT_EQUAL_HEX8(i, buf[i]);
}


void test_deserialize_header(void)
{
	struct cmp_hdr hdr = { 0 };
	uint8_t buf[CMP_HDR_SIZE];
	uint32_t hdr_size;
	int i;

	for (i = 0; i < CMP_HDR_SIZE; i++)
		buf[i] = (uint8_t)i;


	hdr_size = cmp_hdr_deserialize(buf, sizeof(buf), &hdr);

	TEST_ASSERT_FALSE(cmp_is_error(hdr_size));
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE, hdr_size);
	TEST_ASSERT_EQUAL_HEX(hdr.version, 0x0001);
	TEST_ASSERT_EQUAL_HEX(hdr.cmp_size, 0x020304);
	TEST_ASSERT_EQUAL_HEX(hdr.original_size, 0x050607);
	TEST_ASSERT_EQUAL_HEX(hdr.mode, 0x08);
	TEST_ASSERT_EQUAL_HEX(hdr.preprocess, 0x09);
	TEST_ASSERT_EQUAL_HEX(hdr.model_rate, 0x0A);
	TEST_ASSERT_EQUAL_HEX(hdr.model_id, 0x0B0C0D0E0F10);
	TEST_ASSERT_EQUAL_HEX(hdr.pass_count, 0x11);
	TEST_ASSERT_EQUAL_HEX(hdr.compression_par, 0x1213);
}


void test_hdr_serialize_returns_error_when_parmertes_are_to_big(void)
{
	struct cmp_hdr hdr = { 0 };
	uint8_t buf[CMP_HDR_SIZE];
	uint32_t hdr_size;
	struct bitstream_writer bs;

	hdr.original_size = 1 << CMP_HDR_BITS_ORIGINAL_SIZE;

	bitstream_writer_init(&bs, buf, sizeof(buf));
	hdr_size = cmp_hdr_serialize(&bs, &hdr);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_SRC_SIZE_WRONG, hdr_size);

	hdr.original_size = (1 << CMP_HDR_BITS_ORIGINAL_SIZE) - 1;
	hdr.version = UINT16_MAX + 1;

	bitstream_writer_init(&bs, buf, sizeof(buf));
	hdr_size = cmp_hdr_serialize(&bs, &hdr);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_INT_BITSTREAM, hdr_size);

	hdr.version = UINT16_MAX;
	hdr.cmp_size = 1 << CMP_HDR_BITS_CMP_SIZE;

	bitstream_writer_init(&bs, buf, sizeof(buf));
	hdr_size = cmp_hdr_serialize(&bs, &hdr);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_INT_BITSTREAM, hdr_size);

	hdr.cmp_size = (1 << CMP_HDR_BITS_CMP_SIZE) - 1;
	hdr.mode = UINT8_MAX + 1;

	bitstream_writer_init(&bs, buf, sizeof(buf));
	hdr_size = cmp_hdr_serialize(&bs, &hdr);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_INT_BITSTREAM, hdr_size);

	hdr.mode = UINT8_MAX;
	hdr.preprocess = UINT8_MAX + 1;

	bitstream_writer_init(&bs, buf, sizeof(buf));
	hdr_size = cmp_hdr_serialize(&bs, &hdr);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_INT_BITSTREAM, hdr_size);

	hdr.preprocess = UINT8_MAX;

	bitstream_writer_init(&bs, buf, sizeof(buf));
	hdr_size = cmp_hdr_serialize(&bs, &hdr);

	TEST_ASSERT_CMP_SUCCESS(hdr_size);
}


void test_detect_null_hdr_during_serialize(void)
{
	uint8_t buf[CMP_HDR_SIZE];
	struct bitstream_writer bs;
	uint32_t ret;

	bitstream_writer_init(&bs, buf, sizeof(buf));

	ret = cmp_hdr_serialize(&bs, NULL);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_INT_HDR, ret);
}

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


void test_serialize_header_with_extended_header(void)
{
	struct cmp_hdr hdr = { 0 };
	uint8_t buf[CMP_HDR_SIZE + CMP_EXT_HDR_SIZE];
	uint32_t hdr_size;
	int i;
	struct bitstream_writer bs;

	memset(buf, 0xAB, sizeof(buf));
	bitstream_writer_init(&bs, buf, sizeof(buf));
	hdr.version_flag = 0x00;
	hdr.version_id = 0x0001;
	hdr.compressed_size = 0x020304;
	hdr.original_size = 0x050607;
	hdr.identifier = 0x08090A0B0C0D;
	hdr.sequence_number = 0xE;
	hdr.preprocessing = 0x00;
	hdr.checksum_enabled = 0x01;
	hdr.encoder_type = 0x07;
	hdr.model_rate = 0x10;
	hdr.encoder_param = 0x1112;
	hdr.encoder_outlier = 0x131415;

	hdr_size = cmp_hdr_serialize(&bs, &hdr);

	TEST_ASSERT_CMP_SUCCESS(hdr_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + CMP_EXT_HDR_SIZE, hdr_size);
	for (i = 0; i < CMP_HDR_SIZE + CMP_EXT_HDR_SIZE; i++)
		TEST_ASSERT_EQUAL_HEX8(i, buf[i]);
}

void test_serialize_header_without_extended_header(void)
{
	struct cmp_hdr hdr = { 0 };
	uint8_t buf[CMP_HDR_SIZE + CMP_EXT_HDR_SIZE];
	uint32_t hdr_size;
	int i;
	struct bitstream_writer bs;

	memset(buf, 0xAB, sizeof(buf));
	bitstream_writer_init(&bs, buf, sizeof(buf));
	hdr.version_flag = 0x00;
	hdr.version_id = 0x0001;
	hdr.compressed_size = 0x020304;
	hdr.original_size = 0x050607;
	hdr.identifier = 0x08090A0B0C0D;
	hdr.sequence_number = 0xE;
	hdr.preprocessing = CMP_PREPROCESS_NONE;
	hdr.checksum_enabled = 0;
	hdr.encoder_type = CMP_ENCODER_UNCOMPRESSED;
	hdr.model_rate = 0x10;
	hdr.encoder_param = 0x1112;
	hdr.encoder_outlier = 0x131415;

	hdr_size = cmp_hdr_serialize(&bs, &hdr);

	TEST_ASSERT_CMP_SUCCESS(hdr_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE, hdr_size);
	for (i = 0; i < CMP_HDR_SIZE - 1; i++)
		TEST_ASSERT_EQUAL_HEX8(i, buf[i]);
	TEST_ASSERT_EQUAL_HEX8(0, buf[i++]);
	for (; i < CMP_HDR_SIZE + CMP_EXT_HDR_SIZE; i++)
		TEST_ASSERT_EQUAL_HEX8(0xAB, buf[i]);
}


void test_deserialize_header_with_extended_header(void)
{
	struct cmp_hdr hdr = { 0 };
	struct cmp_hdr expected_hdr;
	uint8_t buf[CMP_HDR_SIZE + CMP_EXT_HDR_SIZE];
	uint32_t hdr_size;
	int i;

	expected_hdr.version_flag = 0x00;
	expected_hdr.version_id = 0x0001;
	expected_hdr.compressed_size = 0x020304;
	expected_hdr.original_size = 0x050607;
	expected_hdr.identifier = 0x08090A0B0C0D;
	expected_hdr.sequence_number = 0xE;
	expected_hdr.preprocessing = 0x00;
	expected_hdr.checksum_enabled = 0x01;
	expected_hdr.encoder_type = 0x07;
	expected_hdr.model_rate = 0x10;
	expected_hdr.encoder_param = 0x1112;
	expected_hdr.encoder_outlier = 0x131415;
	for (i = 0; i < CMP_HDR_SIZE + CMP_EXT_HDR_SIZE; i++)
		buf[i] = (uint8_t)i;

	hdr_size = cmp_hdr_deserialize(buf, sizeof(buf), &hdr);

	TEST_ASSERT_CMP_SUCCESS(hdr_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + CMP_EXT_HDR_SIZE, hdr_size);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.version_flag, hdr.version_flag);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.version_id, hdr.version_id);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.compressed_size, hdr.compressed_size);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.original_size, hdr.original_size);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.identifier, hdr.identifier);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.sequence_number, hdr.sequence_number);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.preprocessing, hdr.preprocessing);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.checksum_enabled, hdr.checksum_enabled);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.encoder_type, hdr.encoder_type);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.model_rate, hdr.model_rate);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.encoder_param, hdr.encoder_param);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.encoder_outlier, hdr.encoder_outlier);
}


void test_deserialize_header_without_extended_header(void)
{
	struct cmp_hdr hdr = { 0 };
	struct cmp_hdr expected_hdr;
	uint8_t buf[CMP_HDR_SIZE + CMP_EXT_HDR_SIZE];
	uint32_t hdr_size;
	int i;

	expected_hdr.version_flag = 0x00;
	expected_hdr.version_id = 0x0001;
	expected_hdr.compressed_size = 0x020304;
	expected_hdr.original_size = 0x050607;
	expected_hdr.identifier = 0x08090A0B0C0D;
	expected_hdr.sequence_number = 0xE;
	expected_hdr.preprocessing = CMP_PREPROCESS_NONE;
	expected_hdr.checksum_enabled = 0;
	expected_hdr.encoder_type = CMP_ENCODER_UNCOMPRESSED;
	expected_hdr.model_rate = 0;
	expected_hdr.encoder_param = 0;
	expected_hdr.encoder_outlier = 0;
	for (i = 0; i < CMP_HDR_SIZE + CMP_EXT_HDR_SIZE; i++)
		buf[i] = (uint8_t)i;
	buf[CMP_HDR_SIZE - 1] = 0;

	hdr_size = cmp_hdr_deserialize(buf, sizeof(buf), &hdr);

	TEST_ASSERT_CMP_SUCCESS(hdr_size);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE, hdr_size);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.version_flag, hdr.version_flag);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.version_id, hdr.version_id);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.compressed_size, hdr.compressed_size);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.original_size, hdr.original_size);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.identifier, hdr.identifier);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.sequence_number, hdr.sequence_number);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.preprocessing, hdr.preprocessing);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.checksum_enabled, hdr.checksum_enabled);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.encoder_type, hdr.encoder_type);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.model_rate, hdr.model_rate);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.encoder_param, hdr.encoder_param);
	TEST_ASSERT_EQUAL_HEX(expected_hdr.encoder_outlier, hdr.encoder_outlier);
}


void test_hdr_serialize_detects_when_a_field_is_too_big(void)
{
#define TEST_HDR_FIELD_TOO_BIG(field, bits_for_field, exp_error)    \
	do {                                                        \
		struct cmp_hdr hdr = { 0 };                         \
		uint8_t buf[CMP_HDR_SIZE + CMP_EXT_HDR_SIZE];       \
		uint32_t hdr_size;                                  \
		struct bitstream_writer bs;                         \
		bitstream_writer_init(&bs, buf, sizeof(buf));       \
		TEST_ASSERT(bits_for_field < bitsizeof(long long)); \
		TEST_ASSERT(bits_for_field > 0);                    \
		hdr.preprocessing = CMP_PREPROCESS_DIFF;            \
		hdr.field = 1ULL << bits_for_field;                 \
		hdr_size = cmp_hdr_serialize(&bs, &hdr);            \
		TEST_ASSERT_EQUAL_CMP_ERROR(exp_error, hdr_size);   \
		/* now it should work */                            \
		bitstream_rewind(&bs);                              \
		hdr.field--;                                        \
		hdr_size = cmp_hdr_serialize(&bs, &hdr);            \
		TEST_ASSERT_CMP_SUCCESS(hdr_size);                  \
	} while (0)

	TEST_HDR_FIELD_TOO_BIG(version_id, CMP_HDR_BITS_VERSION_ID, CMP_ERR_INT_BITSTREAM);
	TEST_HDR_FIELD_TOO_BIG(version_flag, CMP_HDR_BITS_VERSION_FLAG, CMP_ERR_INT_BITSTREAM);
	TEST_HDR_FIELD_TOO_BIG(compressed_size, CMP_HDR_BITS_COMPRESSED_SIZE,
			       CMP_ERR_INT_BITSTREAM);
	TEST_HDR_FIELD_TOO_BIG(original_size, CMP_HDR_BITS_ORIGINAL_SIZE, CMP_ERR_SRC_SIZE_WRONG);
	TEST_HDR_FIELD_TOO_BIG(identifier, CMP_HDR_BITS_IDENTIFIER, CMP_ERR_INT_BITSTREAM);
	/* TEST_HDR_FIELD_TOO_BIG(sequence_number, CMP_HDR_BITS_SEQUENCE_NUMBER, */
	/*		       CMP_ERR_INT_BITSTREAM); */
	TEST_HDR_FIELD_TOO_BIG(preprocessing, CMP_HDR_BITS_METHOD_PREPROCESSING,
			       CMP_ERR_INT_BITSTREAM);
	TEST_HDR_FIELD_TOO_BIG(checksum_enabled, CMP_HDR_BITS_METHOD_CHECKSUM_ENABLED,
			       CMP_ERR_INT_BITSTREAM);
	TEST_HDR_FIELD_TOO_BIG(encoder_type, CMP_HDR_BITS_METHOD_ENCODER_TYPE,
			       CMP_ERR_INT_BITSTREAM);

	TEST_HDR_FIELD_TOO_BIG(model_rate, CMP_EXT_HDR_BITS_MODEL_ADAPTATION, CMP_ERR_INT_BITSTREAM);
	TEST_HDR_FIELD_TOO_BIG(encoder_param, CMP_EXT_HDR_BITS_ENCODER_PARAM, CMP_ERR_INT_BITSTREAM);
	/* outlier_par can not be to big  */
	/* TEST_HDR_FIELD_TOO_BIG(encoder_outlier, CMP_EXT_HDR_BITS_ENCODER_OUTLIER, CMP_ERR_INT_BITSTREAM); */
#undef TEST_HDR_FIELD_TOO_BIG
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


void test_detect_null_hdr_during_deserialize(void)
{
	uint8_t buf[CMP_HDR_SIZE] = { 0 };
	uint32_t ret;

	ret = cmp_hdr_deserialize(buf, sizeof(buf), NULL);

	TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_INT_HDR, ret);
}

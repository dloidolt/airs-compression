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
#include "../lib/cmp_header.h"


static const char *cmp_type_name(enum cmp_type dtype)
{
	switch (dtype) {
	case CMP_I16:
		return "CMP_I16";
	case CMP_U16:
		return "CMP_U16";
	case CMP_I16_IN_I32:
		return "CMP_I16_IN_I32";
	default:
		return "unknown data type";
	}
}


static void run_encoder_test(const struct t_fixture *fix, enum cmp_encoder_type type,
			     uint32_t encoder_param, uint32_t encoder_outlier,
			     const int32_t *samples, uint32_t sample_count, const uint8_t *expected,
			     uint32_t expected_size, uint32_t expected_outlier)

{
	struct arena *a = clear_test_arena();
	struct test_src const src = make_test_src(a, fix->dtype, samples, sample_count);
	uint64_t output_buf[5]; /* enough for all tests */
	uint32_t output_size;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	memset(output_buf, 0xFF, sizeof(output_buf));
	params.primary_encoder_type = type;
	params.primary_encoder_param = encoder_param;
	params.primary_encoder_outlier = encoder_outlier;

	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	output_size = fix->compress(&ctx, output_buf, sizeof(output_buf), src.data, src.size);

	TEST_ASSERT_CMP_SUCCESS(output_size);
	TEST_ASSERT_EQUAL_MESSAGE(CMP_HDR_SIZE + expected_size, output_size,
				  cmp_type_name(fix->dtype));
	TEST_ASSERT_EQUAL_HEX8_ARRAY_MESSAGE(expected, cmp_hdr_get_cmp_data(output_buf),
					     expected_size, cmp_type_name(fix->dtype));
	{
		struct cmp_hdr expected_hdr = { 0 };

		expected_hdr.compressed_size = output_size;
		expected_hdr.original_size = src.packed_size;
		expected_hdr.encoder_type = type;
		expected_hdr.encoder_param = encoder_param;
		expected_hdr.encoder_outlier = expected_outlier;
		expected_hdr.original_dtype = fix->dtype;
		TEST_ASSERT_CMP_HDR(output_buf, output_size, expected_hdr);
	}
}


static void run_encoder_test_16(enum cmp_encoder_type type, uint32_t encoder_param,
				uint32_t encoder_outlier, const int32_t *samples,
				uint32_t sample_count, const uint8_t *expected,
				uint32_t expected_size, uint32_t expected_hdr_outlier)

{
	const struct t_fixture *fixs_16[] = { &t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32 };
	size_t i;
	for (i = 0; i < ARRAY_SIZE(fixs_16); i++) {
		run_encoder_test(fixs_16[i], type, encoder_param, encoder_outlier, samples,
				 sample_count, expected, expected_size, expected_hdr_outlier);
	}
}


void test_golomb_zero_param1_encodes_normal_values_16(void)
{
	const uint32_t encoder_param = 1;
	const int32_t data[] = { -8, 7, -1, 0 };
	const uint8_t expected[] = { 0xFF, 0xFF, 0x7F, 0xFF, 0x68 };
	const uint32_t expected_outlier = 16;

	run_encoder_test_16(CMP_ENCODER_GOLOMB_ZERO, encoder_param, 0, data, ARRAY_SIZE(data),
			    expected, sizeof(expected), expected_outlier);
}


void test_golomb_zero_param1_encodes_lowest_outlier_16(void)
{
	const uint32_t encoder_param = 1;
	const int32_t data[] = { 8 };
	const uint8_t expected[] = { 0x00, 0x08, 0x00 };
	const uint32_t expected_outlier = 16;

	run_encoder_test_16(CMP_ENCODER_GOLOMB_ZERO, encoder_param, 0, data, ARRAY_SIZE(data),
			    expected, sizeof(expected), expected_outlier);
}


void test_golomb_zero_param1_encodes_highest_outlier_16(void)
{
	const uint32_t encoder_param = 1;
	const int32_t data[] = { INT16_MIN };
	const uint8_t expected[] = { 0x7F, 0xFF, 0x80 };
	const uint32_t expected_outlier = 16;

	run_encoder_test_16(CMP_ENCODER_GOLOMB_ZERO, encoder_param, 0, data, ARRAY_SIZE(data),
			    expected, sizeof(expected), expected_outlier);
}


void test_golomb_zero_param10_encodes_normal_values_16(void)
{
	const uint32_t encoder_param = 10;
	const int32_t data[] = { 82, 4, 0 };
	const uint8_t expected[] = { 0xFF, 0xFF, 0x57, 0x88 };
	const uint32_t expected_outlier = 165;

	run_encoder_test_16(CMP_ENCODER_GOLOMB_ZERO, encoder_param, 0, data, ARRAY_SIZE(data),
			    expected, sizeof(expected), expected_outlier);
}


void test_golomb_zero_param10_encodes_lowest_outlier_16(void)
{
	const uint32_t encoder_param = 10;
	const int32_t data[] = { -83 };
	const uint8_t expected[] = { 0x00, 0x0A, 0x50 };
	const uint32_t expected_outlier = 165;

	run_encoder_test_16(CMP_ENCODER_GOLOMB_ZERO, encoder_param, 0, data, ARRAY_SIZE(data),
			    expected, sizeof(expected), expected_outlier);
}


void test_golomb_zero_param10_encodes_highest_outlier_16(void)
{
	const uint32_t encoder_param = 10;
	const int32_t data[] = { INT16_MIN };
	const uint8_t expected[] = { 0x0F, 0xFF, 0xF0 };
	const uint32_t expected_outlier = 165;

	run_encoder_test_16(CMP_ENCODER_GOLOMB_ZERO, encoder_param, 0, data, ARRAY_SIZE(data),
			    expected, sizeof(expected), expected_outlier);
}


void test_golomb_zero_param_max_encodes_normal_values_16(void)
{
	/* with this encoder_param/g_par we can encode all values, no outlier encoding */
	const uint32_t encoder_param = UINT16_MAX;
	const int32_t data[] = { 0, INT16_MIN };
	const uint8_t expected[] = { 0x00, 0x01, 0x40, 0x00, 0x40 };
	const uint32_t expected_outlier = 0xFFFF0;

	run_encoder_test_16(CMP_ENCODER_GOLOMB_ZERO, encoder_param, 0, data, ARRAY_SIZE(data),
			    expected, sizeof(expected), expected_outlier);
}


void test_golomb_multi_param1_encodes_normal_values(void)
{
	const uint32_t encoder_param = 1;
	const uint32_t encoder_outlier = 5;
	const int32_t data[] = { 0, 2 };
	const uint8_t expected[] = { 0x78 };

	run_encoder_test_16(CMP_ENCODER_GOLOMB_MULTI, encoder_param, encoder_outlier, data,
			    ARRAY_SIZE(data), expected, sizeof(expected), encoder_outlier);
}


void test_golomb_multi_encodes_2bits_outliers(void)
{
	const uint32_t encoder_param = 1;
	const uint32_t encoder_outlier = 5;
	const int32_t data[] = { -3, 3, -4, 4 };
	const uint8_t expected[] = { 0xF8, 0xF9, 0xFA, 0xFB };

	run_encoder_test_16(CMP_ENCODER_GOLOMB_MULTI, encoder_param, encoder_outlier, data,
			    ARRAY_SIZE(data), expected, sizeof(expected), encoder_outlier);
}


void test_golomb_multi_encodes_4bits_outliers(void)
{
	const uint32_t encoder_param = 1;
	const uint32_t encoder_outlier = 5;
	const int32_t data[] = { -5, 10 };
	const uint8_t expected[] = { 0xFC, 0x9F, 0xBC };

	run_encoder_test_16(CMP_ENCODER_GOLOMB_MULTI, encoder_param, encoder_outlier, data,
			    ARRAY_SIZE(data), expected, sizeof(expected), encoder_outlier);
}


void test_golomb_multi_encodes_largest_16bits_outliers(void)
{
	const uint32_t encoder_param = 1;
	const uint32_t encoder_outlier = 5;
	const int32_t data[] = { INT16_MIN };
	const uint8_t expected[] = { 0xFF, 0xF7, 0xFF, 0xD0 };

	run_encoder_test_16(CMP_ENCODER_GOLOMB_MULTI, encoder_param, encoder_outlier, data,
			    ARRAY_SIZE(data), expected, sizeof(expected), encoder_outlier);
}


void test_golomb_multi_param1_clamps_outlier_at_max_normal_value(void)
{
	const uint32_t encoder_param = 1;
	const uint32_t encoder_outlier = 42;
	const int32_t data[] = { -12 };
	const uint8_t expected[] = { 0xFF, 0xFF, 0xFE };
	const uint32_t expected_outlier = 24;

	run_encoder_test_16(CMP_ENCODER_GOLOMB_MULTI, encoder_param, encoder_outlier, data,
			    ARRAY_SIZE(data), expected, sizeof(expected), expected_outlier);
}


void test_golomb_multi_param1_clamps_outlier_at_minimum_outlier_value(void)
{
	const uint32_t encoder_param = 1;
	const uint32_t encoder_outlier = 42;
	const int32_t data[] = { 12 };
	const uint8_t expected[] = { 0xFF, 0xFF, 0xFF, 0x00 };
	const uint32_t expected_outlier = 24;

	run_encoder_test_16(CMP_ENCODER_GOLOMB_MULTI, encoder_param, encoder_outlier, data,
			    ARRAY_SIZE(data), expected, sizeof(expected), expected_outlier);
}


void test_golomb_multi_param1_clamps_outlier_at_max_16_bit_outlier_value(void)
{
	const uint32_t encoder_param = 1;
	const uint32_t encoder_outlier = 42;
	const int32_t data[] = { INT16_MIN };
	const uint8_t expected[] = { 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xE7 };
	const uint32_t expected_outlier = 24;

	run_encoder_test_16(CMP_ENCODER_GOLOMB_MULTI, encoder_param, encoder_outlier, data,
			    ARRAY_SIZE(data), expected, sizeof(expected), expected_outlier);
}


void test_golomb_multi_param_max_encodes_zero_value(void)
{
	const uint32_t encoder_param = UINT16_MAX;
	const uint32_t encoder_outlier = UINT32_MAX;
	const int32_t data[] = { 0 };
	const uint8_t expected[] = { 0x00, 0x00 };
	const uint32_t expected_outlier = 0xFFFE9;

	run_encoder_test_16(CMP_ENCODER_GOLOMB_MULTI, encoder_param, encoder_outlier, data,
			    ARRAY_SIZE(data), expected, sizeof(expected), expected_outlier);
}


void test_golomb_multi_param_max_encodes_largest_16_bit_value(void)
{
	const uint32_t encoder_param = UINT16_MAX;
	const uint32_t encoder_outlier = UINT32_MAX;
	const int32_t data[] = { INT16_MIN };
	const uint8_t expected[] = { 0x80, 0x00, 0x00 };
	const uint32_t expected_outlier = 0xFFFE9;

	run_encoder_test_16(CMP_ENCODER_GOLOMB_MULTI, encoder_param, encoder_outlier, data,
			    ARRAY_SIZE(data), expected, sizeof(expected), expected_outlier);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_uncompressed_primary_then_compressed_secondary(const struct t_fixture *fix)
{
	const int32_t data_primary[] = { 0x0FF, 0x000, 0X0AB };
	const int32_t data_secondary[] = { 82, 4, 0 };
	const uint8_t expected_pri16[] = { 0, 0xFF, 0, 0, 0, 0xAB };
	const uint8_t expected_sec16[] = { 0xFF, 0XFF, 0x57, 0x88 };
	struct arena *a = clear_test_arena();
	struct test_src const src_pri =
		make_test_src(a, fix->dtype, data_primary, ARRAY_SIZE(data_primary));
	struct test_src const src_sec =
		make_test_src(a, fix->dtype, data_secondary, ARRAY_SIZE(data_secondary));
	DST_ALIGNED_U8 dst_buf_pri[CMP_HDR_SIZE + sizeof(expected_pri16)];
	DST_ALIGNED_U8 dst_buf_sec[CMP_HDR_SIZE + sizeof(expected_sec16)];
	const void *exp_data;
	uint32_t exp_size;
	uint32_t dst_size_pri, dst_size_sec;
	struct cmp_context ctx;
	struct cmp_params params = { 0 };
	struct cmp_hdr expected_hdr = { 0 };

	params.primary_encoder_type = CMP_ENCODER_UNCOMPRESSED;
	params.secondary_iterations = 1;
	params.secondary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.secondary_encoder_param = 10;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	dst_size_pri =
		fix->compress(&ctx, dst_buf_pri, sizeof(dst_buf_pri), src_pri.data, src_pri.size);
	dst_size_sec =
		fix->compress(&ctx, dst_buf_sec, sizeof(dst_buf_sec), src_sec.data, src_sec.size);

	/* 1st pass */
	TEST_ASSERT_CMP_SUCCESS(dst_size_pri);
	exp_data = expected_pri16;
	exp_size = sizeof(expected_pri16);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + exp_size, dst_size_pri);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(exp_data, cmp_hdr_get_cmp_data(dst_buf_pri), exp_size);
	expected_hdr.original_size = src_pri.packed_size;
	expected_hdr.compressed_size = CMP_HDR_SIZE + exp_size;
	expected_hdr.encoder_type = CMP_ENCODER_UNCOMPRESSED;
	expected_hdr.original_dtype = fix->dtype;
	expected_hdr.preprocess_param = params.secondary_iterations;
	TEST_ASSERT_CMP_HDR(dst_buf_pri, dst_size_pri, expected_hdr);
	/* 2nd pass */
	TEST_ASSERT_CMP_SUCCESS(dst_size_sec);
	exp_data = expected_sec16;
	exp_size = sizeof(expected_sec16);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + exp_size, dst_size_sec);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(exp_data, cmp_hdr_get_cmp_data(dst_buf_sec), exp_size);
	expected_hdr.sequence_number = 1;
	expected_hdr.compressed_size = CMP_HDR_SIZE + exp_size;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 10;
	expected_hdr.encoder_outlier = 165;
	TEST_ASSERT_CMP_HDR(dst_buf_sec, dst_size_sec, expected_hdr);
}


TEST_MATRIX([&t_fix_u16, &t_fix_i16, &t_fix_i16_in_i32])
void test_use_secondary_encoder_for_second_pass(const struct t_fixture *fix)
{
	const int32_t data_primary[] = { 0, 2 };
	const uint8_t expected_primary[] = { 0x78 };
	const int32_t data_secondary[] = { 1, 3, 4 };
	const uint8_t expected_secondary[] = { 0x36, 0xBC };
	struct arena *a = clear_test_arena();
	struct test_src const src_pri =
		make_test_src(a, fix->dtype, data_primary, ARRAY_SIZE(data_primary));
	struct test_src const src_sec =
		make_test_src(a, fix->dtype, data_secondary, ARRAY_SIZE(data_secondary));
	DST_ALIGNED_U8 dst_buf_pri[CMP_HDR_SIZE + sizeof(expected_primary)];
	DST_ALIGNED_U8 dst_buf_sec[CMP_HDR_SIZE + sizeof(expected_secondary)];
	uint32_t dst_size_pri, dst_size_sec;
	struct cmp_context ctx;
	struct cmp_hdr expected_hdr = { 0 };
	struct cmp_params params = { 0 };

	params.primary_encoder_type = CMP_ENCODER_GOLOMB_MULTI;
	params.primary_encoder_param = 1;
	params.primary_encoder_outlier = 23;
	params.secondary_iterations = 1;
	params.secondary_encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	params.secondary_encoder_param = 10;
	TEST_ASSERT_CMP_SUCCESS(cmp_initialise(&ctx, &params, NULL, 0));

	dst_size_pri =
		fix->compress(&ctx, dst_buf_pri, sizeof(dst_buf_pri), src_pri.data, src_pri.size);
	dst_size_sec =
		fix->compress(&ctx, dst_buf_sec, sizeof(dst_buf_sec), src_sec.data, src_sec.size);

	/* 1st pass */
	TEST_ASSERT_CMP_SUCCESS(dst_size_pri);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_primary), dst_size_pri);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_primary, cmp_hdr_get_cmp_data(dst_buf_pri),
				     sizeof(expected_primary));

	expected_hdr.compressed_size = CMP_HDR_SIZE + sizeof(expected_primary);
	expected_hdr.original_size = src_pri.packed_size;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_MULTI;
	expected_hdr.encoder_param = 1;
	expected_hdr.encoder_outlier = 23;
	expected_hdr.original_dtype = fix->dtype;
	expected_hdr.preprocess_param = params.secondary_iterations;
	TEST_ASSERT_CMP_HDR(dst_buf_pri, dst_size_pri, expected_hdr);
	/* 2nd pass */
	TEST_ASSERT_CMP_SUCCESS(dst_size_sec);
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE + sizeof(expected_secondary), dst_size_sec);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_secondary, cmp_hdr_get_cmp_data(dst_buf_sec),
				     ARRAY_SIZE(expected_secondary));
	expected_hdr.sequence_number = 1;
	expected_hdr.compressed_size = CMP_HDR_SIZE + sizeof(expected_secondary);
	expected_hdr.original_size = src_sec.packed_size;
	expected_hdr.encoder_type = CMP_ENCODER_GOLOMB_ZERO;
	expected_hdr.encoder_param = 10;
	expected_hdr.encoder_outlier = 165;
	TEST_ASSERT_CMP_HDR(dst_buf_sec, dst_size_sec, expected_hdr);
}

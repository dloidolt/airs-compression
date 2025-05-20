/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Utilities for testing the compression library
 */


#include <unity.h>
#include "../lib/cmp_errors.h"


void assert_equal_cmp_error_internal(enum cmp_error expected_error, uint32_t cmp_ret_code,
				     int line);


/**
 * @brief Validates successful compression function call
 *
 * @param cmp_ret_code	return code from compression library function
 */
#define TEST_ASSERT_CMP_SUCCESS(cmp_ret_code)                            \
	do {                                                             \
		uint32_t _cmp_ret = (cmp_ret_code);                      \
		TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_NO_ERROR, _cmp_ret); \
	} while (0)


/**
 * @brief Checks compression function failure
 *
 * @param cmp_ret_code	return code from compression library function
 */
#define TEST_ASSERT_CMP_FAILURE(cmp_ret_code) TEST_ASSERT_TRUE(cmp_is_error(cmp_ret_code))


/**
 * @brief Asserts compression error code equality
 *
 * @param expected_CMP_ERROR	expected error code
 * @param cmp_ret_code		compression library return code
 */

#define TEST_ASSERT_EQUAL_CMP_ERROR(expected_CMP_ERROR, cmp_ret_code) \
	assert_equal_cmp_error_internal(expected_CMP_ERROR, cmp_ret_code, __LINE__)


/**
 * @brief Asserts that a compressed data header matches the expected header
 *
 * The model_id is ignored.
 *
 * @param compressed_data	pointer to the compressed data buffer
 * @param size			size of the compressed data buffer
 * @param expected_hdr		constant pointer to the expected cmp_hdr structure
 */

#define TEST_ASSERT_CMP_HDR(compressed_data, size, expected_hdr)                                   \
	do {                                                                                       \
		struct cmp_hdr assert_hdr;                                                         \
		TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(compressed_data, size, &assert_hdr));  \
		expected_hdr.version_flag = 1;		      /* always expected */                \
		expected_hdr.version_id = CMP_VERSION_NUMBER; /* always expected */                \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.version_flag, assert_hdr.version_flag,      \
					  "Version cmp lib flag mismatch");                        \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.version_id, assert_hdr.version_id,          \
					  "header version ID mismatch");                           \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.compressed_size,                            \
					  assert_hdr.compressed_size,                              \
					  "header compressed data size mismatch");                 \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.original_size, assert_hdr.original_size,    \
					  "header original size mismatch");                        \
		assert_hdr.identifier = expected_hdr.identifier; /* ignore this field */           \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.sequence_number,                            \
					  assert_hdr.sequence_number,                              \
					  "header sequence number mismatch");                      \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.preprocessing, assert_hdr.preprocessing,    \
					  "header preprocessing mismatch");                        \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.checksum_enabled,                           \
					  assert_hdr.checksum_enabled,                             \
					  "Checksum enable mismatch");                             \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.encoder_type, assert_hdr.encoder_type,      \
					  "header encoder mismatch");                              \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.model_rate, assert_hdr.model_rate,          \
					  "header model adaptation rate mismatch");                \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.encoder_param, assert_hdr.encoder_param,    \
					  "header encoder parameter mismatch");                    \
		TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.encoder_outlier,                            \
					  assert_hdr.encoder_outlier,                              \
					  "header outlier parameter mismatch");                    \
		TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&expected_hdr, &assert_hdr, sizeof(expected_hdr), \
						 "header mismatch");                               \
	} while (0)


/**
 * @brief retrieve pointer to compressed data following the header
 *
 * @warning Assumes the compressed data block starts with a valid header.
 *
 * @param cmp_data	pointer to the start of the compressed data (header)
 *
 * @returns a pointer to the first byte of compressed data after the header
 *
 */

void *cmp_hdr_get_cmp_data(void *cmp_data);

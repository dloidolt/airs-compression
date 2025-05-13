/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Utilities for testing the compression library
 */

#include <stdio.h>

#include <unity.h>
#include <unity_internals.h>


#include "../lib/cmp_errors.h"



/**
 * @brief Validates successful compression function call
 *
 * @param cmp_ret_code	return code from compression library function
 */
#define TEST_ASSERT_CMP_SUCCESS(cmp_ret_code)				\
	do {								\
		uint32_t _cmp_ret = (cmp_ret_code);			\
		TEST_ASSERT_EQUAL_CMP_ERROR(CMP_ERR_NO_ERROR, _cmp_ret);\
	} while (0)


/**
 * @brief Checks compression function failure
 *
 * @param cmp_ret_code	return code from compression library function
 */
#define TEST_ASSERT_CMP_FAILURE(cmp_ret_code) TEST_ASSERT_TRUE(cmp_is_error(cmp_ret_code))


/**
 * @brief Converts compression error enum to string
 *
 * @param error		Compression error code
 *
 * @returns error code string
 */

static __inline const char *cmp_error_enum_to_str(enum cmp_error error)
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
	case CMP_ERR_DST_UNALIGNED:
		return "CMP_ERR_DST_UNALIGNED";
	case CMP_ERR_SRC_NULL:
		return "CMP_ERR_SRC_NULL";
	case CMP_ERR_SRC_SIZE_WRONG:
		return "CMP_ERR_SRC_SIZE_WRONG";
	case CMP_ERR_DST_TOO_SMALL:
		return "CMP_ERR_DST_TOO_SMALL";
	case CMP_ERR_SRC_SIZE_MISMATCH:
		return "CMP_ERR_SRC_SIZE_MISMATCH";
	case CMP_ERR_TIMESTAMP_INVALID:
		return "CMP_ERR_TIMESTAMP_INVALID";
	case CMP_ERR_INT_HDR:
		return "CMP_ERR_INT_HDR";
	case CMP_ERR_INT_ENCODER:
		return "CMP_ERR_INT_ENCODER";
	case CMP_ERR_INT_BITSTREAM:
		return "CMP_ERR_INT_BITSTREAM";
	case CMP_ERR_MAX_CODE:
	default:
		TEST_FAIL_MESSAGE("Missing error name");
		return "Unknown error";
	}
}


/**
 * @brief Generates a descriptive error message
 *
 * @param expected	expected error code
 * @param actual	actual error code
 *
 * @returns formatted error message string
 */

static __inline const char *gen_cmp_error_message(enum cmp_error expected,
						  enum cmp_error actual)
{
	enum{ CMP_TEST_MESSAGE_BUF_SIZE = 128 };
	static char message[CMP_TEST_MESSAGE_BUF_SIZE];

	snprintf(message, sizeof(message), "Expected %s Was %s.",
		 cmp_error_enum_to_str(expected), cmp_error_enum_to_str(actual));
	return message;
}


/**
 * @brief Asserts compression error code equality
 *
 * @param expected_CMP_ERROR	expected error code
 * @param cmp_ret_code		compression library return code
 */

#define TEST_ASSERT_EQUAL_CMP_ERROR(expected_CMP_ERROR, cmp_ret_code) \
  assert_equal_cmp_error_internal(expected_CMP_ERROR, cmp_ret_code, __LINE__)
static __inline void assert_equal_cmp_error_internal(enum cmp_error expected_error,
					    uint32_t cmp_ret_code, int line)
{
	enum cmp_error actual_error = cmp_get_error_code(cmp_ret_code);
	const char* message = gen_cmp_error_message(expected_error, actual_error);

	UNITY_TEST_ASSERT_EQUAL_INT(expected_error, actual_error, line,
				    message);
}


/**
 * @brief Asserts that a compressed data header matches the expected header
 *
 * The model_id is ignored.
 *
 * @param compressed_data	pointer to the compressed data buffer
 * @param size			size of the compressed data buffer
 * @param expected_hdr		constant pointer to the expected cmp_hdr structure
 */

#define TEST_ASSERT_CMP_HDR(compressed_data, size, expected_hdr)							\
do {															\
	struct cmp_hdr assert_hdr;											\
	TEST_ASSERT_CMP_SUCCESS(cmp_hdr_deserialize(compressed_data, size, &assert_hdr));				\
	TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.version, assert_hdr.version, "header version mismatch");			\
	TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.cmp_size, assert_hdr.cmp_size, "header compressed data size mismatch");	\
	TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.original_size, assert_hdr.original_size, "header original size mismatch");\
	TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.mode, assert_hdr.mode, "header mode mismatch");				\
	TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.preprocess, assert_hdr.preprocess, "header preprocessing mismatch");	\
	TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.model_rate, assert_hdr.model_rate, "model rate mismatch");		\
	TEST_ASSERT_EQUAL_MESSAGE(expected_hdr.pass_count, assert_hdr.pass_count, "pass counter mismatch");		\
	assert_hdr.model_id = expected_hdr.model_id; /* ignore to check model id*/					\
	TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&expected_hdr, &assert_hdr, sizeof(expected_hdr), "header mismatch");		\
} while (0)

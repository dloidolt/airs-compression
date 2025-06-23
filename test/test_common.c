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


#include "test_common.h"
#include "../lib/cmp_errors.h"
#include "../lib/common/header_private.h"


void *cmp_hdr_get_cmp_data(void *header)
{
	struct cmp_hdr hdr;
	uint32_t hdr_size;

	TEST_ASSERT_NOT_NULL(header);
	hdr_size = cmp_hdr_deserialize(header, CMP_HDR_MAX_SIZE, &hdr);
	TEST_ASSERT_CMP_SUCCESS(hdr_size);
	return (uint8_t *)header + hdr_size;
}


/**
 * @brief Converts compression error enum to string
 *
 * @param error		Compression error code
 *
 * @returns error code string
 */

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
	case CMP_ERR_WORK_BUF_UNALIGNED:
		return "CMP_ERR_WORK_BUF_UNALIGNED";
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
	case CMP_ERR_HDR_CMP_SIZE_TOO_LARGE:
		return "CMP_ERR_HDR_CMP_SIZE_TOO_LARGE";
	case CMP_ERR_HDR_ORIGINAL_TOO_LARGE:
		return "CMP_ERR_HDR_ORIGINAL_TOO_LARGE";
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

static const char *gen_cmp_error_message(enum cmp_error expected, enum cmp_error actual)
{
	enum { CMP_TEST_MESSAGE_BUF_SIZE = 128 };
	static char message[CMP_TEST_MESSAGE_BUF_SIZE];

	snprintf(message, sizeof(message), "Expected %s Was %s.", cmp_error_enum_to_str(expected),
		 cmp_error_enum_to_str(actual));
	return message;
}


/**
 * @brief Asserts compression error code equality
 *
 * @param expected_error	expected error code
 * @param cmp_ret_code		compression library return code
 */

void assert_equal_cmp_error_internal(enum cmp_error expected_error, uint32_t cmp_ret_code, int line)
{
	enum cmp_error actual_error = cmp_get_error_code(cmp_ret_code);
	const char *message = gen_cmp_error_message(expected_error, actual_error);

	UNITY_TEST_ASSERT_EQUAL_INT(expected_error, actual_error, line, message);
}

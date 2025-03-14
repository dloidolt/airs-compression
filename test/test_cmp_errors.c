/**
 * @file   test_cmp_errors.c
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
 * @brief data compression error tests
 */

#include <unity.h>

#include "../lib/cmp.h"
#include "../lib/cmp_errors.h"
#include "../lib/common/err_private.h"


void test_cmp_is_error(void)
{
	TEST_ASSERT_FALSE(cmp_is_error(0));
	TEST_ASSERT_FALSE(cmp_is_error(CMP_ERROR(MAX_CODE))); /* This error code is not used for a error */

	TEST_ASSERT_TRUE(cmp_is_error(CMP_ERROR(MAX_CODE)+1));
	TEST_ASSERT_TRUE(cmp_is_error(-1U));
}


void test_cmp_get_error_code(void)
{
	TEST_ASSERT_EQUAL(CMP_ERR_NO_ERROR, cmp_get_error_code(0));
	TEST_ASSERT_EQUAL(CMP_ERR_NO_ERROR, cmp_get_error_code(CMP_ERROR(MAX_CODE)));
	TEST_ASSERT_EQUAL(CMP_ERR_GENERIC, cmp_get_error_code(CMP_ERROR(GENERIC)));
}

/**
 * @file   test_header.c
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
 * @brief data compression header tests
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <unity.h>

/* #include <header.h> */
#include <cmp_errors.h>
#include <cmp.h>


void test_serialize_header(void)
{
	struct cmp_hdr hdr = {0};
	uint8_t buf[CMP_HDR_SIZE];
	uint32_t hdr_size;
	int i;

	memset(buf, 0xAB, sizeof(buf));
	hdr.version = 0x0001;
	hdr.cmp_size = 0x020304;
	hdr.original_size = 0x050607;

	hdr_size = cmp_hdr_serialize(buf, sizeof(buf), &hdr);

	TEST_ASSERT_FALSE(cmp_is_error(hdr_size));
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE, hdr_size);
	for (i = 0; i < CMP_HDR_SIZE; i++)
		TEST_ASSERT_EQUAL_HEX8(i, buf[i]);
}


void test_deserialize_header(void)
{
	struct cmp_hdr hdr = {0};
	uint8_t buf[CMP_HDR_SIZE];
	uint32_t hdr_size;
	int i;

	for (i = 0; i < CMP_HDR_SIZE; i++)
		buf[i] = (uint8_t)i;


	hdr_size = cmp_hdr_deserialize(buf, sizeof(buf), &hdr);

	TEST_ASSERT_FALSE(cmp_is_error(hdr_size));
	TEST_ASSERT_EQUAL(CMP_HDR_SIZE, hdr_size);
	TEST_ASSERT_EQUAL(hdr.version, 0x0001);
}


void test_hdr_serialize_returns_error_when_parmertes_are_to_big(void)
{
	struct cmp_hdr hdr = {0};
	uint8_t buf[CMP_HDR_SIZE];
	uint32_t hdr_size;

	hdr.version = UINT16_MAX+1;

	hdr_size = cmp_hdr_serialize(buf, sizeof(buf), &hdr);

	TEST_ASSERT_EQUAL(CMP_ERR_INT_HDR, cmp_get_error_code(hdr_size));

	hdr.version = UINT16_MAX;
	hdr.cmp_size = MAX_CMP_SIZE+1;

	hdr_size = cmp_hdr_serialize(buf, sizeof(buf), &hdr);

	TEST_ASSERT_EQUAL(CMP_ERR_INT_HDR, cmp_get_error_code(hdr_size));

	hdr.cmp_size = MAX_CMP_SIZE;
	hdr.original_size = MAX_ORIGINAL_SIZE+1;

	hdr_size = cmp_hdr_serialize(buf, sizeof(buf), &hdr);

	TEST_ASSERT_EQUAL(CMP_ERR_INT_HDR, cmp_get_error_code(hdr_size));
}

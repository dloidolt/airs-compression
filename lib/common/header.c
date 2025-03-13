/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Compression header implementation
 */


#include <stdint.h>
#include <string.h>

#include "header.h"
#include "err_private.h"
#include "cmp_errors.h"

/** maximum value for a 24-bit unsigned integer */
#define CMP_UINT24_MAX 0xFFFFFF


void *cmp_hdr_get_cmp_data(void *header)
{
	return (uint8_t *)header + CMP_HDR_SIZE;
}


static uint32_t serialize_u24(uint8_t *dst, uint32_t value)
{
	if (value > CMP_UINT24_MAX)
		return CMP_ERROR(INT_HDR);

	dst[0] = (uint8_t)(value >> 16);
	dst[1] = (uint8_t)((value >> 8) & 0xFF);
	dst[2] = (uint8_t)(value & 0xFF);

	return 3;
}


static uint32_t serialize_u16(uint8_t *dst, uint32_t value)
{
	if (value > UINT16_MAX)
		return CMP_ERROR(INT_HDR);

	dst[0] = (uint8_t)(value >> 8);
	dst[1] = (uint8_t)(value & 0xFF);

	return 2;
}


uint32_t cmp_hdr_serialize(void *dst, uint32_t dst_size, const struct cmp_hdr *hdr)
{
	uint8_t *dst8 = dst;
	uint32_t s;
	(void)dst_size;
	/* CMP_ASSERT(hdr != NULL) */
	/* if (!hdr) */
	/*	return CMP_HDR_SIZE; */

	s = serialize_u16(dst8, hdr->version);
	if (cmp_is_error_int(s))
		return s;
	dst8 += s;

	s = serialize_u24(dst8, hdr->cmp_size);
	if (cmp_is_error_int(s))
		return s;
	dst8 += s;

	s = serialize_u24(dst8, hdr->original_size);
	if (cmp_is_error_int(s))
		return s;

	return CMP_HDR_SIZE;
}


/* no size checks */
static const uint8_t *deserialize_u16(const uint8_t *pos, uint32_t *read_value)
{
	/* CMP_ASSERT(read_value != NULL); */

	*read_value = (uint16_t)(pos[0] << 8);
	*read_value |= pos[1];

	return pos + 2;
}


/* no size checks */
static const uint8_t *deserialize_u24(const uint8_t *pos, uint32_t *read_value)
{
	/* CMP_ASSERT(read_value != NULL); */

	*read_value = (uint32_t)pos[0] << 16;
	*read_value |= (uint32_t)pos[1] << 8;
	*read_value |= pos[2];

	return pos + 3;
}


uint32_t cmp_hdr_deserialize(const void *src, uint32_t src_size, struct cmp_hdr *hdr)
{
	/* CMP_ASSERT(hdr != NULL) */
	const uint8_t *pos = src;
	(void)src_size;

	/* if (hdr == NULL) */
	/*	return */
	/* if (src == NULL) */
	/*	return */
	/* if (src_size < CMP_HDR_SIZE) */
	/*	return */
	memset(hdr, 0x00, sizeof(*hdr));

	pos = deserialize_u16(pos, &hdr->version);
	pos = deserialize_u24(pos, &hdr->cmp_size);
	deserialize_u24(pos, &hdr->original_size);

	return CMP_HDR_SIZE;
}

/**
 * @file   cmp.c
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
 * @brief AIRS data compression implementation
 */

#include <stdint.h>
#include <string.h>

#include "../cmp.h"
#include "../common/err_private.h"
#include "../common/header.h"


unsigned int cmp_is_error(uint32_t code)
{
	return cmp_is_error_int(code);
}


uint32_t cmp_compress_bound(uint32_t src_size)
{
	uint32_t const bound = CMP_HDR_SIZE + (src_size + 1) / 2 * 2; /* round up to next multiple of 2 */

	/* Check overflow */
	if (bound <= src_size)
		return (CMP_ERROR(SRC_SIZE_WRONG));

	if (bound > CMP_MAX_CMP_SIZE)
		return (CMP_ERROR(SRC_SIZE_WRONG));

	return bound;
}


uint32_t cmp_cal_work_buf_size(const struct cmp_params *params, uint32_t src_size)
{
	/* TODO: implement this */
	(void)params;
	(void)src_size; /* TODO: what if not a multiple of sizeof(uint16_t) */
	return 0;
}


uint32_t cmp_initialise(struct cmp_context *ctx, const struct cmp_params *params,
			void *work_buf, uint32_t work_buf_size)
{
	(void)work_buf;
	(void)work_buf_size;

	if (ctx == NULL)
		return CMP_ERROR(CONTEXT_INVALID);

	cmp_deinitialise(ctx);

	if (params == NULL)
		return CMP_ERROR(PARAMS_INVALID);

	return CMP_ERROR(NO_ERROR);
}


uint32_t cmp_compress_u16(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
			  const uint16_t *src, uint32_t src_size)
{
	uint32_t i;
	uint32_t cmp_size = CMP_HDR_SIZE;

	if (ctx == NULL)
		return CMP_ERROR(CONTEXT_INVALID);
	if (dst == NULL)
		return CMP_ERROR(DST_NULL);
	if (src == NULL)
		return CMP_ERROR(SRC_NULL);
	if (src_size == 0)
		return CMP_ERROR(SRC_SIZE_WRONG);
	if (src_size % sizeof(*src) != 0)
		return CMP_ERROR(SRC_SIZE_WRONG);
	if (src_size > CMP_MAX_ORIGINAL_SIZE)
		return CMP_ERROR(SRC_SIZE_WRONG);

	cmp_size += src_size; /* TODO: overflow */
	if (cmp_size > dst_capacity)
		return CMP_ERROR(DST_TOO_SMALL);

	/* Copy the src buffer to the destination in big-endian */
	for (i = 0; i < src_size/sizeof(*src); i++) {
		uint8_t *p = (uint8_t *)dst + CMP_HDR_SIZE;

		p[i*2]   = (uint8_t)(src[i] >> 8);
		p[i*2+1] = (uint8_t)(src[i] & 0xFF);
	}

	{ /* Build header */
		struct cmp_hdr hdr = {0};
		uint32_t return_val;

		hdr.version = CMP_VERSION_NUMBER;
		hdr.cmp_size = cmp_size;
		hdr.original_size = src_size;
		return_val = cmp_hdr_serialize(dst, dst_capacity, &hdr);
		if (cmp_is_error_int(return_val))
			return return_val;
	}

	return cmp_size;
}


uint32_t cmp_reset(struct cmp_context *ctx)
{
	if (ctx == NULL)
		return CMP_ERROR(CONTEXT_INVALID);

	return CMP_ERROR(NO_ERROR);
}


void cmp_deinitialise(struct cmp_context *ctx)
{
	if (ctx)
		memset(ctx, 0, sizeof(*ctx));
}

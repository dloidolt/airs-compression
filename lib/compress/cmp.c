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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../cmp.h"
#include "../common/err_private.h"
#include "../common/header.h"


uint32_t cmp_compress_bound(uint32_t num_bufs, uint32_t buf_size)
{
	return CMP_HDR_SIZE + num_bufs * buf_size;
}


uint32_t cmp_cal_work_buf_size(uint32_t max_addition_size)
{
	return max_addition_size; /* TODO: what if not a multiple of sizeof(uint16_t) */
}


uint32_t cmp_compress16(void *dst, uint32_t dst_capacity,
			const uint16_t *src_bufs[],
			uint32_t num_src_bufs, uint32_t src_buf_size,
			const struct cmp_params *params,
			void *work_buf, uint32_t work_buf_size)
{
	size_t i;
	struct cmp_context ctx;
	uint32_t cmp_size = cmp_initialise(&ctx, dst, dst_capacity, params,
					   work_buf, work_buf_size);
	if (cmp_is_error(cmp_size))
		return cmp_size;

	for (i = 0; i < num_src_bufs; i++) {
		cmp_size = cmp_feed16(&ctx, src_bufs[i] ,src_buf_size);
		if (cmp_is_error(cmp_size))
			return cmp_size;
	}

	return cmp_size;
}


uint32_t cmp_initialise(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
			const struct cmp_params *params,
			void *work_buf, uint32_t work_buf_size)
{
	(void)work_buf;
	(void)work_buf_size;
	(void)dst_capacity;

	if (ctx == NULL)
		return CMP_ERROR(NO_CONTEXT);

	cmp_deinitialise(ctx);

	if (params == NULL)
		return CMP_ERROR(NO_PARAMS);

	/* if (dst == NULL) */
		/* return */
	/* if (dst_capacity < CMP_HDR_SIZE) */
		/* return */

	ctx->dst = dst;

	return cmp_reset(ctx);
}


uint32_t cmp_feed16(struct cmp_context *ctx, const uint16_t *src, uint32_t size)
{
	size_t i;

	if (ctx == NULL)
		return CMP_ERROR(NO_CONTEXT);
	if (src == NULL)
		return CMP_ERROR(NO_SRC_DATA);
	if (size % sizeof(*src) != 0)
		return CMP_ERROR(INVALID_SRC_SIZE);
	if (size == 0)
		return CMP_ERROR(INVALID_SRC_SIZE);

	for (i = 0; i < size/sizeof(*src); i++) {
		uint8_t *p = (uint8_t *)ctx->dst + ctx->hdr.cmp_size; /* TODO: overflow */

		p[i*2]   = (uint8_t)(src[i] >> 8);
		p[i*2+1] = (uint8_t)(src[i] & 0xFF);
	}

	ctx->hdr.cmp_size += size; /* TODO: overflow */
	ctx->hdr.original_size += size; /* TODO: overflow */
	cmp_hdr_serialize(ctx->dst, ctx->hdr.cmp_size, &ctx->hdr);
	/* if (cmp_is_error(s)) */
	/* 	return */

	return ctx->hdr.cmp_size;
}


uint32_t cmp_reset(struct cmp_context *ctx)
{
	/* if (!ctx) */
	/* 	return CMP_ERROR(NO_CONTEXT); */

	ctx->hdr.version = CMP_VERSION_NUMBER;
	ctx->hdr.cmp_size = CMP_HDR_SIZE;
	ctx->hdr.original_size = 0;
	cmp_hdr_serialize(ctx->dst, ctx->hdr.cmp_size, &ctx->hdr);
	/* if (cmp_is_error(s)) */
	/* 	return */

	return CMP_HDR_SIZE;
}


void cmp_deinitialise(struct cmp_context *ctx)
{
	if (ctx)
		memset(ctx, 0, sizeof(*ctx));
}

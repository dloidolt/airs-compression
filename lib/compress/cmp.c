/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data compression implementation
 */

#include <stdint.h>
#include <string.h>

#include "preprocess.h"
#include "../cmp.h"
#include "../common/err_private.h"
#include "../common/header.h"


/**
 * @brief rounds up a number to the next multiple of 2
 *
 * @param n	integer to be rounded
 *
 * @returns next even number or the input if already even
 */

#define ROUND_UP_TO_NEXT_2(n) (((n) + 1U) & ~1U)


unsigned int cmp_is_error(uint32_t code)
{
	return cmp_is_error_int(code);
}


uint32_t cmp_compress_bound(uint32_t src_size)
{
	uint32_t const bound = CMP_HDR_SIZE + ROUND_UP_TO_NEXT_2(src_size);

	/* Check overflow */
	if (bound <= src_size)
		return (CMP_ERROR(SRC_SIZE_WRONG));

	if (bound > CMP_MAX_CMP_SIZE)
		return (CMP_ERROR(SRC_SIZE_WRONG));

	return bound;
}


uint32_t cmp_cal_work_buf_size(const struct cmp_params *params, uint32_t src_size)
{
	switch (params->preprocess) {
	case CMP_PREPROCESS_IWT:
		return ROUND_UP_TO_NEXT_2(src_size);
	case CMP_PREPROCESS_NONE:
	case CMP_PREPROCESS_DIFF:
		return 0;
	default:
		return (CMP_ERROR(SRC_SIZE_WRONG));
	}
}


uint32_t cmp_initialise(struct cmp_context *ctx, const struct cmp_params *params,
			void *work_buf, uint32_t work_buf_size)
{
	const struct preprocessing_method *prepocess;

	if (ctx == NULL)
		return CMP_ERROR(CONTEXT_INVALID);

	cmp_deinitialise(ctx);

	if (params == NULL)
		return CMP_ERROR(PARAMS_INVALID);

	prepocess = preprocessing_get_method(params->preprocess);
	if (prepocess == NULL)
		return CMP_ERROR(PARAMS_INVALID);

	if (prepocess->get_work_buf_size(2))
		if (!work_buf)
			return CMP_ERROR(WORK_BUF_NULL);

	ctx->params = *params;
	ctx->work_buf = work_buf;
	ctx->work_buf_size = work_buf_size;

	return CMP_ERROR(NO_ERROR);
}


uint32_t cmp_compress_u16(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
			  const uint16_t *src, uint32_t src_size)
{
	uint32_t i, n_values;
	uint32_t cmp_size = CMP_HDR_SIZE;
	const struct preprocessing_method *prepocess;

	if (ctx == NULL)
		return CMP_ERROR(CONTEXT_INVALID);
	if (dst == NULL)
		return CMP_ERROR(DST_NULL);

	prepocess = preprocessing_get_method(ctx->params.preprocess);
	if (prepocess == NULL)
		return CMP_ERROR(PARAMS_INVALID);

	n_values = prepocess->init(src, src_size, ctx->work_buf, ctx->work_buf_size);
	if (cmp_is_error_int(n_values))
		return n_values;

	cmp_size += src_size; /* TODO: overflow */
	if (cmp_size > dst_capacity)
		return CMP_ERROR(DST_TOO_SMALL);

	for (i = 0; i < n_values; i++) {
		int16_t const prepocessed_value = prepocess->process(i, src, ctx->work_buf);
		uint8_t *p = (uint8_t *)dst + CMP_HDR_SIZE;

		/* Copy the src buffer to the destination in big-endian */
		p[i*2]   = (uint8_t)(prepocessed_value >> 8);
		p[i*2+1] = (uint8_t)(prepocessed_value & 0xFF);
	}

	{ /* Build header */
		struct cmp_hdr hdr = {0};
		uint32_t return_val;

		hdr.version = CMP_VERSION_NUMBER;
		hdr.cmp_size = cmp_size;
		hdr.original_size = src_size;
		hdr.mode = ctx->params.mode;
		hdr.preprocess = prepocess->type;
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

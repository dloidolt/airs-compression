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
#include "encoder.h"
#include "../cmp.h"
#include "../common/err_private.h"
#include "../common/bitstream_writer.h"
#include "../common/header.h"

/**
 * @brief Returns the maximum of two values.
 *
 * @param a	first value to compare
 * @param b	second value to compare
 *
 * @returns the larger of a and  b
 */
#define max(a, b) ((a) > (b) ? (a) : (b))


/**
 * @brief  fall back dummy implementation of a get_current_timestamp() function
 *
 * @returns 0
 */

static uint64_t fallback_get_current_timestamp(void)
{
	static uint64_t cnt;

	return cnt++;
}


/**
 * Function pointer to a function returning a current 48-bit timestamp
 * initialised with the cmp_set_timestamp_func() function
 */

static uint64_t (*g_get_current_timestamp)(void) = fallback_get_current_timestamp;


void cmp_set_timestamp_func(uint64_t (*get_current_timestamp_func)(void))
{
	if (get_current_timestamp_func)
		g_get_current_timestamp = get_current_timestamp_func;
	else
		g_get_current_timestamp = fallback_get_current_timestamp;
}


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

	if (bound >= 1ULL << CMP_HDR_BITS_COMPRESSED_SIZE)
		return (CMP_ERROR(SRC_SIZE_WRONG));

	return bound;
}


uint32_t cmp_cal_work_buf_size(const struct cmp_params *params, uint32_t src_size)
{
	const struct preprocessing_method *preprocess;
	uint32_t primary_work_buf_size, secondary_work_buf_size;

	if (params == NULL)
		return CMP_ERROR(PARAMS_INVALID);

	if (params->primary_preprocessing == CMP_PREPROCESS_MODEL)
		return CMP_ERROR(PARAMS_INVALID);

	preprocess = preprocessing_get_method(params->primary_preprocessing);
	if (preprocess == NULL)
		return CMP_ERROR(PARAMS_INVALID);
	primary_work_buf_size = preprocess->get_work_buf_size(src_size);

	if (params->secondary_iterations) {
		preprocess = preprocessing_get_method(params->secondary_preprocessing);
		if (preprocess == NULL)
			return CMP_ERROR(PARAMS_INVALID);
		secondary_work_buf_size = preprocess->get_work_buf_size(src_size);
	} else {
		secondary_work_buf_size = 0;
	}

	return max(primary_work_buf_size, secondary_work_buf_size);
}


/** Maximum allowed model adaptation rate parameter  */
#define CMP_MAX_MODEL_RATE 16U

/**
 * @brief Updates the model value based on new data and adaptation rate
 *
 * @param data		new data value to incorporate into the model
 * @param model		current model value
 * @param model_rate	model adaptation rate; higher values make the model adapt
 *			more slowly to new data; must be less than or equal to
 *			CMP_MAX_MODEL_RATE
 * @returns the updated model value
 */

static uint16_t update_model(uint16_t data, uint16_t model, unsigned int model_rate)
{
	uint32_t const weighted_data = data * (CMP_MAX_MODEL_RATE - model_rate);
	uint32_t const weighted_model = model * model_rate;

	return (uint16_t)((weighted_model + weighted_data) / CMP_MAX_MODEL_RATE);
}


uint32_t cmp_initialise(struct cmp_context *ctx, const struct cmp_params *params, void *work_buf,
			uint32_t work_buf_size)
{
	uint32_t const min_src_size = 2;
	uint32_t work_buf_needed;
	uint32_t error_code;

	if (ctx == NULL)
		return CMP_ERROR(CONTEXT_INVALID);

	cmp_deinitialise(ctx);

	work_buf_needed = cmp_cal_work_buf_size(params, min_src_size);
	if (cmp_is_error_int(work_buf_needed))
		return work_buf_needed;

	if (work_buf_needed && !work_buf)
		return CMP_ERROR(WORK_BUF_NULL);

	if (work_buf_needed && work_buf_size == 0)
		return CMP_ERROR(WORK_BUF_TOO_SMALL);

	if (params->model_rate > CMP_MAX_MODEL_RATE &&
	    params->secondary_preprocessing == CMP_PREPROCESS_MODEL)
		return CMP_ERROR(PARAMS_INVALID);

	if (params->secondary_iterations >= (1ULL << CMP_HDR_BITS_SEQUENCE_NUMBER))
		return CMP_ERROR(PARAMS_INVALID);

	error_code = cmp_encoder_params_check(params->primary_encoder_type,
					      params->primary_encoder_param,
					      params->primary_encoder_outlier);
	if (cmp_is_error_int(error_code))
		return error_code;

	if (params->secondary_iterations) {
		error_code = cmp_encoder_params_check(params->secondary_encoder_type,
						      params->secondary_encoder_param,
						      params->secondary_encoder_outlier);
		if (cmp_is_error_int(error_code))
			return error_code;
	}

	ctx->params = *params;
	ctx->work_buf = work_buf;
	ctx->work_buf_size = work_buf_size;

	return cmp_reset(ctx);
}


uint32_t cmp_compress_u16(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
			  const uint16_t *src, uint32_t src_size)
{
	uint32_t i, ret, n_values;
	enum cmp_preprocessing selected_preprocessing;
	enum cmp_encoder_type selected_encoder_type;
	uint32_t selected_encoder_param;
	uint32_t selected_outlier;
	struct bitstream_writer bs;
	struct cmp_encoder enc;
	const struct preprocessing_method *prepocess;
	int model_is_needed = 0;
	struct cmp_hdr hdr = { 0 };

	if (!ctx)
		return CMP_ERROR(CONTEXT_INVALID);

	if (ctx->sequence_number == 0 || ctx->sequence_number > ctx->params.secondary_iterations) {
		ret = cmp_reset(ctx);
		if (cmp_is_error_int(ret))
			return ret;
		selected_preprocessing = ctx->params.primary_preprocessing;
		selected_encoder_type = ctx->params.primary_encoder_type;
		selected_encoder_param = ctx->params.primary_encoder_param;
		selected_outlier = ctx->params.primary_encoder_outlier;
		ctx->model_size = src_size;
	} else {
		selected_preprocessing = ctx->params.secondary_preprocessing;
		selected_encoder_type = ctx->params.secondary_encoder_type;
		selected_encoder_param = ctx->params.secondary_encoder_param;
		selected_outlier = ctx->params.secondary_encoder_outlier;
		/*
		 * When using model preprocessing the size of the data to
		 * compression is not allowed to change unit a reset.
		 */
		if (ctx->params.secondary_preprocessing == CMP_PREPROCESS_MODEL &&
		    src_size != ctx->model_size)
			return CMP_ERROR(SRC_SIZE_MISMATCH);
	}

	/* Do we need a model? */
	if (ctx->params.secondary_preprocessing == CMP_PREPROCESS_MODEL &&
	    ctx->params.secondary_iterations != 0) {
		model_is_needed = 1;

		if (ctx->work_buf_size < src_size)
			return CMP_ERROR(WORK_BUF_TOO_SMALL);
	}

	ret = bitstream_writer_init(&bs, dst, dst_capacity);
	if (cmp_is_error_int(ret))
		return ret;

	ret = cmp_encoder_init(&enc, selected_encoder_type, selected_encoder_param,
			       selected_outlier, &bs);
	if (cmp_is_error_int(ret))
		return ret;

	hdr.version_flag = 1;
	hdr.version_id = CMP_VERSION_NUMBER;
	hdr.original_size = src_size;
	hdr.compressed_size = 0; /* place holder, not know right now */
	hdr.identifier = ctx->identifier;
	hdr.sequence_number = ctx->sequence_number;
	hdr.preprocessing = selected_preprocessing;
	hdr.encoder_type = selected_encoder_type;
	hdr.model_rate = ctx->params.model_rate;
	hdr.encoder_param = selected_encoder_param;
	hdr.encoder_outlier = enc.outlier;
	ret = cmp_hdr_serialize(&bs, &hdr);
	if (cmp_is_error_int(ret))
		return ret;

	prepocess = preprocessing_get_method(selected_preprocessing);
	if (prepocess == NULL)
		return CMP_ERROR(PARAMS_INVALID);

	n_values = prepocess->init(src, src_size, ctx->work_buf, ctx->work_buf_size);
	if (cmp_is_error_int(n_values))
		return n_values;

	for (i = 0; i < n_values; i++) {
		int16_t const value = prepocess->process(i, src, ctx->work_buf);

		ret = cmp_encoder_encode_s16(&enc, value);
		if (cmp_is_error_int(ret))
			return ret;

		if (model_is_needed) {
			uint16_t *model = ctx->work_buf;

			if (ctx->sequence_number == 0)
				model[i] = src[i];
			else
				model[i] = update_model(src[i], model[i], ctx->params.model_rate);
		}
	}

	hdr.compressed_size = cmp_encoder_finish(&enc);
	if (cmp_is_error_int(hdr.compressed_size))
		return hdr.compressed_size;

	/*
	 * Now that we have the final compressed size, rewind the bitstream and
	 * re-serialize the header with the correct cmp_size.
	 */
	ret = bitstream_rewind(&bs);
	if (cmp_is_error_int(ret))
		return ret;
	ret = cmp_hdr_serialize(&bs, &hdr);
	if (cmp_is_error_int(ret))
		return ret;

	ctx->sequence_number++;
	return hdr.compressed_size;
}


uint32_t cmp_reset(struct cmp_context *ctx)
{
	uint64_t const timestamp = g_get_current_timestamp();

	if (ctx == NULL)
		return CMP_ERROR(CONTEXT_INVALID);

	if (timestamp > ((uint64_t)1 << 48) - 1)
		return CMP_ERROR(TIMESTAMP_INVALID);

	ctx->sequence_number = 0;
	ctx->identifier = timestamp;
	ctx->model_size = 0;

	return CMP_ERROR(NO_ERROR);
}


void cmp_deinitialise(struct cmp_context *ctx)
{
	if (ctx)
		memset(ctx, 0, sizeof(*ctx));
}

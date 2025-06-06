/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Compression Fuzz Target
 */

#include <stdlib.h>
#include <string.h>
#include "fuzz_helpers.h"
#include "cmp.h"
#include "cmp_errors.h"

#define FUZZ_MAX_BUF_SIZE (1 << 25)


struct fuzz_data {
	const uint8_t *data;
	size_t size;
};


/*
 * Safely consume 'n' bytes from the fuzz data. If not enough bytes are
 * available, the destination is zeroed and we return 0.
 */
static int consume_bytes(struct fuzz_data *f, void *dest, size_t n)
{
	if (f->size < n) {
		memset(dest, 0, n);
		f->size = 0;
		return 0;
	}

	if (n)
		memcpy(dest, f->data, n);
	f->data += n;
	f->size -= n;
	return 1;
}


static uint32_t consume_u32(struct fuzz_data *f)
{
	uint32_t val = 0;

	consume_bytes(f, &val, sizeof(val));
	return val;
}


static uint8_t consume_u8(struct fuzz_data *f)
{
	uint8_t val = 0;

	consume_bytes(f, &val, sizeof(val));
	return val;
}


/* returns 0 or 1 Randomly */
static uint8_t consume_rand(struct fuzz_data *f)
{
	return consume_u8(f) % 2;
}


static size_t remaining(struct fuzz_data *f)
{
	return f->size;
}


static struct cmp_params *params_create(struct fuzz_data *f)
{
	struct cmp_params *params;

	/* Randomly return NULL to test error paths */
	if (consume_rand(f))
		return NULL;

	params = FUZZ_malloc(sizeof(*params));

	params->primary_preprocessing = consume_u32(f);
	params->primary_encoder_type = consume_u32(f);
	params->primary_encoder_param = consume_u32(f);
	params->primary_encoder_outlier = consume_u32(f);

	params->secondary_iterations = consume_u32(f);
	params->secondary_preprocessing = consume_u32(f);
	params->secondary_encoder_type = consume_u32(f);
	params->secondary_encoder_param = consume_u32(f);
	params->secondary_encoder_outlier = consume_u32(f);

	params->checksum_enabled = consume_u32(f);
	params->uncompressed_fallback_enabled = consume_u32(f);

	return params;
}


void assert_check_result(uint32_t cmp_result)
{
	switch (cmp_get_error_code(cmp_result)) {
	/* expected errors */
	case CMP_ERR_NO_ERROR:
	case CMP_ERR_DST_NULL:
	case CMP_ERR_DST_TOO_SMALL:
	case CMP_ERR_WORK_BUF_TOO_SMALL:
	case CMP_ERR_SRC_NULL:
	case CMP_ERR_SRC_SIZE_WRONG:
	case CMP_ERR_SRC_SIZE_MISMATCH:
	case CMP_ERR_GENERIC:
	case CMP_ERR_DST_UNALIGNED:
	case CMP_ERR_PARAMS_INVALID:
	case CMP_ERR_WORK_BUF_NULL:
	case CMP_ERR_TIMESTAMP_INVALID:
		break;

	/* unexpected errors */
	case CMP_ERR_CONTEXT_INVALID:
	case CMP_ERR_WORK_BUF_UNALIGNED:
	case CMP_ERR_HDR_CMP_SIZE_TOO_LARGE:
	case CMP_ERR_HDR_ORIGINAL_TOO_LARGE:
	case CMP_ERR_INT_HDR:
	case CMP_ERR_INT_ENCODER:
	case CMP_ERR_INT_BITSTREAM:
	case CMP_ERR_MAX_CODE:
	default:
		FUZZ_ASSERT_MSG(0, "Invalid or unhandled cmp_error code");
	}
}


static uint64_t timestamp_stub_good(void)
{
	return 0x123456789ABC;
}


static uint64_t timestamp_stub_bad(void)
{
	return 0xFFFFFFFFFFFFFFFF;
}


uint32_t adjusted_size(uint32_t request_size, uint32_t fuzz_size)
{
	if (!cmp_is_error(request_size)) {
		/* make it randomly smaller */
		if (fuzz_size <= request_size)
			return fuzz_size;
	}

	/* return randomly 0 or a valid size */
	if (fuzz_size <= FUZZ_MAX_BUF_SIZE)
		return fuzz_size;

	return 0;
}


int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	struct fuzz_data f;

	struct cmp_params *params;
	struct cmp_context ctx;

	void *work_buf = NULL;
	void *dst = NULL;
	void *dst2 = NULL;
	uint16_t *src = NULL;
	uint16_t *src2 = NULL;
	uint32_t work_buf_size = 0, dst_capacity = 0, dst2_capacity = 0, src_size = 0,
		 src2_size = 0;

	uint32_t init_result = 0, cmp_result = 0, cmp2_result = 0, reset_result = 0;
	uint32_t s;

	f.data = data;
	f.size = size;

	s = consume_u32(&f);
	if (s <= size)
		src_size = s;
	src = FUZZ_malloc(src_size);
	consume_bytes(&f, src, src_size);

	params = params_create(&f);

	if (consume_rand(&f)) {
		cmp_set_timestamp_func(NULL);
	} else {
		if (consume_rand(&f))
			cmp_set_timestamp_func(&timestamp_stub_good);
		else
			cmp_set_timestamp_func(&timestamp_stub_bad);
	}


	work_buf_size = cmp_cal_work_buf_size(params, src_size);
	work_buf_size = adjusted_size(work_buf_size, consume_u32(&f));
	work_buf = FUZZ_malloc(work_buf_size);


	init_result = cmp_initialise(&ctx, params, work_buf, work_buf_size);
	assert_check_result(init_result);
	if (cmp_is_error(init_result))
		goto out;


	dst_capacity = cmp_compress_bound(src_size);
	assert_check_result(dst_capacity);
	dst_capacity = adjusted_size(dst_capacity, consume_u32(&f));
	dst = FUZZ_malloc(dst_capacity);


	/* first compression */
	cmp_result = cmp_compress_u16(&ctx, dst, dst_capacity, src, src_size);
	assert_check_result(cmp_result);

	/* Randomly reset context between runs */
	if (consume_rand(&f)) {
		reset_result = cmp_reset(&ctx);
		assert_check_result(reset_result);
	}


	/* second compression */
	src2_size = (uint32_t)remaining(&f);
	src2 = FUZZ_malloc(src2_size);
	consume_bytes(&f, src2, src2_size);

	dst2_capacity = cmp_compress_bound(src2_size);
	assert_check_result(dst2_capacity);
	dst2_capacity = adjusted_size(dst2_capacity, consume_u32(&f));
	dst2 = FUZZ_malloc(dst2_capacity);

	cmp2_result = cmp_compress_u16(&ctx, dst2, dst2_capacity, src2, src2_size);
	assert_check_result(cmp2_result);


	cmp_deinitialise(&ctx);

out:
	free(work_buf);
	free(dst);
	free(src);
	free(dst2);
	free(src2);
	free(params);

	return 0;
}

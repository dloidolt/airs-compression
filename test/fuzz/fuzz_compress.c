/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2026
 * @copyright GPL-2.0
 *
 * @brief Compression Fuzz Target
 *
 * @warning This code is platform-specific and dependent on the system endianness.
 */


#include <stdint.h>
#include <stdlib.h>
#include "fuzz.h"
#include "fuzz_helpers.h"
#include "../../lib/cmp.h"
#include "../../lib/cmp_header.h"
#include "../../lib/cmp_errors.h"


static void assert_cmp_result(uint32_t cmp_result)
{
	enum cmp_error err = cmp_get_error_code(cmp_result);
	switch (err) {
	case CMP_ERR_NO_ERROR:

	case CMP_ERR_GENERIC:
	case CMP_ERR_PARAMS_INVALID:

	case CMP_ERR_DST_TOO_SMALL:
	case CMP_ERR_DST_NULL:
	case CMP_ERR_DST_UNALIGNED:

	case CMP_ERR_SRC_SIZE_WRONG:
	case CMP_ERR_SRC_NULL:
	case CMP_ERR_SRC_SIZE_MISMATCH:

	case CMP_ERR_WORK_BUF_TOO_SMALL:
	case CMP_ERR_WORK_BUF_NULL:
	case CMP_ERR_WORK_BUF_UNALIGNED:

	case CMP_ERR_HDR_CMP_SIZE_TOO_LARGE:
	case CMP_ERR_HDR_ORIGINAL_TOO_LARGE:
	case CMP_ERR_HDR_UNSUPPORTED:

	case CMP_ERR_CONTEXT_INVALID:
		break;

	case CMP_ERR_INT_HDR:
		FUZZ_ASSERT(0);
	case CMP_ERR_INT_ENCODER:
		FUZZ_ASSERT(0);
	case CMP_ERR_INT_BITSTREAM:
		FUZZ_ASSERT(0);
	case CMP_ERR_MAX_CODE:
		FUZZ_ASSERT(0);
	default:
		FUZZ_ASSERT(0);
	};
}


int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	struct fuzz_consume f = fuzz_consume_init(data, size);

	struct cmp_params *params;
	struct cmp_context ctx;

	uint32_t src_size, work_buf_size, dst_cap, cmp_result;
	void *src = NULL, *work_buf = NULL, *dst = NULL;


	FUZZ_ASSERT(size <= UINT32_MAX);
	src_size = fuzz_consume_range(&f, 0, (uint32_t)size);
	src = fuzz_malloc(src_size);
	fuzz_consume_n_bytes(&f, src, src_size);

	params = fuzz_consume_alloc_cmp_params(&f);

	if (fuzz_consume_bool(&f)) {
		work_buf_size = cmp_cal_work_buf_size(params, src_size);
		if (cmp_is_error(work_buf_size))
			goto out;
	} else {
		work_buf_size = fuzz_consume_range(&f, 0, 2 * (CMP_HDR_MAX_COMPRESSED_SIZE + 10));
	}
	work_buf = fuzz_malloc(work_buf_size);

	cmp_hdr_set_identifier(fuzz_consume_u32(&f));

	if (cmp_is_error(cmp_initialise(&ctx, params, work_buf, work_buf_size)))
		goto out;

	if (fuzz_consume_bool(&f)) {
		dst_cap = cmp_compress_bound(src_size);
		if (cmp_is_error(dst_cap)) {
			if (cmp_get_error_code(dst_cap) == CMP_ERR_HDR_CMP_SIZE_TOO_LARGE) {
				dst_cap = CMP_HDR_MAX_COMPRESSED_SIZE;
			} else {
				if (cmp_get_error_code(dst_cap) == CMP_ERR_HDR_ORIGINAL_TOO_LARGE)
					goto out;
				else
					FUZZ_ASSERT(0);
			}
		}
	} else {
		dst_cap = fuzz_consume_range(&f, 0, CMP_HDR_MAX_COMPRESSED_SIZE + 10);
	}
	dst = fuzz_malloc(dst_cap);

	cmp_result = cmp_compress_u16(&ctx, dst, dst_cap, src, src_size);
	assert_cmp_result(cmp_result);

out:
	free(src);
	free(params);
	free(work_buf);
	free(dst);

	return 0;
}

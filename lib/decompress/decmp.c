/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data decompression implementation
 */

#include <stdint.h>
#include <stdlib.h>

#include "../cmp.h"
#include "../cmp_errors.h"
#include "../common/header_private.h"
#include "../common/err_private.h"
#include "../common/compiler.h"
#include "arena.h"
#include "read_bitstream.h"
#include "decmp.h"

/* Decompress single 16-bit buffer */
static uint32_t decompress_single_u16(const void *src, uint32_t src_size, const struct cmp_hdr *hdr,
				      uint16_t *dst)
{
	struct bit_decoder dec;
	uint32_t samples = hdr->original_size / sizeof(uint16_t); /* TODO: what if it unevne? */
	uint32_t i;
	uint32_t decoded_value;
	uint16_t last_decoded_value = 0;

	if (bit_init_decoder(&dec, src, src_size) == 0)
		return CMP_ERROR(GENERIC);


	for (i = 0; i < samples; i++) {
		switch (hdr->encoder_type) {
		case CMP_ENCODER_UNCOMPRESSED:
			/* TODO: if (i % 2) */
			bit_refill(&dec);
			decoded_value = bit_read_bits32(&dec, bitsizeof(dst[0]));
			break;
		case CMP_ENCODER_GOLOMB_ZERO:
		case CMP_ENCODER_GOLOMB_MULTI:
		default:
			return CMP_ERROR(GENERIC);
		}

		switch (hdr->preprocessing) {
		case CMP_PREPROCESS_NONE:
			dst[i] = (uint16_t)decoded_value;
			break;
		case CMP_PREPROCESS_DIFF:
			dst[i] = (uint16_t)decoded_value + last_decoded_value;
			last_decoded_value = dst[i];
			break;
		case CMP_PREPROCESS_IWT:
		case CMP_PREPROCESS_MODEL:
		default:
			return CMP_ERROR(GENERIC);
		}
	}

	return hdr->original_size;
}


struct decmp_result *decompress_batch_u16(struct arena *a, const void *srcs[], uint32_t src_sizes[],
					  uint32_t src_count)
{
	uint32_t i;
	/* the struct is zero initialized through the arena */
	struct decmp_result *res = ARENA_NEW(a, struct decmp_result);

	res->decmp = ARENA_NEW_ARRAY(a, src_count, uint16_t *);
	res->decmp_size = ARENA_NEW_ARRAY(a, src_count, uint32_t);

	for (i = 0; i < src_count; i++) {
		uint16_t *dst;
		const void *data_after_hdr;
		uint32_t data_after_hdr_size;
		struct cmp_hdr hdr = { 0 };
		uint32_t hdr_size = cmp_hdr_deserialize(srcs[i], src_sizes[i], &hdr);

		if (cmp_is_error_int(hdr_size)) {
			res->decmp_size[i] = hdr_size;
			continue;
		}

		dst = arena_alloc(a, hdr.original_size, sizeof(uint8_t),
				  __alignof__(*res->decmp[i]));

		/* TODO: maybe include this in cmp_hdr_deserialize */
		data_after_hdr = (const uint8_t *)srcs[i] + hdr_size;
		data_after_hdr_size = src_sizes[i] - hdr_size;

		res->decmp_size[i] =
			decompress_single_u16(data_after_hdr, data_after_hdr_size, &hdr, dst);
		if (!cmp_is_error_int(res->decmp_size[i]))
			res->decmp[i] = dst;

		res->count++;
	}

	return res;
}

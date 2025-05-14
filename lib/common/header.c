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
#include "bitstream_writer.h"


void *cmp_hdr_get_cmp_data(void *header)
{
	return (uint8_t *)header + CMP_HDR_SIZE;
}


uint32_t cmp_hdr_serialize(struct bitstream_writer *bs, const struct cmp_hdr *hdr)
{
	uint32_t start_size, end_size;

	if (!hdr)
		return CMP_ERROR(INT_HDR);

	if (hdr->original_size >= 1ULL << CMP_HDR_BITS_ORIGINAL_SIZE)
		return CMP_ERROR(SRC_SIZE_WRONG);

	start_size = bitstream_size(bs);
	if (cmp_is_error_int(start_size))
		return start_size;

/* Local macro for this function only */
#define CMP_DO_WRITE_OR_RETURN(bs_ptr, val_to_write, len_to_write)                        \
	do {                                                                              \
		uint32_t __err_code;                                                      \
		__err_code = bitstream_write64((bs_ptr), (val_to_write), (len_to_write)); \
		if (cmp_is_error_int(__err_code))                                         \
			return __err_code;                                                \
	} while (0)

	CMP_DO_WRITE_OR_RETURN(bs, hdr->version, CMP_HDR_BITS_VERSION);
	CMP_DO_WRITE_OR_RETURN(bs, hdr->cmp_size, CMP_HDR_BITS_CMP_SIZE);
	CMP_DO_WRITE_OR_RETURN(bs, hdr->original_size, CMP_HDR_BITS_ORIGINAL_SIZE);
	CMP_DO_WRITE_OR_RETURN(bs, hdr->mode, CMP_HDR_BITS_MODE);
	CMP_DO_WRITE_OR_RETURN(bs, hdr->preprocess, CMP_HDR_BITS_PREPROCESS);
	CMP_DO_WRITE_OR_RETURN(bs, hdr->model_rate, CMP_HDR_BITS_MODEL_RATE);
	CMP_DO_WRITE_OR_RETURN(bs, hdr->model_id, CMP_HDR_BITS_MODEL_ID);
	CMP_DO_WRITE_OR_RETURN(bs, hdr->pass_count, CMP_HDR_BITS_PASS_COUNT);
	CMP_DO_WRITE_OR_RETURN(bs, hdr->compression_par, CMP_HDR_BITS_COMPRESSION_PAR);
#undef CMP_DO_WRITE_OR_RETURN

	end_size = bitstream_flush(bs);
	if (cmp_is_error_int(end_size))
		return end_size;

	return end_size - start_size;
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


static const uint8_t *deserialize_u48(const uint8_t *pos, uint64_t *read_value)
{
	*read_value = ((uint64_t)pos[0] << 40) | ((uint64_t)pos[1] << 32) |
		      ((uint64_t)pos[2] << 24) | ((uint64_t)pos[3] << 16) |
		      ((uint64_t)pos[4] << 8) | ((uint64_t)pos[5]);
	return pos + 6;
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
	pos = deserialize_u24(pos, &hdr->original_size);
	hdr->mode = *pos++;
	hdr->preprocess = *pos++;
	hdr->model_rate = *pos++;
	pos = deserialize_u48(pos, &hdr->model_id);
	hdr->pass_count = *pos++;
	pos = deserialize_u16(pos, &hdr->compression_par);

	return (uint32_t)(pos - (const uint8_t *)src);
}

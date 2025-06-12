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

#define XXH_INLINE_ALL
#define XXH_STATIC_LINKING_ONLY
#define XXH_NO_STDLIB
#include "xxhash.h"

uint32_t cmp_hdr_serialize(struct bitstream_writer *bs, const struct cmp_hdr *hdr)
{
	uint32_t start_size, end_size;

	if (!hdr)
		return CMP_ERROR(INT_HDR);

	if (hdr->compressed_size > CMP_HDR_MAX_COMPRESSED_SIZE)
		return CMP_ERROR(HDR_CMP_SIZE_TOO_LARGE);

	if (hdr->original_size > CMP_HDR_MAX_ORIGINAL_SIZE)
		return CMP_ERROR(HDR_ORIGINAL_TOO_LARGE);

	if (hdr->identifier > ((uint64_t)1 << CMP_HDR_BITS_IDENTIFIER) - 1)
		return CMP_ERROR(TIMESTAMP_INVALID);

	start_size = bitstream_size(bs);
	if (cmp_is_error_int(start_size))
		return start_size;

#define CMP_DO_WRITE_OR_RETURN(bs_ptr, val_to_write, len_to_write)                            \
	do {                                                                                  \
		uint32_t write_err_code;                                                      \
		write_err_code = bitstream_write64((bs_ptr), (val_to_write), (len_to_write)); \
		if (cmp_is_error_int(write_err_code))                                         \
			return write_err_code;                                                \
	} while (0)

	CMP_DO_WRITE_OR_RETURN(bs, hdr->version_flag, CMP_HDR_BITS_VERSION_FLAG);
	CMP_DO_WRITE_OR_RETURN(bs, hdr->version_id, CMP_HDR_BITS_VERSION_ID);

	CMP_DO_WRITE_OR_RETURN(bs, hdr->compressed_size, CMP_HDR_BITS_COMPRESSED_SIZE);
	CMP_DO_WRITE_OR_RETURN(bs, hdr->original_size, CMP_HDR_BITS_ORIGINAL_SIZE);

	CMP_DO_WRITE_OR_RETURN(bs, hdr->identifier, CMP_HDR_BITS_IDENTIFIER);
	CMP_DO_WRITE_OR_RETURN(bs, hdr->sequence_number, CMP_HDR_BITS_SEQUENCE_NUMBER);

	/* internal structure of the compression method */
	CMP_DO_WRITE_OR_RETURN(bs, hdr->preprocessing, CMP_HDR_BITS_METHOD_PREPROCESSING);
	CMP_DO_WRITE_OR_RETURN(bs, hdr->checksum_enabled, CMP_HDR_BITS_METHOD_CHECKSUM_ENABLED);
	CMP_DO_WRITE_OR_RETURN(bs, hdr->encoder_type, CMP_HDR_BITS_METHOD_ENCODER_TYPE);

	if (hdr->preprocessing != CMP_PREPROCESS_NONE ||
	    hdr->encoder_type != CMP_ENCODER_UNCOMPRESSED) {
		CMP_DO_WRITE_OR_RETURN(bs, hdr->model_rate, CMP_EXT_HDR_BITS_MODEL_ADAPTATION);
		CMP_DO_WRITE_OR_RETURN(bs, hdr->encoder_param, CMP_EXT_HDR_BITS_ENCODER_PARAM);
		CMP_DO_WRITE_OR_RETURN(bs, hdr->encoder_outlier, CMP_EXT_HDR_BITS_ENCODER_OUTLIER);
	}
#undef CMP_DO_WRITE_OR_RETURN

	end_size = bitstream_flush(bs);
	if (cmp_is_error_int(end_size))
		return end_size;

	return end_size - start_size;
}


static uint16_t extract_u16be(const uint8_t *buf)
{
	return (uint16_t)(buf[0] << 8) | (uint16_t)buf[1];
}


static uint32_t extract_u24be(const uint8_t *buf)
{
	return (uint32_t)buf[0] << 16 | (uint32_t)buf[1] << 8 | (uint32_t)buf[2];
}


static uint64_t extract_u48be(const uint8_t *buf)
{
	return (uint64_t)buf[0] << 40 | (uint64_t)buf[1] << 32 | (uint64_t)buf[2] << 24 |
	       (uint64_t)buf[3] << 16 | (uint64_t)buf[4] << 8 | (uint64_t)buf[5];
}


uint32_t cmp_hdr_deserialize(const void *src, uint32_t src_size, struct cmp_hdr *hdr)
{
	const uint8_t *start = src;
	uint8_t method;
	uint16_t version;

	(void)src_size;

	if (!hdr)
		return CMP_ERROR(INT_HDR);
	if (!src)
		return CMP_ERROR(INT_HDR);
	if (src_size < CMP_HDR_SIZE)
		return CMP_ERROR(INT_HDR);
	memset(hdr, 0x00, sizeof(*hdr));

	version = extract_u16be(start + CMP_HDR_OFFSET_VERSION);
	hdr->version_flag = (version >> CMP_HDR_BITS_VERSION_ID) & 1U;
	hdr->version_id = version & ((1 << CMP_HDR_BITS_VERSION_ID) - 1);

	hdr->compressed_size = extract_u24be(start + CMP_HDR_OFFSET_COMPRESSED_SIZE);
	hdr->original_size = extract_u24be(start + CMP_HDR_OFFSET_ORIGINAL_SIZE);
	hdr->identifier = extract_u48be(start + CMP_HDR_OFFSET_IDENTIFIER);
	hdr->sequence_number = start[CMP_HDR_OFFSET_SEQUENCE_NUMBER];

	method = start[CMP_HDR_OFFSET_METHOD];
	hdr->preprocessing = (method >> 4) & 0XF;
	hdr->checksum_enabled = (method >> 3) & 0x1;
	hdr->encoder_type = method & 0x7;

	/* have extended header? */
	if (hdr->preprocessing == CMP_PREPROCESS_NONE &&
	    hdr->encoder_type == CMP_ENCODER_UNCOMPRESSED)
		return CMP_HDR_SIZE;

	if (src_size < CMP_HDR_SIZE + CMP_EXT_HDR_SIZE) {
		memset(hdr, 0x00, sizeof(*hdr));
		return CMP_ERROR(INT_HDR);
	}

	hdr->model_rate = start[CMP_EXT_HDR_OFFSET_MODEL_RATE];
	hdr->encoder_param = extract_u16be(start + CMP_EXT_HDR_OFFSET_ENCODER_PARAM);
	hdr->encoder_outlier = extract_u24be(start + CMP_EXT_HDR_OFFSET_OUTLIER_PARAM);

	return CMP_HDR_SIZE + CMP_EXT_HDR_SIZE;
}


uint32_t cmp_checksum(const uint16_t *data, uint32_t size)
{
	if (XXH_CPU_LITTLE_ENDIAN) {
		uint32_t i;
		XXH32_state_t state;

		(void)XXH32_reset(&state, CHECKSUM_SEED);
		/* Convert to big-endian to get consistent checksums across
		 * different CPU architectures.
		 */
		for (i = 0; i < size / sizeof(*data); i++) {
			uint16_t big_endian = __builtin_bswap16(data[i]);

			(void)XXH32_update(&state, &big_endian, sizeof(data[i]));
		}
		return XXH32_digest(&state);
	} else {
		return XXH32(data, size, CHECKSUM_SEED);
	}
}

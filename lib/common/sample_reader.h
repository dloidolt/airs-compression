#ifndef SAMPLE_READER_H
#define SAMPLE_READER_H

#include <stdint.h>
#include <stddef.h>

#include "../cmp.h"
#include "err_private.h"
#include "bithacks.h"


struct sample_desc {
	const void *data;
	uint32_t num_samples;
	uint8_t stride;
	enum cmp_type dtype;
};


static __inline uint32_t cal_num_samples_round_up(uint32_t src_size, enum cmp_type src_type)
{
	switch (src_type) {
	case CMP_I16:
	case CMP_U16:
		if (src_size >= UINT32_MAX)
			return CMP_ERROR(HDR_ORIGINAL_TOO_LARGE);
		return div_round_up_u32(src_size, sizeof(int16_t));

	case CMP_I16_IN_I32:
		return div_round_up_u32(src_size, sizeof(int32_t));

	default:
		return CMP_ERROR(PARAMS_INVALID);
	}
}


static __inline uint32_t sample_read_src_init(struct sample_desc *src_desc, const void *src,
					      uint32_t src_size, enum cmp_type src_type)
{
	uint8_t stride;

	if (!src)
		return CMP_ERROR(SRC_NULL);

	if (src_size == 0)
		return CMP_ERROR(SRC_SIZE_WRONG);

	switch (src_type) {
	case CMP_I16:
	case CMP_U16:
		stride = sizeof(int16_t);
		break;
	case CMP_I16_IN_I32:
		stride = sizeof(int32_t);
		break;
	default:
		return CMP_ERROR(PARAMS_INVALID);
	}

	if (src_size % stride != 0)
		return CMP_ERROR(SRC_SIZE_WRONG);

	src_desc->data = src;
	src_desc->num_samples = cal_num_samples_round_up(src_size, src_type);
	src_desc->stride = stride;
	src_desc->dtype = src_type;

	return CMP_ERROR(NO_ERROR);
}


/**
 * @brief Reads a 16-bit signed integer from the sample data
 *
 * @param desc	pointer to the sample descriptor
 * @param i	index of the sample to read
 *
 * @return the 16-bit signed integer at index i
 */

static __inline int16_t sample_read_i16(const struct sample_desc *desc, uint32_t i)
{
	const void *addr = (const uint8_t *)desc->data + ((size_t)i * desc->stride);

	/* Assume samples size == stride size */
	if (desc->stride == sizeof(int32_t))
		return (int16_t)(*(const uint32_t *)addr & 0xFFFFU);

	return *(const int16_t *)addr;
}


static __inline uint32_t cal_packed_size(uint32_t num_samples)
{
	return num_samples * sizeof(int16_t);
}


static __inline uint32_t get_packed_size(const struct sample_desc *desc)
{
	return cal_packed_size(desc->num_samples);
}

#endif /* SAMPLE_READER_H */

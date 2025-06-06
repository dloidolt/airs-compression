/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2026
 * @copyright GPL-2.0
 *
 * @brief Fuzzing helper functions implementation
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fuzz_helpers.h"
#include "../../lib/cmp.h"


void *fuzz_malloc(size_t size)
{
	void *mem;

	if (size == 0)
		return NULL;

	mem = malloc(size);
	FUZZ_ASSERT(mem);
	return mem;
}


static int bit_width(uint32_t n)
{
	/* assert that unsigned bitsizeof(int) == 32 */
	if (n == 0)
		return 0;
	return 32 - __builtin_clz(n);
}


static int byte_width(uint32_t n)
{
	return (bit_width(n) + 7) / 8;
}


struct fuzz_consume fuzz_consume_init(const uint8_t *data, size_t size)
{
	struct fuzz_consume f;

	f.beg = data;
	f.end = data + size;
	return f;
}


void fuzz_consume_n_bytes(struct fuzz_consume *f, void *dst, size_t n)
{
	ptrdiff_t available = f->end - f->beg;

	if ((ptrdiff_t)n <= 0)
		return;

	if (available <= 0) {
		memset(dst, 0, n);
	} else if (available >= (ptrdiff_t)n) {
		memcpy(dst, f->beg, n);
		f->beg += n;
	} else {
		memcpy(dst, f->beg, (size_t)available);
		memset((uint8_t *)dst + available, 0, n - (size_t)available);
		f->beg += available;
	}
}


uint32_t fuzz_consume_u32(struct fuzz_consume *f)
{
	uint32_t val = 0;

	fuzz_consume_n_bytes(f, &val, sizeof(val));
	return val;
}


uint8_t fuzz_consume_u8(struct fuzz_consume *f)
{
	uint8_t val = 0;

	if (f->end - f->beg > 0)
		val = *f->beg++;

	return val;
}


uint8_t fuzz_consume_bool(struct fuzz_consume *f)
{
	return fuzz_consume_u8(f) & 0x1; /* we ignore the other bits */
}


uint32_t fuzz_consume_range(struct fuzz_consume *f, uint32_t min, uint32_t max)
{
	uint32_t range = max - min;
	int const bytes_needed = byte_width(range);
	uint32_t result = 0;
	uint8_t buf[4] = { 0 };
	size_t i;

	FUZZ_ASSERT(min <= max);

	fuzz_consume_n_bytes(f, buf, (size_t)bytes_needed);

	for (i = 0; i < sizeof(buf); i++)
		result = (result << 8) | buf[i];

	if (range == UINT32_MAX)
		return result;

	return min + (result % (range + 1));
}


struct cmp_params *fuzz_consume_alloc_cmp_params(struct fuzz_consume *f)
{
	struct cmp_params *params = fuzz_malloc(sizeof(*params));

	/* Intentionally u8 by preprocessing, encoder_type, iterations,
	 * model_rate: valid range is covered by u8 */
	params->primary_preprocessing = fuzz_consume_u8(f);
	params->primary_encoder_type = fuzz_consume_u8(f);
	params->primary_encoder_param = fuzz_consume_u32(f);
	params->primary_encoder_outlier = fuzz_consume_u32(f);

	params->secondary_iterations = fuzz_consume_u8(f);
	params->secondary_preprocessing = fuzz_consume_u8(f);
	params->secondary_encoder_type = fuzz_consume_u8(f);
	params->secondary_encoder_param = fuzz_consume_u32(f);
	params->secondary_encoder_outlier = fuzz_consume_u32(f);
	params->model_rate = fuzz_consume_u8(f);

	params->checksum_enabled = fuzz_consume_u8(f);
	params->uncompressed_fallback_enabled = fuzz_consume_u8(f);

	return params;
}

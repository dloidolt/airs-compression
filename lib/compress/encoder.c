/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Compression Encoder Implementation
 */

#include <limits.h>
#include <string.h>

#include "encoder.h"
#include "../cmp.h"
#include "../common/bitstream_writer.h"
#include "../common/err_private.h"
#include "../common/bithacks.h"
#include "../common/compiler.h"

#define CMP_GOLOMB_MAX_CODEWORD_BITS 32

/* In the worst case, each sample is encoded as an escape (max codeword + raw sample bits) */
#define CMP_MAX_BITS_PER_SAMPLE (CMP_GOLOMB_MAX_CODEWORD_BITS + CMP_NUM_BITS_PER_SAMPLE)


/**
 * @brief Returns floor(log2(x)) for integers
 *
 * @param x	input parameter
 *
 * @returns the result of floor(log2(x)) or UINT_MAX if x = 0
 */

static unsigned int ilog2(uint32_t x)
{
	compile_time_assert(sizeof(unsigned int) >= sizeof(uint32_t),
			    _expect_unsigned_int_to_be_at_least_32_bit);

	if (x == 0)
		return UINT_MAX;

	return bitsizeof(x) - 1 - (unsigned int)__builtin_clz(x);
}


/**
 * @brief Calculates the first value that cannot be encoded with golomb_encode()
 *
 * @param g_par		Golomb parameter
 * @param n_bits	Number of bits used to represent uncompressed samples
 * @param encoder_type	Used Golomb encoder type
 *
 * @returns the first value that cannot be encoded with golomb_encode() due to
 *	exceeding the maximum codeword length, or 0 on failure
 */

static uint32_t golomb_upper_bound(uint32_t g_par, enum cmp_encoder_type encoder_type,
				   unsigned int n_bits)
{
	uint32_t cutoff, first_invalid_group, first_invalid_value;

	if (g_par < CMP_MIN_GOLOMB_PAR || g_par > CMP_MAX_GOLOMB_PAR)
		return 0;

	if (n_bits > CMP_NUM_BITS_PER_SAMPLE)
		return 0;

	/* How many values live in group 0? */
	cutoff = (2U << ilog2(g_par)) - g_par;

	/*
	 * Determine the minimum group number that would cause a codeword length
	 * overflow.
	 *
	 * The length of a "non 0 group" is calculated as:
	 * len = group_num + 1      // unary_code
	 *     + ilog2(g_par) + 1   // remainder code (fix for a given g_par)
	 *
	 * We want to find the group_num for which len first exceeds
	 * CMP_GOLOMB_MAX_CODEWORD_BITS.
	 * This means we are looking for len to be CMP_GOLOMB_MAX_CODEWORD_BITS + 1.
	 * So: (CMP_GOLOMB_MAX_CODEWORD_BITS + 1) = group_num + ilog2(g_par) + 2
	 * Rearranging for group_num:
	 */
	first_invalid_group = CMP_GOLOMB_MAX_CODEWORD_BITS + 1 - (ilog2(g_par) + 2);

	/*
	 * Convert that group number into the corresponding value.
	 * Each non-zero group contains g_par values.
	 */
	first_invalid_value = cutoff + first_invalid_group * g_par;

	/* 4) MULTI variant: Reserve space for all used multi escape symbols */
	if (encoder_type == CMP_ENCODER_GOLOMB_MULTI) {
		uint32_t num_escape_symbols = (n_bits + 1) / 2;

		if (first_invalid_value > num_escape_symbols)
			first_invalid_value -= num_escape_symbols;
		else
			return 0;
	}

	return first_invalid_value;
}


/**
 * @brief Calculate an optimal outlier parameter for zero escape mechanism
 *
 * @param g_par		Golomb parameter
 * @param n_bits	Number of bits used to represent uncompressed samples
 *
 * @returns the highest optimal outlier value for a given Golomb parameter, when
 *	the zero escape mechanism is used or 0 on fail
 * @warning value might be to high to encode with golomb_encode()
 *
 * @detail We are looking for the smallest v such that:
 *   len_escape < len_golomb(v+1)  (1)  // v+1 so zero can be used as escape symbol
 * where
 *   len_escape = ilog2(g_par)+1        // bits for the zero symbol
 *              + n_bits,               // raw value that follows
 * and
 *   len_golomb(x) = group_num(x) + 1   // unary prefix
 *                 + ilog2(g_par)+1    // binary suffix
 * for x >= cutoff.
 *
 * (Note: If x < cutoff, len_golomb(x) = ilog2(g_par)+1. In this scenario,
 * (1) becomes n_bits < 0, which is impossible.  Thus, the crossover point where
 * escape becomes cheaper must occur when v+1 >= cutoff.)
 *
 * (1) simplifies to:
 *   n_bits < group_num(v+1) + 1,
 * -> n_bits =< group_num(v+1).
 *
 * With
 *   group_num(v) = floor((v − cutoff) / g_par),
 * we get
 *   n_bits =< floor((v+1 - cutoff) / g_par)  (2)
 *
 * The lowest v value that satisfies (2) is therefore the last element of
 * group (n_bits-1):
 *    v_low = cutoff + n_bits * g_par - 1
 *
 * That value becomes our outlier threshold – every mapped sample from
 * that value onwards will be encoded via the zero-escape method.
 */

static uint32_t golomb_optimal_outlier_zero(uint32_t g_par, unsigned int n_bits)
{
	uint32_t cutoff;
	uint64_t outlier;

	if (g_par < CMP_MIN_GOLOMB_PAR || g_par > CMP_MAX_GOLOMB_PAR)
		return 0;

	if (n_bits < 1 || n_bits > CMP_GOLOMB_MAX_CODEWORD_BITS)
		return 0;

	/* size of group 0 */
	cutoff = (2U << ilog2(g_par)) - g_par;

	/*
	 * Calculate last member in group (n_bits-1).
	 * Use 64-bit to prevent overflow when g_par is large.
	 */
	outlier = cutoff + (uint64_t)n_bits * g_par - 1;

	/*
	 * Cap at UINT32_MAX since we're returning uint32_t and our encoding
	 * functions can't handle larger values anyway.
	 */
	if (outlier > UINT32_MAX)
		return UINT32_MAX;

	return (uint32_t)outlier;
}


uint32_t cmp_encoder_init(struct cmp_encoder *enc, enum cmp_encoder_type encoder_type,
			  uint32_t encoder_param, uint32_t outlier)
{
	if (!enc)
		return CMP_ERROR(INT_ENCODER);

	memset(enc, 0, sizeof(*enc));
	enc->encoder_type = encoder_type;

	switch (enc->encoder_type) {
	case CMP_ENCODER_UNCOMPRESSED:
		break;

	case CMP_ENCODER_GOLOMB_ZERO:
	case CMP_ENCODER_GOLOMB_MULTI:
		if (encoder_param < CMP_MIN_GOLOMB_PAR || encoder_param > CMP_MAX_GOLOMB_PAR)
			return CMP_ERROR(PARAMS_INVALID);
		enc->g_par = encoder_param;
		enc->g_par_log2 = ilog2(encoder_param);

		if (enc->encoder_type == CMP_ENCODER_GOLOMB_ZERO)
			enc->outlier =
				golomb_optimal_outlier_zero(enc->g_par, CMP_NUM_BITS_PER_SAMPLE);
		else
			enc->outlier = outlier;

		/* ensure we do not Golomb-encode too large values */
		enc->outlier =
			min_u32(enc->outlier, golomb_upper_bound(enc->g_par, enc->encoder_type,
								 CMP_NUM_BITS_PER_SAMPLE));
		if (enc->outlier == 0)
			return CMP_ERROR(PARAMS_INVALID);
		break;

	default:
		return CMP_ERROR(PARAMS_INVALID);
	}

	return CMP_ERROR(NO_ERROR);
}


uint32_t cmp_encoder_params_check(enum cmp_encoder_type encoder_type, uint32_t encoder_param,
				  uint32_t outlier)
{
	struct cmp_encoder enc_dummy;

	return cmp_encoder_init(&enc_dummy, encoder_type, encoder_param, outlier);
}


/**
 * @brief Sign-extend a value to fill the full width of the integer type
 *
 * @param value		value to sign-extend
 * @param n_bits	number of bits used to represent the value (including sign
 *			bit) in range [0, 32], if 0 returns the value unchained
 *
 * @see https://graphics.stanford.edu/~seander/bithacks.html#VariableSignExtend
 * @returns the sign-extended value
 */


static int32_t sign_extend(int32_t value, unsigned int n_bits)
{
	compile_time_assert((-1 >> 1) == -1, Arithmetic_shift_need);
	unsigned int const extend_bits = (bitsizeof(value) - n_bits) & (bitsizeof(value) - 1);

	return (int32_t)((uint32_t)value << extend_bits) >> extend_bits;
}


/**
 * Map a signed integer to unsigned via ZigZag encoding
 *
 * @param value		signed integer to map
 * @param n_bits	number of bits needed to represent the highest possible
 *			value in range [1, 32]; 0 if treated as 32 bits
 *
 * This function maps negative values to uneven numbers and positive values to
 * even numbers: 0 -> 0, -1 -> 1, 1 -> 2, -2 -> 3, ...  value_MAX -> 2^n_bits - 2,
 *               value_MIN -> 2^n_bits - 1
 *
 * This is needed because Golomb code only works with unsigned values
 * @see https://stackoverflow.com/questions/4533076/google-protocol-buffers-zigzag-encoding
 *
 * @returns a ZigZag encoded unsigned integer
 */

static uint32_t map_to_unsigned(int32_t value, unsigned int n_bits)
{
	compile_time_assert((-1 >> 1) == -1, Arithmetic_shift_need);
	uint32_t const reg_mask = bitsizeof(value) - 1;

	/*
	 * The arithmetic right shift of a negative number (value >> (n_bits - 1))
	 * results in -1 (all bits set), and for a non-negative number, it
	 * results in 0.
	 */
	value = sign_extend(value, n_bits);
	return (((uint32_t)value << 1) ^ (uint32_t)(value >> ((n_bits - 1) & reg_mask)));
}


/**
 * @brief forms a codeword according to the Golomb code
 *
 * @param value		Value to be encoded, must be smaller than
 *			golomb_upper_bound()
 * @param g_par		Golomb parameter (have to be bigger than 0)
 * @param g_par_log2	Is ilog2(g_par) calculate outside function for better
 *			performance
 * @param bs		Pointer to a bitstream writer; must be initialised by
 *			the caller
 *
 * @warning there is no check of the validity of the input parameters!
 * @returns an error code, which can be checked using cmp_is_error()
 */

static uint32_t golomb_encode(uint32_t value, uint32_t g_par, uint32_t g_par_log2,
			      struct bitstream_writer *bs)
{
	uint32_t const cutoff = (2U << g_par_log2) - g_par; /* members in group 0 */

	if (value < cutoff) /* group 0 */
		return bitstream_write32(bs, value, g_par_log2 + 1);

	{ /* other groups */
		uint32_t const reg_mask = bitsizeof(value) - 1;
		uint32_t const group_num = (value - cutoff) / g_par;
		uint32_t const remainder = (value - cutoff) - group_num * g_par;
		uint32_t const unary_code = (1U << (group_num & reg_mask)) - 1;
		uint32_t const base_codeword = cutoff << 1;
		uint32_t len = g_par_log2 + 1;
		uint32_t codeword = unary_code << ((len + 1) & reg_mask);

		codeword += base_codeword + remainder;
		len += 1 + group_num; /* length of the codeword */

		return bitstream_write32(bs, codeword, len);
	}
}


uint32_t cmp_encoder_encode_s16(const struct cmp_encoder *enc, int16_t value,
				struct bitstream_writer *bs)
{
	uint32_t ret;

	if (!enc || !bs)
		return CMP_ERROR(INT_ENCODER);

	switch (enc->encoder_type) {
	case CMP_ENCODER_UNCOMPRESSED:
		ret = bitstream_write32(bs, (uint16_t)value, bitsizeof(value));
		break;

	case CMP_ENCODER_GOLOMB_ZERO: {
		uint16_t const mapped = (uint16_t)map_to_unsigned(value, bitsizeof(value));

		if (mapped < enc->outlier) {
			/* add 1 for non-outlier values to make space for 0 as escape symbol */
			ret = golomb_encode((uint32_t)mapped + 1, enc->g_par, enc->g_par_log2, bs);
		} else {
			/* encoded 0 indites that encoded mapped data are following */
			ret = golomb_encode(0, enc->g_par, enc->g_par_log2, bs);
			if (cmp_is_error_int(ret))
				return ret;
			ret = bitstream_write32(bs, mapped, bitsizeof(value));
		}
		break;
	}

	case CMP_ENCODER_GOLOMB_MULTI: {
		uint16_t const mapped = (uint16_t)map_to_unsigned(value, bitsizeof(value));

		if (mapped < enc->outlier) {
			ret = golomb_encode(mapped, enc->g_par, enc->g_par_log2, bs);
		} else {
			/*
			 * Multi-escape:
			 * 1. Determine the "escape level" based on how many raw
			 *    bits are needed for diff = mapped - outlier.
			 *    level 0: 1-2 raw bits (including 0)
			 *    level 1: 3-4 raw bits
			 *    ...
			 * 2. Golomb-encode the escape_symbol = outlier + escape_level.
			 * 3. Append 'diff' using raw bits
			 */
			uint32_t const diff = mapped - enc->outlier;
			unsigned int const level = diff < 4 ? 0 : ilog2(diff) / 2;

			ret = golomb_encode(enc->outlier + level, enc->g_par, enc->g_par_log2, bs);
			if (cmp_is_error_int(ret))
				return ret;
			ret = bitstream_write32(bs, diff, (level + 1) * 2);
		}
		break;
	}

	default:
		return CMP_ERROR(PARAMS_INVALID);
	}

	return ret;
}


uint64_t cmp_encoder_max_compressed_size(uint32_t size)
{
	uint64_t const n_samples = DIV_ROUND_UP((uint64_t)size * 8, CMP_NUM_BITS_PER_SAMPLE);

	return DIV_ROUND_UP(n_samples * CMP_MAX_BITS_PER_SAMPLE, 8);
}

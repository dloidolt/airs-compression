/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Compression Encoder Header File
 */

#ifndef CMP_ENCODER_H
#define CMP_ENCODER_H

#include <stdint.h>
#include "../cmp.h"
#include "../common/bitstream_writer.h"

#define CMP_MIN_GOLOMB_PAR 1
#define CMP_MAX_GOLOMB_PAR UINT16_MAX

/* The current plan is to only decode uint16_t values */
#define CMP_NUM_BITS_PER_SAMPLE bitsizeof(uint16_t)


/**
 * @brief Compression encoder state structure
 *
 * @warning This structure MUST NOT be directly manipulated by external code.
 *	Always use the provided API functions to interact with the structure.
 */

struct cmp_encoder {
	enum cmp_encoder_type encoder_type; /** Algorithm used for encoding samples */

	/* Golomb parameters (used only in GOLOMB modes, otherwise ignored) */
	uint32_t g_par;	     /**< Golomb parameter */
	uint32_t g_par_log2; /**< Precomputed log2(Golomb parameter) for performance */
	uint32_t outlier;    /**< Threshold value for encoding outliers */
};


/**
 * @brief Initialize a compression encoder
 *
 * Sets up the encoder structure with the provided compression parameters and
 * bitstream writer.
 *
 * @param enc		Pointer to the encoder structure to initialize
 * @param encoder_type	Type of encoder to use
 * @param encoder_param	Parameter specific to the chosen encoder_type
 * @param outlier	Outlier parameter needed for CMP_ENCODER_GOLOMB_MULTI
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

uint32_t cmp_encoder_init(struct cmp_encoder *enc, enum cmp_encoder_type encoder_type,
			  uint32_t encoder_param, uint32_t outlier);


/**
 * @brief Encode a 16-bit signed sample
 *
 * @param enc		Pointer to a successful initialised encoder structure
 * @param value		16-bit signed sample to encode
 * @param bs		Pointer to a bitstream writer; must be initialised and
 *			provided by the caller
 *
 * @note The caller is responsible for flushing the bitstream when encoding is
 *       complete to ensure all buffered bits are written. This function can
 *       only fail if the bitstream is small than cmp_compress_bound(), checking
 *       for this can be done with bitstream_error() or bitstream_flush().
 */

void cmp_encoder_encode_s16(const struct cmp_encoder *enc, int16_t value,
			    struct bitstream_writer *bs);


/**
 * @brief Checks if the given encoder type and parameter are valid
 *
 * @param encoder_type	Encoder type to check
 * @param encoder_param	Parameter for the encoder
 * @param outlier	Outlier parameter needed for CMP_ENCODER_GOLOMB_MULTI
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

uint32_t cmp_encoder_params_check(enum cmp_encoder_type encoder_type, uint32_t encoder_param,
				  uint32_t outlier);


/**
 * @brief Calculates the maximum worst cased compressed size
 *
 * @param size	Size of the data uncompressed
 *
 * @returns maximum possible compressed size in bytes, can be larger than the
 *	maximum values that can be stored in the compressed size header field
 */

uint64_t cmp_encoder_max_compressed_size(uint32_t size);


#endif /* CMP_ENCODER_H */

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
#include "bitstream_write.h"
#include "../cmp.h"

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
	struct bitstream_writer *bs; /**< Pointer to a bitstream write */
	enum cmp_mode mode;	     /** Algorithm used for encoding samples  */

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
 * @param params	Pointer to compression parameters
 * @param bs		Pointer to a bitstream writer; must be initialized and
 *			provided by the caller
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

uint32_t cmp_encoder_init(struct cmp_encoder *enc, const struct cmp_params *params,
			  struct bitstream_writer *bs);


/**
 * @brief Encode a 16-bit signed sample
 *
 * @param enc		Pointer to initialized encoder structure
 * @param value		16-bit signed sample to encode
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

uint32_t cmp_encoder_encode_s16(struct cmp_encoder *enc, int16_t value);


/**
 * @brief Finalizes the encoding process
 *
 * This function should be called after all samples have been encoded.
 *
 * @param enc Pointer to the cmp_encoder structure.
 *
 * @returns size of the bitstream on success, an error code, which can be
 *	checked using cmp_is_error()  on failure
 */

uint32_t cmp_encoder_finish(struct cmp_encoder *enc);


#endif /* CMP_ENCODER_H */

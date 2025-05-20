/**
 * @mainpage  AIRS PortAble Compression Engine (AIRSPACE)
 * Please see @ref cmp.h for the compression API documentation.
 *
 * @file cmp.h
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Data Compression API
 *
 * - Setup: Create a compression context with cmp_initialise()
 * - Process: Compress data using cmp_compress_u16()
 * - Reset compression context using cmp_reset()
 * - Clean-up: Optionally destroy context with cmp_deinitialise()
 *
 * @see @ref examples/ directory for usage examples
 *
 * @warning The interface is not frozen yet and may change in future versions.
 * Changes may include:
 *    - New compression parameters
 *    - Change/Extended API functionality
 */

#ifndef CMP_H
#define CMP_H

/* ======   Dependency   ====== */
#include <stdint.h>

/* ====== Utility Macros  ====== */
/** Convert a token to a string literal */
#define CMP_QUOTE(str) #str
/** Expand a macro and convert the result to a string literal */
#define CMP_EXPAND_AND_QUOTE(str) CMP_QUOTE(str)

/* ====== Version Information ====== */
#define CMP_VERSION_MAJOR 0   /**< major part of the version ID */
#define CMP_VERSION_MINOR 2   /**< minor part of the version ID */
#define CMP_VERSION_RELEASE 0 /**< release part of the version ID */

/**
 * @brief complete version number
 */
#define CMP_VERSION_NUMBER \
	(CMP_VERSION_MAJOR * 100 * 100 + CMP_VERSION_MINOR * 100 + CMP_VERSION_RELEASE)

/**
 * @brief complete version string
 */
#define CMP_VERSION_STRING \
	CMP_EXPAND_AND_QUOTE(CMP_VERSION_MAJOR.CMP_VERSION_MINOR.CMP_VERSION_RELEASE)

/* ====== Parameter Selection ====== */
/**
 * @brief Preprocessing techniques for encoding
 *
 * This enum defines the available preprocessing methods that can be applied
 * before encoding. Preprocessing techniques aim to improve compression
 * efficiency.
 */

enum cmp_preprocessing {
	CMP_PREPROCESS_NONE, /**< No preprocessing is applied to the data */
	CMP_PREPROCESS_DIFF, /**< Differences between neighbouring values are computed */
	CMP_PREPROCESS_IWT,  /**< Integer Wavelet Transform preprocessing */
	CMP_PREPROCESS_MODEL /**< Subtracts a model based on previously compressed data */
};


/**
 * @brief available compression encoders
 */

enum cmp_encoder_type {
	CMP_ENCODER_UNCOMPRESSED, /**< Uncompressed mode */
	CMP_ENCODER_GOLOMB_ZERO	  /**< Golomb encoder with zero escape mechanism */
};


/**
 * @brief parameters used for compression
 *
 * @warning number and names of the compression parameters are TBD
 */

struct cmp_params {
	/* Preprocessing Settings */
	enum cmp_preprocessing primary_preprocessing; /**< Preprocessing applied on the first pass */
	enum cmp_preprocessing
		secondary_preprocessing; /**< Preprocessing applied on subsequent passes */
	uint32_t secondary_iterations; /**< Maximum repeats for secondary preprocessing (0 disables secondary_preprocessing) */
	uint32_t model_rate; /**< Rate at which the model adapts during model-based preprocessing */

	/* Data Encoding Settings */
	enum cmp_encoder_type encoder_type; /**< Compression encoder type */
	uint32_t encoder_param;		    /**< Encoder parameter */
};


/**
 * @brief compression context
 *
 * This structure maintains the state of an ongoing compression process.
 *
 * @warning This structure MUST NOT be directly manipulated by external code.
 *	Always use the provided API functions to interact with the compression
 *	context.
 */

struct cmp_context {
	struct cmp_params params; /**< Compression parameters used in the current context */
	void *work_buf;		  /**< Pointer to the working buffer */
	uint32_t work_buf_size;	  /**< Size of the working buffer in bytes */
	uint32_t model_size;	  /**< Size of the model used in the model-based preprocessing */
	uint64_t identifier;	  /**< Identifier for the compression model */
	uint8_t sequence_number;  /**< Number of compression passes performed since the last reset */
};


/* ======  Setup Functions   ====== */
/**
 * @brief Sets a custom function to retrieve the current timestamp.
 *
 * This function allows to specify a custom function that returns a current
 * 48-bit timestamp. This timestamp is used by the compression for example, to
 * generate unique model identifiers.
 *
 * @param get_current_timestamp_func	pointer to a function returning a 48-bit
 *					timestamp; if NULL, a fallback function
 *					will be set
 */

void cmp_set_timestamp_func(uint64_t (*get_current_timestamp_func)(void));


/* ====== Compression Helper Functions ====== */
/**
 * @brief tells if a result is an error code
 *
 * @param code	return value to check
 *
 * @returns non-zero if the code is an error
 */

unsigned int cmp_is_error(uint32_t code);


/**
 * @brief get the maximum compressed size in a worst-case scenario
 *
 * In this scenario the input data are not compressible. This function is
 * primarily useful for memory allocation purposes (destination buffer size).
 *
 * @param size	size of the data to compress
 *
 * @returns the compressed size in the worst-case scenario or an error if the
 *	bound size is larger than the maximum compressed size (CMP_MAX_CMP_SIZE),
 *	which can be checked using cmp_is_error()
 */

uint32_t cmp_compress_bound(uint32_t size);


/**
 * @brief calculates the size needed for the compression working buffer
 *
 * @param params	pointer to a compression parameters struct used to
 *			compress the data
 * @param src_size	size of a source data buffer in bytes
 *
 * @returns the minimum size needed for a compression working buffer (can be 0
 *	is no working buffer is needed) or an error, which can be checked using
 *	cmp_is_error()
 */

uint32_t cmp_cal_work_buf_size(const struct cmp_params *params, uint32_t src_size);


/* ======   Compression Functions   ====== */
/**
 * @brief initialises a compression context
 *
 * @param ctx		pointer to a compression context struct to initialise
 * @param params	pointer to a compression parameters struct used to
 *			compress the data
 * @param work_buf	pointer to a working buffer (can be NULL if not needed)
 * @param work_buf_size	size of the working buffer in bytes; needed size can be
 *			calculated with cmp_cal_work_buf_size(src_size)
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

uint32_t cmp_initialise(struct cmp_context *ctx, const struct cmp_params *params, void *work_buf,
			uint32_t work_buf_size);

/**
 * @brief compresses an unsigned 16-bit data buffer
 *
 * You can use this function repeatedly to compress more data of the same size.
 *
 * @param ctx		pointer to a compression context; must have been
 *			initialised once with cmp_initialise()
 * @param dst		the buffer to compress the src buffers into
 * @param dst_capacity	size of the dst buffer; may be any size, but
 *			cmp_compress_bound(src_size) is guaranteed to be large
 *			enough
 * @param src		pointer to the data to compress
 * @param src_size	size of the data to compress, must be the same for every
 *			source buffer until the context is reset
 *
 * @returns the compressed size or an error, which can be checked using
 *	cmp_is_error()
 */

uint32_t cmp_compress_u16(struct cmp_context *ctx, void *dst, uint32_t dst_capacity,
			  const uint16_t *src, uint32_t src_size);


/**
 * @brief resets the compression context
 *
 * @param ctx	pointer to a compression context to reset
 *
 * @returns an error code, which can be checked using cmp_is_error()
 */

uint32_t cmp_reset(struct cmp_context *ctx);


/**
 * @brief destroys a compression context
 *
 * This function is optional and can be used if the context is no longer needed.
 *
 * @param ctx	pointer to the compression context to be destroyed
 */

void cmp_deinitialise(struct cmp_context *ctx);


#endif /* CMP_H */

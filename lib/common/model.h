/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2026
 * @copyright GPL-2.0
 *
 * @brief Model preprocessing update calculation
 */

#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>

#include "../cmp.h"
#include "compiler.h"


/** Maximum allowed model adaptation rate parameter  */
enum { CMP_MAX_MODEL_RATE = 16 };


static __inline int16_t update_model_16(int32_t data, int32_t model, int model_rate)
{
#define MODEL_SHIFT_BITS 4
	compile_time_assert(CMP_MAX_MODEL_RATE == 1 << MODEL_SHIFT_BITS,
			    _CMP_MAX_MODEL_RATE_MODEL_SHIFT_BITS_mismatch);
	return (int16_t)(((model * model_rate) + (data * (CMP_MAX_MODEL_RATE - model_rate))) >>
			 MODEL_SHIFT_BITS);
}


static __inline int16_t update_model(int16_t data, int16_t model, int model_rate,
				     enum cmp_type dtype)
{
	switch (dtype) {
	case CMP_I16:
	case CMP_I16_IN_I32:
		return update_model_16(data, model, model_rate);
	case CMP_U16:
	case CMP_RAW12:
	default:
		return update_model_16((uint16_t)data, (uint16_t)model, model_rate);
	}
}

#endif /* MODEL_H */

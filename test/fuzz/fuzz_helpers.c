/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Helper functions for fuzzing implementation
 */

#include <stdlib.h>
#include <stdint.h>

#include "fuzz_helpers.h"


void *FUZZ_malloc(size_t size)
{
	if (size > 0) {
		void *const mem = malloc(size);

		FUZZ_ASSERT(mem);
		return mem;
	}
	return NULL;
}

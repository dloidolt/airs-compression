/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2026
 *
 * @brief Portable stop watch function
 */

#ifndef CLOCK_H
#define CLOCK_H
#include <stdint.h>

uint64_t clock_get_time(void);
uint64_t clock_get_elapsed_nano(uint64_t start_time);

#endif /* CLOCK_H */

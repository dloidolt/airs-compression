/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Collection of utility functions
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdio.h>


/**
 * @brief checks if the given stream is a console
 *
 * Determines whether the specified standard stream is associated with a
 * terminal (console) or not.
 *
 * @param std_stream	standard stream to be checked (e.g., stdin, stdout, stderr)
 *
 * @returns non-zero if the stream is a terminal, otherwise 0.
 *
 * @warning this function is platform-dependent and may not work on non-POSIX systems.
 */

int util_is_console(FILE *std_stream);


/**
 * @brief forces the stdin stream to be treated as a console. Intended for test purposes.
 */

void util_force_stdin_console(void);


/**
 * @brief forces the stdout stream to be treated as a console. Intended for test purposes.
 */

void util_force_stdout_console(void);

#endif /* UTIL_H */

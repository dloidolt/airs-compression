/*
 * @file   util.h
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 *
 * @copyright GPLv2
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * @brief collection of utility functions
 */

#ifndef UTIL_H
#define UTIL_H

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>


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

void util_force_stdin_consol(void);


/**
 * @brief forces the stdout stream to be treated as a console. Intended for test purposes.
 */

void util_force_stdout_consol(void);


struct hr_fmt {
	double value;
	int precision;
	const char *suffix;
};

/**
 * @brief converts a size in bytes into a human-readable format
 *
 * This function takes a size in bytes and prepares components for
 * pretty-printing it in a scaled way. The returned components are meant to be
 * passed in precision, value, and suffix order to a "%.*f%s" format string.
 * Example:
 *   struct hr_fmt hrs = util_make_human_readable(1<<10, 0);
 *   printf("%.*f%s\n", hrs.precision, hrs.value, hrs.suffix);
 *   >>> 1.000 KiB
 *
 * @param size		size in bytes to be converted
 * @param verbose	if non-zero, the function outputs a detailed verbose
 *			format without scaling down the size, except for very
 *			large values
 *
 * @returns a struct containing the scaled value, precision, and the suffix
 *	string for the size.
 */

struct hr_fmt util_make_human_readable(uint64_t size, int verbose);

#endif /* UTIL_H */

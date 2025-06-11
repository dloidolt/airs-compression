/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Utility functions implementation
 *
 * @see Inspired by the Zstandard open-source project, see zstd/programs/util.c.
 */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "util.h"

/**
 * @brief check if the stream is associated with a console
 */
#define UTIL_IS_CONSOLE(stdStream) isatty(fileno(stdStream))

/**  Flag to force stdin as a console */
static int g_force_stdin_console;

/**  Flag to force stdout as a console */
static int g_force_stdout_console;


void util_force_stdin_console(void)
{
	g_force_stdin_console = 1;
}


void util_force_stdout_console(void)
{
	g_force_stdout_console = 1;
}

int util_is_console(FILE *std_stream)
{
	assert(std_stream);

	if (g_force_stdin_console && std_stream == stdin)
		return 1;
	if (g_force_stdout_console && std_stream == stdout)
		return 1;

	return UTIL_IS_CONSOLE(std_stream);
}


struct hr_fmt util_make_human_readable(uint64_t size, int verbose)
{
	struct hr_fmt hrs;

	if (verbose) {
		/*
		 * In verbose mode, do not scale sizes down, except in the case of
		 * values that exceed the integral precision of a double.
		 */
		if (size >= (1ull << 53)) {
			hrs.value = (double)size / (1ull << 20);
			hrs.suffix = " MiB";
			/*
			 * At worst, a double representation of a maximal size will be
			 * accurate to better than tens of kilobytes.
			 */
			hrs.precision = 2;
		} else {
			hrs.value = (double)size;
			hrs.suffix = " B";
			hrs.precision = 0;
		}
	} else {
		/* In regular mode, scale sizes down and use suffixes. */
		if (size >= (1ull << 60)) {
			hrs.value = (double)size / (1ull << 60);
			hrs.suffix = " EiB";
		} else if (size >= (1ull << 50)) {
			hrs.value = (double)size / (1ull << 50);
			hrs.suffix = " PiB";
		} else if (size >= (1ull << 40)) {
			hrs.value = (double)size / (1ull << 40);
			hrs.suffix = " TiB";
		} else if (size >= (1ull << 30)) {
			hrs.value = (double)size / (1ull << 30);
			hrs.suffix = " GiB";
		} else if (size >= (1ull << 20)) {
			hrs.value = (double)size / (1ull << 20);
			hrs.suffix = " MiB";
		} else if (size >= (1ull << 10)) {
			hrs.value = (double)size / (1ull << 10);
			hrs.suffix = " KiB";
		} else {
			hrs.value = (double)size;
			hrs.suffix = " B";
		}

		if (hrs.value >= 100 || (uint64_t)hrs.value == size)
			hrs.precision = 0;
		else if (hrs.value >= 10)
			hrs.precision = 1;
		else if (hrs.value > 1)
			hrs.precision = 2;
		else
			hrs.precision = 3;
	}

	return hrs;
}

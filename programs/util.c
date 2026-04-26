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

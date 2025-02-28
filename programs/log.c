/**
 * @file   log.c
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
 */

#include <stdlib.h>
#include <limits.h>

#include "log.h"
#include "util.h"

/**
 * Holds a logging configuration
 */
struct log_state {
	enum log_level current_level;
	enum log_color_status color_status;
};

/**
 * This global variable holds the current configuration for logging.
 */
struct log_state g_log_state = { LOG_LEVEL_DEFAULT, LOG_COLOR_DEFAULT };


void log_setup_color(void)
{
#if defined(_WIN32) || defined(_WIN64)
	/* Windows gets no color! */
	log_state.color_status = LOG_COLOR_DISABLED;
#else
	const char *no_color = getenv("NO_COLOR");
	const char *force_color = getenv("CLICOLOR_FORCE");
	const char *cli_color = getenv("CLICOLOR");

	if (no_color != NULL && no_color[0] != '\0') {
		g_log_state.color_status = LOG_COLOR_DISABLED;
		return;
	}

	if (force_color != NULL && force_color[0] != '\0') {
		g_log_state.color_status = LOG_COLOR_ENABLED;
		return;
	}

	if (cli_color != NULL && cli_color[0] == '0') {
		g_log_state.color_status = LOG_COLOR_DISABLED;
		return;
	}

	if (UTIL_IS_CONSOLE(LOG_STREAM))
		g_log_state.color_status = LOG_COLOR_ENABLED;
	else
		g_log_state.color_status = LOG_COLOR_DISABLED;
#endif
}


void log_increase_verbosity(void)
{
	if (g_log_state.current_level < LOG_LEVEL_MAX)
		g_log_state.current_level++;
}


void log_decrease_verbosity(void)
{
	if (g_log_state.current_level > LOG_LEVEL_QUIET)
		g_log_state.current_level--;
}


void log_set_level(enum log_level level)
{
	g_log_state.current_level = level;
}


enum log_level log_get_level(void)
{
	return g_log_state.current_level;
}


void log_set_color(enum log_color_status status)
{
	g_log_state.color_status = status;
}


enum log_color_status log_get_color(void)
{
	return g_log_state.color_status;
}

/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Logging functions for various log levels with color support.
 *
 * A flexible logging interface with configurable verbosity levels and optional
 * color support for terminal output. It is designed to be lightweight and easy
 * to integrate.
 *
 * Color support can be enabled or disabled at runtime. When enabled, different
 * log levels will be displayed with distinct colors to improve readability.
 *
 * @warning this is not thread safe
 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <errno.h>

#include "../lib/cmp_errors.h"

/**
 * The log levels determine which messages will be displayed based on the current
 * verbosity setting.
 */
enum log_level {
	LOG_LEVEL_QUIET = 0, /**< Suppresses all output */
	LOG_LEVEL_ERROR,     /**< Only errors are shown */
	LOG_LEVEL_WARNING,   /**< Errors and warnings are shown */
	LOG_LEVEL_INFO,      /**< Errors, warnings, and info messages are shown */
	LOG_LEVEL_DEBUG,     /**< All messages except trace are shown */
	LOG_LEVEL_MAX        /**< All messages including trace are shown */
};
#define LOG_LEVEL_DEFAULT LOG_LEVEL_INFO

/**
 * When enabled, log messages will use ANSI color codes to highlight different
 * message types in terminal output.
 */
enum log_color_status { LOG_COLOR_DISABLED = 0, LOG_COLOR_ENABLED };
#define LOG_COLOR_DEFAULT LOG_COLOR_DISABLED

#define LOG_COLOR_ERROR   (log_get_color() ? "\033[1;31m" : "")
#define LOG_COLOR_WARNING (log_get_color() ? "\033[1;33m" : "")
#define LOG_COLOR_INFO    (log_get_color() ? "\033[1;34m" : "")
#define LOG_COLOR_GRAY    (log_get_color() ? "\033[1;30m" : "")
#define LOG_COLOR_DEBUG   LOG_COLOR_GRAY
#define LOG_COLOR_TRACE   LOG_COLOR_GRAY
#define LOG_COLOR_RESET   (log_get_color() ? "\033[0m" : "")


__extension__
#define LOG_F(stream, ...) fprintf(stream, __VA_ARGS__)
#define LOG_STDOUT(...)    LOG_F(stdout, __VA_ARGS__)
#define LOG_STREAM         stderr
#define LOG_STDERR(...)    LOG_F(LOG_STREAM, __VA_ARGS__)
#define LOG_PLAIN(level, ...)                    \
	do {                                     \
		if (log_get_level() >= (level))  \
			LOG_STDERR(__VA_ARGS__); \
	} while (0)

#define LOG_PREFIX_NAME "airspace"
#define LOG_PREFIX(level, level_name, color, ...)                                    \
	do {                                                                         \
		LOG_PLAIN(level, "%s: %s%s%s: ", LOG_PREFIX_NAME, color, level_name, \
			  LOG_COLOR_RESET);                                          \
		if (log_get_level() >= LOG_LEVEL_MAX)                                \
			LOG_PLAIN(level, "%s:%d: ", __FILE__, __LINE__);             \
		LOG_PLAIN(level, __VA_ARGS__);                                       \
	} while (0)

#define LOG_MSG(level, level_name, color, ...)                     \
	do {                                                       \
		LOG_PREFIX(level, level_name, color, __VA_ARGS__); \
		LOG_PLAIN(level, "\n");                            \
	} while (0)

/* Logging functions for various log levels with color support. */
#define LOG_ERROR(...)   LOG_MSG(LOG_LEVEL_ERROR, "error", LOG_COLOR_ERROR, __VA_ARGS__)
#define LOG_WARNING(...) LOG_MSG(LOG_LEVEL_WARNING, "warning", LOG_COLOR_WARNING, __VA_ARGS__)
#define LOG_INFO(...)    LOG_MSG(LOG_LEVEL_WARNING, "info", LOG_COLOR_WARNING, __VA_ARGS__)
#define LOG_DEBUG(...)   LOG_MSG(LOG_LEVEL_DEBUG, "debug", LOG_COLOR_GRAY, __VA_ARGS__)
#define LOG_TRACE(...)   LOG_MSG(LOG_LEVEL_MAX, "trace", LOG_COLOR_TRACE, __VA_ARGS__)

/**
 * @brief logs an error message with errno information
 * @param ... format string and arguments for the message
 */
#define LOG_ERROR_WITH_ERRNO(...)                                                            \
	do {                                                                                 \
		LOG_PREFIX(LOG_LEVEL_ERROR, "error", LOG_COLOR_ERROR, __VA_ARGS__);          \
		LOG_PLAIN(LOG_LEVEL_ERROR, ": %s (os error: %d)\n", strerror(errno), errno); \
	} while (0)

/**
 * @brief logs an error with CMP library error information
 * @param cmp_ret_val	return value from a (de)compression library function
 * @param ...		format string and arguments for the message
 */
#define LOG_ERROR_CMP(cmp_ret_val, ...)                                                            \
	do {                                                                                       \
		LOG_PREFIX(LOG_LEVEL_ERROR, "error", LOG_COLOR_ERROR, __VA_ARGS__);                \
		LOG_PLAIN(LOG_LEVEL_ERROR, "%s (error: %d)\n", cmp_get_error_message(cmp_ret_val), \
			  cmp_get_error_code(cmp_ret_val));                                        \
	} while (0)


/** @brief increases the verbosity level by one step */
void log_increase_verbosity(void);

/** @brief decreases the verbosity level by one step */
void log_decrease_verbosity(void);

/**
 * @brief Set the current log verbosity level
 *
 * @param level		new log level to set
 */
void log_set_level(enum log_level level);

/**
 * @brief get the current log verbosity level
 *
 * @returns the current log level
 */
enum log_level log_get_level(void);

/**
 * @brief configure color output based on environment
 *
 * Detects terminal capabilities and environment variables to determine whether
 * to enable colored output.
 */
void log_setup_color(void);

/**
 * @brief enable or disable colored log output
 *
 * @param status LOG_COLOR_ENABLED or LOG_COLOR_DISABLED
 */
void log_set_color(enum log_color_status status);

/**
 * @brief Get the current color status
 *
 * @return enum log_color_status The current color status
 */
enum log_color_status log_get_color(void);

#endif /* LOG_H */

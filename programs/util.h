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

#define UTIL_IS_CONSOLE(stdStream) isatty(fileno(stdStream))

#endif /* UTIL_H */

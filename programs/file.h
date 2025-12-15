/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Handles file I/O
 */

#ifndef FILE_H
#define FILE_H

#include <stddef.h>
#include <stdint.h>

#include "../lib/cmp.h"


#define STD_OUT_MARK "//*-stdout-*//" /**< Marker for output redirection to standard output */
#define STD_IN_MARK  "//*-stdin-*//"  /**< Marker for input redirection from standard input */
#define NULL_MARK    "/dev/null"      /**< Marker for null output (discarding data) */

int file_get_size_u32(const char *filename, uint32_t *file_size32);
int file_load(const char *filename, void *buffer, size_t buffer_size);
int file_save_be16(const char *filename, uint16_t *buffer, size_t size);

uint32_t file_compress(struct cmp_context *ctx, const char *dst_filename, const char *src_filename);

#endif /* FILE_H */

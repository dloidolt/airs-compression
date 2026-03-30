/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Platform-independent file I/O
 */

#ifndef FILE_H
#define FILE_H

#include <stdint.h>

#include "os.h"
#define STR_SLICE_API static __inline
#include "str_slice.h"
#include "arena.h"

static const struct s8 STD_IN_MARK_S8 = S8(STD_IN_MARK);
static const struct s8 STD_OUT_MARK_S8 = S8(STD_OUT_MARK);
static const struct s8 STD_ERR_MARK_S8 = S8(STD_ERR_MARK);

/**
 * @brief reads a file into the arena
 *
 * @param perm	arena to read into
 * @param path	file path or STD_IN_MARK
 *
 * @returns an os_load; on success .status == OS_OK,  On any error .status is non-zero.
 *
 * @warning The returned structure and all data buffers are allocated from the
 *	provided arena and remain valid only as long as the provided arena is
 *	not reset or destroyed.
 */
struct os_load file_read(struct arena *perm, struct s8 path);

/** @brief same as file_read() but reads data as big-endian uint16 values  */
struct os_load file_read_be16(struct arena *perm, struct s8 path);


/**
 * @brief saves data to a file
 *
 * Does nothing and returns success when path equals NULL_MARK.
 *
 * @returns 0 on success, otherwise error
 */
int file_write(struct arena scratch, struct s8 path, const void *buf, uint32_t buf_size);

/** @brief same as file_write() but saves the data as big-endian uint16 values  */
int file_save_be16(struct arena scratch, struct s8 path, const uint16_t *buf, uint32_t buf_size);


/**
 * @brief checks if the given stream is connected to a terminal
 *
 * @param stream_mark	stream to test (e.g., STD_IN_MARK, STD_OUT_MARK, STD_ERR_MARK)
 *
 * @returns non-zero if the stream is a terminal, 0 otherwise
 */
int file_is_console(struct s8 stream_mark);

/** @brief forces stdin to be treated as a console. Intended for testing. */
void file_force_stdin_console(void);

/** @brief forces stdout to be treated as a console. Intended for testing. */
void file_force_stdout_console(void);

#endif /* FILE_H */

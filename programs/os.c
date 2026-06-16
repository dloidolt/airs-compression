/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2026
 * @copyright GPL-2.0
 *
 * @brief Platform-specific implementations
 *
 * Currently only POSIX is implemented
 */

#include "os.h"

/*
 * POSIX platform implementation
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>


struct os_load os_read(const char *path, void *buf, uint32_t buf_size)
{
	FILE *fp;
	size_t bytes_read;
	struct os_load r = { 0 };

	assert(path);
	assert(buf);

	if (!strcmp(path, STD_IN_MARK)) {
		fp = stdin;
	} else {
		fp = fopen(path, "rb");
		if (!fp) {
			r.status = OS_CANTOPEN;
			return r;
		}
	}

	bytes_read = fread(buf, 1, buf_size, fp);
	assert(bytes_read <= buf_size);
	if (bytes_read == buf_size) {
		int c = fgetc(fp);

		if (c != EOF) {
			r.status = OS_TOOBIG;
			(void)ungetc(c, fp); /* put the byte back */
		}
	}

	if (ferror(fp) && r.status == OS_OK)
		r.status = OS_IOERR;

	if (fp != stdin)
		if (fclose(fp) && r.status == OS_OK)
			r.status = OS_CANTCLOSE;

	r.buffer = buf;
	assert(bytes_read <= INT32_MAX);
	r.size = (uint32_t)bytes_read;
	return r;
}


enum os_status os_save_from_buffer(const char *path, const void *buf, uint32_t buf_size)
{
	FILE *fp;
	size_t bytes_written;
	enum os_status r = OS_OK;

	assert(path);
	assert(buf);

	if (!strcmp(path, STD_OUT_MARK)) {
		fp = stdout;
	} else {
		fp = fopen(path, "wb");
		if (!fp)
			return OS_CANTOPEN;
	}

	bytes_written = fwrite(buf, 1, buf_size, fp);
	if (bytes_written != buf_size)
		r = OS_IOERR;

	if (fp != stdout)
		if (fclose(fp) && r == OS_OK)
			r = OS_CANTCLOSE;

	return r;
}


int os_make_directory(const char *path)
{
	enum { DIR_DEFAULT_MODE = 0777 };
	int r;

	r = mkdir(path, DIR_DEFAULT_MODE);
	if (r != 0 && errno == EEXIST) {
		if (os_is_directory(path)) {
			return 0;
		}
		errno = EEXIST;
	}

	return r;
}


void os_log(const char *msg, uint32_t msg_size)
{
	(void)fwrite(msg, 1, msg_size, stderr);
}


int os_is_console(const char *stream_marker, size_t stream_marker_len)
{
	if (stream_marker_len == sizeof(STD_IN_MARK) - 1 &&
	    !memcmp(stream_marker, STD_IN_MARK, stream_marker_len))
		return isatty(STDIN_FILENO);

	if (stream_marker_len == sizeof(STD_OUT_MARK) - 1 &&
	    !memcmp(stream_marker, STD_OUT_MARK, stream_marker_len))
		return isatty(STDOUT_FILENO);

	if (stream_marker_len == sizeof(STD_ERR_MARK) - 1 &&
	    !memcmp(stream_marker, STD_ERR_MARK, stream_marker_len))
		return isatty(STDERR_FILENO);

	return 0;
}


int os_is_directory(const char *path)
{
	struct stat st;

	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}


int os_directory_is_empty(const char *path)
{
	DIR *dir;
	struct dirent *entry;

	dir = opendir(path);
	if (!dir)
		return 0;

	errno = 0;
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
			(void)closedir(dir);
			return 0;
		}
	}
	(void)closedir(dir);

	if (errno != 0)
		return 0;

	return 1;
}


int os_is_regular_file(const char *path)
{
	struct stat st;

	return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}


int os_remove(const char *path)
{
	return remove(path);
}

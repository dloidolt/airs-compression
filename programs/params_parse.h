/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Parsing and printing functions for the compression parameters
 */

#ifndef PARAMS_PARSE_H
#define PARAMS_PARSE_H

#include <cmp.h>
#include "log.h"
#include "../lib/decompress/arena.h"

enum cmp_parse_status {
	CMP_PARSE_OK = 0,
	CMP_PARSE_EMPTY_STR,
	CMP_PARSE_MISSING_EQUAL,
	CMP_PARSE_INVALID_KEY,
	CMP_PARSE_INVALID_VALUE
};

/**
 * @brief parses a "key=value,key2=value2" string of compression parameters and
 * updates the params struct/
 *
 * @param str		A null-terminated C string containing key-value pairs.
 *			The expected format is "key1=value1,key2=value2,...".
 *			Whitespace around keys, values, '=', and ',' is tolerated.
 *			If the keys are the same, the last one wins.
 * @param params	A pointer to a 'struct cmp_params' to be populated with the
 *			parsed values. Must not be NULL. no change when not
 *			parsed
 *
 * @returns enum cmp_parse_status
 *   - CMP_PARSE_OK		on success
 *   - CMP_PARSE_EMPTY_STR	if the string contains no key=value pairs
 *   - CMP_PARSE_MISSING_EQUAL	if a pair is missing '='
 *   - CMP_PARSE_INVALID_KEY	if a key is unknown
 *   - CMP_PARSE_INVALID_VALUE	if a value is malformed or not allowed for the field
 *
 * @note This function stops at the first error encountered.
 */
enum cmp_parse_status cmp_params_parse(const char *str, struct cmp_params *params);

/**
 * @brief serializes a struct cmp_params into a human-readable string
 *
 * @param perm	pointer to an arena used to build the output string
 * @param par	pointer to the compression values to stringify
 *
 * @returns a pointer to a NUL-terminated string allocated within 'perm'
 */
const char *cmp_params_to_string(struct arena *perm, const struct cmp_params *par);

#endif /* PARAMS_PARSE_H */

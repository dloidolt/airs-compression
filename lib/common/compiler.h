/**
 * @file
 * @author Dominik Loidolt (dominik.loidolt@univie.ac.at)
 * @date   2025
 * @copyright GPL-2.0
 *
 * @brief Compatibility utility header for different compilers.
 *
 * @see This is based on the macros from the git source code, see:
 *	git-compat-util.h by @author Linus Torvalds et al.
 */

#ifndef COMPILER_H
#define COMPILER_H

/**
 * @brief Convenience macros to test the versions of gcc (or a compatible compiler)
 *
 * Derived from Linux "Features Test Macro" header
 * Use them like this:
 *	#if GIT_GNUC_PREREQ (2,8)
 *	... code requiring gcc 2.8 or later ...
 *	#endif
 */
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#  define CMP_GNUC_PREREQ(maj, min) ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#  define CMP_GNUC_PREREQ(maj, min) 0
#endif

/**
 * @brief a C89 compile time assertion mechanism
 */
#define compile_time_assert(cond, msg) UNUSED typedef char ASSERT_##msg[(cond) ? 1 : -1]

/**
 * @brieg assert a build-time dependency, as an expression.
 *
 * @param cond	the compile-time condition which must be true
 *
 * Your compile will fail if the condition isn't true, or can't be evaluated
 * by the compiler. This can be used in an expression: its value is "0".
 *
 * Example:
 *	#define foo_to_char(foo)					\
 *		 ((char *)(foo)						\
 *		  + BUILD_ASSERT_OR_ZERO(offsetof(struct foo, string) == 0))
 */
#define BUILD_ASSERT_OR_ZERO(cond) (sizeof(char[1 - 2 * !(cond)]) - 1)


/**
 * @brief Asserts that a variable is an array, not a pointer
 * @param arr The variable to check.
 */

#if CMP_GNUC_PREREQ(3, 1)
/* &arr[0] degrades to a pointer: a different type from an array */
#  define BARF_UNLESS_AN_ARRAY(arr) \
	  BUILD_ASSERT_OR_ZERO(     \
		  !__builtin_types_compatible_p(__typeof__(arr), __typeof__(&(arr)[0])))
#else
#  define BARF_UNLESS_AN_ARRAY(arr) 0
#endif


/**
 * @brief get the number of elements in a visible array
 *
 * @param x	the array whose size you want
 *
 * This does not work on pointers, or arrays declared as [], or
 * function parameters.  With correct compiler support, such usage
 * will cause a build error (see the BUILD_ASSERT_OR_ZERO macro).
 */
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]) + BARF_UNLESS_AN_ARRAY(x))

/**
 * We assume that a byte as 8 bits
 */
#define bitsizeof(x) (8 * sizeof(x))

#define maximum_signed_value_of_type(a) (INTMAX_MAX >> (bitsizeof(intmax_t) - bitsizeof(a)))
#define maximum_unsigned_value_of_type(a) (UINTMAX_MAX >> (bitsizeof(uintmax_t) - bitsizeof(a)))

/**
 * @brief marks a function parameter that is always unused.
 *
 * It also can be used to annotate a function, a variable, or a type that is
 * always unused.
 *
 * A callback interface may dictate that a function accepts a
 * parameter at that position, but the implementation of the function
 * may not need to use the parameter.  In such a case, mark the parameter
 * with UNUSED.
 *
 * When a parameter may be used or unused, depending on conditional
 * compilation, consider using MAYBE_UNUSED instead.
 */

#if CMP_GNUC_PREREQ(4, 5) || defined(__clang__)
#  define UNUSED __attribute__((unused)) __attribute__((deprecated("parameter declared as UNUSED")))
#elif defined(__GNUC__)
#  define UNUSED __attribute__((unused)) __attribute__((deprecated))
#else
#  define UNUSED
#endif


/**
 * @brief marks a function parameter that may be unused, but whose use is not an
 * error.
 *
 * It also can be used to annotate a function, a variable, or a type that may be
 * unused.
 *
 * Depending on a configuration, all uses of such a thing may become
 * #ifdef'ed away.  Marking it with UNUSED would give a warning in a
 * compilation where it is indeed used, and not marking it at all
 * would give a warning in a compilation where it is unused.  In such
 * a case, MAYBE_UNUSED is the appropriate annotation to use.
 */

#define MAYBE_UNUSED __attribute__((__unused__))


/**
 * @brief Defines an aligned type
 *
 * @code{.c}
 * ALIGNED_TYPE(16, uint16_t) aligned_int = 42;
 * ALIGNED_TYPE(32, uint8_t) aligned_array[128];
 *
 * // The default-alignment equivalent would be
 * uint16_t aligned_int = 42;
 * uint8_t aligned_array[128];
 * @endcode
 * @see FFmpeg DECLARE_ALIGNED in libavutil/mem_internal.h
 *
 * @param n Minimum alignment in bytes
 * @param t Type
 */

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#  define ALIGNED_TYPE(n, t) _Alignas((n)) t
#elif defined(__INTEL_COMPILER) && __INTEL_COMPILER < 1110 || defined(__SUNPRO_C)
#  define ALIGNED_TYPE(n, t) t __attribute__((aligned(n)))
#elif defined(__GNUC__) || defined(__clang__)
#  define ALIGNED_TYPE(n, t) t __attribute__((aligned(n)))
#elif defined(_MSC_VER)
#  define ALIGNED_TYPE(n, t) __declspec(align(n)) t
#else
#  warning "ALIGNED_TYPE: Compiler does not support explicit alignment. Array may not be aligned."
#  define ALIGNED_TYPE(n, t) t
#endif

#endif /*  COMPILER_H */

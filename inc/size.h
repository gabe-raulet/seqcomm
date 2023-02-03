#ifndef SIZE_H_
#define SIZE_H_

#include <limits.h>
#include <stdint.h>

/*
 * Force size_t to be what we want (unsigned 64-bit integer).
 * Otherwise, refuse to compile.
 */
#if !defined(SIZE_MAX) || !defined(ULONG_MAX) || !defined(UINT64_MAX)
#error "one or more of {SIZE_MAX, ULONG_MAX, UINT64_MAX} is/are not defined"
#endif

#if (SIZE_MAX != ULONG_MAX) || (SIZE_MAX != UINT64_MAX)
#error "size_t must be unsigned long (unsigned 64-bit integer type)"
#endif

/*
 * Use this to force any signed/unsigned integer type into int
 * without overflow.
 */
#define safe_int(x) ((x) <= INT_MAX? (x) : INT_MAX)
#define is_safe_int(x) ((x) <= INT_MAX)

/*
 * Gets the next power-of-two greater than or equal to x (size_t).
 */
static inline size_t next_pow2(size_t x)
{
    if (x != 0)
        x--;

    x |= x>>1;
    x |= x>>2;
    x |= x>>4;
    x |= x>>8;
    x |= x>>16;
    x |= x>>32;

    return x+1;
}

#endif

/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include <strings.h>

/**
 * @brief Compares two strings without regard to the case of ASCII letters.
 *
 * @param[in] aIn_pFirst  The first string.
 * @param[in] aIn_pSecond The second string.
 * @return The sign of the first difference, or 0.
 */
int
strcasecmp(const char* aIn_pFirst, const char* aIn_pSecond)
{
        return (strncasecmp(aIn_pFirst, aIn_pSecond, (size_t) -1));
}

/**
 * @brief Compares at most a given number of bytes of two strings without regard to case.
 *
 * @param[in] aIn_pFirst  The first string.
 * @param[in] aIn_pSecond The second string.
 * @param[in] aCount      The most bytes to compare.
 * @return The sign of the first difference, or 0.
 */
int
strncasecmp(const char* aIn_pFirst, const char* aIn_pSecond, size_t aCount)
{
        for(size_t index = 0; index < aCount; index++)
        {
                int first  = tolower((unsigned char) aIn_pFirst[ index ]);
                int second = tolower((unsigned char) aIn_pSecond[ index ]);

                if(first != second)
                        return (first < second ? -1 : 1);
                if(first == '\0')
                        return (0);
        }
        return (0);
}

/**
 * @brief The position of the lowest set bit, counted from 1.
 *
 * @param[in] aValue The value.
 * @return The position, or 0 when no bit is set.
 */
int
ffs(int aValue)
{
        return (__builtin_ffs(aValue));
}

/**
 * @brief Zeroes bytes in a way the compiler does not remove, for secrets about to go out of scope.
 *
 * @param[out] aOut_pBytes The bytes.
 * @param[in]  aCount      How many.
 */
void
explicit_bzero(void* aOut_pBytes, size_t aCount)
{
        memset(aOut_pBytes, 0, aCount);
        /* The memory clobber tells the compiler the zeroes are read, so the memset stays. */
        __asm__ __volatile__("" : : "r"(aOut_pBytes) : "memory");
}

/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/** What reading an integer found. */
typedef struct Integer
{
        unsigned long long magnitude;
        bool               isNegative;
        bool               didOverflow;
} tInteger;

/**
 * @brief The value of a digit in bases up to 36.
 *
 * @param[in] aCharacter The character.
 * @return The value, or 36 for a character that is no digit.
 */
static int
DigitValue(int aCharacter)
{
        if(isdigit(aCharacter))
                return (aCharacter - '0');
        if(isalpha(aCharacter))
                return (tolower(aCharacter) - 'a' + 10);
        return (36);
}

/**
 * @brief Reads an integer as strtol does: white space, a sign, a base prefix, digits.
 *
 * Base 0 reads 0x as hexadecimal, a leading 0 as octal and anything else as decimal; base 16 also
 * takes 0x. A 0x without a hexadecimal digit after it reads as the digit 0.
 *
 * @param[in]  aIn_pText   The text.
 * @param[out] aOut_ppEnd  Where reading stopped, or NULL: the text itself when no digit was read.
 * @param[in]  aBase       0, or 2 to 36.
 * @param[out] aOut_pValue The magnitude, the sign and whether the magnitude overflowed.
 * @return false when the base is invalid; errno is then EINVAL.
 */
static bool
ReadInteger(const char* aIn_pText, char** aOut_ppEnd, int aBase, tInteger* aOut_pValue)
{
        const unsigned char* pAt = (const unsigned char*) aIn_pText;

        aOut_pValue->magnitude   = 0;
        aOut_pValue->isNegative  = false;
        aOut_pValue->didOverflow = false;
        if(aOut_ppEnd != NULL)
                *aOut_ppEnd = (char*) aIn_pText;
        if(aBase < 0 || aBase == 1 || aBase > 36)
        {
                errno = EINVAL;
                return (false);
        }
        while(isspace(*pAt))
                pAt++;
        if(*pAt == '+' || *pAt == '-')
                aOut_pValue->isNegative = *pAt++ == '-';
        if((aBase == 0 || aBase == 16) && pAt[ 0 ] == '0' && tolower(pAt[ 1 ]) == 'x'
           && DigitValue(pAt[ 2 ]) < 16)
        {
                pAt  += 2;
                aBase = 16;
        } else if(aBase == 0)
                aBase = pAt[ 0 ] == '0' ? 8 : 10;
        const unsigned char* pDigits = pAt;

        for(int digit; (digit = DigitValue(*pAt)) < aBase; pAt++)
        {
                if(aOut_pValue->magnitude > (ULLONG_MAX - (unsigned) digit) / (unsigned) aBase)
                        aOut_pValue->didOverflow = true;
                aOut_pValue->magnitude = aOut_pValue->magnitude * (unsigned) aBase
                        + (unsigned) digit;
        }
        if(pAt == pDigits)
        {
                aOut_pValue->isNegative = false;
                return (true);
        }
        if(aOut_ppEnd != NULL)
                *aOut_ppEnd = (char*) pAt;
        return (true);
}

/**
 * @brief Reads a signed integer and clamps it to a range, setting ERANGE when it does not fit.
 *
 * @param[in]  aIn_pText  The text.
 * @param[out] aOut_ppEnd Where reading stopped, or NULL.
 * @param[in]  aBase      0, or 2 to 36.
 * @param[in]  aMinimum   The lowest value.
 * @param[in]  aMaximum   The highest value.
 * @return The value.
 */
static long long
ReadSigned(const char* aIn_pText,
           char**      aOut_ppEnd,
           int         aBase,
           long long   aMinimum,
           long long   aMaximum)
{
        tInteger value;

        if(!ReadInteger(aIn_pText, aOut_ppEnd, aBase, &value))
                return (0);
        unsigned long long limit = value.isNegative ? 0ULL - (unsigned long long) aMinimum
                                                    : (unsigned long long) aMaximum;

        if(value.didOverflow || value.magnitude > limit)
        {
                errno = ERANGE;
                return (value.isNegative ? aMinimum : aMaximum);
        }
        if(value.isNegative)
                return (value.magnitude == limit ? aMinimum : -(long long) value.magnitude);
        return ((long long) value.magnitude);
}

/**
 * @brief Reads an unsigned integer; a minus sign negates it in the unsigned type, as C says.
 *
 * @param[in]  aIn_pText  The text.
 * @param[out] aOut_ppEnd Where reading stopped, or NULL.
 * @param[in]  aBase      0, or 2 to 36.
 * @param[in]  aMaximum   The highest value; past it, ERANGE and aMaximum.
 * @return The value.
 */
static unsigned long long
ReadUnsigned(const char* aIn_pText, char** aOut_ppEnd, int aBase, unsigned long long aMaximum)
{
        tInteger value;

        if(!ReadInteger(aIn_pText, aOut_ppEnd, aBase, &value))
                return (0);
        if(value.didOverflow || value.magnitude > aMaximum)
        {
                errno = ERANGE;
                return (aMaximum);
        }
        return (value.isNegative ? (0ULL - value.magnitude) & aMaximum : value.magnitude);
}

/**
 * @brief Reads a long.
 *
 * @param[in]  aIn_pText  The text.
 * @param[out] aOut_ppEnd Where reading stopped, or NULL.
 * @param[in]  aBase      0, or 2 to 36.
 * @return The value; LONG_MIN or LONG_MAX with ERANGE when it does not fit.
 */
long
strtol(const char* restrict aIn_pText, char** restrict aOut_ppEnd, int aBase)
{
        return ((long) ReadSigned(aIn_pText, aOut_ppEnd, aBase, LONG_MIN, LONG_MAX));
}

/**
 * @brief Reads a long long.
 *
 * @param[in]  aIn_pText  The text.
 * @param[out] aOut_ppEnd Where reading stopped, or NULL.
 * @param[in]  aBase      0, or 2 to 36.
 * @return The value; LLONG_MIN or LLONG_MAX with ERANGE when it does not fit.
 */
long long
strtoll(const char* restrict aIn_pText, char** restrict aOut_ppEnd, int aBase)
{
        return (ReadSigned(aIn_pText, aOut_ppEnd, aBase, LLONG_MIN, LLONG_MAX));
}

/**
 * @brief Reads an intmax_t.
 *
 * @param[in]  aIn_pText  The text.
 * @param[out] aOut_ppEnd Where reading stopped, or NULL.
 * @param[in]  aBase      0, or 2 to 36.
 * @return The value; INTMAX_MIN or INTMAX_MAX with ERANGE when it does not fit.
 */
intmax_t
strtoimax(const char* restrict aIn_pText, char** restrict aOut_ppEnd, int aBase)
{
        return ((intmax_t) ReadSigned(aIn_pText, aOut_ppEnd, aBase, INTMAX_MIN, INTMAX_MAX));
}

/**
 * @brief Reads an unsigned long.
 *
 * @param[in]  aIn_pText  The text.
 * @param[out] aOut_ppEnd Where reading stopped, or NULL.
 * @param[in]  aBase      0, or 2 to 36.
 * @return The value; ULONG_MAX with ERANGE when it does not fit.
 */
unsigned long
strtoul(const char* restrict aIn_pText, char** restrict aOut_ppEnd, int aBase)
{
        return ((unsigned long) ReadUnsigned(aIn_pText, aOut_ppEnd, aBase, ULONG_MAX));
}

/**
 * @brief Reads an unsigned long long.
 *
 * @param[in]  aIn_pText  The text.
 * @param[out] aOut_ppEnd Where reading stopped, or NULL.
 * @param[in]  aBase      0, or 2 to 36.
 * @return The value; ULLONG_MAX with ERANGE when it does not fit.
 */
unsigned long long
strtoull(const char* restrict aIn_pText, char** restrict aOut_ppEnd, int aBase)
{
        return (ReadUnsigned(aIn_pText, aOut_ppEnd, aBase, ULLONG_MAX));
}

/**
 * @brief Reads a uintmax_t.
 *
 * @param[in]  aIn_pText  The text.
 * @param[out] aOut_ppEnd Where reading stopped, or NULL.
 * @param[in]  aBase      0, or 2 to 36.
 * @return The value; UINTMAX_MAX with ERANGE when it does not fit.
 */
uintmax_t
strtoumax(const char* restrict aIn_pText, char** restrict aOut_ppEnd, int aBase)
{
        return ((uintmax_t) ReadUnsigned(aIn_pText, aOut_ppEnd, aBase, UINTMAX_MAX));
}

/**
 * @brief Reads a decimal int, clamped to its range.
 *
 * @param[in] aIn_pText The text.
 * @return The value.
 */
int
atoi(const char* aIn_pText)
{
        return ((int) ReadSigned(aIn_pText, NULL, 10, INT_MIN, INT_MAX));
}

/**
 * @brief Reads a decimal long.
 *
 * @param[in] aIn_pText The text.
 * @return The value.
 */
long
atol(const char* aIn_pText)
{
        return (strtol(aIn_pText, NULL, 10));
}

/**
 * @brief Reads a decimal long long.
 *
 * @param[in] aIn_pText The text.
 * @return The value.
 */
long long
atoll(const char* aIn_pText)
{
        return (strtoll(aIn_pText, NULL, 10));
}

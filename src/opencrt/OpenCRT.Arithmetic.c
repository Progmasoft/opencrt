/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <inttypes.h>
#include <stdlib.h>

/**
 * @brief The absolute value of an int.
 *
 * @param[in] aValue The value, not INT_MIN.
 * @return The absolute value.
 */
int
abs(int aValue)
{
        return (aValue < 0 ? -aValue : aValue);
}

/**
 * @brief The absolute value of a long.
 *
 * @param[in] aValue The value, not LONG_MIN.
 * @return The absolute value.
 */
long
labs(long aValue)
{
        return (aValue < 0 ? -aValue : aValue);
}

/**
 * @brief The absolute value of a long long.
 *
 * @param[in] aValue The value, not LLONG_MIN.
 * @return The absolute value.
 */
long long
llabs(long long aValue)
{
        return (aValue < 0 ? -aValue : aValue);
}

/**
 * @brief The absolute value of an intmax_t.
 *
 * @param[in] aValue The value, not INTMAX_MIN.
 * @return The absolute value.
 */
intmax_t
imaxabs(intmax_t aValue)
{
        return (aValue < 0 ? -aValue : aValue);
}

/**
 * @brief The quotient, rounded toward zero, and the remainder of two ints.
 *
 * @param[in] aNumerator   The numerator.
 * @param[in] aDenominator The denominator, not 0.
 * @return Both.
 */
div_t
div(int aNumerator, int aDenominator)
{
        div_t result = { aNumerator / aDenominator, aNumerator % aDenominator };

        return (result);
}

/**
 * @brief The quotient and the remainder of two longs.
 *
 * @param[in] aNumerator   The numerator.
 * @param[in] aDenominator The denominator, not 0.
 * @return Both.
 */
ldiv_t
ldiv(long aNumerator, long aDenominator)
{
        ldiv_t result = { aNumerator / aDenominator, aNumerator % aDenominator };

        return (result);
}

/**
 * @brief The quotient and the remainder of two long longs.
 *
 * @param[in] aNumerator   The numerator.
 * @param[in] aDenominator The denominator, not 0.
 * @return Both.
 */
lldiv_t
lldiv(long long aNumerator, long long aDenominator)
{
        lldiv_t result = { aNumerator / aDenominator, aNumerator % aDenominator };

        return (result);
}

/**
 * @brief The quotient and the remainder of two intmax_t.
 *
 * @param[in] aNumerator   The numerator.
 * @param[in] aDenominator The denominator, not 0.
 * @return Both.
 */
imaxdiv_t
imaxdiv(intmax_t aNumerator, intmax_t aDenominator)
{
        imaxdiv_t result = { aNumerator / aDenominator, aNumerator % aDenominator };

        return (result);
}

/** The state of rand: xorshift64*, never zero. */
static unsigned long long gsRandomState = 0x9E3779B97F4A7C15ULL ^ 1;

/**
 * @brief Sets the sequence of rand; the same seed gives the same sequence.
 *
 * @param[in] aSeed The seed.
 */
void
srand(unsigned aSeed)
{
        gsRandomState = 0x9E3779B97F4A7C15ULL ^ aSeed;
}

/**
 * @brief The next value of a sequence for simulations and tests, not for secrets.
 *
 * @return A value from 0 to RAND_MAX.
 */
int
rand(void)
{
        gsRandomState ^= gsRandomState >> 12;
        gsRandomState ^= gsRandomState << 25;
        gsRandomState ^= gsRandomState >> 27;
        return ((int) ((gsRandomState * 0x2545F4914F6CDD1DULL) >> 33));
}

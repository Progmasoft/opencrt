/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

/*
 * The functions of <math.h> whose result is exact, written on the bits of the values: no rounding
 * of their own, so no table and no approximation. sqrt is the instruction, which rounds correctly.
 * As math_errhandling is MATH_ERREXCEPT, none of them sets errno.
 */

/**
 * @brief The bits of a double.
 *
 * @param[in] aValue The value.
 * @return The bits.
 */
static uint64_t
BitsOf(double aValue)
{
        uint64_t bits;

        __builtin_memcpy(&bits, &aValue, sizeof(bits));
        return (bits);
}

/**
 * @brief The double of some bits.
 *
 * @param[in] aBits The bits.
 * @return The value.
 */
static double
DoubleOf(uint64_t aBits)
{
        double value;

        __builtin_memcpy(&value, &aBits, sizeof(value));
        return (value);
}

/**
 * @brief The unbiased exponent of a double, or 1024 for an infinity or a NaN.
 *
 * @param[in] aBits The bits.
 * @return The exponent.
 */
static int
ExponentOf(uint64_t aBits)
{
        return ((int) ((aBits >> 52) & 0x7FF) - 1023);
}

/**
 * @brief The absolute value.
 *
 * @param[in] aValue The value.
 * @return The value without its sign.
 */
double
fabs(double aValue)
{
        return (DoubleOf(BitsOf(aValue) & ~(UINT64_C(1) << 63)));
}

/**
 * @brief The absolute value of a float.
 *
 * @param[in] aValue The value.
 * @return The value without its sign.
 */
float
fabsf(float aValue)
{
        return (aValue < 0 || (aValue == 0 && 1 / aValue < 0) ? -aValue : aValue);
}

/**
 * @brief The absolute value of a long double.
 *
 * @param[in] aValue The value.
 * @return The value without its sign.
 */
long double
fabsl(long double aValue)
{
        return (signbit(aValue) ? -aValue : aValue);
}

/**
 * @brief A magnitude with the sign of another value.
 *
 * @param[in] aMagnitude The magnitude.
 * @param[in] aSign      The value whose sign is taken.
 * @return The result.
 */
double
copysign(double aMagnitude, double aSign)
{
        uint64_t sign = BitsOf(aSign) & (UINT64_C(1) << 63);

        return (DoubleOf((BitsOf(aMagnitude) & ~(UINT64_C(1) << 63)) | sign));
}

/**
 * @brief A float magnitude with the sign of another float.
 *
 * @param[in] aMagnitude The magnitude.
 * @param[in] aSign      The value whose sign is taken.
 * @return The result.
 */
float
copysignf(float aMagnitude, float aSign)
{
        float magnitude = fabsf(aMagnitude);

        return (signbit(aSign) ? -magnitude : magnitude);
}

/**
 * @brief A long double magnitude with the sign of another.
 *
 * @param[in] aMagnitude The magnitude.
 * @param[in] aSign      The value whose sign is taken.
 * @return The result.
 */
long double
copysignl(long double aMagnitude, long double aSign)
{
        long double magnitude = fabsl(aMagnitude);

        return (signbit(aSign) ? -magnitude : magnitude);
}

/**
 * @brief The value with its fraction dropped, toward zero.
 *
 * @param[in] aValue The value.
 * @return The integer, with the sign of the value; an infinity or a NaN as it is.
 */
double
trunc(double aValue)
{
        uint64_t bits     = BitsOf(aValue);
        int      exponent = ExponentOf(bits);

        if(exponent >= 52)
                return (aValue);
        if(exponent < 0)
                return (DoubleOf(bits & (UINT64_C(1) << 63)));
        return (DoubleOf(bits & ~((UINT64_C(1) << (52 - exponent)) - 1)));
}

/**
 * @brief The largest integer not above the value.
 *
 * @param[in] aValue The value.
 * @return The integer.
 */
double
floor(double aValue)
{
        double whole = trunc(aValue);

        return (aValue < 0 && whole != aValue ? whole - 1 : whole);
}

/**
 * @brief The smallest integer not below the value.
 *
 * @param[in] aValue The value.
 * @return The integer; -0.0 for a value between -1 and 0.
 */
double
ceil(double aValue)
{
        double whole = trunc(aValue);

        return (aValue > 0 && whole != aValue ? whole + 1 : whole);
}

/**
 * @brief The nearest integer, halves away from zero.
 *
 * @param[in] aValue The value.
 * @return The integer.
 */
double
round(double aValue)
{
        double whole = trunc(aValue);

        /* The fraction aValue - whole is exact below 2^52, where a fraction exists. */
        return (fabs(aValue - whole) >= 0.5 ? whole + copysign(1.0, aValue) : whole);
}

/* The float and long double forms go through double, which holds every float exactly; for long
 * double the x87 format needs its own bits, below. */

/**
 * @brief The float value with its fraction dropped.
 *
 * @param[in] aValue The value.
 * @return The integer.
 */
float
truncf(float aValue)
{
        return ((float) trunc(aValue));
}

/**
 * @brief The largest integer not above a float.
 *
 * @param[in] aValue The value.
 * @return The integer.
 */
float
floorf(float aValue)
{
        return ((float) floor(aValue));
}

/**
 * @brief The smallest integer not below a float.
 *
 * @param[in] aValue The value.
 * @return The integer.
 */
float
ceilf(float aValue)
{
        return ((float) ceil(aValue));
}

/**
 * @brief The nearest integer to a float, halves away from zero.
 *
 * @param[in] aValue The value.
 * @return The integer.
 */
float
roundf(float aValue)
{
        return ((float) round(aValue));
}

/**
 * @brief The long double value with its fraction dropped.
 *
 * @param[in] aValue The value.
 * @return The integer.
 */
long double
truncl(long double aValue)
{
#if __LDBL_MANT_DIG__ == 64
        unsigned char bytes[ sizeof(long double) ];
        uint64_t      mantissa;
        uint16_t      signExponent;

        __builtin_memcpy(bytes, &aValue, sizeof(bytes));
        __builtin_memcpy(&mantissa, bytes, sizeof(mantissa));
        __builtin_memcpy(&signExponent, bytes + 8, sizeof(signExponent));
        int exponent = (signExponent & 0x7FFF) - 16383;

        if(exponent >= 63)
                return (aValue);
        if(exponent < 0)
                return (signExponent >> 15 ? -0.0L : 0.0L);
        mantissa &= ~((UINT64_C(1) << (63 - exponent)) - 1);
        __builtin_memcpy(bytes, &mantissa, sizeof(mantissa));
        __builtin_memcpy(&aValue, bytes, sizeof(bytes));
        return (aValue);
#else
        return (trunc((double) aValue));
#endif
}

/**
 * @brief The largest integer not above a long double.
 *
 * @param[in] aValue The value.
 * @return The integer.
 */
long double
floorl(long double aValue)
{
        long double whole = truncl(aValue);

        return (aValue < 0 && whole != aValue ? whole - 1 : whole);
}

/**
 * @brief The smallest integer not below a long double.
 *
 * @param[in] aValue The value.
 * @return The integer.
 */
long double
ceill(long double aValue)
{
        long double whole = truncl(aValue);

        return (aValue > 0 && whole != aValue ? whole + 1 : whole);
}

/**
 * @brief The nearest integer to a long double, halves away from zero.
 *
 * @param[in] aValue The value.
 * @return The integer.
 */
long double
roundl(long double aValue)
{
        long double whole = truncl(aValue);

        return (fabsl(aValue - whole) >= 0.5L ? whole + copysignl(1.0L, aValue) : whole);
}

/**
 * @brief The nearest integer as a long, halves away from zero.
 *
 * @param[in] aValue The value.
 * @return The integer; LONG_MIN for a NaN or a value out of range, where C leaves it unspecified
 *         and a plain conversion would be undefined.
 */
long
lround(double aValue)
{
        double whole = round(aValue);

        /* 2^63, the first double above the range of long. */
        const double limit = 9223372036854775808.0;

        if(isnan(whole) || whole < -limit || whole >= limit)
                return (LONG_MIN);
        return ((long) whole);
}

/**
 * @brief The nearest integer as a long long, halves away from zero.
 *
 * @param[in] aValue The value.
 * @return The integer; LLONG_MIN for a NaN or a value out of range.
 */
long long
llround(double aValue)
{
        return ((long long) lround(aValue));
}

/**
 * @brief The smaller of two values; a NaN loses to a number.
 *
 * @param[in] aFirst  The first value.
 * @param[in] aSecond The second value.
 * @return The smaller.
 */
double
fmin(double aFirst, double aSecond)
{
        if(isnan(aFirst))
                return (aSecond);
        if(isnan(aSecond))
                return (aFirst);
        return (aFirst < aSecond || (aFirst == aSecond && signbit(aFirst)) ? aFirst : aSecond);
}

/**
 * @brief The larger of two values; a NaN loses to a number.
 *
 * @param[in] aFirst  The first value.
 * @param[in] aSecond The second value.
 * @return The larger.
 */
double
fmax(double aFirst, double aSecond)
{
        if(isnan(aFirst))
                return (aSecond);
        if(isnan(aSecond))
                return (aFirst);
        return (aFirst > aSecond || (aFirst == aSecond && !signbit(aFirst)) ? aFirst : aSecond);
}

/**
 * @brief The positive difference: x - y when x is larger, +0 otherwise.
 *
 * @param[in] aFirst  x.
 * @param[in] aSecond y.
 * @return The difference, or a NaN when either is one.
 */
double
fdim(double aFirst, double aSecond)
{
        if(isnan(aFirst) || isnan(aSecond))
                return (aFirst + aSecond);
        return (aFirst > aSecond ? aFirst - aSecond : 0.0);
}

/**
 * @brief The square root, correctly rounded by the instruction.
 *
 * @param[in] aValue The value.
 * @return The root; a NaN for a negative value.
 */
double
sqrt(double aValue)
{
        return (__builtin_sqrt(aValue));
}

/**
 * @brief The square root of a float, correctly rounded.
 *
 * @param[in] aValue The value.
 * @return The root.
 */
float
sqrtf(float aValue)
{
        return (__builtin_sqrtf(aValue));
}

/**
 * @brief A value times 2^n, rounded once, as if exact and then rounded.
 *
 * @param[in] aValue    The value.
 * @param[in] aExponent n.
 * @return The result; an infinity or zero when it is out of range.
 */
double
scalbn(double aValue, int aExponent)
{
        /*
         * At most two steps of 2^1023 or 2^-969 bring n into the range of one multiplication. Below,
         * each step stays 53 binary places above the subnormals, so only the last one rounds.
         */
        for(int step = 0; step < 2 && aExponent > 1023; step++)
        {
                aValue    *= 0x1p1023;
                aExponent -= 1023;
        }
        for(int step = 0; step < 2 && aExponent < -1022; step++)
        {
                aValue    *= 0x1p-969;
                aExponent += 969;
        }
        aExponent = aExponent > 1023 ? 1023 : aExponent < -1022 ? -1022 : aExponent;
        return (aValue * DoubleOf((uint64_t) (1023 + aExponent) << 52));
}

/**
 * @brief A value times 2^n.
 *
 * @param[in] aValue    The value.
 * @param[in] aExponent n.
 * @return The result.
 */
double
ldexp(double aValue, int aExponent)
{
        return (scalbn(aValue, aExponent));
}

/**
 * @brief Splits a value into a fraction in [0.5, 1) and a power of two.
 *
 * @param[in]  aValue        The value.
 * @param[out] aOut_pExponent The power of two; 0 for zero, an infinity or a NaN.
 * @return The fraction, with the sign of the value; zero, an infinity or a NaN as it is.
 */
double
frexp(double aValue, int* aOut_pExponent)
{
        uint64_t bits     = BitsOf(aValue);
        int      exponent = ExponentOf(bits);

        *aOut_pExponent = 0;
        if(aValue == 0 || exponent == 1024)
                return (aValue);
        if(exponent == -1023)
        {
                /* A subnormal: make it normal first. */
                double fraction = frexp(aValue * 0x1p64, aOut_pExponent);

                *aOut_pExponent -= 64;
                return (fraction);
        }
        *aOut_pExponent = exponent + 1;
        return (DoubleOf((bits & ~(UINT64_C(0x7FF) << 52)) | (UINT64_C(1022) << 52)));
}

/**
 * @brief Splits a value into its integer part and its fraction, both with its sign.
 *
 * @param[in]  aValue        The value.
 * @param[out] aOut_pInteger The integer part.
 * @return The fraction; ±0 for an infinity, a NaN for a NaN.
 */
double
modf(double aValue, double* aOut_pInteger)
{
        *aOut_pInteger = trunc(aValue);
        if(isinf(aValue))
                return (copysign(0.0, aValue));
        return (copysign(aValue - *aOut_pInteger, aValue));
}

/**
 * @brief The significand of a finite double that is not zero as an integer with its leading bit at
 *        52, and its biased exponent: a subnormal's bits are shifted up and its exponent lowered.
 *
 * @param[in]  aBits          The bits of the value.
 * @param[out] aOut_pExponent The biased exponent, below 1 for a subnormal.
 * @return The significand.
 */
static uint64_t
Normalized(uint64_t aBits, int* aOut_pExponent)
{
        int      exponent    = (int) ((aBits >> 52) & 0x7FF);
        uint64_t significand = aBits & ((UINT64_C(1) << 52) - 1);

        if(exponent != 0)
                significand |= UINT64_C(1) << 52;
        else
                exponent = 1;
        for(; (significand >> 52) == 0; exponent--)
                significand <<= 1;
        *aOut_pExponent = exponent;
        return (significand);
}

/**
 * @brief The remainder of x / y with the quotient rounded toward zero, exact.
 *
 * @param[in] aDividend x.
 * @param[in] aDivisor  y.
 * @return x - n * y with the sign of x; a NaN when y is 0 or x is an infinity or either is a NaN.
 */
double
fmod(double aDividend, double aDivisor)
{
        if(isnan(aDividend) || isnan(aDivisor) || isinf(aDividend) || aDivisor == 0)
                return ((aDividend * aDivisor) / (aDividend * aDivisor));
        if(isinf(aDivisor) || fabs(aDividend) < fabs(aDivisor))
                return (aDividend);
        int      xExponent;
        int      yExponent;
        uint64_t x = Normalized(BitsOf(aDividend), &xExponent);
        uint64_t y = Normalized(BitsOf(aDivisor), &yExponent);

        /* Long division, one binary place at a time, keeping only the remainder: with both leading
         * bits at 52, x stays below 2y at each step. */
        for(; xExponent > yExponent; xExponent--)
        {
                x   = x >= y ? x - y : x;
                x <<= 1;
        }
        x = x >= y ? x - y : x;
        /* x * 2^(yExponent - 1075) is the remainder; scalbn rounds nothing, as it fits. */
        return (copysign(x == 0 ? 0.0 : scalbn((double) x, yExponent - 1075), aDividend));
}

/**
 * @brief A quiet NaN; the payload text is not used.
 *
 * @param[in] aIn_pPayload The payload text.
 * @return The NaN.
 */
double
nan(const char* aIn_pPayload)
{
        (void) aIn_pPayload;
        return (__builtin_nan(""));
}

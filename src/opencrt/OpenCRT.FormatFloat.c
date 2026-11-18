/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "opencrt/OpenCRT.Format.h"

/*
 * Floating-point conversions, exact: a finite value is m * 2^e with an integer m, so it is the
 * integer m * 2^e, or m * 5^-e divided by 10^-e, written in decimal with a big integer of base 10^9.
 * Rounding to the precision is then on exact digits, to nearest with ties to even, as the default
 * rounding mode asks. The largest case, the smallest subnormal of the x87 format, has 11 514 digits.
 */

/** The decimal digits of the largest exact value, with room for a carry. */
#define OPENCRT_FLOAT_DIGITS 11600

/** The limbs of base 10^9 that hold them. */
#define OPENCRT_FLOAT_LIMBS (OPENCRT_FLOAT_DIGITS / 9 + 2)

/** A value as an integer and a power of two, and what kind of value it is. */
typedef struct Parts
{
        uint64_t mantissa;
        int      exponent;
        bool     isNegative;
        bool     isInfinite;
        bool     isNan;
} tParts;

/** The exact decimal digits of a value: digits, and how many come before the decimal point. */
typedef struct Decimal
{
        char   digits[ OPENCRT_FLOAT_DIGITS ];
        size_t count;
        long   point;
} tDecimal;

#if __LDBL_MANT_DIG__ != 64
/**
 * @brief Splits a double into its parts.
 *
 * @param[in]  aValue     The value.
 * @param[out] aOut_pParts The parts.
 */
static void
SplitDouble(double aValue, tParts* aOut_pParts)
{
        uint64_t bits;

        __builtin_memcpy(&bits, &aValue, sizeof(bits));
        int      biased   = (int) ((bits >> 52) & 0x7FF);
        uint64_t fraction = bits & ((UINT64_C(1) << 52) - 1);

        aOut_pParts->isNegative = (bits >> 63) != 0;
        aOut_pParts->isInfinite = biased == 0x7FF && fraction == 0;
        aOut_pParts->isNan      = biased == 0x7FF && fraction != 0;
        aOut_pParts->mantissa   = biased == 0 ? fraction : fraction | (UINT64_C(1) << 52);
        aOut_pParts->exponent   = (biased == 0 ? 1 : biased) - 1075;
}
#endif

/**
 * @brief Splits a long double into its parts: the x87 format on amd64, binary64 elsewhere.
 *
 * @param[in]  aValue      The value.
 * @param[out] aOut_pParts The parts.
 */
static void
Split(long double aValue, tParts* aOut_pParts)
{
#if __LDBL_MANT_DIG__ == 64
        unsigned char bytes[ sizeof(long double) ];
        uint64_t      mantissa;
        uint16_t      signExponent;

        __builtin_memcpy(bytes, &aValue, sizeof(bytes));
        __builtin_memcpy(&mantissa, bytes, sizeof(mantissa));
        __builtin_memcpy(&signExponent, bytes + 8, sizeof(signExponent));
        int biased = signExponent & 0x7FFF;

        aOut_pParts->isNegative = (signExponent >> 15) != 0;
        aOut_pParts->mantissa   = mantissa;
        aOut_pParts->exponent   = (biased == 0 ? 1 : biased) - 16383 - 63;
        aOut_pParts->isInfinite = biased == 0x7FFF && (mantissa << 1) == 0;
        /* A set exponent without the integer bit is an encoding the x87 treats as invalid. */
        aOut_pParts->isNan = (biased == 0x7FFF && (mantissa << 1) != 0)
                || (biased != 0 && (mantissa >> 63) == 0);
#else
        SplitDouble((double) aValue, aOut_pParts);
#endif
}

/**
 * @brief Multiplies a big integer of base 10^9, least significant limb first, by a small factor.
 *
 * @param[in,out] aInOut_pLimbs The limbs.
 * @param[in,out] aInOut_pCount How many limbs are used.
 * @param[in]     aFactor       The factor, below 2^32.
 */
static void
Multiply(uint32_t* aInOut_pLimbs, size_t* aInOut_pCount, uint32_t aFactor)
{
        uint64_t carry = 0;

        for(size_t index = 0; index < *aInOut_pCount; index++)
        {
                uint64_t product       = (uint64_t) aInOut_pLimbs[ index ] * aFactor + carry;
                aInOut_pLimbs[ index ] = (uint32_t) (product % 1000000000);
                carry                  = product / 1000000000;
        }
        while(carry != 0)
        {
                aInOut_pLimbs[ (*aInOut_pCount)++ ] = (uint32_t) (carry % 1000000000);
                carry                              /= 1000000000;
        }
}

/**
 * @brief The exact decimal digits of a finite value that is not zero.
 *
 * @param[in]  aIn_pParts    The value.
 * @param[out] aOut_pDecimal The digits, without leading zeros.
 */
static void
ToDecimal(const tParts* aIn_pParts, tDecimal* aOut_pDecimal)
{
        uint32_t limbs[ OPENCRT_FLOAT_LIMBS ];
        size_t   count    = 0;
        uint64_t mantissa = aIn_pParts->mantissa;
        int      exponent = aIn_pParts->exponent;

        for(; mantissa != 0; mantissa /= 1000000000)
                limbs[ count++ ] = (uint32_t) (mantissa % 1000000000);
        long fractionDigits = 0;

        if(exponent >= 0)
        {
                for(; exponent >= 29; exponent -= 29)
                        Multiply(limbs, &count, UINT32_C(1) << 29);
                Multiply(limbs, &count, UINT32_C(1) << exponent);
        } else
        {
                /* m * 2^-k = m * 5^k / 10^k. */
                fractionDigits = -exponent;
                for(; exponent <= -13; exponent += 13)
                        Multiply(limbs, &count, 1220703125);
                uint32_t five = 1;

                for(; exponent < 0; exponent++)
                        five *= 5;
                Multiply(limbs, &count, five);
        }
        /* The most significant limb without leading zeros, then nine digits for each other limb. */
        char* pAt = aOut_pDecimal->digits;
        char  first[ 10 ];
        int   used = 0;

        for(uint32_t top = limbs[ count - 1 ]; top != 0; top /= 10)
                first[ used++ ] = (char) ('0' + top % 10);
        while(used > 0)
                *pAt++ = first[ --used ];
        for(size_t index = count - 1; index-- > 0;)
        {
                uint32_t limb = limbs[ index ];

                for(int digit = 8; digit >= 0; digit--)
                {
                        pAt[ digit ] = (char) ('0' + limb % 10);
                        limb        /= 10;
                }
                pAt += 9;
        }
        aOut_pDecimal->count = (size_t) (pAt - aOut_pDecimal->digits);
        aOut_pDecimal->point = (long) aOut_pDecimal->count - fractionDigits;
}

/**
 * @brief Whether rounding a decimal to its first digits goes up: to nearest, ties to even.
 *
 * @param[in] aIn_pDecimal The digits.
 * @param[in] aKeep        How many digits are kept, below the count; may be 0 or negative.
 * @return true when the kept digits go up by one unit of their last place.
 */
static bool
IsRoundingUp(const tDecimal* aIn_pDecimal, long aKeep)
{
        const char* pDigits = aIn_pDecimal->digits;

        /* Before the first digit, the digit cut is a zero. */
        if(aKeep < 0)
                return (false);
        char next     = pDigits[ aKeep ];
        bool isSticky = false;

        for(size_t index = (size_t) aKeep + 1; index < aIn_pDecimal->count && !isSticky; index++)
                isSticky = pDigits[ index ] != '0';
        bool isOdd = aKeep > 0 && (pDigits[ aKeep - 1 ] - '0') % 2 != 0;

        return (next > '5' || (next == '5' && (isSticky || isOdd)));
}

/**
 * @brief Adds one unit to the last of the digits of a decimal, carrying: 99.9 becomes 100.
 *
 * @param[in,out] aInOut_pDecimal The digits.
 */
static void
Increment(tDecimal* aInOut_pDecimal)
{
        char* pDigits = aInOut_pDecimal->digits;
        bool  isCarry = true;

        for(long index = (long) aInOut_pDecimal->count - 1; isCarry && index >= 0; index--)
        {
                isCarry          = pDigits[ index ] == '9';
                pDigits[ index ] = isCarry ? '0' : (char) (pDigits[ index ] + 1);
        }
        if(isCarry)
        {
                /* All nines: a 1 before them, one place more before the point. */
                pDigits[ 0 ]                        = '1';
                pDigits[ aInOut_pDecimal->count++ ] = '0';
                aInOut_pDecimal->point++;
        }
}

/**
 * @brief Keeps the first digits of a decimal and rounds the rest away, to nearest, ties to even.
 *
 * Keeping none or fewer rounds to zero, which leaves no digits, or to one unit of the place just
 * before the first digit kept.
 *
 * @param[in,out] aInOut_pDecimal The digits.
 * @param[in]     aKeep           How many digits to keep; may be 0 or negative.
 */
static void
Round(tDecimal* aInOut_pDecimal, long aKeep)
{
        if(aKeep >= (long) aInOut_pDecimal->count)
                return;
        bool isUp = IsRoundingUp(aInOut_pDecimal, aKeep);

        if(aKeep > 0)
        {
                aInOut_pDecimal->count = (size_t) aKeep;
                if(isUp)
                        Increment(aInOut_pDecimal);
                return;
        }
        /* Nothing kept: zero, or one unit of the place before the first digit. */
        aInOut_pDecimal->count = 0;
        if(isUp)
        {
                aInOut_pDecimal->digits[ 0 ] = '1';
                aInOut_pDecimal->count       = 1;
                aInOut_pDecimal->point      += 1 - aKeep;
        }
}

/**
 * @brief The digit at a place of a decimal; zeros before and after its digits.
 *
 * @param[in] aIn_pDecimal The decimal.
 * @param[in] aIndex       The place, counted from the first digit.
 * @return The digit.
 */
static char
DigitAt(const tDecimal* aIn_pDecimal, long aIndex)
{
        if(aIndex < 0 || aIndex >= (long) aIn_pDecimal->count)
                return ('0');
        return (aIn_pDecimal->digits[ aIndex ]);
}

/** The text of a conversion, built before it is padded into its field. */
typedef struct Text
{
        char   bytes[ 64 ];
        size_t length;
} tText;

/**
 * @brief Appends digits of a decimal to the output through a small buffer.
 *
 * @param[in,out] aInOut_pSink    The sink.
 * @param[in]     aIn_pDecimal    The decimal.
 * @param[in]     aFirst          The first place.
 * @param[in]     aCount          How many places.
 */
static void
PutDigits(__OpenCRT_Format_tSink* aInOut_pSink,
          const tDecimal*         aIn_pDecimal,
          long                    aFirst,
          long                    aCount)
{
        char chunk[ 64 ];
        int  used = 0;

        for(long index = 0; index < aCount; index++)
        {
                chunk[ used++ ] = DigitAt(aIn_pDecimal, aFirst + index);
                if(used == (int) sizeof(chunk))
                {
                        __OpenCRT_Format_Put(aInOut_pSink, chunk, sizeof(chunk));
                        used = 0;
                }
        }
        __OpenCRT_Format_Put(aInOut_pSink, chunk, (size_t) used);
}

/**
 * @brief Writes the exponent of an e or a conversion: a sign and at least two, or one, digits.
 *
 * @param[out] aOut_pText    The text.
 * @param[in]  aLetter       e, E, p or P.
 * @param[in]  aExponent     The exponent.
 * @param[in]  aMinimumDigits 2 for e, 1 for a.
 */
static void
Exponent(tText* aOut_pText, char aLetter, long aExponent, int aMinimumDigits)
{
        char digits[ 12 ];
        int  count     = 0;
        long magnitude = aExponent < 0 ? -aExponent : aExponent;

        aOut_pText->length                        = 0;
        aOut_pText->bytes[ aOut_pText->length++ ] = aLetter;
        aOut_pText->bytes[ aOut_pText->length++ ] = aExponent < 0 ? '-' : '+';
        do
        {
                digits[ count++ ] = (char) ('0' + magnitude % 10);
                magnitude        /= 10;
        } while(magnitude != 0 || count < aMinimumDigits);
        while(count > 0)
                aOut_pText->bytes[ aOut_pText->length++ ] = digits[ --count ];
}

/** The parts of a decimal conversion, before the field is laid out. */
typedef struct Layout
{
        /** The places of the integer part and of the fraction, from the first digit. */
        long integerFirst;
        long integerCount;
        long fractionCount;
        bool hasPoint;
        /** The exponent text of an e conversion, or empty. */
        tText exponent;
} tLayout;

/**
 * @brief Writes a decimal conversion laid out as integer part, point, fraction and exponent.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pSpec    The specification.
 * @param[in]     aIn_pPrefix  The sign.
 * @param[in]     aIn_pDecimal The digits.
 * @param[in]     aIn_pLayout  The layout.
 */
static void
WriteLayout(__OpenCRT_Format_tSink*       aInOut_pSink,
            const __OpenCRT_Format_tSpec* aIn_pSpec,
            const char*                   aIn_pPrefix,
            const tDecimal*               aIn_pDecimal,
            const tLayout*                aIn_pLayout)
{
        size_t prefix = strlen(aIn_pPrefix);
        size_t body   = (size_t) aIn_pLayout->integerCount + aIn_pLayout->hasPoint
                + (size_t) aIn_pLayout->fractionCount + aIn_pLayout->exponent.length;
        size_t total = prefix + body;
        size_t fill  = aIn_pSpec->width > total ? aIn_pSpec->width - total : 0;
        size_t zeros = aIn_pSpec->isZero && !aIn_pSpec->isLeft ? fill : 0;

        if(!aIn_pSpec->isLeft && zeros == 0)
                __OpenCRT_Format_Repeat(aInOut_pSink, ' ', fill);
        __OpenCRT_Format_Put(aInOut_pSink, aIn_pPrefix, prefix);
        __OpenCRT_Format_Repeat(aInOut_pSink, '0', zeros);
        PutDigits(aInOut_pSink, aIn_pDecimal, aIn_pLayout->integerFirst, aIn_pLayout->integerCount);
        if(aIn_pLayout->hasPoint)
                __OpenCRT_Format_Put(aInOut_pSink, ".", 1);
        PutDigits(aInOut_pSink,
                  aIn_pDecimal,
                  aIn_pLayout->integerFirst + aIn_pLayout->integerCount,
                  aIn_pLayout->fractionCount);
        __OpenCRT_Format_Put(aInOut_pSink,
                             aIn_pLayout->exponent.bytes,
                             aIn_pLayout->exponent.length);
        if(aIn_pSpec->isLeft)
                __OpenCRT_Format_Repeat(aInOut_pSink, ' ', fill);
}

/**
 * @brief Lays out an f conversion of a rounded decimal.
 *
 * @param[in]  aIn_pDecimal The digits, rounded to the precision.
 * @param[in]  aPrecision   The digits after the point.
 * @param[in]  aIsAlternate Whether the point stays when no digit follows it.
 * @param[out] aOut_pLayout The layout.
 */
static void
LayoutFixed(const tDecimal* aIn_pDecimal, long aPrecision, bool aIsAlternate, tLayout* aOut_pLayout)
{
        long point = aIn_pDecimal->count == 0 ? 1 : aIn_pDecimal->point;

        memset(aOut_pLayout, 0, sizeof(*aOut_pLayout));
        /* At least one digit before the point: 0.5 places a zero there. */
        aOut_pLayout->integerFirst  = point > 0 ? 0 : point - 1;
        aOut_pLayout->integerCount  = point > 0 ? point : 1;
        aOut_pLayout->fractionCount = aPrecision;
        aOut_pLayout->hasPoint      = aPrecision > 0 || aIsAlternate;
        if(aIn_pDecimal->count == 0)
        {
                aOut_pLayout->integerFirst = -1;
                aOut_pLayout->integerCount = 1;
        }
}

/**
 * @brief Lays out an e conversion of a decimal rounded to its significant digits.
 *
 * @param[in]  aIn_pDecimal The digits, the first not zero unless the value is zero.
 * @param[in]  aPrecision   The digits after the point.
 * @param[in]  aIsAlternate Whether the point stays when no digit follows it.
 * @param[in]  aLetter      e or E.
 * @param[out] aOut_pLayout The layout.
 */
static void
LayoutExponent(const tDecimal* aIn_pDecimal,
               long            aPrecision,
               bool            aIsAlternate,
               char            aLetter,
               tLayout*        aOut_pLayout)
{
        memset(aOut_pLayout, 0, sizeof(*aOut_pLayout));
        aOut_pLayout->integerFirst  = 0;
        aOut_pLayout->integerCount  = 1;
        aOut_pLayout->fractionCount = aPrecision;
        aOut_pLayout->hasPoint      = aPrecision > 0 || aIsAlternate;
        Exponent(&aOut_pLayout->exponent,
                 aLetter,
                 aIn_pDecimal->count == 0 ? 0 : aIn_pDecimal->point - 1,
                 2);
}

/**
 * @brief How many places of a fraction to keep for g: all, or without its trailing zeros.
 *
 * @param[in] aIn_pDecimal The digits.
 * @param[in] aFirst       The place of the first fraction digit.
 * @param[in] aCount       How many fraction places g would write.
 * @return The places to write.
 */
static long
TrimmedFraction(const tDecimal* aIn_pDecimal, long aFirst, long aCount)
{
        while(aCount > 0 && DigitAt(aIn_pDecimal, aFirst + aCount - 1) == '0')
                aCount--;
        return (aCount);
}

/**
 * @brief Rounds and lays out a g conversion: f or e by the exponent, trailing zeros dropped unless #.
 *
 * @param[in,out] aInOut_pDecimal The digits.
 * @param[in]     aIn_pSpec       The specification.
 * @param[in]     aPrecision      The precision, 6 when left out.
 * @param[in]     aLetter         e or E.
 * @param[out]    aOut_pLayout    The layout.
 */
static void
LayoutGeneral(tDecimal*                     aInOut_pDecimal,
              const __OpenCRT_Format_tSpec* aIn_pSpec,
              long                          aPrecision,
              char                          aLetter,
              tLayout*                      aOut_pLayout)
{
        long significant = aPrecision == 0 ? 1 : aPrecision;

        Round(aInOut_pDecimal, significant);
        long exponent = aInOut_pDecimal->count == 0 ? 0 : aInOut_pDecimal->point - 1;

        if(exponent < significant && exponent >= -4)
                LayoutFixed(aInOut_pDecimal,
                            significant - 1 - exponent,
                            aIn_pSpec->isAlternate,
                            aOut_pLayout);
        else
                LayoutExponent(aInOut_pDecimal,
                               significant - 1,
                               aIn_pSpec->isAlternate,
                               aLetter,
                               aOut_pLayout);
        if(aIn_pSpec->isAlternate)
                return;
        long first = aOut_pLayout->integerFirst + aOut_pLayout->integerCount;

        aOut_pLayout->fractionCount = TrimmedFraction(aInOut_pDecimal,
                                                      first,
                                                      aOut_pLayout->fractionCount);
        aOut_pLayout->hasPoint      = aOut_pLayout->fractionCount > 0;
}

/**
 * @brief Writes an f, e or g conversion of a finite value.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pSpec    The specification.
 * @param[in]     aIn_pParts   The value.
 * @param[in]     aIn_pPrefix  The sign.
 */
static void
WriteDecimal(__OpenCRT_Format_tSink*       aInOut_pSink,
             const __OpenCRT_Format_tSpec* aIn_pSpec,
             const tParts*                 aIn_pParts,
             const char*                   aIn_pPrefix)
{
        tDecimal decimal;
        char     conversion = (char) (aIn_pSpec->conversion | 0x20);
        long     precision  = aIn_pSpec->precision < 0 ? 6 : aIn_pSpec->precision;
        char     letter = aIn_pSpec->conversion == 'E' || aIn_pSpec->conversion == 'G' ? 'E' : 'e';
        tLayout  layout;

        decimal.count = 0;
        decimal.point = 1;
        if(aIn_pParts->mantissa != 0)
                ToDecimal(aIn_pParts, &decimal);
        if(conversion == 'f')
        {
                Round(&decimal, decimal.point + precision);
                LayoutFixed(&decimal, precision, aIn_pSpec->isAlternate, &layout);
        } else if(conversion == 'e')
        {
                Round(&decimal, precision + 1);
                LayoutExponent(&decimal, precision, aIn_pSpec->isAlternate, letter, &layout);
        } else
                LayoutGeneral(&decimal, aIn_pSpec, precision, letter, &layout);
        WriteLayout(aInOut_pSink, aIn_pSpec, aIn_pPrefix, &decimal, &layout);
}

/**
 * @brief Rounds the fraction of a hexadecimal conversion to a number of digits, ties to even.
 *
 * @param[in]     aFraction     The 64 bits after the leading one.
 * @param[in]     aKeep         How many hex digits to keep, 0 to 16.
 * @param[in]     aIn_pDigits   The digits, lower or upper case.
 * @param[out]    aOut_pText    The kept digits.
 * @param[out]    aOut_pLeading The digit before the point: 1, or 2 when rounding carries into it,
 *                              as glibc and musl write it (0x1.f rounded to no digit is 0x2).
 * @return How many digits were written, aKeep.
 */
static int
RoundHexadecimal(uint64_t    aFraction,
                 int         aKeep,
                 const char* aIn_pDigits,
                 char*       aOut_pText,
                 char*       aOut_pLeading)
{
        uint64_t kept = aFraction;

        *aOut_pLeading = '1';
        if(aKeep == 0)
        {
                /* The cut follows the leading 1, which is odd: a half or more makes it 2. */
                if(aFraction >= UINT64_C(1) << 63)
                        *aOut_pLeading = '2';
                return (0);
        }
        if(aKeep < 16)
        {
                int      dropped = 64 - 4 * aKeep;
                uint64_t rest    = aFraction & ((UINT64_C(1) << dropped) - 1);
                uint64_t half    = UINT64_C(1) << (dropped - 1);

                kept = aFraction >> dropped;
                if(rest > half || (rest == half && (kept & 1) != 0))
                        kept++;
                if(kept >> (4 * aKeep) != 0)
                {
                        kept           = 0;
                        *aOut_pLeading = '2';
                }
        }
        for(int digit = 0; digit < aKeep; digit++)
                aOut_pText[ digit ] = aIn_pDigits[ (kept >> (4 * (aKeep - 1 - digit))) & 0xF ];
        return (aKeep);
}

/** The parts of an a conversion before it is laid out. */
typedef struct Hexadecimal
{
        char leading;
        char fraction[ 16 ];
        int  count;
        long exponent;
} tHexadecimal;

/**
 * @brief The digits and the exponent of an a conversion, normalized and rounded to the precision.
 *
 * @param[in]  aIn_pSpec    The specification.
 * @param[in]  aIn_pParts   The value.
 * @param[out] aOut_pHex    The digits and the exponent.
 */
static void
ToHexadecimal(const __OpenCRT_Format_tSpec* aIn_pSpec,
              const tParts*                 aIn_pParts,
              tHexadecimal*                 aOut_pHex)
{
        const char* pDigits  = aIn_pSpec->conversion == 'A' ? "0123456789ABCDEF"
                                                            : "0123456789abcdef";
        uint64_t    mantissa = aIn_pParts->mantissa;

        aOut_pHex->leading  = '0';
        aOut_pHex->count    = 0;
        aOut_pHex->exponent = 0;
        if(mantissa == 0)
                return;
        /* Put the leading one in bit 63: the 16 hex digits below it are the fraction. */
        int shift = __builtin_clzll(mantissa);
        int keep  = aIn_pSpec->precision < 0 || aIn_pSpec->precision > 16
                ? 16
                : (int) aIn_pSpec->precision;

        mantissa          <<= shift;
        aOut_pHex->exponent = (long) aIn_pParts->exponent + 63 - shift;
        aOut_pHex->count    = RoundHexadecimal(mantissa << 1,
                                               keep,
                                               pDigits,
                                               aOut_pHex->fraction,
                                               &aOut_pHex->leading);
        while(aIn_pSpec->precision < 0 && aOut_pHex->count > 0
              && aOut_pHex->fraction[ aOut_pHex->count - 1 ] == '0')
                aOut_pHex->count--;
}

/**
 * @brief The body of an a conversion: the leading digit, the point when one is written, and the
 *        fraction.
 *
 * @param[in]  aIn_pSpec The specification.
 * @param[in]  aIn_pHex  The digits.
 * @param[in]  aZeros    The zeros the precision adds after the fraction.
 * @param[out] aOut_pBody At least 18 bytes.
 * @return The length of the body.
 */
static size_t
HexadecimalBody(const __OpenCRT_Format_tSpec* aIn_pSpec,
                const tHexadecimal*           aIn_pHex,
                long                          aZeros,
                char*                         aOut_pBody)
{
        size_t length = 0;

        aOut_pBody[ length++ ] = aIn_pHex->leading;
        if(aIn_pHex->count > 0 || aZeros > 0 || aIn_pSpec->isAlternate)
                aOut_pBody[ length++ ] = '.';
        memcpy(aOut_pBody + length, aIn_pHex->fraction, (size_t) aIn_pHex->count);
        return (length + (size_t) aIn_pHex->count);
}

/**
 * @brief Writes an a or A conversion: 0x1.hhhp+d, normalized, rounded to the precision, ties to even.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pSpec    The specification.
 * @param[in]     aIn_pParts   The value.
 * @param[in]     aIn_pPrefix  The sign.
 */
static void
WriteHexadecimal(__OpenCRT_Format_tSink*       aInOut_pSink,
                 const __OpenCRT_Format_tSpec* aIn_pSpec,
                 const tParts*                 aIn_pParts,
                 const char*                   aIn_pPrefix)
{
        bool         isUpper = aIn_pSpec->conversion == 'A';
        tHexadecimal hex;
        tText        exponentText;
        char         prefix[ 8 ];
        char         body[ 24 ];

        ToHexadecimal(aIn_pSpec, aIn_pParts, &hex);
        long zeros = aIn_pSpec->precision > hex.count ? aIn_pSpec->precision - hex.count : 0;

        Exponent(&exponentText, isUpper ? 'P' : 'p', hex.exponent, 1);
        strcpy(stpcpy(prefix, aIn_pPrefix), isUpper ? "0X" : "0x");
        size_t length     = HexadecimalBody(aIn_pSpec, &hex, zeros, body);
        size_t total      = strlen(prefix) + length + (size_t) zeros + exponentText.length;
        size_t fill       = aIn_pSpec->width > total ? aIn_pSpec->width - total : 0;
        bool   isZeroFill = aIn_pSpec->isZero && !aIn_pSpec->isLeft;

        __OpenCRT_Format_Repeat(aInOut_pSink, ' ', aIn_pSpec->isLeft || isZeroFill ? 0 : fill);
        __OpenCRT_Format_Put(aInOut_pSink, prefix, strlen(prefix));
        __OpenCRT_Format_Repeat(aInOut_pSink, '0', isZeroFill ? fill : 0);
        __OpenCRT_Format_Put(aInOut_pSink, body, length);
        __OpenCRT_Format_Repeat(aInOut_pSink, '0', (size_t) zeros);
        __OpenCRT_Format_Put(aInOut_pSink, exponentText.bytes, exponentText.length);
        __OpenCRT_Format_Repeat(aInOut_pSink, ' ', aIn_pSpec->isLeft ? fill : 0);
}

/**
 * @brief Writes an infinity or a NaN: its sign and inf or nan, padded with spaces, never zeros,
 *        which would read as digits.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pSpec    The specification.
 * @param[in]     aIn_pParts   The value.
 * @param[in]     aIn_pPrefix  The sign.
 */
static void
WriteSpecial(__OpenCRT_Format_tSink*       aInOut_pSink,
             const __OpenCRT_Format_tSpec* aIn_pSpec,
             const tParts*                 aIn_pParts,
             const char*                   aIn_pPrefix)
{
        static const char* const kWords[ 2 ][ 2 ] = { { "inf", "INF" }, { "nan", "NAN" } };
        __OpenCRT_Format_tSpec   spaces           = *aIn_pSpec;
        bool isUpper = aIn_pSpec->conversion >= 'A' && aIn_pSpec->conversion <= 'Z';

        spaces.isZero = false;
        __OpenCRT_Format_Field(aInOut_pSink,
                               &spaces,
                               aIn_pPrefix,
                               0,
                               kWords[ aIn_pParts->isNan ][ isUpper ],
                               3);
}

/**
 * @brief Writes a floating-point value for an f, F, e, E, g, G, a or A conversion.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pSpec    The specification.
 * @param[in]     aValue       The value.
 */
void
__OpenCRT_Format_Float(__OpenCRT_Format_tSink*       aInOut_pSink,
                       const __OpenCRT_Format_tSpec* aIn_pSpec,
                       long double                   aValue)
{
        tParts      parts;
        const char* pPrefix;

        Split(aValue, &parts);
        pPrefix = parts.isNegative ? "-" : aIn_pSpec->isPlus ? "+" : aIn_pSpec->isSpace ? " " : "";
        if(parts.isInfinite || parts.isNan)
                WriteSpecial(aInOut_pSink, aIn_pSpec, &parts, pPrefix);
        else if(aIn_pSpec->conversion == 'a' || aIn_pSpec->conversion == 'A')
                WriteHexadecimal(aInOut_pSink, aIn_pSpec, &parts, pPrefix);
        else
                WriteDecimal(aInOut_pSink, aIn_pSpec, &parts, pPrefix);
}

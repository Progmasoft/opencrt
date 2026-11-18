/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "opencrt/OpenCRT.FormatSpec.h"

/**
 * @brief Reads a decimal number of a format, such as a width.
 *
 * @param[in,out] aInOut_ppAt Where it starts; moved past it.
 * @return The number, at most INT_MAX + 1, so that a larger one is refused.
 */
static long
ReadNumber(const char** aInOut_ppAt)
{
        long number = 0;

        for(; **aInOut_ppAt >= '0' && **aInOut_ppAt <= '9'; (*aInOut_ppAt)++)
        {
                if(number <= INT_MAX)
                        number = number * 10 + (**aInOut_ppAt - '0');
        }
        return (number);
}

/**
 * @brief Reads "n$" after a %, a * or a .*, if it is there.
 *
 * @param[in,out] aInOut_ppAt Where it may start; moved past it when it is there.
 * @return The position, 0 when there is none, or -1 for one out of range.
 */
static int
ReadPosition(const char** aInOut_ppAt)
{
        const char* pAt    = *aInOut_ppAt;
        long        number = ReadNumber(&pAt);

        if(pAt == *aInOut_ppAt || *pAt != '$')
                return (0);
        *aInOut_ppAt = pAt + 1;
        return (number >= 1 && number <= OPENCRT_FORMATSPEC_POSITIONS ? (int) number : -1);
}

/**
 * @brief Reads the flags of a specification.
 *
 * @param[in,out] aInOut_ppAt Where they start; moved past them.
 * @param[out]    aOut_pSpec  The specification.
 */
static void
ReadFlags(const char** aInOut_ppAt, __OpenCRT_Format_tSpec* aOut_pSpec)
{
        for(const char* pFlag;
            (pFlag = strchr("-+ #0", **aInOut_ppAt)) != NULL && **aInOut_ppAt != '\0';
            (*aInOut_ppAt)++)
        {
                aOut_pSpec->isLeft      |= *pFlag == '-';
                aOut_pSpec->isPlus      |= *pFlag == '+';
                aOut_pSpec->isSpace     |= *pFlag == ' ';
                aOut_pSpec->isAlternate |= *pFlag == '#';
                aOut_pSpec->isZero      |= *pFlag == '0';
        }
}

/**
 * @brief Reads a width or a precision: digits, or a * with its position.
 *
 * @param[in,out] aInOut_ppAt          Where it starts; moved past it.
 * @param[out]    aOut_pValue          The digits' value, when there is no *.
 * @param[out]    aOut_pHasArgument    Whether it is a *.
 * @param[out]    aOut_pPosition       The position of the *, or 0.
 */
static void
ReadAmount(const char** aInOut_ppAt,
           long*        aOut_pValue,
           bool*        aOut_pHasArgument,
           int*         aOut_pPosition)
{
        if(**aInOut_ppAt != '*')
        {
                *aOut_pValue = ReadNumber(aInOut_ppAt);
                return;
        }
        (*aInOut_ppAt)++;
        *aOut_pHasArgument = true;
        *aOut_pPosition    = ReadPosition(aInOut_ppAt);
}

/**
 * @brief Reads a length modifier.
 *
 * @param[in,out] aInOut_ppAt Where it may start; moved past it.
 * @return The modifier.
 */
static tLength
ReadLength(const char** aInOut_ppAt)
{
        static const char    kLetters[] = "hljztL";
        static const tLength kSingles[] = { kShort,      kLongLength,    kIntMaxLength,
                                            kSizeLength, kPtrdiffLength, kLongDoubleLength };
        const char*          pLetter    = strchr(kLetters, **aInOut_ppAt);

        if(**aInOut_ppAt == '\0' || pLetter == NULL)
                return (kDefault);
        bool isDouble = (**aInOut_ppAt == 'h' || **aInOut_ppAt == 'l')
                && (*aInOut_ppAt)[ 1 ] == **aInOut_ppAt;

        *aInOut_ppAt += isDouble ? 2 : 1;
        if(isDouble)
                return (*pLetter == 'h' ? kChar : kLongLongLength);
        return (kSingles[ pLetter - kLetters ]);
}

/** The classes of conversions, which share the types they read. */
enum
{
        kIntegerClass,
        kFloatClass,
        kCharacterClass,
        kStringClass,
        kPointerClass,
        kClassCount,
};

/** The type each class reads for each length modifier, kNone where they do not go together. */
static const tKind kKindsByClass[ kClassCount ][ kLongDoubleLength + 1 ] = {
        [kIntegerClass] = { kInt, kInt, kInt, kLong, kLongLong, kIntMax, kSize, kPtrdiff, kNone },
        [kFloatClass] = { kDouble, kNone, kNone, kDouble, kNone, kNone, kNone, kNone, kLongDouble },
        [kCharacterClass] = { kInt, kNone, kNone, kWideChar, kNone, kNone, kNone, kNone, kNone },
        [kStringClass]    = { kPointer, kNone, kNone, kPointer, kNone, kNone, kNone, kNone, kNone },
        [kPointerClass]   = { kPointer, kNone, kNone, kNone, kNone, kNone, kNone, kNone, kNone },
};

/**
 * @brief The type the value of a conversion is read as.
 *
 * @param[in] aIn_pRaw The specification.
 * @return The type, or kNone for a conversion and modifier that do not go together.
 */
tKind
__OpenCRT_FormatSpec_KindOf(const tRaw* aIn_pRaw)
{
        static const char* const kConversions[ kClassCount ]
                = { "diouxX", "fFeEgGaA", "c", "s", "p" };
        char conversion = aIn_pRaw->spec.conversion;

        for(int index = 0; conversion != '\0' && index < kClassCount; index++)
        {
                if(strchr(kConversions[ index ], conversion) != NULL)
                        return (kKindsByClass[ index ][ aIn_pRaw->length ]);
        }
        return (kNone);
}

/**
 * @brief Reads the conversion character, taking XSI's %C and %S as %lc and %ls.
 *
 * @param[in,out] aInOut_ppAt Where it is; moved past it.
 * @param[in,out] aInOut_pRaw The specification, with its length modifier.
 */
static void
ReadConversion(const char** aInOut_ppAt, tRaw* aInOut_pRaw)
{
        char conversion = **aInOut_ppAt;

        if(conversion != '\0')
                (*aInOut_ppAt)++;
        if((conversion == 'C' || conversion == 'S') && aInOut_pRaw->length == kDefault)
        {
                conversion          = conversion == 'C' ? 'c' : 's';
                aInOut_pRaw->length = kLongLength;
        }
        aInOut_pRaw->spec.conversion = conversion;
}

/**
 * @brief Whether OpenCRT takes a specification: positions in range, a width and a precision that
 *        fit an int, and a conversion that goes with its modifier.
 *
 * @param[in] aIn_pRaw The specification.
 * @param[in] aWidth   The width as read.
 * @return true when it does.
 */
static bool
IsValid(const tRaw* aIn_pRaw, long aWidth)
{
        if(aIn_pRaw->position < 0 || aIn_pRaw->widthPosition < 0 || aIn_pRaw->precisionPosition < 0)
                return (false);
        if(aWidth > INT_MAX || aIn_pRaw->spec.precision > INT_MAX)
                return (false);
        return (__OpenCRT_FormatSpec_KindOf(aIn_pRaw) != kNone);
}

/**
 * @brief Reads one conversion specification, after its %.
 *
 * @param[in,out] aInOut_ppAt Where it starts; moved past it.
 * @param[out]    aOut_pRaw   The specification.
 * @return false for one OpenCRT refuses: %n, an unknown conversion, a bad position or modifier.
 */
bool
__OpenCRT_FormatSpec_Read(const char** aInOut_ppAt, tRaw* aOut_pRaw)
{
        long width = 0;

        memset(aOut_pRaw, 0, sizeof(*aOut_pRaw));
        aOut_pRaw->spec.precision = -1;
        aOut_pRaw->position       = ReadPosition(aInOut_ppAt);
        ReadFlags(aInOut_ppAt, &aOut_pRaw->spec);
        ReadAmount(aInOut_ppAt, &width, &aOut_pRaw->hasWidthArgument, &aOut_pRaw->widthPosition);
        aOut_pRaw->spec.width = (size_t) width;
        if(**aInOut_ppAt == '.')
        {
                (*aInOut_ppAt)++;
                aOut_pRaw->spec.precision = 0;
                ReadAmount(aInOut_ppAt,
                           &aOut_pRaw->spec.precision,
                           &aOut_pRaw->hasPrecisionArgument,
                           &aOut_pRaw->precisionPosition);
        }
        aOut_pRaw->length = ReadLength(aInOut_ppAt);
        ReadConversion(aInOut_ppAt, aOut_pRaw);
        return (IsValid(aOut_pRaw, width));
}

/**
 * @brief Reads the next argument as an integer type.
 *
 * @param[in,out] aInOut_pList The arguments.
 * @param[in]     aKind        The type.
 * @return The value, widened to long long.
 */
static long long
FetchInteger(va_list* aInOut_pList, tKind aKind)
{
        switch(aKind)
        {
        case kLong:
                return (va_arg(*aInOut_pList, long));
        case kLongLong:
                return (va_arg(*aInOut_pList, long long));
        case kIntMax:
                return (va_arg(*aInOut_pList, intmax_t));
        case kSize:
                return ((long long) va_arg(*aInOut_pList, size_t));
        case kPtrdiff:
                return (va_arg(*aInOut_pList, ptrdiff_t));
        default:
                return (va_arg(*aInOut_pList, int));
        }
}

/**
 * @brief Reads the next argument as a type.
 *
 * @param[in,out] aInOut_pList The arguments.
 * @param[in]     aKind        The type.
 * @return The value.
 */
static tValue
Fetch(va_list* aInOut_pList, tKind aKind)
{
        tValue value;

        value.integer = 0;
        if(aKind == kDouble)
                value.floating = va_arg(*aInOut_pList, double);
        else if(aKind == kLongDouble)
                value.floating = va_arg(*aInOut_pList, long double);
        else if(aKind == kPointer)
                value.pPointer = va_arg(*aInOut_pList, const void*);
        else if(aKind != kNone)
                value.integer = FetchInteger(aInOut_pList, aKind);
        return (value);
}

/**
 * @brief Records the type of a position, refusing a position given two types.
 *
 * @param[in,out] aInOut_pKinds The type of each position.
 * @param[in,out] aInOut_pHighest The highest position seen.
 * @param[in]     aPosition     The position.
 * @param[in]     aKind         The type.
 * @return false when the position already has another type.
 */
static bool
Claim(tKind aInOut_pKinds[ OPENCRT_FORMATSPEC_POSITIONS + 1 ],
      int*  aInOut_pHighest,
      int   aPosition,
      tKind aKind)
{
        if(aInOut_pKinds[ aPosition ] != kNone && aInOut_pKinds[ aPosition ] != aKind)
                return (false);
        aInOut_pKinds[ aPosition ] = aKind;
        *aInOut_pHighest           = aPosition > *aInOut_pHighest ? aPosition : *aInOut_pHighest;
        return (true);
}

/**
 * @brief Records the types of the positions a specification uses.
 *
 * @param[in]     aIn_pRaw       The specification, with a position.
 * @param[in,out] aInOut_pKinds  The type of each position.
 * @param[in,out] aInOut_pHighest The highest position seen.
 * @return false when a * has no position or a position gets two types.
 */
static bool
ClaimPositions(const tRaw* aIn_pRaw,
               tKind       aInOut_pKinds[ OPENCRT_FORMATSPEC_POSITIONS + 1 ],
               int*        aInOut_pHighest)
{
        if(!Claim(aInOut_pKinds,
                  aInOut_pHighest,
                  aIn_pRaw->position,
                  __OpenCRT_FormatSpec_KindOf(aIn_pRaw)))
                return (false);
        if(aIn_pRaw->hasWidthArgument
           && (aIn_pRaw->widthPosition == 0
               || !Claim(aInOut_pKinds, aInOut_pHighest, aIn_pRaw->widthPosition, kInt)))
                return (false);
        return (!aIn_pRaw->hasPrecisionArgument
                || (aIn_pRaw->precisionPosition != 0
                    && Claim(aInOut_pKinds, aInOut_pHighest, aIn_pRaw->precisionPosition, kInt)));
}

/**
 * @brief The next conversion specification of a format, %% skipped.
 *
 * @param[in,out] aInOut_ppAt Where to look; moved past the specification.
 * @param[out]    aOut_pRaw   The specification.
 * @param[out]    aOut_pIsValid Whether OpenCRT takes it.
 * @return false when the format has no more.
 */
static bool
NextSpec(const char** aInOut_ppAt, tRaw* aOut_pRaw, bool* aOut_pIsValid)
{
        while((*aInOut_ppAt = strchr(*aInOut_ppAt, '%')) != NULL)
        {
                (*aInOut_ppAt)++;
                if(**aInOut_ppAt != '%')
                {
                        *aOut_pIsValid = __OpenCRT_FormatSpec_Read(aInOut_ppAt, aOut_pRaw);
                        return (true);
                }
                (*aInOut_ppAt)++;
        }
        return (false);
}

/**
 * @brief Reads every specification of a format, recording the types of the positions they use.
 *
 * @param[in]  aIn_pFormat     The format.
 * @param[out] aOut_pKinds     The type of each position.
 * @param[out] aOut_pHighest   The highest position used.
 * @param[out] aOut_pIsPlaced  Whether the format uses positions.
 * @return false for a format OpenCRT refuses, positions mixed with plain conversions included.
 */
static bool
Scan(const char* aIn_pFormat,
     tKind       aOut_pKinds[ OPENCRT_FORMATSPEC_POSITIONS + 1 ],
     int*        aOut_pHighest,
     bool*       aOut_pIsPlaced)
{
        int         counts[ 2 ] = { 0, 0 };
        const char* pAt         = aIn_pFormat;
        tRaw        raw;
        bool        isValid = true;

        while(isValid && NextSpec(&pAt, &raw, &isValid))
        {
                bool isPlaced = raw.position != 0;

                counts[ isPlaced ]++;
                if(!isValid)
                        break;
                isValid = isPlaced ? ClaimPositions(&raw, aOut_pKinds, aOut_pHighest)
                                   : raw.widthPosition == 0 && raw.precisionPosition == 0;
        }
        *aOut_pIsPlaced = counts[ 1 ] > 0;
        return (isValid && (counts[ 0 ] == 0 || counts[ 1 ] == 0));
}

/**
 * @brief Checks every specification of a format and, for positions, reads every argument first.
 *
 * @param[in]     aIn_pFormat       The format.
 * @param[in,out] aInOut_pArguments The arguments, with their list started.
 * @return false for a format OpenCRT refuses.
 */
bool
__OpenCRT_FormatSpec_Prepare(const char* aIn_pFormat, tArguments* aInOut_pArguments)
{
        tKind kinds[ OPENCRT_FORMATSPEC_POSITIONS + 1 ] = { kNone };
        int   highest                                   = 0;

        if(!Scan(aIn_pFormat, kinds, &highest, &aInOut_pArguments->isPositional))
                return (false);
        for(int position = 1; position <= highest; position++)
        {
                if(kinds[ position ] == kNone)
                        return (false);
                aInOut_pArguments->values[ position ] = Fetch(&aInOut_pArguments->list,
                                                              kinds[ position ]);
        }
        return (true);
}

/**
 * @brief The value of an argument: the next one, or the one at a position.
 *
 * @param[in,out] aInOut_pArguments The arguments.
 * @param[in]     aPosition         The position, or 0.
 * @param[in]     aKind             The type.
 * @return The value.
 */
tValue
__OpenCRT_FormatSpec_Argument(tArguments* aInOut_pArguments, int aPosition, tKind aKind)
{
        if(aInOut_pArguments->isPositional)
                return (aInOut_pArguments->values[ aPosition ]);
        return (Fetch(&aInOut_pArguments->list, aKind));
}

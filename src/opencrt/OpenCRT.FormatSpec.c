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

/**
 * @brief The type of an integer argument for a length modifier.
 *
 * @param[in] aLength The modifier.
 * @return The type, or kNone for L.
 */
static tKind
IntegerKind(tLength aLength)
{
        static const tKind kKinds[] = {
                [kDefault]          = kInt,
                [kChar]             = kInt,
                [kShort]            = kInt,
                [kLongLength]       = kLong,
                [kLongLongLength]   = kLongLong,
                [kIntMaxLength]     = kIntMax,
                [kSizeLength]       = kSize,
                [kPtrdiffLength]    = kPtrdiff,
                [kLongDoubleLength] = kNone,
        };

        return (kKinds[ aLength ]);
}

/**
 * @brief The type the value of a conversion is read as.
 *
 * @param[in] aIn_pRaw The specification.
 * @return The type, or kNone for a conversion and modifier that do not go together.
 */
tKind
__OpenCRT_FormatSpec_KindOf(const tRaw* aIn_pRaw)
{
        char    conversion = aIn_pRaw->spec.conversion;
        tLength length     = aIn_pRaw->length;

        if(conversion == '\0')
                return (kNone);
        if(strchr("diouxX", conversion) != NULL)
                return (IntegerKind(length));
        if(strchr("fFeEgGaA", conversion) != NULL)
                return (length == kLongDoubleLength ? kLongDouble
                                : length <= kLongLength && length != kChar && length != kShort
                                ? kDouble
                                : kNone);
        if(conversion == 'c' && (length == kDefault || length == kLongLength))
                return (length == kDefault ? kInt : kWideChar);
        if(conversion == 's' && (length == kDefault || length == kLongLength))
                return (kPointer);
        return (conversion == 'p' && length == kDefault ? kPointer : kNone);
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
        char conversion   = **aInOut_ppAt;

        if(conversion != '\0')
                (*aInOut_ppAt)++;
        /* XSI: %C and %S are %lc and %ls. */
        if((conversion == 'C' || conversion == 'S') && aOut_pRaw->length == kDefault)
        {
                conversion        = conversion == 'C' ? 'c' : 's';
                aOut_pRaw->length = kLongLength;
        }
        aOut_pRaw->spec.conversion = conversion;
        bool hasValidPlaces        = aOut_pRaw->position >= 0 && aOut_pRaw->widthPosition >= 0
                && aOut_pRaw->precisionPosition >= 0;

        return (hasValidPlaces && width <= INT_MAX && aOut_pRaw->spec.precision <= INT_MAX
                && __OpenCRT_FormatSpec_KindOf(aOut_pRaw) != kNone);
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
 * @brief Checks every specification of a format and, for positions, reads every argument first.
 *
 * @param[in]     aIn_pFormat       The format.
 * @param[in,out] aInOut_pArguments The arguments, with their list started.
 * @return false for a format OpenCRT refuses.
 */
bool
__OpenCRT_FormatSpec_Prepare(const char* aIn_pFormat, tArguments* aInOut_pArguments)
{
        tKind       kinds[ OPENCRT_FORMATSPEC_POSITIONS + 1 ] = { kNone };
        int         highest                                   = 0;
        int         counts[ 2 ]                               = { 0, 0 };
        const char* pAt                                       = aIn_pFormat;
        tRaw        raw;
        bool        isValid = true;

        while(isValid && NextSpec(&pAt, &raw, &isValid))
        {
                bool isPlaced = raw.position != 0;

                counts[ isPlaced ]++;
                if(isValid && isPlaced)
                        isValid = ClaimPositions(&raw, kinds, &highest);
                else if(isValid)
                        isValid = raw.widthPosition == 0 && raw.precisionPosition == 0;
        }
        if(!isValid || (counts[ 0 ] > 0 && counts[ 1 ] > 0))
                return (false);
        aInOut_pArguments->isPositional = counts[ 1 ] > 0;
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

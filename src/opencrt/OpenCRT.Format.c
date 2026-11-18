/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include "opencrt/OpenCRT.Format.h"

/*
 * Arguments come in order, or by position (%2$s) as POSIX has it; a format uses one way only. Every
 * position from 1 to the highest must be used, so that each argument's type is known before it is
 * read. %n is refused: it lets a format write to memory, the path of format string attacks.
 */

/** The most positions a format may use: NL_ARGMAX. */
#define OPENCRT_FORMAT_POSITIONS NL_ARGMAX

/** The type an argument is read as. */
typedef enum Kind
{
        kNone,
        kInt,
        kLong,
        kLongLong,
        kIntMax,
        kSize,
        kPtrdiff,
        kDouble,
        kLongDouble,
        kPointer,
        kWideChar,
} tKind;

/** The length modifier of a conversion. */
typedef enum Length
{
        kDefault,
        kChar,
        kShort,
        kLongLength,
        kLongLongLength,
        kIntMaxLength,
        kSizeLength,
        kPtrdiffLength,
        kLongDoubleLength,
} tLength;

/** A value read from the arguments. */
typedef union Value
{
        long long   integer;
        long double floating;
        const void* pPointer;
} tValue;

/** A conversion specification as written: its width and precision may still be arguments. */
typedef struct Raw
{
        __OpenCRT_Format_tSpec spec;
        tLength                length;
        /** The position of the value, or 0 for the next argument. */
        int position;
        /** Whether the width is an argument, and its position or 0. */
        bool hasWidthArgument;
        int  widthPosition;
        /** Whether the precision is an argument, and its position or 0. */
        bool hasPrecisionArgument;
        int  precisionPosition;
} tRaw;

/**
 * @brief Writes bytes to the sink and counts them.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pBytes   The bytes.
 * @param[in]     aCount       How many.
 */
void
__OpenCRT_Format_Put(__OpenCRT_Format_tSink* aInOut_pSink, const char* aIn_pBytes, size_t aCount)
{
        if(aCount == 0)
                return;
        aInOut_pSink->fn_Put(aInOut_pSink, aIn_pBytes, aCount);
        aInOut_pSink->length += aCount;
}

/**
 * @brief Writes a byte several times.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aByte        The byte.
 * @param[in]     aCount       How many times.
 */
void
__OpenCRT_Format_Repeat(__OpenCRT_Format_tSink* aInOut_pSink, char aByte, size_t aCount)
{
        char chunk[ 32 ];

        memset(chunk, aByte, sizeof(chunk));
        for(; aCount > sizeof(chunk); aCount -= sizeof(chunk))
                __OpenCRT_Format_Put(aInOut_pSink, chunk, sizeof(chunk));
        __OpenCRT_Format_Put(aInOut_pSink, chunk, aCount);
}

/**
 * @brief Writes a field: its prefix, zeros, its body, padded to the width.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pSpec    The specification, for the width and the - flag.
 * @param[in]     aIn_pPrefix  The sign or base prefix.
 * @param[in]     aZeros       The zeros after the prefix.
 * @param[in]     aIn_pBody    The digits or the text.
 * @param[in]     aBodyLength  The length of the body.
 */
void
__OpenCRT_Format_Field(__OpenCRT_Format_tSink*       aInOut_pSink,
                       const __OpenCRT_Format_tSpec* aIn_pSpec,
                       const char*                   aIn_pPrefix,
                       size_t                        aZeros,
                       const char*                   aIn_pBody,
                       size_t                        aBodyLength)
{
        size_t prefix  = strlen(aIn_pPrefix);
        size_t total   = prefix + aZeros + aBodyLength;
        size_t padding = aIn_pSpec->width > total ? aIn_pSpec->width - total : 0;

        if(!aIn_pSpec->isLeft)
                __OpenCRT_Format_Repeat(aInOut_pSink, ' ', padding);
        __OpenCRT_Format_Put(aInOut_pSink, aIn_pPrefix, prefix);
        __OpenCRT_Format_Repeat(aInOut_pSink, '0', aZeros);
        __OpenCRT_Format_Put(aInOut_pSink, aIn_pBody, aBodyLength);
        if(aIn_pSpec->isLeft)
                __OpenCRT_Format_Repeat(aInOut_pSink, ' ', padding);
}

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

        while(**aInOut_ppAt >= '0' && **aInOut_ppAt <= '9')
        {
                if(number <= INT_MAX)
                        number = number * 10 + (**aInOut_ppAt - '0');
                (*aInOut_ppAt)++;
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
        return (number >= 1 && number <= OPENCRT_FORMAT_POSITIONS ? (int) number : -1);
}

/**
 * @brief Reads the flags of a specification.
 *
 * @param[in,out] aInOut_ppAt  Where they start; moved past them.
 * @param[out]    aOut_pSpec   The specification.
 */
static void
ReadFlags(const char** aInOut_ppAt, __OpenCRT_Format_tSpec* aOut_pSpec)
{
        for(;; (*aInOut_ppAt)++)
        {
                switch(**aInOut_ppAt)
                {
                case '-':
                        aOut_pSpec->isLeft = true;
                        break;
                case '+':
                        aOut_pSpec->isPlus = true;
                        break;
                case ' ':
                        aOut_pSpec->isSpace = true;
                        break;
                case '#':
                        aOut_pSpec->isAlternate = true;
                        break;
                case '0':
                        aOut_pSpec->isZero = true;
                        break;
                default:
                        return;
                }
        }
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
        const char* pAt = *aInOut_ppAt;
        tLength     length;

        switch(*pAt)
        {
        case 'h':
                length = pAt[ 1 ] == 'h' ? kChar : kShort;
                break;
        case 'l':
                length = pAt[ 1 ] == 'l' ? kLongLongLength : kLongLength;
                break;
        case 'j':
                length = kIntMaxLength;
                break;
        case 'z':
                length = kSizeLength;
                break;
        case 't':
                length = kPtrdiffLength;
                break;
        case 'L':
                length = kLongDoubleLength;
                break;
        default:
                return (kDefault);
        }
        *aInOut_ppAt += length == kChar || length == kLongLongLength ? 2 : 1;
        return (length);
}

/**
 * @brief The type the value of a conversion is read as.
 *
 * @param[in] aIn_pRaw The specification.
 * @return The type, or kNone for a conversion and modifier that do not go together.
 */
static tKind
KindOf(const tRaw* aIn_pRaw)
{
        static const tKind kIntegers[] = {
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

        switch(aIn_pRaw->spec.conversion)
        {
        case 'd':
        case 'i':
        case 'u':
        case 'o':
        case 'x':
        case 'X':
                return (kIntegers[ aIn_pRaw->length ]);
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
        case 'a':
        case 'A':
                if(aIn_pRaw->length == kDefault || aIn_pRaw->length == kLongLength)
                        return (kDouble);
                return (aIn_pRaw->length == kLongDoubleLength ? kLongDouble : kNone);
        case 'c':
                if(aIn_pRaw->length == kDefault)
                        return (kInt);
                return (aIn_pRaw->length == kLongLength ? kWideChar : kNone);
        case 's':
                return (aIn_pRaw->length == kDefault || aIn_pRaw->length == kLongLength ? kPointer
                                                                                        : kNone);
        case 'p':
                return (aIn_pRaw->length == kDefault ? kPointer : kNone);
        default:
                return (kNone);
        }
}

/**
 * @brief Reads one conversion specification, after its %.
 *
 * @param[in,out] aInOut_ppAt Where it starts; moved past it.
 * @param[out]    aOut_pRaw   The specification.
 * @return false for one OpenCRT refuses: %n, an unknown conversion, a bad position or modifier.
 */
static bool
ReadSpec(const char** aInOut_ppAt, tRaw* aOut_pRaw)
{
        memset(aOut_pRaw, 0, sizeof(*aOut_pRaw));
        aOut_pRaw->spec.precision = -1;
        aOut_pRaw->position       = ReadPosition(aInOut_ppAt);
        ReadFlags(aInOut_ppAt, &aOut_pRaw->spec);
        if(**aInOut_ppAt == '*')
        {
                (*aInOut_ppAt)++;
                aOut_pRaw->hasWidthArgument = true;
                aOut_pRaw->widthPosition    = ReadPosition(aInOut_ppAt);
        } else
                aOut_pRaw->spec.width = (size_t) ReadNumber(aInOut_ppAt);
        if(**aInOut_ppAt == '.')
        {
                (*aInOut_ppAt)++;
                if(**aInOut_ppAt == '*')
                {
                        (*aInOut_ppAt)++;
                        aOut_pRaw->hasPrecisionArgument = true;
                        aOut_pRaw->precisionPosition    = ReadPosition(aInOut_ppAt);
                } else
                        aOut_pRaw->spec.precision = ReadNumber(aInOut_ppAt);
        }
        aOut_pRaw->length = ReadLength(aInOut_ppAt);
        char conversion   = *(*aInOut_ppAt)++;

        /* XSI: %C and %S are %lc and %ls. */
        if((conversion == 'C' || conversion == 'S') && aOut_pRaw->length == kDefault)
        {
                conversion        = conversion == 'C' ? 'c' : 's';
                aOut_pRaw->length = kLongLength;
        }
        aOut_pRaw->spec.conversion = conversion;
        return (aOut_pRaw->position >= 0 && aOut_pRaw->widthPosition >= 0
                && aOut_pRaw->precisionPosition >= 0 && aOut_pRaw->spec.width <= INT_MAX
                && aOut_pRaw->spec.precision <= INT_MAX && KindOf(aOut_pRaw) != kNone);
}

/** The arguments of a format, read in order or by position. */
typedef struct Arguments
{
        va_list list;
        bool    isPositional;
        tValue  values[ OPENCRT_FORMAT_POSITIONS + 1 ];
} tArguments;

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
        switch(aKind)
        {
        case kInt:
        case kWideChar:
                value.integer = va_arg(*aInOut_pList, int);
                break;
        case kLong:
                value.integer = va_arg(*aInOut_pList, long);
                break;
        case kLongLong:
                value.integer = va_arg(*aInOut_pList, long long);
                break;
        case kIntMax:
                value.integer = va_arg(*aInOut_pList, intmax_t);
                break;
        case kSize:
                value.integer = (long long) va_arg(*aInOut_pList, size_t);
                break;
        case kPtrdiff:
                value.integer = va_arg(*aInOut_pList, ptrdiff_t);
                break;
        case kDouble:
                value.floating = va_arg(*aInOut_pList, double);
                break;
        case kLongDouble:
                value.floating = va_arg(*aInOut_pList, long double);
                break;
        case kPointer:
                value.pPointer = va_arg(*aInOut_pList, const void*);
                break;
        case kNone:
                break;
        }
        return (value);
}

/**
 * @brief Records the type of a position, refusing a position given two types.
 *
 * @param[in,out] aInOut_pKinds The type of each position.
 * @param[in]     aPosition     The position.
 * @param[in]     aKind         The type.
 * @return false when the position already has another type.
 */
static bool
Claim(tKind aInOut_pKinds[ OPENCRT_FORMAT_POSITIONS + 1 ], int aPosition, tKind aKind)
{
        if(aInOut_pKinds[ aPosition ] != kNone && aInOut_pKinds[ aPosition ] != aKind)
                return (false);
        aInOut_pKinds[ aPosition ] = aKind;
        return (true);
}

/**
 * @brief Checks every specification of a format and, for positions, reads every argument first.
 *
 * @param[in]     aIn_pFormat      The format.
 * @param[in,out] aInOut_pArguments The arguments.
 * @return false for a format OpenCRT refuses.
 */
static bool
Prepare(const char* aIn_pFormat, tArguments* aInOut_pArguments)
{
        tKind kinds[ OPENCRT_FORMAT_POSITIONS + 1 ] = { kNone };
        int   highest                               = 0;
        int   plain                                 = 0;
        int   placed                                = 0;

        for(const char* pAt = aIn_pFormat; (pAt = strchr(pAt, '%')) != NULL;)
        {
                tRaw raw;

                pAt++;
                if(*pAt == '%')
                {
                        pAt++;
                        continue;
                }
                if(!ReadSpec(&pAt, &raw))
                        return (false);
                plain  += raw.position == 0;
                placed += raw.position != 0;
                if(raw.position != 0)
                {
                        bool isClaimed = Claim(kinds, raw.position, KindOf(&raw))
                                && (!raw.hasWidthArgument
                                    || (raw.widthPosition != 0
                                        && Claim(kinds, raw.widthPosition, kInt)))
                                && (!raw.hasPrecisionArgument
                                    || (raw.precisionPosition != 0
                                        && Claim(kinds, raw.precisionPosition, kInt)));

                        if(!isClaimed)
                                return (false);
                        highest = raw.position > highest ? raw.position : highest;
                        highest = raw.widthPosition > highest ? raw.widthPosition : highest;
                        highest = raw.precisionPosition > highest ? raw.precisionPosition : highest;
                } else if(raw.widthPosition != 0 || raw.precisionPosition != 0)
                        return (false);
        }
        if(plain > 0 && placed > 0)
                return (false);
        aInOut_pArguments->isPositional = placed > 0;
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
static tValue
Argument(tArguments* aInOut_pArguments, int aPosition, tKind aKind)
{
        if(aInOut_pArguments->isPositional)
                return (aInOut_pArguments->values[ aPosition ]);
        return (Fetch(&aInOut_pArguments->list, aKind));
}

/**
 * @brief Writes an integer conversion: d, i, u, o, x or X.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pRaw     The specification.
 * @param[in]     aValue       The argument, as read.
 */
static void
WriteInteger(__OpenCRT_Format_tSink* aInOut_pSink, const tRaw* aIn_pRaw, long long aValue)
{
        static const unsigned long long kMasks[] = {
                [kChar]    = UCHAR_MAX,
                [kShort]   = USHRT_MAX,
                [kDefault] = UINT_MAX,
        };
        const __OpenCRT_Format_tSpec* pSpec      = &aIn_pRaw->spec;
        char                          conversion = pSpec->conversion;
        bool                          isSigned   = conversion == 'd' || conversion == 'i';
        unsigned long long            magnitude  = (unsigned long long) aValue;
        const char*                   pPrefix    = "";

        if(aIn_pRaw->length <= kShort)
        {
                /* hh, h and int values arrive as int; keep the bits of their own type. */
                if(isSigned)
                        aValue = aIn_pRaw->length == kChar   ? (signed char) aValue
                                : aIn_pRaw->length == kShort ? (short) aValue
                                                             : (int) aValue;
                magnitude = (unsigned long long) aValue & kMasks[ aIn_pRaw->length ];
        }
        if(isSigned)
        {
                magnitude = aValue < 0 ? 0ULL - (unsigned long long) aValue
                                       : (unsigned long long) aValue;
                pPrefix   = aValue < 0 ? "-" : pSpec->isPlus ? "+" : pSpec->isSpace ? " " : "";
        }
        unsigned    base = conversion == 'o' ? 8 : conversion == 'x' || conversion == 'X' ? 16 : 10;
        const char* pDigits = conversion == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
        char        buffer[ 24 ];
        size_t      length = 0;

        for(unsigned long long rest = magnitude; rest != 0; rest /= base)
                buffer[ sizeof(buffer) - ++length ] = pDigits[ rest % base ];
        size_t precision = pSpec->precision < 0 ? 1 : (size_t) pSpec->precision;
        size_t zeros     = precision > length ? precision - length : 0;

        if(pSpec->isAlternate && base == 8 && zeros == 0
           && (length == 0 || buffer[ sizeof(buffer) - length ] != '0'))
                zeros = 1;
        if(pSpec->isAlternate && base == 16 && magnitude != 0)
                pPrefix = conversion == 'X' ? "0X" : "0x";
        size_t used = strlen(pPrefix) + zeros + length;

        if(pSpec->isZero && !pSpec->isLeft && pSpec->precision < 0 && pSpec->width > used)
                zeros += pSpec->width - used;
        __OpenCRT_Format_Field(aInOut_pSink,
                               pSpec,
                               pPrefix,
                               zeros,
                               buffer + sizeof(buffer) - length,
                               length);
}

/**
 * @brief Writes %p: 0x and the address in hexadecimal, 0x0 for a null pointer.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pRaw     The specification.
 * @param[in]     aIn_pPointer The pointer.
 */
static void
WritePointer(__OpenCRT_Format_tSink* aInOut_pSink, const tRaw* aIn_pRaw, const void* aIn_pPointer)
{
        tRaw hexadecimal = *aIn_pRaw;

        hexadecimal.spec.conversion = 'x';
        hexadecimal.length          = kLongLength;
        if(aIn_pPointer == NULL)
        {
                __OpenCRT_Format_Field(aInOut_pSink, &aIn_pRaw->spec, "", 0, "0x0", 3);
                return;
        }
        hexadecimal.spec.isAlternate = true;
        WriteInteger(aInOut_pSink, &hexadecimal, (long long) (uintptr_t) aIn_pPointer);
}

/**
 * @brief Writes %s, or %ls through the multibyte conversion of the locale.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pRaw     The specification.
 * @param[in]     aIn_pString  The string; NULL writes (null), so a missing string cannot crash.
 * @return false when a wide character has no multibyte form in the locale.
 */
static bool
WriteString(__OpenCRT_Format_tSink* aInOut_pSink, const tRaw* aIn_pRaw, const void* aIn_pString)
{
        const __OpenCRT_Format_tSpec* pSpec = &aIn_pRaw->spec;
        size_t limit = pSpec->precision < 0 ? (size_t) -1 : (size_t) pSpec->precision;

        if(aIn_pString == NULL)
                aIn_pString = aIn_pRaw->length == kLongLength ? (const void*) L"(null)"
                                                              : (const void*) "(null)";
        if(aIn_pRaw->length != kLongLength)
        {
                const char* pText = aIn_pString;

                __OpenCRT_Format_Field(aInOut_pSink, pSpec, "", 0, pText, strnlen(pText, limit));
                return (true);
        }
        /* The bytes of the wide string, at most limit, whole characters only. */
        char           bytes[ 256 ];
        size_t         length = 0;
        const wchar_t* pWide  = aIn_pString;
        mbstate_t      state;

        memset(&state, 0, sizeof(state));
        for(; *pWide != L'\0' && length < sizeof(bytes) && length < limit; pWide++)
        {
                char   character[ 8 ];
                size_t count = wcrtomb(character, *pWide, &state);

                if(count == (size_t) -1)
                        return (false);
                if(count > limit - length || count > sizeof(bytes) - length)
                        break;
                memcpy(bytes + length, character, count);
                length += count;
        }
        __OpenCRT_Format_Field(aInOut_pSink, pSpec, "", 0, bytes, length);
        return (true);
}

/**
 * @brief Writes %c, or %lc through the multibyte conversion of the locale.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pRaw     The specification.
 * @param[in]     aValue       The argument.
 * @return false when the wide character has no multibyte form in the locale.
 */
static bool
WriteCharacter(__OpenCRT_Format_tSink* aInOut_pSink, const tRaw* aIn_pRaw, long long aValue)
{
        char   bytes[ 8 ];
        size_t count = 1;

        if(aIn_pRaw->length == kLongLength)
        {
                mbstate_t state;

                memset(&state, 0, sizeof(state));
                count = wcrtomb(bytes, (wchar_t) aValue, &state);
                if(count == (size_t) -1)
                        return (false);
        } else
                bytes[ 0 ] = (char) (unsigned char) aValue;
        __OpenCRT_Format_Field(aInOut_pSink, &aIn_pRaw->spec, "", 0, bytes, count);
        return (true);
}

/**
 * @brief Writes one conversion, once its width and precision are known.
 *
 * @param[in,out] aInOut_pSink      The sink.
 * @param[in]     aIn_pRaw          The specification.
 * @param[in,out] aInOut_pArguments The arguments.
 * @return false when a wide character cannot be written.
 */
static bool
WriteConversion(__OpenCRT_Format_tSink* aInOut_pSink, tRaw* aIn_pRaw, tArguments* aInOut_pArguments)
{
        if(aIn_pRaw->hasWidthArgument)
        {
                long long width
                        = Argument(aInOut_pArguments, aIn_pRaw->widthPosition, kInt).integer;

                /* A negative width is the - flag and its magnitude. */
                aIn_pRaw->spec.isLeft |= width < 0;
                aIn_pRaw->spec.width   = (size_t) (width < 0 ? -width : width);
        }
        if(aIn_pRaw->hasPrecisionArgument)
        {
                long long precision
                        = Argument(aInOut_pArguments, aIn_pRaw->precisionPosition, kInt).integer;

                aIn_pRaw->spec.precision = precision < 0 ? -1 : (long) precision;
        }
        tValue value = Argument(aInOut_pArguments, aIn_pRaw->position, KindOf(aIn_pRaw));

        switch(aIn_pRaw->spec.conversion)
        {
        case 'c':
                return (WriteCharacter(aInOut_pSink, aIn_pRaw, value.integer));
        case 's':
                return (WriteString(aInOut_pSink, aIn_pRaw, value.pPointer));
        case 'p':
                WritePointer(aInOut_pSink, aIn_pRaw, value.pPointer);
                return (true);
        case 'd':
        case 'i':
        case 'u':
        case 'o':
        case 'x':
        case 'X':
                WriteInteger(aInOut_pSink, aIn_pRaw, value.integer);
                return (true);
        default:
                __OpenCRT_Format_Float(aInOut_pSink, &aIn_pRaw->spec, value.floating);
                return (true);
        }
}

/**
 * @brief Formats the arguments of a printf format into a sink.
 *
 * @param[in,out] aInOut_pSink  The sink.
 * @param[in]     aIn_pFormat   The format.
 * @param[in]     aIn_Arguments The arguments.
 * @return The length of the output, or -1 with errno: EINVAL, EILSEQ or EOVERFLOW.
 */
int
__OpenCRT_Format_Write(__OpenCRT_Format_tSink* aInOut_pSink,
                       const char*             aIn_pFormat,
                       va_list                 aIn_Arguments)
{
        tArguments arguments;
        bool       isOk = true;

        va_copy(arguments.list, aIn_Arguments);
        if(!Prepare(aIn_pFormat, &arguments))
        {
                va_end(arguments.list);
                errno = EINVAL;
                return (-1);
        }
        for(const char* pAt = aIn_pFormat; isOk && *pAt != '\0';)
        {
                const char* pPercent = strchr(pAt, '%');
                size_t      literal  = pPercent == NULL ? strlen(pAt) : (size_t) (pPercent - pAt);

                __OpenCRT_Format_Put(aInOut_pSink, pAt, literal);
                pAt += literal;
                if(pPercent == NULL)
                        break;
                pAt++;
                if(*pAt == '%')
                {
                        __OpenCRT_Format_Put(aInOut_pSink, "%", 1);
                        pAt++;
                        continue;
                }
                tRaw raw;

                ReadSpec(&pAt, &raw);
                isOk = WriteConversion(aInOut_pSink, &raw, &arguments);
        }
        va_end(arguments.list);
        if(!isOk)
        {
                errno = EILSEQ;
                return (-1);
        }
        if(aInOut_pSink->length > INT_MAX)
        {
                errno = EOVERFLOW;
                return (-1);
        }
        return ((int) aInOut_pSink->length);
}

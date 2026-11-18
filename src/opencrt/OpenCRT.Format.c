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
#include "opencrt/OpenCRT.FormatSpec.h"

/*
 * The output of the printf engine, every conversion of C99 included: %n stores how many bytes the
 * output has so far, in the type its length modifier names.
 */

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
 * @brief The magnitude of an integer argument and its sign, in the type its modifier names.
 *
 * hh, h and int values arrive as int; they keep the bits of their own type.
 *
 * @param[in]  aIn_pRaw         The specification.
 * @param[in]  aValue           The argument, as read.
 * @param[out] aOut_pIsNegative Whether a signed conversion is negative.
 * @return The magnitude.
 */
static unsigned long long
Magnitude(const tRaw* aIn_pRaw, long long aValue, bool* aOut_pIsNegative)
{
        static const unsigned long long kMasks[] = { [kDefault] = UINT_MAX,
                                                     [kChar]    = UCHAR_MAX,
                                                     [kShort]   = USHRT_MAX };
        bool isSigned = aIn_pRaw->spec.conversion == 'd' || aIn_pRaw->spec.conversion == 'i';

        *aOut_pIsNegative = false;
        if(!isSigned)
                return (aIn_pRaw->length <= kShort
                                ? (unsigned long long) aValue & kMasks[ aIn_pRaw->length ]
                                : (unsigned long long) aValue);
        if(aIn_pRaw->length == kChar)
                aValue = (signed char) aValue;
        else if(aIn_pRaw->length == kShort)
                aValue = (short) aValue;
        else if(aIn_pRaw->length == kDefault)
                aValue = (int) aValue;
        *aOut_pIsNegative = aValue < 0;
        return (aValue < 0 ? 0ULL - (unsigned long long) aValue : (unsigned long long) aValue);
}

/**
 * @brief Writes the digits of a magnitude at the end of a buffer.
 *
 * @param[in]  aMagnitude The magnitude.
 * @param[in]  aBase      8, 10 or 16.
 * @param[in]  aIsUpper   Whether hexadecimal digits are upper case.
 * @param[out] aOut_pEnd  One past the last byte of the buffer, which has room for 22 digits.
 * @return How many digits; none for 0.
 */
static size_t
Digits(unsigned long long aMagnitude, unsigned aBase, bool aIsUpper, char* aOut_pEnd)
{
        const char* pDigits = aIsUpper ? "0123456789ABCDEF" : "0123456789abcdef";
        size_t      length  = 0;

        for(; aMagnitude != 0; aMagnitude /= aBase)
                *(aOut_pEnd - ++length) = pDigits[ aMagnitude % aBase ];
        return (length);
}

/**
 * @brief The base of an integer conversion.
 *
 * @param[in] aConversion d, i, u, o, x or X.
 * @return 8, 10 or 16.
 */
static unsigned
BaseOf(char aConversion)
{
        if(aConversion == 'o')
                return (8);
        return (aConversion == 'x' || aConversion == 'X' ? 16 : 10);
}

/**
 * @brief The prefix of an integer conversion: a sign for d and i, 0x or 0X for # with x or X.
 *
 * @param[in] aIn_pSpec    The specification.
 * @param[in] aIsNegative  Whether the value is negative.
 * @param[in] aIsZero      Whether the value is zero.
 * @return The prefix.
 */
static const char*
IntegerPrefix(const __OpenCRT_Format_tSpec* aIn_pSpec, bool aIsNegative, bool aIsZero)
{
        char conversion = aIn_pSpec->conversion;

        if(conversion == 'd' || conversion == 'i')
                return (aIsNegative                  ? "-"
                                : aIn_pSpec->isPlus  ? "+"
                                : aIn_pSpec->isSpace ? " "
                                                     : "");
        if(aIn_pSpec->isAlternate && BaseOf(conversion) == 16 && !aIsZero)
                return (conversion == 'X' ? "0X" : "0x");
        return ("");
}

/**
 * @brief The zeros the 0 flag adds to reach the width, when no precision is given.
 *
 * @param[in] aIn_pSpec The specification.
 * @param[in] aUsed     What the field holds already.
 * @return How many zeros.
 */
static size_t
ZeroFill(const __OpenCRT_Format_tSpec* aIn_pSpec, size_t aUsed)
{
        bool isFilled = aIn_pSpec->isZero && !aIn_pSpec->isLeft && aIn_pSpec->precision < 0;

        return (isFilled && aIn_pSpec->width > aUsed ? aIn_pSpec->width - aUsed : 0);
}

/**
 * @brief The zeros between the prefix and the digits: for the precision, for # with o, and for
 *        the 0 flag up to the width.
 *
 * @param[in] aIn_pSpec    The specification.
 * @param[in] aIn_pDigits  The digits.
 * @param[in] aLength      How many.
 * @param[in] aPrefix      The length of the prefix.
 * @return How many zeros.
 */
static size_t
IntegerZeros(const __OpenCRT_Format_tSpec* aIn_pSpec,
             const char*                   aIn_pDigits,
             size_t                        aLength,
             size_t                        aPrefix)
{
        size_t precision = aIn_pSpec->precision < 0 ? 1 : (size_t) aIn_pSpec->precision;
        size_t zeros     = precision > aLength ? precision - aLength : 0;
        /* # puts a 0 before octal digits that do not already start with one. */
        bool needsOctalZero = aIn_pSpec->isAlternate && aIn_pSpec->conversion == 'o' && zeros == 0;

        zeros += needsOctalZero && (aLength == 0 || aIn_pDigits[ 0 ] != '0');
        return (zeros + ZeroFill(aIn_pSpec, aPrefix + zeros + aLength));
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
        const __OpenCRT_Format_tSpec* pSpec = &aIn_pRaw->spec;
        bool                          isNegative;
        unsigned long long            magnitude = Magnitude(aIn_pRaw, aValue, &isNegative);
        char                          buffer[ 24 ];
        size_t                        length  = Digits(magnitude,
                                                       BaseOf(pSpec->conversion),
                                                       pSpec->conversion == 'X',
                                                       buffer + sizeof(buffer));
        const char*                   pDigits = buffer + sizeof(buffer) - length;
        const char*                   pPrefix = IntegerPrefix(pSpec, isNegative, magnitude == 0);
        size_t                        zeros = IntegerZeros(pSpec, pDigits, length, strlen(pPrefix));

        __OpenCRT_Format_Field(aInOut_pSink, pSpec, pPrefix, zeros, pDigits, length);
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

        if(aIn_pPointer == NULL)
        {
                __OpenCRT_Format_Field(aInOut_pSink, &aIn_pRaw->spec, "", 0, "0x0", 3);
                return;
        }
        hexadecimal.spec.conversion  = 'x';
        hexadecimal.spec.isAlternate = true;
        hexadecimal.length           = kLongLength;
        WriteInteger(aInOut_pSink, &hexadecimal, (long long) (uintptr_t) aIn_pPointer);
}

/**
 * @brief Writes %ls through the multibyte conversion of the locale: whole characters, at most the
 *        precision in bytes.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pSpec    The specification.
 * @param[in]     aIn_pWide    The wide string.
 * @return false when a wide character has no multibyte form in the locale.
 */
static bool
WriteWideString(__OpenCRT_Format_tSink*       aInOut_pSink,
                const __OpenCRT_Format_tSpec* aIn_pSpec,
                const wchar_t*                aIn_pWide)
{
        size_t    limit = aIn_pSpec->precision < 0 ? (size_t) -1 : (size_t) aIn_pSpec->precision;
        char      bytes[ 256 ] = { 0 };
        size_t    length       = 0;
        mbstate_t state;

        memset(&state, 0, sizeof(state));
        for(; *aIn_pWide != L'\0' && length < limit; aIn_pWide++)
        {
                char   character[ 8 ];
                size_t count = wcrtomb(character, *aIn_pWide, &state);

                if(count == (size_t) -1)
                        return (false);
                if(count > limit - length || count > sizeof(bytes) - length)
                        break;
                memcpy(bytes + length, character, count);
                length += count;
        }
        __OpenCRT_Format_Field(aInOut_pSink, aIn_pSpec, "", 0, bytes, length);
        return (true);
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

        if(aIn_pRaw->length == kLongLength)
                return (WriteWideString(aInOut_pSink,
                                        pSpec,
                                        aIn_pString != NULL ? aIn_pString : L"(null)"));
        const char* pText = aIn_pString != NULL ? aIn_pString : "(null)";
        size_t      limit = pSpec->precision < 0 ? (size_t) -1 : (size_t) pSpec->precision;

        __OpenCRT_Format_Field(aInOut_pSink, pSpec, "", 0, pText, strnlen(pText, limit));
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

        bytes[ 0 ] = (char) (unsigned char) aValue;
        if(aIn_pRaw->length == kLongLength)
        {
                mbstate_t state;

                memset(&state, 0, sizeof(state));
                count = wcrtomb(bytes, (wchar_t) aValue, &state);
                if(count == (size_t) -1)
                        return (false);
        }
        __OpenCRT_Format_Field(aInOut_pSink, &aIn_pRaw->spec, "", 0, bytes, count);
        return (true);
}

/**
 * @brief Writes %n: stores how many bytes the output has so far, in the type its modifier names.
 *
 * @param[in] aIn_pSink   The sink, whose length is the count.
 * @param[in] aLength     The length modifier.
 * @param[in] aIn_pTarget Where the count goes; a null pointer stores nothing.
 */
static void
WriteCount(const __OpenCRT_Format_tSink* aIn_pSink, tLength aLength, const void* aIn_pTarget)
{
        size_t count   = aIn_pSink->length;
        void*  pTarget = (void*) (uintptr_t) aIn_pTarget;

        if(pTarget == NULL)
                return;
        switch(aLength)
        {
        case kChar:
                *(signed char*) pTarget = (signed char) count;
                break;
        case kShort:
                *(short*) pTarget = (short) count;
                break;
        case kLongLength:
                *(long*) pTarget = (long) count;
                break;
        case kLongLongLength:
                *(long long*) pTarget = (long long) count;
                break;
        case kIntMaxLength:
                *(intmax_t*) pTarget = (intmax_t) count;
                break;
        case kSizeLength:
                *(size_t*) pTarget = count;
                break;
        case kPtrdiffLength:
                *(ptrdiff_t*) pTarget = (ptrdiff_t) count;
                break;
        default:
                *(int*) pTarget = (int) count;
                break;
        }
}

/**
 * @brief Reads the width and the precision a * asks for: a negative width is the - flag, a negative
 *        precision none.
 *
 * @param[in,out] aInOut_pRaw       The specification.
 * @param[in,out] aInOut_pArguments The arguments.
 */
static void
ResolveAmounts(tRaw* aInOut_pRaw, tArguments* aInOut_pArguments)
{
        if(aInOut_pRaw->hasWidthArgument)
        {
                long long width = __OpenCRT_FormatSpec_Argument(aInOut_pArguments,
                                                                aInOut_pRaw->widthPosition,
                                                                kInt)
                                          .integer;

                aInOut_pRaw->spec.isLeft |= width < 0;
                aInOut_pRaw->spec.width   = (size_t) (width < 0 ? -width : width);
        }
        if(aInOut_pRaw->hasPrecisionArgument)
        {
                long long precision = __OpenCRT_FormatSpec_Argument(aInOut_pArguments,
                                                                    aInOut_pRaw->precisionPosition,
                                                                    kInt)
                                              .integer;

                aInOut_pRaw->spec.precision = precision < 0 ? -1 : (long) precision;
        }
}

/**
 * @brief Writes one conversion.
 *
 * @param[in,out] aInOut_pSink      The sink.
 * @param[in]     aIn_pRaw          The specification.
 * @param[in,out] aInOut_pArguments The arguments.
 * @return false when a wide character cannot be written.
 */
static bool
WriteConversion(__OpenCRT_Format_tSink* aInOut_pSink, tRaw* aIn_pRaw, tArguments* aInOut_pArguments)
{
        ResolveAmounts(aIn_pRaw, aInOut_pArguments);
        char   conversion = aIn_pRaw->spec.conversion;
        tValue value      = __OpenCRT_FormatSpec_Argument(aInOut_pArguments,
                                                          aIn_pRaw->position,
                                                          __OpenCRT_FormatSpec_KindOf(aIn_pRaw));

        if(conversion == 'c')
                return (WriteCharacter(aInOut_pSink, aIn_pRaw, value.integer));
        if(conversion == 's')
                return (WriteString(aInOut_pSink, aIn_pRaw, value.pPointer));
        if(conversion == 'n')
                WriteCount(aInOut_pSink, aIn_pRaw->length, value.pPointer);
        else if(conversion == 'p')
                WritePointer(aInOut_pSink, aIn_pRaw, value.pPointer);
        else if(strchr("diouxX", conversion) != NULL)
                WriteInteger(aInOut_pSink, aIn_pRaw, value.integer);
        else
                __OpenCRT_Format_Float(aInOut_pSink, &aIn_pRaw->spec, value.floating);
        return (true);
}

/**
 * @brief Writes the text of a format and its conversions, once the format is known to be valid.
 *
 * @param[in,out] aInOut_pSink      The sink.
 * @param[in]     aIn_pFormat       The format.
 * @param[in,out] aInOut_pArguments The arguments.
 * @return false when a wide character cannot be written.
 */
static bool
WriteAll(__OpenCRT_Format_tSink* aInOut_pSink,
         const char*             aIn_pFormat,
         tArguments*             aInOut_pArguments)
{
        for(const char* pAt = aIn_pFormat; *pAt != '\0';)
        {
                const char* pPercent = strchr(pAt, '%');
                size_t      literal  = pPercent == NULL ? strlen(pAt) : (size_t) (pPercent - pAt);
                tRaw        raw;

                __OpenCRT_Format_Put(aInOut_pSink, pAt, literal);
                if(pPercent == NULL)
                        return (true);
                pAt = pPercent + 1;
                if(*pAt == '%')
                {
                        __OpenCRT_Format_Put(aInOut_pSink, "%", 1);
                        pAt++;
                        continue;
                }
                __OpenCRT_FormatSpec_Read(&pAt, &raw);
                if(!WriteConversion(aInOut_pSink, &raw, aInOut_pArguments))
                        return (false);
        }
        return (true);
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
        int        error = 0;

        va_copy(arguments.list, aIn_Arguments);
        if(!__OpenCRT_FormatSpec_Prepare(aIn_pFormat, &arguments))
                error = EINVAL;
        else if(!WriteAll(aInOut_pSink, aIn_pFormat, &arguments))
                error = EILSEQ;
        else if(aInOut_pSink->length > INT_MAX)
                error = EOVERFLOW;
        va_end(arguments.list);
        if(error != 0)
        {
                errno = error;
                return (-1);
        }
        return ((int) aInOut_pSink->length);
}

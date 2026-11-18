/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Test.h"

/**
 * @brief Formats and checks the text and the length snprintf returns.
 *
 * @param[in] aLine       The line of the case, for the report.
 * @param[in] aIn_pExpected The text.
 * @param[in] aIn_pFormat The format, then its arguments.
 */
static void
CheckFormat(int aLine, const char* aIn_pExpected, const char* aIn_pFormat, ...)
{
        static char buffer[ 8192 ];
        va_list     arguments;

        va_start(arguments, aIn_pFormat);
        int length = vsnprintf(buffer, sizeof(buffer), aIn_pFormat, arguments);

        va_end(arguments);
        Test_CheckString(aIn_pFormat, aLine, buffer, aIn_pExpected);
        Test_CheckInteger(aIn_pFormat, aLine, "length", length, (long long) strlen(aIn_pExpected));
}

/**
 * @brief Formats a long output and checks its length and its first characters.
 *
 * @param[in] aLine         The line of the case.
 * @param[in] aLength       The length.
 * @param[in] aIn_pPrefix   The first characters.
 * @param[in] aIn_pFormat   The format, then its arguments.
 */
static void
CheckLongPrefix(int aLine, size_t aLength, const char* aIn_pPrefix, const char* aIn_pFormat, ...)
{
        static char buffer[ 8192 ];
        va_list     arguments;

        va_start(arguments, aIn_pFormat);
        int length = vsnprintf(buffer, sizeof(buffer), aIn_pFormat, arguments);

        va_end(arguments);
        buffer[ strlen(aIn_pPrefix) ] = '\0';
        Test_CheckString(aIn_pFormat, aLine, buffer, aIn_pPrefix);
        Test_CheckInteger(aIn_pFormat, aLine, "length", length, (long long) aLength);
}

/**
 * @brief Formats with a format the compiler does not check, for formats that must be refused.
 *
 * @param[out] aOut_pBuffer The buffer.
 * @param[in]  aSize        Its size.
 * @param[in]  aIn_pFormat  The format, then its arguments.
 * @return What vsnprintf returns.
 */
static int
Unchecked(char* aOut_pBuffer, size_t aSize, const char* aIn_pFormat, ...)
{
        va_list arguments;

        va_start(arguments, aIn_pFormat);
        int length = vsnprintf(aOut_pBuffer, aSize, aIn_pFormat, arguments);

        va_end(arguments);
        return (length);
}

#include "PrintfCases.inc"

/** @brief The integer conversions with every flag, width, precision and length. */
static void
Integers(void)
{
        CheckFormat(__LINE__, "42 -42 +42  42", "%d %i %+d % d", 42, -42, 42, 42);
        CheckFormat(__LINE__, "00042|   42|42   |-0042", "%05d|%5d|%-5d|%05d", 42, 42, 42, -42);
        CheckFormat(__LINE__, "  042|0042", "%5.3d|%.4d", 42, 42);
        CheckFormat(__LINE__, "|   |", "|%.0d%3.0d|", 0, 0);
        CheckFormat(__LINE__, "2147483647 -2147483648", "%d %d", INT_MAX, INT_MIN);
        CheckFormat(__LINE__,
                    "9223372036854775807 -9223372036854775808",
                    "%ld %lld",
                    LONG_MAX,
                    LLONG_MIN);
        CheckFormat(__LINE__, "4294967295 18446744073709551615", "%u %lu", UINT_MAX, ULONG_MAX);
        CheckFormat(__LINE__, "ff FF 0xff 0XFF 0", "%x %X %#x %#X %#x", 255, 255, 255, 255, 0);
        CheckFormat(__LINE__, "17 017 0 0", "%o %#o %#o %#.0o", 15, 15, 0, 0);
        CheckFormat(__LINE__, "0x000ff|0x0000ff", "%#07x|%#.6x", 255, 255);
        CheckFormat(__LINE__, "-1 255 -1 65535", "%hhd %hhu %hd %hu", 255, 255, 65535, 65535);
        CheckFormat(__LINE__, "-128 127", "%hhd %hhd", 128, 127);
        CheckFormat(__LINE__,
                    "123 456 789 -5",
                    "%jd %zu %td %zd",
                    (intmax_t) 123,
                    (size_t) 456,
                    (ptrdiff_t) 789,
                    (ptrdiff_t) -5);
        CheckFormat(__LINE__, "[      42]", "[%*d]", 8, 42);
        CheckFormat(__LINE__, "[42      ]", "[%*d]", -8, 42);
        CheckFormat(__LINE__, "[00042]", "[%.*d]", 5, 42);
        CheckFormat(__LINE__, "[42]", "[%.*d]", -1, 42);
        CheckFormat(__LINE__, "100% done", "%d%% done", 100);
}

/** @brief %c, %s, %p, and their wide forms. */
static void
CharactersAndStrings(void)
{
        CheckFormat(__LINE__, "a|  b|c  |", "%c|%3c|%-3c|", 'a', 'b', 'c');
        CheckFormat(__LINE__, "hello|  hel|hel  |", "%s|%5.3s|%-5.3s|", "hello", "hello", "hello");
        CheckFormat(__LINE__, "(null)", "%s", (char*) NULL);
        CheckFormat(__LINE__, "0x0 0x1234", "%p %p", (void*) NULL, (void*) 0x1234);
        CheckFormat(__LINE__, "     0x1234", "%11p", (void*) 0x1234);
        CheckFormat(__LINE__, "wide x", "%ls %lc", L"wide", (wint_t) L'x');
        CheckFormat(__LINE__, "wi", "%.2ls", L"wide");
        CheckFormat(__LINE__, "(null)", "%ls", (wchar_t*) NULL);
        CheckFormat(__LINE__, "\xE9", "%lc", (wint_t) 0xDFE9);
}

/** @brief %n stores the count so far, in the type of its modifier. */
static void
Counts(void)
{
        char        buffer[ 8 ];
        int         count      = -1;
        signed char small      = -1;
        short       middle     = -1;
        long        large      = -1;
        long long   larger     = -1;
        intmax_t    widest     = -1;
        size_t      size       = 0;
        ptrdiff_t   distance   = -1;
        int         positional = -1;

        TEST_INTEGER(snprintf(buffer, sizeof(buffer), "ab%ncd", &count), 4);
        TEST_INTEGER(count, 2);
        TEST_STRING(buffer, "abcd");
        TEST_INTEGER(snprintf(buffer, sizeof(buffer), "%n", &count), 0);
        TEST_INTEGER(count, 0);
        /* The count is of the whole output, not of what fits. */
        TEST_INTEGER(snprintf(buffer, 4, "abcdefgh%n!", &count), 9);
        TEST_INTEGER(count, 8);
        TEST_INTEGER(snprintf(buffer,
                              sizeof(buffer),
                              "%d%hhn%hn%ln%lln%jn%zn%tn",
                              123,
                              &small,
                              &middle,
                              &large,
                              &larger,
                              &widest,
                              &size,
                              &distance),
                     3);
        TEST_CHECK(small == 3 && middle == 3 && large == 3 && larger == 3);
        TEST_CHECK(widest == 3 && size == 3 && distance == 3);
        TEST_INTEGER(snprintf(buffer, sizeof(buffer), "%2$s%1$n", &positional, "xyz"), 3);
        TEST_INTEGER(positional, 3);
}

/** @brief Unknown conversions and bad modifiers are refused. */
static void
Refusals(void)
{
        char buffer[ 16 ] = "unchanged";

        errno = 0;
        TEST_INTEGER(Unchecked(buffer, sizeof(buffer), "%Ln", buffer), -1);
        TEST_INTEGER(errno, EINVAL);
        TEST_STRING(buffer, "");
        TEST_INTEGER(Unchecked(buffer, sizeof(buffer), "%q", 1), -1);
        TEST_INTEGER(Unchecked(buffer, sizeof(buffer), "%Ld", 1), -1);
        TEST_INTEGER(Unchecked(buffer, sizeof(buffer), "trailing %"), -1);
        errno = 0;
        TEST_INTEGER(Unchecked(buffer, sizeof(buffer), "%lc", (wint_t) 0x263A), -1);
        TEST_INTEGER(errno, EILSEQ);
}

/** @brief Arguments by position, as POSIX has them. */
static void
Positions(void)
{
        CheckFormat(__LINE__, "b a b", "%2$s %1$s %2$s", "a", "b");
        CheckFormat(__LINE__, "[   x] 3.50", "[%2$*1$s] %3$.2f", 4, "x", 3.5);
        CheckFormat(__LINE__, "[ 7]", "[%1$*2$d]", 7, 2);
        char buffer[ 16 ];

        TEST_INTEGER(Unchecked(buffer, sizeof(buffer), "%1$d %d", 1, 2), -1);
        TEST_INTEGER(Unchecked(buffer, sizeof(buffer), "%2$d", 1, 2), -1);
        TEST_INTEGER(Unchecked(buffer, sizeof(buffer), "%1$d %1$s", 1), -1);
        TEST_INTEGER(Unchecked(buffer, sizeof(buffer), "%10$d", 1), -1);
}

/** @brief The buffer keeps what fits and always ends with a null character. */
static void
Truncation(void)
{
        char buffer[ 8 ];

        TEST_INTEGER(snprintf(buffer, sizeof(buffer), "%s", "abcdefghijk"), 11);
        TEST_STRING(buffer, "abcdefg");
        TEST_INTEGER(snprintf(buffer, 1, "%d", 12345), 5);
        TEST_STRING(buffer, "");
        TEST_INTEGER(snprintf(NULL, 0, "%d-%s", 42, "x"), 4);
        TEST_INTEGER(sprintf(buffer, "%03d", 7), 3);
        TEST_STRING(buffer, "007");
        TEST_INTEGER(snprintf(buffer, sizeof(buffer), "%2000d", 1), 2000);
        TEST_STRING(buffer, "       ");
}

/** @brief What glibc writes differently on purpose: %La with a leading 1, %a of subnormals. */
static void
Hexadecimal(void)
{
        CheckFormat(__LINE__, "0x1p+0 0x1.999999999999999ap-4", "%La %La", 1.0L, 0.1L);
        CheckFormat(__LINE__, "-0x1.8p+0 0x1.fffffffffffffffep+16383", "%La %La", -1.5L, LDBL_MAX);
        CheckFormat(__LINE__, "0x1p-16445", "%La", 0x0.0000000000000002p-16382L);
        CheckFormat(__LINE__, "0x1.0p+0 0x2p+0", "%.1La %.0La", 1.03L, 1.5L);
        CheckFormat(__LINE__, "0x1p-1074 0x1.2688b70e62bp-1030", "%a %a", 5e-324, 1e-310);
        CheckFormat(__LINE__, "0x2.000p+0", "%.3a", 1.99999);
        CheckFormat(__LINE__, "0x0p+0 -0x0p+0", "%a %a", 0.0, -0.0);
}

/** @brief Infinities and NaNs, with their sign, never zero-padded. */
static void
SpecialValues(void)
{
        double infinity = __builtin_inf();
        double nan      = __builtin_nan("");

        CheckFormat(__LINE__,
                    "inf -inf INF nan NAN",
                    "%f %f %F %g %G",
                    infinity,
                    -infinity,
                    infinity,
                    nan,
                    nan);
        CheckFormat(__LINE__, "  inf|-inf |+inf", "%05f|%-5f|%+e", infinity, -infinity, infinity);
        CheckFormat(__LINE__, "-nan", "%f", -nan);
        CheckFormat(__LINE__, "inf", "%Lf", (long double) infinity);
}

static const Test_tCase kCases[] = {
        { "Doubles", Doubles },         { "LongDoubles", LongDoubles },
        { "Integers", Integers },       { "CharactersAndStrings", CharactersAndStrings },
        { "Counts", Counts },           { "Refusals", Refusals },
        { "Positions", Positions },     { "Truncation", Truncation },
        { "Hexadecimal", Hexadecimal }, { "SpecialValues", SpecialValues },
};

TEST_SUITE(gTest_Printf, kCases);

/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>

#include "Test.h"

/** @brief The wide string functions. */
static void
WideStrings(void)
{
        wchar_t  buffer[ 32 ];
        wchar_t* pSave  = NULL;
        wchar_t  text[] = L"  one two";

        TEST_INTEGER(wcslen(L"wide"), 4);
        TEST_INTEGER(wcsnlen(L"wide", 2), 2);
        wcscpy(buffer, L"ab");
        wcscat(buffer, L"cd");
        wcsncat(buffer, L"efgh", 2);
        TEST_INTEGER(wcscmp(buffer, L"abcdef"), 0);
        TEST_INTEGER(wcsncmp(L"abX", L"abY", 2), 0);
        TEST_INTEGER(wcscmp(L"a", L"b") < 0, 1);
        TEST_CHECK(wcschr(buffer, L'c') == buffer + 2);
        TEST_CHECK(wcsrchr(L"abca", L'a') != NULL);
        TEST_CHECK(wcsstr(buffer, L"cde") == buffer + 2);
        TEST_CHECK(wcsstr(buffer, L"") == buffer);
        TEST_CHECK(wcsstr(buffer, L"x") == NULL);
        TEST_INTEGER(wcsspn(L"aabc", L"a"), 2);
        TEST_INTEGER(wcscspn(L"aabc", L"c"), 3);
        TEST_CHECK(wcspbrk(L"hello", L"lo") != NULL);
        wmemmove(buffer + 1, buffer, 3);
        TEST_INTEGER(wmemcmp(buffer, L"aabcef", 6), 0);
        wmemset(buffer, L'z', 3);
        TEST_CHECK(wmemchr(buffer, L'z', 3) == buffer);
        TEST_STRING(wcstok(text, L" ", &pSave) != NULL ? "token" : "none", "token");
        TEST_INTEGER(wcscmp(wcstok(NULL, L" ", &pSave), L"two"), 0);
        TEST_CHECK(wcstok(NULL, L" ", &pSave) == NULL);
        wcsncpy(buffer, L"ab", 5);
        TEST_INTEGER(buffer[ 4 ], 0);
        TEST_INTEGER(wcsxfrm(buffer, L"key", 32), 3);
        TEST_INTEGER(wcscoll(L"a", L"a"), 0);
}

/** @brief The C locale: every byte is a character, ASCII as itself, the others as U+DF80 to U+DFFF. */
static void
Multibyte(void)
{
        wchar_t     wide[ 8 ];
        char        bytes[ 8 ];
        mbstate_t   state;
        const char* pSource = "a\xE9z";

        memset(&state, 0, sizeof(state));
        TEST_INTEGER(mbsinit(&state), 1);
        TEST_INTEGER(btowc('A'), 'A');
        TEST_INTEGER(btowc(0xE9), 0xDFE9);
        TEST_INTEGER(btowc(EOF), (long long) WEOF);
        TEST_INTEGER(wctob(0xDFE9), 0xE9);
        TEST_INTEGER(wctob(0xE9), EOF);
        TEST_INTEGER(mbrtowc(wide, "\xE9", 1, &state), 1);
        TEST_INTEGER(wide[ 0 ], 0xDFE9);
        TEST_INTEGER(mbrtowc(wide, "", 1, &state), 0);
        TEST_INTEGER((long long) mbrtowc(wide, "x", 0, &state), (long long) (size_t) -2);
        TEST_INTEGER(mbrlen("x", 1, &state), 1);
        TEST_INTEGER(wcrtomb(bytes, 0xDFE9, &state), 1);
        TEST_INTEGER((unsigned char) bytes[ 0 ], 0xE9);
        errno = 0;
        TEST_INTEGER((long long) wcrtomb(bytes, 0x263A, &state), (long long) (size_t) -1);
        TEST_INTEGER(errno, EILSEQ);
        TEST_INTEGER(mbsrtowcs(wide, &pSource, 8, &state), 3);
        TEST_CHECK(pSource == NULL);
        TEST_INTEGER(wide[ 1 ], 0xDFE9);
        const wchar_t* pWide = wide;

        TEST_INTEGER(wcsrtombs(bytes, &pWide, 8, &state), 3);
        TEST_STRING(bytes, "a\xE9z");
        TEST_INTEGER(mbstowcs(NULL, "four", 0), 4);
        TEST_INTEGER(wcstombs(NULL, L"four", 0), 4);
        const wchar_t bad[] = { L'a', 0x263A, 0 };

        TEST_INTEGER((long long) wcstombs(bytes, bad, 8), (long long) (size_t) -1);
        TEST_INTEGER(mblen("x", 1), 1);
        TEST_INTEGER(mblen(NULL, 0), 0);
        TEST_INTEGER(mbtowc(wide, "q", 1), 1);
        TEST_INTEGER(wctomb(bytes, L'q'), 1);
        TEST_INTEGER(wctomb(NULL, L'q'), 0);
        TEST_INTEGER(MB_CUR_MAX, 1);
}

/** @brief The wide classes are those of ASCII. */
static void
WideClasses(void)
{
        TEST_CHECK(iswalpha(L'a') && iswdigit(L'7') && iswspace(L'\t') && iswupper(L'Q'));
        TEST_CHECK(!iswalpha(0xDFE9) && !iswprint(0x263A));
        TEST_INTEGER(towupper(L'a'), L'A');
        TEST_INTEGER(towlower(0x263A), 0x263A);
        TEST_CHECK(iswctype(L'f', wctype("xdigit")));
        TEST_INTEGER(wctype("nothing"), 0);
        TEST_INTEGER(towctrans(L'b', wctrans("toupper")), L'B');
        TEST_INTEGER(wctrans("nothing"), 0);
}

/** @brief The C locale is the one locale. */
static void
Locales(void)
{
        TEST_STRING(setlocale(LC_ALL, NULL), "C");
        TEST_STRING(setlocale(LC_ALL, ""), "C");
        TEST_STRING(setlocale(LC_NUMERIC, "POSIX"), "C");
        TEST_CHECK(setlocale(LC_ALL, "tr_TR.UTF-8") == NULL);
        TEST_CHECK(setlocale(99, "C") == NULL);
        struct lconv* pConventions = localeconv();

        TEST_STRING(pConventions->decimal_point, ".");
        TEST_STRING(pConventions->thousands_sep, "");
        TEST_INTEGER(pConventions->frac_digits, CHAR_MAX);
}

/**
 * @brief Checks a double result bit for bit, so that -0.0 and 0.0 differ.
 *
 * @param[in] aLine     The line.
 * @param[in] aActual   The result.
 * @param[in] aExpected The value it should be.
 */
static void
CheckDouble(int aLine, double aActual, double aExpected)
{
        char actual[ 64 ];
        char expected[ 64 ];

        snprintf(actual, sizeof(actual), "%a", aActual);
        snprintf(expected, sizeof(expected), "%a", aExpected);
        Test_CheckString("math", aLine, actual, expected);
}

/** @brief The exact functions of <math.h>, at their edges. */
static void
ExactMath(void)
{
        CheckDouble(__LINE__, fabs(-0.0), 0.0);
        CheckDouble(__LINE__, copysign(3.0, -0.0), -3.0);
        CheckDouble(__LINE__, trunc(-2.7), -2.0);
        CheckDouble(__LINE__, trunc(-0.5), -0.0);
        CheckDouble(__LINE__, floor(-0.5), -1.0);
        CheckDouble(__LINE__, floor(0.5), 0.0);
        CheckDouble(__LINE__, ceil(-0.5), -0.0);
        CheckDouble(__LINE__, ceil(4.0000001), 5.0);
        CheckDouble(__LINE__, round(2.5), 3.0);
        CheckDouble(__LINE__, round(-2.5), -3.0);
        CheckDouble(__LINE__, round(0.49999999999999994), 0.0);
        CheckDouble(__LINE__, floor(0x1p60 + 0.0), 0x1p60);
        TEST_CHECK(isnan(floor(NAN)) && isinf(ceil(INFINITY)));
        TEST_INTEGER(lround(-3.5), -4);
        TEST_INTEGER(lround(NAN), LONG_MIN);
        TEST_INTEGER(llround(1e300), LLONG_MIN);
        CheckDouble(__LINE__, fmin(NAN, 1.0), 1.0);
        CheckDouble(__LINE__, fmax(-0.0, 0.0), 0.0);
        CheckDouble(__LINE__, fdim(1.0, 3.0), 0.0);
        CheckDouble(__LINE__, sqrt(2.0), 1.4142135623730951);
        TEST_CHECK(isnan(sqrt(-1.0)));
        TEST_CHECK(sqrtf(16.0F) == 4.0F);
        int exponent = 0;

        CheckDouble(__LINE__, frexp(8.0, &exponent), 0.5);
        TEST_INTEGER(exponent, 4);
        CheckDouble(__LINE__, frexp(5e-324, &exponent), 0.5);
        TEST_INTEGER(exponent, -1073);
        CheckDouble(__LINE__, ldexp(1.0, -1074), 5e-324);
        CheckDouble(__LINE__, ldexp(1.0, 1024), INFINITY);
        CheckDouble(__LINE__, scalbn(0x1.8p-1022, -1), 0x1.8p-1023);
        CheckDouble(__LINE__, scalbn(0x1p-1074, 2097), 0x1p1023);
        CheckDouble(__LINE__, scalbn(0x1p-1074, 2098), INFINITY);
        CheckDouble(__LINE__, scalbn(3.0, -1075), 0x1p-1073);
        double whole;

        CheckDouble(__LINE__, modf(-3.25, &whole), -0.25);
        CheckDouble(__LINE__, whole, -3.0);
        CheckDouble(__LINE__, modf(-INFINITY, &whole), -0.0);
        CheckDouble(__LINE__, fmod(5.5, 2.0), 1.5);
        CheckDouble(__LINE__, fmod(-5.5, 2.0), -1.5);
        CheckDouble(__LINE__, fmod(1e300, 3.0), 0.0);
        CheckDouble(__LINE__, fmod(1e300, 7.0), 1.0);
        CheckDouble(__LINE__, fmod(0x1p-1070, 0x1.8p-1073), 0x1p-1074);
        CheckDouble(__LINE__, fmod(0x1.FFFFFFFFFFFFFp1023, 0x1p-1074), 0.0);
        CheckDouble(__LINE__, fmod(0x1p-1070, 0x1p-1073), 0.0);
        CheckDouble(__LINE__, fmod(3.0, INFINITY), 3.0);
        TEST_CHECK(isnan(fmod(1.0, 0.0)) && isnan(fmod(INFINITY, 2.0)));
        TEST_CHECK(isnan(nan("")));
        TEST_CHECK(floorl(-2.5L) == -3.0L && ceill(2.25L) == 3.0L && roundl(-0.5L) == -1.0L);
        TEST_CHECK(truncl(0x1p62L + 0.5L) == 0x1p62L && truncl(-0.75L) == 0.0L
                   && signbit(truncl(-0.75L)));
        TEST_CHECK(fabsl(-2.0L) == 2.0L && copysignl(2.0L, -1.0L) == -2.0L);
        TEST_CHECK(fabsf(-1.5F) == 1.5F && copysignf(1.0F, -0.0F) == -1.0F);
        TEST_CHECK(truncf(-1.5F) == -1.0F && floorf(1.5F) == 1.0F && ceilf(1.5F) == 2.0F
                   && roundf(1.5F) == 2.0F);
        TEST_INTEGER(fpclassify(5e-324), FP_SUBNORMAL);
        TEST_INTEGER(math_errhandling, MATH_ERREXCEPT);
}

/**
 * @brief Checks gmtime_r, timegm and strftime on one time.
 *
 * @param[in] aSeconds   The time.
 * @param[in] aYear      tm_year.
 * @param[in] aMonth     tm_mon.
 * @param[in] aDay       tm_mday.
 * @param[in] aHour      tm_hour.
 * @param[in] aMinute    tm_min.
 * @param[in] aSecond    tm_sec.
 * @param[in] aWeekday   tm_wday.
 * @param[in] aYearDay   tm_yday.
 * @param[in] aIn_pWeeks What "%G-W%V-%u %U %W %j %a %b %C %y %g" gives.
 */
static void
CheckTime(long long   aSeconds,
          int         aYear,
          int         aMonth,
          int         aDay,
          int         aHour,
          int         aMinute,
          int         aSecond,
          int         aWeekday,
          int         aYearDay,
          const char* aIn_pWeeks)
{
        time_t    seconds = (time_t) aSeconds;
        struct tm fields;
        char      text[ 128 ];

        TEST_CHECK(gmtime_r(&seconds, &fields) == &fields);
        TEST_INTEGER(fields.tm_year, aYear);
        TEST_INTEGER(fields.tm_mon, aMonth);
        TEST_INTEGER(fields.tm_mday, aDay);
        TEST_INTEGER(fields.tm_hour, aHour);
        TEST_INTEGER(fields.tm_min, aMinute);
        TEST_INTEGER(fields.tm_sec, aSecond);
        TEST_INTEGER(fields.tm_wday, aWeekday);
        TEST_INTEGER(fields.tm_yday, aYearDay);
        TEST_INTEGER(timegm(&fields), aSeconds);
        strftime(text, sizeof(text), "%G-W%V-%u %U %W %j %a %b %C %y %g", &fields);
        TEST_STRING(text, aIn_pWeeks);
}

/** @brief Dates across eras, leap years and ISO weeks, checked against Python's datetime. */
static void
Dates(void)
{
        CheckTime(0LL, 70, 0, 1, 0, 0, 0, 4, 0, "1970-W01-4 00 00 001 Thu Jan 19 70 70");
        CheckTime(-1LL, 69, 11, 31, 23, 59, 59, 3, 364, "1970-W01-3 52 52 365 Wed Dec 19 69 70");
        CheckTime(951782400LL, 100, 1, 29, 0, 0, 0, 2, 59, "2000-W09-2 09 09 060 Tue Feb 20 00 00");
        CheckTime(1700000000LL,
                  123,
                  10,
                  14,
                  22,
                  13,
                  20,
                  2,
                  317,
                  "2023-W46-2 46 46 318 Tue Nov 20 23 23");
        CheckTime(-2208988800LL, 0, 0, 1, 0, 0, 0, 1, 0, "1900-W01-1 00 01 001 Mon Jan 19 00 00");
        CheckTime(253402300799LL,
                  8099,
                  11,
                  31,
                  23,
                  59,
                  59,
                  5,
                  364,
                  "9999-W52-5 52 52 365 Fri Dec 99 99 99");
        CheckTime(4102444800LL, 200, 0, 1, 0, 0, 0, 5, 0, "2099-W53-5 00 00 001 Fri Jan 21 00 99");
        CheckTime(-62135596800LL, -1899, 0, 1, 0, 0, 0, 1, 0, "1-W01-1 00 01 001 Mon Jan 00 01 01");
        CheckTime(1609459199LL,
                  120,
                  11,
                  31,
                  23,
                  59,
                  59,
                  4,
                  365,
                  "2020-W53-4 52 52 366 Thu Dec 20 20 20");
        CheckTime(1230768000LL, 109, 0, 1, 0, 0, 0, 4, 0, "2009-W01-4 00 00 001 Thu Jan 20 09 09");
        CheckTime(1104537600LL, 105, 0, 1, 0, 0, 0, 6, 0, "2004-W53-6 00 00 001 Sat Jan 20 05 04");
        CheckTime(1293753600LL,
                  110,
                  11,
                  31,
                  0,
                  0,
                  0,
                  5,
                  364,
                  "2010-W52-5 52 52 365 Fri Dec 20 10 10");
}

/** @brief timegm normalizes fields out of range; asctime and strftime write them. */
static void
Formats(void)
{
        struct tm fields = { 0 };
        char      text[ 128 ];
        time_t    seconds = 1700000000;

        fields.tm_year = 123;
        fields.tm_mon  = 13;
        fields.tm_mday = 0;
        fields.tm_hour = 25;
        TEST_INTEGER(timegm(&fields), 1706749200);
        TEST_INTEGER(fields.tm_mon, 1);
        TEST_INTEGER(fields.tm_mday, 1);
        TEST_INTEGER(fields.tm_hour, 1);
        gmtime_r(&seconds, &fields);
        TEST_STRING(asctime_r(&fields, text), "Tue Nov 14 22:13:20 2023\n");
        TEST_STRING(asctime(&fields), "Tue Nov 14 22:13:20 2023\n");
        TEST_INTEGER(strftime(text,
                              sizeof(text),
                              "%c|%D|%F|%T|%r|%R|%e|%I %p|%A %B|%h|%Z %z|%%|%Ey %Od|%n%t|%q",
                              &fields),
                     125);
        TEST_STRING(text,
                    "Tue Nov 14 22:13:20 2023|11/14/23|2023-11-14|22:13:20|10:13:20 PM|22:13|14|10 "
                    "PM|Tuesday November|Nov|UTC +0000|%|23 14|\n\t|%q");
        TEST_INTEGER(strftime(text, 5, "%Y-%m", &fields), 0);
        TEST_INTEGER(strftime(text, 8, "%Y-%m", &fields), 7);
        fields.tm_year = INT_MAX - 1000;
        errno          = 0;
        TEST_CHECK(asctime_r(&fields, text) == NULL);
        TEST_INTEGER(errno, EOVERFLOW);
        TEST_CHECK(difftime(10, 4) == 6.0);
        TEST_CHECK(difftime((time_t) LONG_MAX, (time_t) LONG_MIN) > 1e19);
        seconds = (time_t) LONG_MAX;
        errno   = 0;
        TEST_CHECK(gmtime(&seconds) == NULL);
        TEST_INTEGER(errno, EOVERFLOW);
}

static const Test_tCase kCases[] = {
        { "WideStrings", WideStrings }, { "Multibyte", Multibyte }, { "WideClasses", WideClasses },
        { "Locales", Locales },         { "ExactMath", ExactMath }, { "Dates", Dates },
        { "Formats", Formats },
};

TEST_SUITE(gTest_Other, kCases);

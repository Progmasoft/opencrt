/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <strings.h>

#include "Test.h"

/** @brief strtol, strtoul and their kin: bases, prefixes, signs, ends and ranges. */
static void
ReadsIntegers(void)
{
        char* pEnd = NULL;

        TEST_INTEGER(strtol("  -42xyz", &pEnd, 10), -42);
        TEST_STRING(pEnd, "xyz");
        TEST_INTEGER(strtol("0x1F", NULL, 0), 31);
        TEST_INTEGER(strtol("0X1f", NULL, 16), 31);
        TEST_INTEGER(strtol("017", NULL, 0), 15);
        TEST_INTEGER(strtol("zz", NULL, 36), 1295);
        TEST_INTEGER(strtol("+7", NULL, 10), 7);
        TEST_INTEGER(strtol("0xg", &pEnd, 16), 0);
        TEST_STRING(pEnd, "xg");
        TEST_INTEGER(strtol("   ", &pEnd, 10), 0);
        TEST_STRING(pEnd, "   ");
        TEST_INTEGER(strtol("-", &pEnd, 10), 0);
        TEST_STRING(pEnd, "-");
        errno = 0;
        TEST_INTEGER(strtol("9223372036854775807", NULL, 10), LONG_MAX);
        TEST_INTEGER(errno, 0);
        TEST_CHECK(strtol("-9223372036854775808", NULL, 10) == LONG_MIN);
        TEST_INTEGER(errno, 0);
        TEST_INTEGER(strtol("9223372036854775808", NULL, 10), LONG_MAX);
        TEST_INTEGER(errno, ERANGE);
        errno = 0;
        TEST_CHECK(strtol("-99999999999999999999999", &pEnd, 10) == LONG_MIN);
        TEST_INTEGER(errno, ERANGE);
        TEST_STRING(pEnd, "");
        errno = 0;
        TEST_CHECK(strtoul("18446744073709551615", NULL, 10) == ULONG_MAX);
        TEST_INTEGER(errno, 0);
        TEST_CHECK(strtoul("-1", NULL, 10) == ULONG_MAX);
        TEST_CHECK(strtoull("18446744073709551616", NULL, 10) == ULLONG_MAX);
        TEST_INTEGER(errno, ERANGE);
        errno = 0;
        TEST_INTEGER(strtol("12", &pEnd, 1), 0);
        TEST_INTEGER(errno, EINVAL);
        TEST_CHECK(strtoimax("-123", NULL, 10) == -123);
        TEST_CHECK(strtoumax("777", NULL, 8) == 511);
        TEST_INTEGER(atoi(" 314abc"), 314);
        TEST_INTEGER(atoi("99999999999"), INT_MAX);
        TEST_INTEGER(atol("-5"), -5);
        TEST_INTEGER(atoll("123456789012"), 123456789012LL);
}

/** @brief abs, div and their kin round toward zero. */
static void
Arithmetic(void)
{
        TEST_INTEGER(abs(-3), 3);
        TEST_INTEGER(labs(-4L), 4);
        TEST_INTEGER(llabs(-5LL), 5);
        TEST_INTEGER(imaxabs(-6), 6);
        div_t d = div(-7, 2);

        TEST_INTEGER(d.quot, -3);
        TEST_INTEGER(d.rem, -1);
        lldiv_t ll = lldiv(7, -2);

        TEST_INTEGER(ll.quot, -3);
        TEST_INTEGER(ll.rem, 1);
        imaxdiv_t m = imaxdiv(100, 7);

        TEST_INTEGER(m.quot, 14);
        TEST_INTEGER(m.rem, 2);
}

/** @brief The same seed gives the same sequence, inside the range. */
static void
RandomSequences(void)
{
        int first[ 8 ];

        srand(42);
        for(int i = 0; i < 8; i++)
                first[ i ] = rand();
        srand(42);
        for(int i = 0; i < 8; i++)
        {
                int value = rand();

                TEST_INTEGER(value, first[ i ]);
                TEST_CHECK(value >= 0 && value <= RAND_MAX);
        }
        srand(43);
        TEST_CHECK(rand() != first[ 0 ]);
}

/**
 * @brief Orders two ints for qsort.
 *
 * @param[in] aIn_pFirst  The first.
 * @param[in] aIn_pSecond The second.
 * @return Their order.
 */
static int
CompareInts(const void* aIn_pFirst, const void* aIn_pSecond)
{
        int first  = *(const int*) aIn_pFirst;
        int second = *(const int*) aIn_pSecond;

        return ((first > second) - (first < second));
}

/** How many comparisons qsort has made, to check that no input makes it quadratic. */
static long gsComparisons;

/**
 * @brief Orders two ints and counts the call.
 *
 * @param[in] aIn_pFirst  The first.
 * @param[in] aIn_pSecond The second.
 * @return Their order.
 */
static int
CountingCompare(const void* aIn_pFirst, const void* aIn_pSecond)
{
        gsComparisons++;
        return (CompareInts(aIn_pFirst, aIn_pSecond));
}

/**
 * @brief Fills an array in one of five shapes: ascending, descending, all equal, scattered, and
 *        alternating ends, the shapes that make a plain quicksort quadratic.
 *
 * @param[out] aOut_pValues The array.
 * @param[in]  aCount       Its size.
 * @param[in]  aShape       0 to 4.
 */
static void
Fill(int* aOut_pValues, int aCount, int aShape)
{
        for(int i = 0; i < aCount; i++)
        {
                int values[ 5 ] = { i,
                                    aCount - i,
                                    7,
                                    (int) ((i * 7919U) % (unsigned) aCount),
                                    i % 2 == 0 ? i : aCount - i };

                aOut_pValues[ i ] = values[ aShape ];
        }
}

/**
 * @brief Counts the places where an array is out of order.
 *
 * @param[in] aIn_pValues The array.
 * @param[in] aCount      Its size.
 * @return The count.
 */
static int
Inversions(const int* aIn_pValues, int aCount)
{
        int count = 0;

        for(int i = 1; i < aCount; i++)
                count += aIn_pValues[ i - 1 ] > aIn_pValues[ i ];
        return (count);
}

/** @brief qsort sorts every shape of input in O(n log n) comparisons, and bsearch finds what is there. */
static void
SortsAndSearches(void)
{
        static int values[ 5000 ];

        for(int shape = 0; shape < 5; shape++)
        {
                Fill(values, 5000, shape);
                gsComparisons = 0;
                qsort(values, 5000, sizeof(int), CountingCompare);
                TEST_INTEGER(Inversions(values, 5000), 0);
                /* n log2 n is about 61000 for 5000; quadratic would be millions. */
                TEST_CHECK(gsComparisons < 400000);
        }
        int  key    = 4242;
        int* pFound = bsearch(&key, values, 5000, sizeof(int), CompareInts);

        TEST_CHECK(pFound != NULL && *pFound == 4242);
        key = -1;
        TEST_CHECK(bsearch(&key, values, 5000, sizeof(int), CompareInts) == NULL);
        TEST_CHECK(bsearch(&key, values, 0, sizeof(int), CompareInts) == NULL);
}

/** @brief The classes of the C locale, and values outside unsigned char. */
static void
ClassifiesCharacters(void)
{
        int counts[ 13 ] = { 0 };

        for(int c = 0; c < 256; c++)
        {
                counts[ 0 ]  += isalnum(c) != 0;
                counts[ 1 ]  += isalpha(c) != 0;
                counts[ 2 ]  += isblank(c) != 0;
                counts[ 3 ]  += iscntrl(c) != 0;
                counts[ 4 ]  += isdigit(c) != 0;
                counts[ 5 ]  += isgraph(c) != 0;
                counts[ 6 ]  += islower(c) != 0;
                counts[ 7 ]  += isprint(c) != 0;
                counts[ 8 ]  += ispunct(c) != 0;
                counts[ 9 ]  += isspace(c) != 0;
                counts[ 10 ] += isupper(c) != 0;
                counts[ 11 ] += isxdigit(c) != 0;
                counts[ 12 ] += isascii(c) != 0;
        }
        const int expected[ 13 ] = { 62, 52, 2, 33, 10, 94, 26, 95, 32, 6, 26, 22, 128 };

        for(int i = 0; i < 13; i++)
                TEST_INTEGER(counts[ i ], expected[ i ]);
        TEST_INTEGER(toupper('q'), 'Q');
        TEST_INTEGER(tolower('Q'), 'q');
        TEST_INTEGER(toupper('1'), '1');
        TEST_INTEGER(isalpha(-1), 0);
        TEST_INTEGER(isalpha(1000), 0);
        TEST_INTEGER(tolower(-1), -1);
        TEST_INTEGER(toascii(0xC1), 0x41);
}

/** @brief strerror and strerror_r. */
static void
DescribesErrors(void)
{
        char buffer[ 64 ];

        TEST_STRING(strerror(ENOENT), "No such file or directory");
        TEST_STRING(strerror(EQFULL), "Interface output queue is full");
        TEST_STRING(strerror(-3), "Unknown error: -3");
        TEST_STRING(strerror(107), "Unknown error: 107");
        TEST_INTEGER(strerror_r(EINVAL, buffer, sizeof(buffer)), 0);
        TEST_STRING(buffer, "Invalid argument");
        TEST_INTEGER(strerror_r(EINVAL, buffer, 5), ERANGE);
        TEST_STRING(buffer, "Inva");
        TEST_INTEGER(strerror_r(999, buffer, sizeof(buffer)), EINVAL);
        TEST_STRING(buffer, "Unknown error: 999");
        errno = 0;
        errno = EDOM;
        TEST_INTEGER(errno, 33);
        TEST_CHECK(&errno == __error());
}

/** @brief strings.h. */
static void
StringsFunctions(void)
{
        char secret[ 8 ] = "hunter2";

        TEST_INTEGER(strcasecmp("HeLLo", "hello"), 0);
        TEST_INTEGER(strcasecmp("a", "B") < 0, 1);
        TEST_INTEGER(strncasecmp("ABCx", "abcy", 3), 0);
        TEST_INTEGER(ffs(0), 0);
        TEST_INTEGER(ffs(0x18), 4);
        TEST_INTEGER(ffs(INT_MIN), 32);
        explicit_bzero(secret, sizeof(secret));
        for(int i = 0; i < 8; i++)
                TEST_INTEGER(secret[ i ], 0);
}

static const Test_tCase kCases[] = {
        { "ReadsIntegers", ReadsIntegers },
        { "Arithmetic", Arithmetic },
        { "RandomSequences", RandomSequences },
        { "SortsAndSearches", SortsAndSearches },
        { "ClassifiesCharacters", ClassifiesCharacters },
        { "DescribesErrors", DescribesErrors },
        { "StringsFunctions", StringsFunctions },
};

TEST_SUITE(gTest_Stdlib, kCases);

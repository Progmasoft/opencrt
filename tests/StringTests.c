/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include "Test.h"

/**
 * @brief Copies with memcpy into a filled buffer and checks every byte of it.
 *
 * @param[in] aIn_pSource The source, 64 bytes.
 * @param[in] aFrom       Where the copy starts in the source.
 * @param[in] aTo         Where it goes in the destination.
 * @param[in] aLength     How many bytes.
 */
static void
CheckCopy(const unsigned char* aIn_pSource, int aFrom, int aTo, int aLength)
{
        unsigned char target[ 64 ];
        int           wrong = 0;

        memset(target, 0xEE, sizeof(target));
        TEST_CHECK(memcpy(target + aTo, aIn_pSource + aFrom, (size_t) aLength) == target + aTo);
        for(int i = 0; i < 64; i++)
        {
                int inside = i >= aTo && i < aTo + aLength;

                wrong += target[ i ] != (inside ? aIn_pSource[ aFrom + i - aTo ] : 0xEE);
        }
        TEST_INTEGER(wrong, 0);
}

/** @brief memcpy at every alignment of source and destination and every length up to 40 bytes. */
static void
CopiesAtEveryAlignment(void)
{
        unsigned char source[ 64 ];

        for(int i = 0; i < 64; i++)
                source[ i ] = (unsigned char) (i * 7 + 1);
        for(int from = 0; from < 9; from++)
        {
                for(int to = 0; to < 9; to++)
                {
                        for(int length = 0; length <= 40; length++)
                                CheckCopy(source, from, to, length);
                }
        }
}

/** @brief memset at every length up to 40 bytes, from an odd address. */
static void
Fills(void)
{
        unsigned char target[ 64 ];

        for(int length = 0; length <= 40; length++)
        {
                int wrong = 0;

                memset(target, 1, sizeof(target));
                memset(target + 3, 0xAB, (size_t) length);
                for(int i = 0; i < 64; i++)
                        wrong += target[ i ] != (i >= 3 && i < 3 + length ? 0xAB : 1);
                TEST_INTEGER(wrong, 0);
        }
}

/** @brief memmove keeps the source when the two overlap in either direction. */
static void
MovesOverlappingBytes(void)
{
        char buffer[ 32 ];

        strcpy(buffer, "0123456789abcdefghij");
        memmove(buffer + 2, buffer, 10);
        TEST_STRING(buffer, "010123456789cdefghij");
        strcpy(buffer, "0123456789abcdefghij");
        memmove(buffer, buffer + 3, 12);
        TEST_STRING(buffer, "3456789abcdecdefghij");
        strcpy(buffer, "0123456789abcdefghijklmnopq");
        memmove(buffer + 8, buffer, 16);
        TEST_STRING(buffer, "012345670123456789abcdefopq");
}

/** @brief The comparisons treat bytes as unsigned char. */
static void
Compares(void)
{
        TEST_INTEGER(memcmp("abc", "abd", 3) < 0, 1);
        TEST_INTEGER(memcmp("abc", "abc", 3), 0);
        TEST_INTEGER(memcmp("\x80", "\x01", 1) > 0, 1);
        TEST_INTEGER(strcmp("abc", "abcd") < 0, 1);
        TEST_INTEGER(strcmp("b", "a") > 0, 1);
        TEST_INTEGER(strcmp("", ""), 0);
        TEST_INTEGER(strcmp("\xff", "a") > 0, 1);
        TEST_INTEGER(strncmp("abcX", "abcY", 3), 0);
        TEST_INTEGER(strncmp("abcX", "abcY", 4) < 0, 1);
        TEST_INTEGER(strcoll("a", "b") < 0, 1);
}

/** @brief Lengths, searches and spans. */
static void
Searches(void)
{
        const char* pText = "hello, world";

        TEST_INTEGER(strlen(""), 0);
        TEST_INTEGER(strlen(pText), 12);
        TEST_INTEGER(strnlen(pText, 5), 5);
        TEST_INTEGER(strnlen(pText, 50), 12);
        TEST_CHECK(strchr(pText, 'o') == pText + 4);
        TEST_CHECK(strrchr(pText, 'o') == pText + 8);
        TEST_CHECK(strchr(pText, '\0') == pText + 12);
        TEST_CHECK(strchr(pText, 'z') == NULL);
        TEST_CHECK(memchr(pText, 'w', 12) == pText + 7);
        TEST_CHECK(memchr(pText, 'w', 7) == NULL);
        TEST_CHECK(strstr(pText, "world") == pText + 7);
        TEST_CHECK(strstr(pText, "") == pText);
        TEST_CHECK(strstr(pText, "worlds") == NULL);
        TEST_CHECK(strstr("aaab", "aab") != NULL);
        TEST_INTEGER(strspn(pText, "leh"), 4);
        TEST_INTEGER(strcspn(pText, ", "), 5);
        TEST_CHECK(strpbrk(pText, "wr") == pText + 7);
        TEST_CHECK(strpbrk(pText, "xyz") == NULL);
}

/** @brief The copies and appends, the cut ones included. */
static void
CopiesStrings(void)
{
        char buffer[ 16 ];

        TEST_CHECK(stpcpy(buffer, "abc") == buffer + 3);
        TEST_STRING(strcat(buffer, "def"), "abcdef");
        TEST_STRING(strncat(buffer, "ghijk", 2), "abcdefgh");
        for(int i = 0; i < 16; i++)
                buffer[ i ] = 'x';
        strncpy(buffer, "ab", 6);
        TEST_INTEGER(buffer[ 2 ] == '\0' && buffer[ 5 ] == '\0' && buffer[ 6 ] == 'x', 1);
        TEST_CHECK(stpncpy(buffer, "abcdef", 3) == buffer + 3);
        TEST_INTEGER(strlcpy(buffer, "a long string", 6), 13);
        TEST_STRING(buffer, "a lon");
        TEST_INTEGER(strlcpy(buffer, "short", sizeof(buffer)), 5);
        TEST_INTEGER(strlcat(buffer, " and more text", sizeof(buffer)), 19);
        TEST_STRING(buffer, "short and more ");
        TEST_INTEGER(strlcat(buffer, "x", 4), 4 + 1);
        TEST_INTEGER(strxfrm(buffer, "key", sizeof(buffer)), 3);
        TEST_STRING(buffer, "key");
        TEST_CHECK(memccpy(buffer, "abc:def", ':', 16) == buffer + 4);
        TEST_CHECK(memccpy(buffer, "abc", ':', 3) == NULL);
}

/** @brief strtok and strtok_r skip runs of separators and keep their place. */
static void
Tokenizes(void)
{
        char  text[]  = ",,one, two,,three,";
        char* pSave   = NULL;
        char  again[] = "a b";

        TEST_STRING(strtok_r(text, ", ", &pSave), "one");
        TEST_STRING(strtok_r(NULL, ", ", &pSave), "two");
        TEST_STRING(strtok_r(NULL, ", ", &pSave), "three");
        TEST_CHECK(strtok_r(NULL, ", ", &pSave) == NULL);
        TEST_CHECK(strtok_r(NULL, ", ", &pSave) == NULL);
        TEST_STRING(strtok(again, " "), "a");
        TEST_STRING(strtok(NULL, " "), "b");
        TEST_CHECK(strtok(NULL, " ") == NULL);
}

static const Test_tCase kCases[] = {
        { "CopiesAtEveryAlignment", CopiesAtEveryAlignment },
        { "Fills", Fills },
        { "MovesOverlappingBytes", MovesOverlappingBytes },
        { "Compares", Compares },
        { "Searches", Searches },
        { "CopiesStrings", CopiesStrings },
        { "Tokenizes", Tokenizes },
};

TEST_SUITE(gTest_String, kCases);

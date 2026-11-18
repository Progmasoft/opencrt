/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/*
 * The C locale, single-byte as POSIX has it: every byte is a character. ASCII maps to itself; the
 * bytes 0x80 to 0xFF map to U+DF80 to U+DFFF, values no real character has, so a byte is never taken
 * for a Unicode character and every byte comes back unchanged.
 */

/** Where the bytes 0x80 to 0xFF go among the wide characters. */
#define OPENCRT_MULTIBYTE_HIGH 0xDF00

/**
 * @brief The wide character of a byte.
 *
 * @param[in] aByte The byte, as an unsigned char, or EOF.
 * @return The wide character, or WEOF for EOF.
 */
wint_t
btowc(int aByte)
{
        if(aByte == EOF || aByte < 0 || aByte > 0xFF)
                return (WEOF);
        return ((wint_t) (aByte < 0x80 ? aByte : OPENCRT_MULTIBYTE_HIGH + aByte));
}

/**
 * @brief The byte of a wide character.
 *
 * @param[in] aCharacter The wide character.
 * @return The byte, or EOF for a character with no byte in the locale.
 */
int
wctob(wint_t aCharacter)
{
        if(aCharacter < 0x80)
                return ((int) aCharacter);
        if(aCharacter >= OPENCRT_MULTIBYTE_HIGH + 0x80
           && aCharacter <= OPENCRT_MULTIBYTE_HIGH + 0xFF)
                return ((int) (aCharacter - OPENCRT_MULTIBYTE_HIGH));
        return (EOF);
}

/**
 * @brief Whether a conversion state is the initial one: always, as the locale has no shift states.
 *
 * @param[in] aIn_pState The state, or NULL.
 * @return Non-zero.
 */
int
mbsinit(const mbstate_t* aIn_pState)
{
        (void) aIn_pState;
        return (1);
}

/**
 * @brief Converts the next character of a multibyte string.
 *
 * @param[out]    aOut_pCharacter The wide character, or NULL.
 * @param[in]     aIn_pBytes      The bytes, or NULL to reset the state.
 * @param[in]     aCount          How many bytes may be read.
 * @param[in,out] aInOut_pState   The state, unused.
 * @return 0 for the null character, 1 for any other, (size_t) -2 when no byte may be read.
 */
size_t
mbrtowc(wchar_t* restrict aOut_pCharacter,
        const char* restrict aIn_pBytes,
        size_t aCount,
        mbstate_t* restrict aInOut_pState)
{
        (void) aInOut_pState;
        if(aIn_pBytes == NULL)
                return (0);
        if(aCount == 0)
                return ((size_t) -2);
        unsigned char byte = (unsigned char) *aIn_pBytes;

        if(aOut_pCharacter != NULL)
                *aOut_pCharacter = (wchar_t) btowc(byte);
        return (byte != 0);
}

/**
 * @brief The length of the next character of a multibyte string.
 *
 * @param[in]     aIn_pBytes    The bytes.
 * @param[in]     aCount        How many bytes may be read.
 * @param[in,out] aInOut_pState The state, unused.
 * @return As mbrtowc.
 */
size_t
mbrlen(const char* restrict aIn_pBytes, size_t aCount, mbstate_t* restrict aInOut_pState)
{
        return (mbrtowc(NULL, aIn_pBytes, aCount, aInOut_pState));
}

/**
 * @brief Converts a wide character to its bytes.
 *
 * @param[out]    aOut_pBytes   At least MB_CUR_MAX bytes, or NULL.
 * @param[in]     aCharacter    The wide character.
 * @param[in,out] aInOut_pState The state, unused.
 * @return 1, or (size_t) -1 with EILSEQ for a character with no byte in the locale.
 */
size_t
wcrtomb(char* restrict aOut_pBytes, wchar_t aCharacter, mbstate_t* restrict aInOut_pState)
{
        (void) aInOut_pState;
        if(aOut_pBytes == NULL)
                return (1);
        int byte = wctob((wint_t) aCharacter);

        if(byte == EOF)
        {
                errno = EILSEQ;
                return ((size_t) -1);
        }
        *aOut_pBytes = (char) byte;
        return (1);
}

/**
 * @brief Converts a multibyte string to wide characters.
 *
 * @param[out]    aOut_pWide    The wide characters, or NULL to count them.
 * @param[in,out] aInOut_ppBytes The string; NULL once its null character is converted.
 * @param[in]     aCount        The most wide characters to write.
 * @param[in,out] aInOut_pState The state, unused.
 * @return How many wide characters were written, the null one not counted.
 */
size_t
mbsrtowcs(wchar_t* restrict aOut_pWide,
          const char** restrict aInOut_ppBytes,
          size_t aCount,
          mbstate_t* restrict aInOut_pState)
{
        const char* pAt     = *aInOut_ppBytes;
        size_t      written = 0;

        (void) aInOut_pState;
        if(aOut_pWide == NULL)
                return (strlen(pAt));
        for(; written < aCount; written++, pAt++)
        {
                aOut_pWide[ written ] = (wchar_t) btowc((unsigned char) *pAt);
                if(*pAt == '\0')
                {
                        *aInOut_ppBytes = NULL;
                        return (written);
                }
        }
        *aInOut_ppBytes = pAt;
        return (written);
}

/**
 * @brief Converts a wide string to multibyte.
 *
 * @param[out]    aOut_pBytes    The bytes, or NULL to count them.
 * @param[in,out] aInOut_ppWide  The wide string; NULL once its null character is converted, or the
 *                               character that has no byte.
 * @param[in]     aCount         The most bytes to write.
 * @param[in,out] aInOut_pState  The state, unused.
 * @return How many bytes were written, the null one not counted, or (size_t) -1 with EILSEQ.
 */
size_t
wcsrtombs(char* restrict aOut_pBytes,
          const wchar_t** restrict aInOut_ppWide,
          size_t aCount,
          mbstate_t* restrict aInOut_pState)
{
        const wchar_t* pAt     = *aInOut_ppWide;
        size_t         written = 0;

        (void) aInOut_pState;
        for(; aOut_pBytes == NULL || written < aCount; written++, pAt++)
        {
                int byte = wctob((wint_t) *pAt);

                if(byte == EOF)
                {
                        if(aOut_pBytes != NULL)
                                *aInOut_ppWide = pAt;
                        errno = EILSEQ;
                        return ((size_t) -1);
                }
                if(aOut_pBytes != NULL)
                        aOut_pBytes[ written ] = (char) byte;
                if(*pAt == L'\0')
                {
                        if(aOut_pBytes != NULL)
                                *aInOut_ppWide = NULL;
                        return (written);
                }
        }
        *aInOut_ppWide = pAt;
        return (written);
}

/**
 * @brief The length of the next character of a multibyte string.
 *
 * @param[in] aIn_pBytes The bytes, or NULL to ask whether the locale has shift states.
 * @param[in] aCount     How many may be read.
 * @return 0 for the null character or for NULL, 1 for another, -1 when no byte may be read.
 */
int
mblen(const char* aIn_pBytes, size_t aCount)
{
        return (mbtowc(NULL, aIn_pBytes, aCount));
}

/**
 * @brief Converts the next character of a multibyte string.
 *
 * @param[out] aOut_pCharacter The wide character, or NULL.
 * @param[in]  aIn_pBytes      The bytes, or NULL to ask whether the locale has shift states.
 * @param[in]  aCount          How many may be read.
 * @return 0 for the null character or for NULL, 1 for another, -1 when no byte may be read.
 */
int
mbtowc(wchar_t* restrict aOut_pCharacter, const char* restrict aIn_pBytes, size_t aCount)
{
        if(aIn_pBytes == NULL)
                return (0);
        if(aCount == 0)
                return (-1);
        return ((int) mbrtowc(aOut_pCharacter, aIn_pBytes, aCount, NULL));
}

/**
 * @brief Converts a wide character to its byte.
 *
 * @param[out] aOut_pBytes At least MB_CUR_MAX bytes, or NULL to ask whether the locale has shift states.
 * @param[in]  aCharacter  The wide character.
 * @return 1, 0 for NULL, or -1 with EILSEQ.
 */
int
wctomb(char* aOut_pBytes, wchar_t aCharacter)
{
        if(aOut_pBytes == NULL)
                return (0);
        return ((int) wcrtomb(aOut_pBytes, aCharacter, NULL));
}

/**
 * @brief Converts a multibyte string to wide characters.
 *
 * @param[out] aOut_pWide The wide characters, or NULL to count them.
 * @param[in]  aIn_pBytes The string.
 * @param[in]  aCount     The most wide characters to write.
 * @return How many were written, the null one not counted.
 */
size_t
mbstowcs(wchar_t* restrict aOut_pWide, const char* restrict aIn_pBytes, size_t aCount)
{
        const char* pBytes = aIn_pBytes;

        return (mbsrtowcs(aOut_pWide, &pBytes, aCount, NULL));
}

/**
 * @brief Converts a wide string to multibyte.
 *
 * @param[out] aOut_pBytes The bytes, or NULL to count them.
 * @param[in]  aIn_pWide   The wide string.
 * @param[in]  aCount      The most bytes to write.
 * @return How many were written, the null one not counted, or (size_t) -1 with EILSEQ.
 */
size_t
wcstombs(char* restrict aOut_pBytes, const wchar_t* restrict aIn_pWide, size_t aCount)
{
        const wchar_t* pWide = aIn_pWide;

        return (wcsrtombs(aOut_pBytes, &pWide, aCount, NULL));
}

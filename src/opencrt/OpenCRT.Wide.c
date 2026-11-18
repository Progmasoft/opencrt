/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <stddef.h>
#include <wchar.h>

/**
 * @brief Counts the wide characters before the null one.
 *
 * @param[in] aIn_pString The string.
 * @return The length.
 */
size_t
wcslen(const wchar_t* aIn_pString)
{
        return (wcsnlen(aIn_pString, (size_t) -1));
}

/**
 * @brief Counts the wide characters before the null one, reading at most a given number.
 *
 * @param[in] aIn_pString The string.
 * @param[in] aLimit      The most to read.
 * @return The length, or aLimit.
 */
size_t
wcsnlen(const wchar_t* aIn_pString, size_t aLimit)
{
        size_t length = 0;

        while(length < aLimit && aIn_pString[ length ] != L'\0')
                length++;
        return (length);
}

/**
 * @brief Copies wide characters between arrays that do not overlap.
 *
 * @param[out] aOut_pDestination The destination.
 * @param[in]  aIn_pSource       The source.
 * @param[in]  aCount            How many.
 * @return aOut_pDestination.
 */
wchar_t*
wmemcpy(wchar_t* restrict aOut_pDestination, const wchar_t* restrict aIn_pSource, size_t aCount)
{
        for(size_t index = 0; index < aCount; index++)
                aOut_pDestination[ index ] = aIn_pSource[ index ];
        return (aOut_pDestination);
}

/**
 * @brief Copies wide characters between arrays that may overlap.
 *
 * @param[out] aOut_pDestination The destination.
 * @param[in]  aIn_pSource       The source.
 * @param[in]  aCount            How many.
 * @return aOut_pDestination.
 */
wchar_t*
wmemmove(wchar_t* aOut_pDestination, const wchar_t* aIn_pSource, size_t aCount)
{
        if((size_t) (aOut_pDestination - aIn_pSource) >= aCount)
        {
                for(size_t index = 0; index < aCount; index++)
                        aOut_pDestination[ index ] = aIn_pSource[ index ];
        } else
        {
                for(size_t index = aCount; index-- > 0;)
                        aOut_pDestination[ index ] = aIn_pSource[ index ];
        }
        return (aOut_pDestination);
}

/**
 * @brief Fills wide characters with one value.
 *
 * @param[out] aOut_pDestination The array.
 * @param[in]  aCharacter        The value.
 * @param[in]  aCount            How many.
 * @return aOut_pDestination.
 */
wchar_t*
wmemset(wchar_t* aOut_pDestination, wchar_t aCharacter, size_t aCount)
{
        for(size_t index = 0; index < aCount; index++)
                aOut_pDestination[ index ] = aCharacter;
        return (aOut_pDestination);
}

/**
 * @brief Compares wide characters as wchar_t values.
 *
 * @param[in] aIn_pFirst  The first array.
 * @param[in] aIn_pSecond The second array.
 * @param[in] aCount      How many.
 * @return The sign of the first difference, or 0.
 */
int
wmemcmp(const wchar_t* aIn_pFirst, const wchar_t* aIn_pSecond, size_t aCount)
{
        for(size_t index = 0; index < aCount; index++)
        {
                if(aIn_pFirst[ index ] != aIn_pSecond[ index ])
                        return (aIn_pFirst[ index ] < aIn_pSecond[ index ] ? -1 : 1);
        }
        return (0);
}

/**
 * @brief Finds the first wide character of a value.
 *
 * @param[in] aIn_pArray  The array.
 * @param[in] aCharacter  The value.
 * @param[in] aCount      How many.
 * @return The character, or NULL.
 */
wchar_t*
wmemchr(const wchar_t* aIn_pArray, wchar_t aCharacter, size_t aCount)
{
        for(size_t index = 0; index < aCount; index++)
        {
                if(aIn_pArray[ index ] == aCharacter)
                        return ((wchar_t*) aIn_pArray + index);
        }
        return (NULL);
}

/**
 * @brief Copies a wide string with its null character.
 *
 * @param[out] aOut_pDestination The destination.
 * @param[in]  aIn_pSource       The string.
 * @return aOut_pDestination.
 */
wchar_t*
wcscpy(wchar_t* restrict aOut_pDestination, const wchar_t* restrict aIn_pSource)
{
        return (wmemcpy(aOut_pDestination, aIn_pSource, wcslen(aIn_pSource) + 1));
}

/**
 * @brief Copies at most a given number of wide characters and fills the rest with nulls.
 *
 * @param[out] aOut_pDestination The destination.
 * @param[in]  aIn_pSource       The string.
 * @param[in]  aCount            The size of the destination.
 * @return aOut_pDestination.
 */
wchar_t*
wcsncpy(wchar_t* restrict aOut_pDestination, const wchar_t* restrict aIn_pSource, size_t aCount)
{
        size_t length = wcsnlen(aIn_pSource, aCount);

        wmemcpy(aOut_pDestination, aIn_pSource, length);
        wmemset(aOut_pDestination + length, L'\0', aCount - length);
        return (aOut_pDestination);
}

/**
 * @brief Appends a wide string.
 *
 * @param[in,out] aInOut_pDestination The string to append to.
 * @param[in]     aIn_pSource         The string to append.
 * @return aInOut_pDestination.
 */
wchar_t*
wcscat(wchar_t* restrict aInOut_pDestination, const wchar_t* restrict aIn_pSource)
{
        wcscpy(aInOut_pDestination + wcslen(aInOut_pDestination), aIn_pSource);
        return (aInOut_pDestination);
}

/**
 * @brief Appends at most a given number of wide characters, then a null one.
 *
 * @param[in,out] aInOut_pDestination The string to append to.
 * @param[in]     aIn_pSource         The string to append.
 * @param[in]     aCount              The most to append.
 * @return aInOut_pDestination.
 */
wchar_t*
wcsncat(wchar_t* restrict aInOut_pDestination, const wchar_t* restrict aIn_pSource, size_t aCount)
{
        wchar_t* pEnd   = aInOut_pDestination + wcslen(aInOut_pDestination);
        size_t   length = wcsnlen(aIn_pSource, aCount);

        wmemcpy(pEnd, aIn_pSource, length);
        pEnd[ length ] = L'\0';
        return (aInOut_pDestination);
}

/**
 * @brief Compares at most a given number of wide characters of two strings.
 *
 * @param[in] aIn_pFirst  The first string.
 * @param[in] aIn_pSecond The second string.
 * @param[in] aCount      The most to compare.
 * @return The sign of the first difference, or 0.
 */
int
wcsncmp(const wchar_t* aIn_pFirst, const wchar_t* aIn_pSecond, size_t aCount)
{
        for(size_t index = 0; index < aCount; index++)
        {
                if(aIn_pFirst[ index ] != aIn_pSecond[ index ])
                        return (aIn_pFirst[ index ] < aIn_pSecond[ index ] ? -1 : 1);
                if(aIn_pFirst[ index ] == L'\0')
                        return (0);
        }
        return (0);
}

/**
 * @brief Compares two wide strings.
 *
 * @param[in] aIn_pFirst  The first string.
 * @param[in] aIn_pSecond The second string.
 * @return The sign of the first difference, or 0.
 */
int
wcscmp(const wchar_t* aIn_pFirst, const wchar_t* aIn_pSecond)
{
        return (wcsncmp(aIn_pFirst, aIn_pSecond, (size_t) -1));
}

/**
 * @brief Compares two wide strings in the collation of the locale: by value in the C locale.
 *
 * @param[in] aIn_pFirst  The first string.
 * @param[in] aIn_pSecond The second string.
 * @return The sign of the first difference, or 0.
 */
int
wcscoll(const wchar_t* aIn_pFirst, const wchar_t* aIn_pSecond)
{
        return (wcscmp(aIn_pFirst, aIn_pSecond));
}

/**
 * @brief Transforms a wide string for wcscmp to order as wcscoll: a copy in the C locale.
 *
 * @param[out] aOut_pDestination The destination.
 * @param[in]  aIn_pSource       The string.
 * @param[in]  aSize             The size of the destination.
 * @return The length of the result; the destination holds it only when it is below aSize.
 */
size_t
wcsxfrm(wchar_t* restrict aOut_pDestination, const wchar_t* restrict aIn_pSource, size_t aSize)
{
        size_t length = wcslen(aIn_pSource);

        if(length < aSize)
                wmemcpy(aOut_pDestination, aIn_pSource, length + 1);
        return (length);
}

/**
 * @brief Finds the first occurrence of a wide character; the null one counts.
 *
 * @param[in] aIn_pString The string.
 * @param[in] aCharacter  The character.
 * @return The character, or NULL.
 */
wchar_t*
wcschr(const wchar_t* aIn_pString, wchar_t aCharacter)
{
        for(;; aIn_pString++)
        {
                if(*aIn_pString == aCharacter)
                        return ((wchar_t*) aIn_pString);
                if(*aIn_pString == L'\0')
                        return (NULL);
        }
}

/**
 * @brief Finds the last occurrence of a wide character; the null one counts.
 *
 * @param[in] aIn_pString The string.
 * @param[in] aCharacter  The character.
 * @return The character, or NULL.
 */
wchar_t*
wcsrchr(const wchar_t* aIn_pString, wchar_t aCharacter)
{
        const wchar_t* pLast = NULL;

        for(;; aIn_pString++)
        {
                if(*aIn_pString == aCharacter)
                        pLast = aIn_pString;
                if(*aIn_pString == L'\0')
                        return ((wchar_t*) pLast);
        }
}

/**
 * @brief Counts the wide characters at the start of a string that are, or are not, in a set.
 *
 * @param[in] aIn_pString The string.
 * @param[in] aIn_pSet    The set.
 * @param[in] aIsIn       Whether to count those in the set.
 * @return The count.
 */
static size_t
Span(const wchar_t* aIn_pString, const wchar_t* aIn_pSet, int aIsIn)
{
        size_t length = 0;

        while(aIn_pString[ length ] != L'\0'
              && (wcschr(aIn_pSet, aIn_pString[ length ]) != NULL) == aIsIn)
                length++;
        return (length);
}

/**
 * @brief Counts the wide characters at the start of a string that are in a set.
 *
 * @param[in] aIn_pString The string.
 * @param[in] aIn_pSet    The set.
 * @return The count.
 */
size_t
wcsspn(const wchar_t* aIn_pString, const wchar_t* aIn_pSet)
{
        return (Span(aIn_pString, aIn_pSet, 1));
}

/**
 * @brief Counts the wide characters at the start of a string that are not in a set.
 *
 * @param[in] aIn_pString The string.
 * @param[in] aIn_pSet    The set.
 * @return The count.
 */
size_t
wcscspn(const wchar_t* aIn_pString, const wchar_t* aIn_pSet)
{
        return (Span(aIn_pString, aIn_pSet, 0));
}

/**
 * @brief Finds the first wide character of a string that is in a set.
 *
 * @param[in] aIn_pString The string.
 * @param[in] aIn_pSet    The set.
 * @return The character, or NULL.
 */
wchar_t*
wcspbrk(const wchar_t* aIn_pString, const wchar_t* aIn_pSet)
{
        const wchar_t* pFound = aIn_pString + wcscspn(aIn_pString, aIn_pSet);

        return (*pFound != L'\0' ? (wchar_t*) pFound : NULL);
}

/**
 * @brief Finds the first occurrence of a wide string in another.
 *
 * @param[in] aIn_pString The string to search.
 * @param[in] aIn_pNeedle The string to find; an empty one is found at the start.
 * @return The occurrence, or NULL.
 */
wchar_t*
wcsstr(const wchar_t* restrict aIn_pString, const wchar_t* restrict aIn_pNeedle)
{
        size_t length = wcslen(aIn_pNeedle);

        for(; *aIn_pString != L'\0' || length == 0; aIn_pString++)
        {
                if(wcsncmp(aIn_pString, aIn_pNeedle, length) == 0)
                        return ((wchar_t*) aIn_pString);
        }
        return (NULL);
}

/**
 * @brief Cuts a wide string into tokens separated by characters of a set.
 *
 * @param[in,out] aInOut_pString The string the first call cuts, NULL for the next calls.
 * @param[in]     aIn_pSet       The separators.
 * @param[in,out] aInOut_ppSave  Where the place is kept between calls.
 * @return The next token, or NULL.
 */
wchar_t*
wcstok(wchar_t* restrict aInOut_pString,
       const wchar_t* restrict aIn_pSet,
       wchar_t** restrict aInOut_ppSave)
{
        wchar_t* pToken = aInOut_pString != NULL ? aInOut_pString : *aInOut_ppSave;

        if(pToken == NULL)
                return (NULL);
        pToken += wcsspn(pToken, aIn_pSet);
        if(*pToken == L'\0')
        {
                *aInOut_ppSave = NULL;
                return (NULL);
        }
        wchar_t* pEnd = pToken + wcscspn(pToken, aIn_pSet);

        if(*pEnd == L'\0')
                *aInOut_ppSave = NULL;
        else
        {
                *pEnd          = L'\0';
                *aInOut_ppSave = pEnd + 1;
        }
        return (pToken);
}

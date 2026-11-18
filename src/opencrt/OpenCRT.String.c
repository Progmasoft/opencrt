/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * A machine word that may alias any object, so that the mem* functions can move one word at a time
 * without breaking the aliasing rules. The str* functions read byte by byte: they never read past
 * the null character that ends a string.
 */
typedef unsigned long __attribute__((__may_alias__)) tWord;

#define OPENCRT_STRING_WORD sizeof(tWord)

/**
 * @brief Tells whether two addresses share their offset inside a machine word.
 *
 * @param[in] aIn_pFirst  The first address.
 * @param[in] aIn_pSecond The second address.
 * @return true when word copies can start on both at once.
 */
static int
AreCoaligned(const void* aIn_pFirst, const void* aIn_pSecond)
{
        return (((uintptr_t) aIn_pFirst ^ (uintptr_t) aIn_pSecond) & (OPENCRT_STRING_WORD - 1))
                == 0;
}

/**
 * @brief Copies bytes from the first to the last, a word at a time where both allow it.
 *
 * It takes no restrict, so that memmove may use it when the destination starts before the source.
 *
 * @param[out] aOut_pDestination The destination.
 * @param[in]  aIn_pSource       The source.
 * @param[in]  aCount            The number of bytes.
 */
static void
CopyForward(unsigned char* aOut_pDestination, const unsigned char* aIn_pSource, size_t aCount)
{
        if(aCount >= OPENCRT_STRING_WORD && AreCoaligned(aOut_pDestination, aIn_pSource))
        {
                while(((uintptr_t) aOut_pDestination & (OPENCRT_STRING_WORD - 1)) != 0)
                {
                        *aOut_pDestination++ = *aIn_pSource++;
                        aCount--;
                }
                for(; aCount >= OPENCRT_STRING_WORD; aCount -= OPENCRT_STRING_WORD)
                {
                        *(tWord*) aOut_pDestination = *(const tWord*) aIn_pSource;
                        aOut_pDestination          += OPENCRT_STRING_WORD;
                        aIn_pSource                += OPENCRT_STRING_WORD;
                }
        }
        while(aCount-- > 0)
                *aOut_pDestination++ = *aIn_pSource++;
}

/**
 * @brief Copies bytes between objects that do not overlap.
 *
 * @param[out] aOut_pDestination The destination.
 * @param[in]  aIn_pSource       The source.
 * @param[in]  aCount            The number of bytes.
 * @return aOut_pDestination.
 */
void*
memcpy(void* restrict aOut_pDestination, const void* restrict aIn_pSource, size_t aCount)
{
        CopyForward(aOut_pDestination, aIn_pSource, aCount);
        return (aOut_pDestination);
}

/**
 * @brief Copies bytes between objects that may overlap.
 *
 * @param[out] aOut_pDestination The destination.
 * @param[in]  aIn_pSource       The source.
 * @param[in]  aCount            The number of bytes.
 * @return aOut_pDestination.
 */
void*
memmove(void* aOut_pDestination, const void* aIn_pSource, size_t aCount)
{
        unsigned char*       pTo   = aOut_pDestination;
        const unsigned char* pFrom = aIn_pSource;

        if(pTo == pFrom || aCount == 0)
                return (aOut_pDestination);
        if((uintptr_t) pTo < (uintptr_t) pFrom || (uintptr_t) pTo >= (uintptr_t) pFrom + aCount)
        {
                CopyForward(pTo, pFrom, aCount);
                return (aOut_pDestination);
        }
        /* The destination starts inside the source: copy from the end. */
        pTo   += aCount;
        pFrom += aCount;
        while(aCount-- > 0)
                *--pTo = *--pFrom;
        return (aOut_pDestination);
}

/**
 * @brief Fills bytes with one value.
 *
 * @param[out] aOut_pDestination The bytes.
 * @param[in]  aValue            The value, converted to unsigned char.
 * @param[in]  aCount            The number of bytes.
 * @return aOut_pDestination.
 */
void*
memset(void* aOut_pDestination, int aValue, size_t aCount)
{
        unsigned char* pTo  = aOut_pDestination;
        unsigned char  byte = (unsigned char) aValue;

        if(aCount >= OPENCRT_STRING_WORD)
        {
                tWord pattern = (tWord) -1 / 0xFF * byte;

                while(((uintptr_t) pTo & (OPENCRT_STRING_WORD - 1)) != 0)
                {
                        *pTo++ = byte;
                        aCount--;
                }
                for(; aCount >= OPENCRT_STRING_WORD; aCount -= OPENCRT_STRING_WORD)
                {
                        *(tWord*) pTo = pattern;
                        pTo          += OPENCRT_STRING_WORD;
                }
        }
        while(aCount-- > 0)
                *pTo++ = byte;
        return (aOut_pDestination);
}

/**
 * @brief Compares bytes as unsigned char.
 *
 * @param[in] aIn_pFirst  The first bytes.
 * @param[in] aIn_pSecond The second bytes.
 * @param[in] aCount      The number of bytes.
 * @return The sign of the first difference, or 0.
 */
int
memcmp(const void* aIn_pFirst, const void* aIn_pSecond, size_t aCount)
{
        const unsigned char* pFirst  = aIn_pFirst;
        const unsigned char* pSecond = aIn_pSecond;

        for(size_t index = 0; index < aCount; index++)
        {
                if(pFirst[ index ] != pSecond[ index ])
                        return (pFirst[ index ] < pSecond[ index ] ? -1 : 1);
        }
        return (0);
}

/**
 * @brief Finds the first byte of a value.
 *
 * @param[in] aIn_pBytes The bytes.
 * @param[in] aValue     The value, converted to unsigned char.
 * @param[in] aCount     The number of bytes.
 * @return The byte, or NULL.
 */
void*
memchr(const void* aIn_pBytes, int aValue, size_t aCount)
{
        const unsigned char* pByte = aIn_pBytes;

        for(size_t index = 0; index < aCount; index++)
        {
                if(pByte[ index ] == (unsigned char) aValue)
                        return ((void*) (pByte + index));
        }
        return (NULL);
}

/**
 * @brief Copies bytes up to and including the first of a value.
 *
 * @param[out] aOut_pDestination The destination.
 * @param[in]  aIn_pSource       The source.
 * @param[in]  aValue            The value, converted to unsigned char.
 * @param[in]  aCount            The most bytes to copy.
 * @return The byte after the copied value in the destination, or NULL when it was not found.
 */
void*
memccpy(void* restrict aOut_pDestination,
        const void* restrict aIn_pSource,
        int    aValue,
        size_t aCount)
{
        unsigned char*       pTo   = aOut_pDestination;
        const unsigned char* pFrom = aIn_pSource;

        for(size_t index = 0; index < aCount; index++)
        {
                pTo[ index ] = pFrom[ index ];
                if(pFrom[ index ] == (unsigned char) aValue)
                        return (pTo + index + 1);
        }
        return (NULL);
}

/**
 * @brief Counts the bytes of a string before its null character.
 *
 * @param[in] aIn_pString The string.
 * @return The length.
 */
size_t
strlen(const char* aIn_pString)
{
        const char* pEnd = aIn_pString;

        while(*pEnd != '\0')
                pEnd++;
        return ((size_t) (pEnd - aIn_pString));
}

/**
 * @brief Counts the bytes of a string, reading at most a given number.
 *
 * @param[in] aIn_pString The string.
 * @param[in] aLimit      The most bytes to read.
 * @return The length, or aLimit when no null character comes first.
 */
size_t
strnlen(const char* aIn_pString, size_t aLimit)
{
        size_t length = 0;

        while(length < aLimit && aIn_pString[ length ] != '\0')
                length++;
        return (length);
}

/**
 * @brief Copies a string with its null character.
 *
 * @param[out] aOut_pDestination The destination.
 * @param[in]  aIn_pSource       The string.
 * @return aOut_pDestination.
 */
char*
strcpy(char* restrict aOut_pDestination, const char* restrict aIn_pSource)
{
        stpcpy(aOut_pDestination, aIn_pSource);
        return (aOut_pDestination);
}

/**
 * @brief Copies a string with its null character.
 *
 * @param[out] aOut_pDestination The destination.
 * @param[in]  aIn_pSource       The string.
 * @return The null character written at the end of the destination.
 */
char*
stpcpy(char* restrict aOut_pDestination, const char* restrict aIn_pSource)
{
        while((*aOut_pDestination = *aIn_pSource++) != '\0')
                aOut_pDestination++;
        return (aOut_pDestination);
}

/**
 * @brief Copies at most a given number of bytes of a string and fills the rest with nulls.
 *
 * @param[out] aOut_pDestination The destination, aCount bytes.
 * @param[in]  aIn_pSource       The string.
 * @param[in]  aCount            The size of the destination.
 * @return aOut_pDestination.
 */
char*
strncpy(char* restrict aOut_pDestination, const char* restrict aIn_pSource, size_t aCount)
{
        stpncpy(aOut_pDestination, aIn_pSource, aCount);
        return (aOut_pDestination);
}

/**
 * @brief Copies at most a given number of bytes of a string and fills the rest with nulls.
 *
 * @param[out] aOut_pDestination The destination, aCount bytes.
 * @param[in]  aIn_pSource       The string.
 * @param[in]  aCount            The size of the destination.
 * @return The first null character written, or the end of the destination when none was.
 */
char*
stpncpy(char* restrict aOut_pDestination, const char* restrict aIn_pSource, size_t aCount)
{
        size_t length = strnlen(aIn_pSource, aCount);

        memcpy(aOut_pDestination, aIn_pSource, length);
        memset(aOut_pDestination + length, 0, aCount - length);
        return (aOut_pDestination + length);
}

/**
 * @brief Appends a string.
 *
 * @param[in,out] aInOut_pDestination The string to append to.
 * @param[in]     aIn_pSource         The string to append.
 * @return aInOut_pDestination.
 */
char*
strcat(char* restrict aInOut_pDestination, const char* restrict aIn_pSource)
{
        strcpy(aInOut_pDestination + strlen(aInOut_pDestination), aIn_pSource);
        return (aInOut_pDestination);
}

/**
 * @brief Appends at most a given number of bytes of a string, then a null character.
 *
 * @param[in,out] aInOut_pDestination The string to append to.
 * @param[in]     aIn_pSource         The string to append.
 * @param[in]     aCount              The most bytes to append.
 * @return aInOut_pDestination.
 */
char*
strncat(char* restrict aInOut_pDestination, const char* restrict aIn_pSource, size_t aCount)
{
        char*  pEnd   = aInOut_pDestination + strlen(aInOut_pDestination);
        size_t length = strnlen(aIn_pSource, aCount);

        memcpy(pEnd, aIn_pSource, length);
        pEnd[ length ] = '\0';
        return (aInOut_pDestination);
}

/**
 * @brief Copies a string into a buffer of a given size, always ending it with a null character.
 *
 * @param[out] aOut_pDestination The buffer.
 * @param[in]  aIn_pSource       The string.
 * @param[in]  aSize             The size of the buffer; 0 writes nothing.
 * @return The length of the source: a value of aSize or more means the copy was cut.
 */
size_t
strlcpy(char* restrict aOut_pDestination, const char* restrict aIn_pSource, size_t aSize)
{
        size_t length = strlen(aIn_pSource);

        if(aSize > 0)
        {
                size_t copied = length < aSize ? length : aSize - 1;

                memcpy(aOut_pDestination, aIn_pSource, copied);
                aOut_pDestination[ copied ] = '\0';
        }
        return (length);
}

/**
 * @brief Appends a string inside a buffer of a given size, always ending it with a null character.
 *
 * @param[in,out] aInOut_pDestination The buffer, holding a string.
 * @param[in]     aIn_pSource         The string to append.
 * @param[in]     aSize               The size of the buffer.
 * @return The length the result would have: a value of aSize or more means it was cut.
 */
size_t
strlcat(char* restrict aInOut_pDestination, const char* restrict aIn_pSource, size_t aSize)
{
        size_t used = strnlen(aInOut_pDestination, aSize);

        if(used == aSize)
                return (aSize + strlen(aIn_pSource));
        return (used + strlcpy(aInOut_pDestination + used, aIn_pSource, aSize - used));
}

/**
 * @brief Compares two strings as unsigned char.
 *
 * @param[in] aIn_pFirst  The first string.
 * @param[in] aIn_pSecond The second string.
 * @return The sign of the first difference, or 0.
 */
int
strcmp(const char* aIn_pFirst, const char* aIn_pSecond)
{
        return (strncmp(aIn_pFirst, aIn_pSecond, SIZE_MAX));
}

/**
 * @brief Compares at most a given number of bytes of two strings as unsigned char.
 *
 * @param[in] aIn_pFirst  The first string.
 * @param[in] aIn_pSecond The second string.
 * @param[in] aCount      The most bytes to compare.
 * @return The sign of the first difference, or 0.
 */
int
strncmp(const char* aIn_pFirst, const char* aIn_pSecond, size_t aCount)
{
        const unsigned char* pFirst  = (const unsigned char*) aIn_pFirst;
        const unsigned char* pSecond = (const unsigned char*) aIn_pSecond;

        for(size_t index = 0; index < aCount; index++)
        {
                if(pFirst[ index ] != pSecond[ index ])
                        return (pFirst[ index ] < pSecond[ index ] ? -1 : 1);
                if(pFirst[ index ] == '\0')
                        return (0);
        }
        return (0);
}

/**
 * @brief Compares two strings in the collation of the locale: byte order in the C locale.
 *
 * @param[in] aIn_pFirst  The first string.
 * @param[in] aIn_pSecond The second string.
 * @return The sign of the first difference, or 0.
 */
int
strcoll(const char* aIn_pFirst, const char* aIn_pSecond)
{
        return (strcmp(aIn_pFirst, aIn_pSecond));
}

/**
 * @brief Transforms a string so that strcmp on the result orders as strcoll: a copy in the C locale.
 *
 * @param[out] aOut_pDestination The destination, or NULL when aSize is 0.
 * @param[in]  aIn_pSource       The string.
 * @param[in]  aSize             The size of the destination.
 * @return The length of the transformed string; the destination holds it only when it is below aSize.
 */
size_t
strxfrm(char* restrict aOut_pDestination, const char* restrict aIn_pSource, size_t aSize)
{
        size_t length = strlen(aIn_pSource);

        if(length < aSize)
                memcpy(aOut_pDestination, aIn_pSource, length + 1);
        return (length);
}

/**
 * @brief Finds the first occurrence of a character in a string; the null character counts.
 *
 * @param[in] aIn_pString The string.
 * @param[in] aCharacter  The character, converted to char.
 * @return The character, or NULL.
 */
char*
strchr(const char* aIn_pString, int aCharacter)
{
        for(;; aIn_pString++)
        {
                if(*aIn_pString == (char) aCharacter)
                        return ((char*) aIn_pString);
                if(*aIn_pString == '\0')
                        return (NULL);
        }
}

/**
 * @brief Finds the last occurrence of a character in a string; the null character counts.
 *
 * @param[in] aIn_pString The string.
 * @param[in] aCharacter  The character, converted to char.
 * @return The character, or NULL.
 */
char*
strrchr(const char* aIn_pString, int aCharacter)
{
        const char* pLast = NULL;

        for(;; aIn_pString++)
        {
                if(*aIn_pString == (char) aCharacter)
                        pLast = aIn_pString;
                if(*aIn_pString == '\0')
                        return ((char*) pLast);
        }
}

/**
 * @brief Marks the bytes of a set in a table of 256 entries.
 *
 * @param[out] aOut_pTable The table, cleared first.
 * @param[in]  aIn_pSet    The set, a string.
 */
static void
MarkSet(unsigned char aOut_pTable[ 256 ], const char* aIn_pSet)
{
        memset(aOut_pTable, 0, 256);
        for(; *aIn_pSet != '\0'; aIn_pSet++)
                aOut_pTable[ (unsigned char) *aIn_pSet ] = 1;
}

/**
 * @brief Counts the bytes at the start of a string that are in a set.
 *
 * @param[in] aIn_pString The string.
 * @param[in] aIn_pSet    The set.
 * @return The count.
 */
size_t
strspn(const char* aIn_pString, const char* aIn_pSet)
{
        unsigned char table[ 256 ];
        size_t        length = 0;

        MarkSet(table, aIn_pSet);
        while(aIn_pString[ length ] != '\0' && table[ (unsigned char) aIn_pString[ length ] ])
                length++;
        return (length);
}

/**
 * @brief Counts the bytes at the start of a string that are not in a set.
 *
 * @param[in] aIn_pString The string.
 * @param[in] aIn_pSet    The set.
 * @return The count.
 */
size_t
strcspn(const char* aIn_pString, const char* aIn_pSet)
{
        unsigned char table[ 256 ];
        size_t        length = 0;

        MarkSet(table, aIn_pSet);
        while(aIn_pString[ length ] != '\0' && !table[ (unsigned char) aIn_pString[ length ] ])
                length++;
        return (length);
}

/**
 * @brief Finds the first byte of a string that is in a set.
 *
 * @param[in] aIn_pString The string.
 * @param[in] aIn_pSet    The set.
 * @return The byte, or NULL.
 */
char*
strpbrk(const char* aIn_pString, const char* aIn_pSet)
{
        const char* pFound = aIn_pString + strcspn(aIn_pString, aIn_pSet);

        return (*pFound != '\0' ? (char*) pFound : NULL);
}

/**
 * @brief Finds the first occurrence of a string in another.
 *
 * @param[in] aIn_pString The string to search.
 * @param[in] aIn_pNeedle The string to find; an empty one is found at the start.
 * @return The occurrence, or NULL.
 */
char*
strstr(const char* aIn_pString, const char* aIn_pNeedle)
{
        size_t length = strlen(aIn_pNeedle);

        if(length == 0)
                return ((char*) aIn_pString);
        for(; (aIn_pString = strchr(aIn_pString, *aIn_pNeedle)) != NULL; aIn_pString++)
        {
                if(strncmp(aIn_pString, aIn_pNeedle, length) == 0)
                        return ((char*) aIn_pString);
        }
        return (NULL);
}

/**
 * @brief Cuts a string into tokens separated by bytes of a set, keeping its place in a pointer.
 *
 * @param[in,out] aInOut_pString The string the first call cuts, NULL for the next calls.
 * @param[in]     aIn_pSet       The separators.
 * @param[in,out] aInOut_ppSave  Where the place is kept between calls.
 * @return The next token, ended with a null character written over its separator, or NULL.
 */
char*
strtok_r(char* restrict aInOut_pString,
         const char* restrict aIn_pSet,
         char** restrict aInOut_ppSave)
{
        char* pToken = aInOut_pString != NULL ? aInOut_pString : *aInOut_ppSave;

        if(pToken == NULL)
                return (NULL);
        pToken += strspn(pToken, aIn_pSet);
        if(*pToken == '\0')
        {
                *aInOut_ppSave = NULL;
                return (NULL);
        }
        char* pEnd = pToken + strcspn(pToken, aIn_pSet);

        if(*pEnd == '\0')
                *aInOut_ppSave = NULL;
        else
        {
                *pEnd          = '\0';
                *aInOut_ppSave = pEnd + 1;
        }
        return (pToken);
}

/**
 * @brief Cuts a string into tokens separated by bytes of a set, keeping its place between calls.
 *
 * @param[in,out] aInOut_pString The string the first call cuts, NULL for the next calls.
 * @param[in]     aIn_pSet       The separators.
 * @return The next token, or NULL.
 */
char*
strtok(char* restrict aInOut_pString, const char* restrict aIn_pSet)
{
        static char* spSave;

        return (strtok_r(aInOut_pString, aIn_pSet, &spSave));
}

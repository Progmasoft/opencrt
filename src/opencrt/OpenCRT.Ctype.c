/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>

/*
 * The classes of the C locale, ASCII. A value that is neither EOF nor an unsigned char is in no
 * class and maps to itself, where the standard leaves the behavior undefined.
 */

/**
 * @brief Tells whether a value is an unsigned char in a range of characters.
 *
 * @param[in] aCharacter The value.
 * @param[in] aFirst     The first character of the range.
 * @param[in] aLast      The last character of the range.
 * @return Non-zero when it is.
 */
static int
IsIn(int aCharacter, int aFirst, int aLast)
{
        return (aCharacter >= aFirst && aCharacter <= aLast);
}

/**
 * @brief Whether the character is a letter or a digit.
 *
 * @param[in] aCharacter The character.
 * @return Non-zero when it is.
 */
int
isalnum(int aCharacter)
{
        return (isalpha(aCharacter) || isdigit(aCharacter));
}

/**
 * @brief Whether the character is a letter.
 *
 * @param[in] aCharacter The character.
 * @return Non-zero when it is.
 */
int
isalpha(int aCharacter)
{
        return (isupper(aCharacter) || islower(aCharacter));
}

/**
 * @brief Whether the character is a space or a tab.
 *
 * @param[in] aCharacter The character.
 * @return Non-zero when it is.
 */
int
isblank(int aCharacter)
{
        return (aCharacter == ' ' || aCharacter == '\t');
}

/**
 * @brief Whether the character is a control character.
 *
 * @param[in] aCharacter The character.
 * @return Non-zero when it is.
 */
int
iscntrl(int aCharacter)
{
        return (IsIn(aCharacter, 0, 0x1F) || aCharacter == 0x7F);
}

/**
 * @brief Whether the character is a decimal digit.
 *
 * @param[in] aCharacter The character.
 * @return Non-zero when it is.
 */
int
isdigit(int aCharacter)
{
        return (IsIn(aCharacter, '0', '9'));
}

/**
 * @brief Whether the character prints and is not a space.
 *
 * @param[in] aCharacter The character.
 * @return Non-zero when it is.
 */
int
isgraph(int aCharacter)
{
        return (IsIn(aCharacter, 0x21, 0x7E));
}

/**
 * @brief Whether the character is a lower case letter.
 *
 * @param[in] aCharacter The character.
 * @return Non-zero when it is.
 */
int
islower(int aCharacter)
{
        return (IsIn(aCharacter, 'a', 'z'));
}

/**
 * @brief Whether the character prints, the space included.
 *
 * @param[in] aCharacter The character.
 * @return Non-zero when it is.
 */
int
isprint(int aCharacter)
{
        return (IsIn(aCharacter, 0x20, 0x7E));
}

/**
 * @brief Whether the character prints and is neither a letter, a digit nor a space.
 *
 * @param[in] aCharacter The character.
 * @return Non-zero when it is.
 */
int
ispunct(int aCharacter)
{
        return (isgraph(aCharacter) && !isalnum(aCharacter));
}

/**
 * @brief Whether the character is white space: space, tab, newline, vertical tab, form feed or carriage return.
 *
 * @param[in] aCharacter The character.
 * @return Non-zero when it is.
 */
int
isspace(int aCharacter)
{
        return (aCharacter == ' ' || IsIn(aCharacter, '\t', '\r'));
}

/**
 * @brief Whether the character is an upper case letter.
 *
 * @param[in] aCharacter The character.
 * @return Non-zero when it is.
 */
int
isupper(int aCharacter)
{
        return (IsIn(aCharacter, 'A', 'Z'));
}

/**
 * @brief Whether the character is a hexadecimal digit.
 *
 * @param[in] aCharacter The character.
 * @return Non-zero when it is.
 */
int
isxdigit(int aCharacter)
{
        return (isdigit(aCharacter) || IsIn(aCharacter, 'a', 'f') || IsIn(aCharacter, 'A', 'F'));
}

/**
 * @brief The lower case of a letter; any other value as it is.
 *
 * @param[in] aCharacter The character.
 * @return The character.
 */
int
tolower(int aCharacter)
{
        return (isupper(aCharacter) ? aCharacter + ('a' - 'A') : aCharacter);
}

/**
 * @brief The upper case of a letter; any other value as it is.
 *
 * @param[in] aCharacter The character.
 * @return The character.
 */
int
toupper(int aCharacter)
{
        return (islower(aCharacter) ? aCharacter - ('a' - 'A') : aCharacter);
}

/**
 * @brief Whether the value is a 7-bit ASCII character.
 *
 * @param[in] aCharacter The value.
 * @return Non-zero when it is.
 */
int
isascii(int aCharacter)
{
        return (IsIn(aCharacter, 0, 0x7F));
}

/**
 * @brief The value with all but its low 7 bits cleared.
 *
 * @param[in] aCharacter The value.
 * @return The character.
 */
int
toascii(int aCharacter)
{
        return (aCharacter & 0x7F);
}

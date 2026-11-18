/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>
#include <string.h>
#include <wctype.h>

/*
 * The wide classes of the C locale: those of ASCII. The other wide characters, the bytes 0x80 to
 * 0xFF among them, are in no class and have no case.
 */

/**
 * @brief The narrow class function a wide character goes to, or -1 outside ASCII.
 *
 * @param[in] aCharacter The wide character.
 * @return The character as an int, or -1.
 */
static int
Narrow(wint_t aCharacter)
{
        return (aCharacter < 0x80 ? (int) aCharacter : -1);
}

/** The names of the classes of wctype, in the order of their numbers from 1. */
static const char* const kClasses[] = {
        "alnum", "alpha", "blank", "cntrl", "digit", "graph",
        "lower", "print", "punct", "space", "upper", "xdigit",
};

/** The functions of the classes, in the same order. */
static int (*const kClassFunctions[])(int) = {
        isalnum, isalpha, isblank, iscntrl, isdigit, isgraph,
        islower, isprint, ispunct, isspace, isupper, isxdigit,
};

/**
 * @brief Whether a wide character is in a class.
 *
 * @param[in] aCharacter The wide character.
 * @param[in] aClass     The class, from wctype.
 * @return Non-zero when it is.
 */
int
iswctype(wint_t aCharacter, wctype_t aClass)
{
        int narrow = Narrow(aCharacter);

        if(aClass < 1 || aClass > (int) (sizeof(kClasses) / sizeof(kClasses[ 0 ])) || narrow < 0)
                return (0);
        return (kClassFunctions[ aClass - 1 ](narrow));
}

/**
 * @brief The class of a name, such as "alpha".
 *
 * @param[in] aIn_pName The name.
 * @return The class, or 0 for a name the locale has no class of.
 */
wctype_t
wctype(const char* aIn_pName)
{
        for(int index = 0; index < (int) (sizeof(kClasses) / sizeof(kClasses[ 0 ])); index++)
        {
                if(strcmp(aIn_pName, kClasses[ index ]) == 0)
                        return (index + 1);
        }
        return (0);
}

/* clang-format off */
/** @brief Whether the wide character is a letter or a digit. @param[in] aCharacter It. @return Non-zero when it is. */
int iswalnum(wint_t aCharacter) { return (iswctype(aCharacter, 1)); }
/** @brief Whether the wide character is a letter. @param[in] aCharacter It. @return Non-zero when it is. */
int iswalpha(wint_t aCharacter) { return (iswctype(aCharacter, 2)); }
/** @brief Whether the wide character is a space or a tab. @param[in] aCharacter It. @return Non-zero when it is. */
int iswblank(wint_t aCharacter) { return (iswctype(aCharacter, 3)); }
/** @brief Whether the wide character is a control character. @param[in] aCharacter It. @return Non-zero when it is. */
int iswcntrl(wint_t aCharacter) { return (iswctype(aCharacter, 4)); }
/** @brief Whether the wide character is a decimal digit. @param[in] aCharacter It. @return Non-zero when it is. */
int iswdigit(wint_t aCharacter) { return (iswctype(aCharacter, 5)); }
/** @brief Whether the wide character prints and is not a space. @param[in] aCharacter It. @return Non-zero when it is. */
int iswgraph(wint_t aCharacter) { return (iswctype(aCharacter, 6)); }
/** @brief Whether the wide character is a lower case letter. @param[in] aCharacter It. @return Non-zero when it is. */
int iswlower(wint_t aCharacter) { return (iswctype(aCharacter, 7)); }
/** @brief Whether the wide character prints. @param[in] aCharacter It. @return Non-zero when it is. */
int iswprint(wint_t aCharacter) { return (iswctype(aCharacter, 8)); }
/** @brief Whether the wide character is punctuation. @param[in] aCharacter It. @return Non-zero when it is. */
int iswpunct(wint_t aCharacter) { return (iswctype(aCharacter, 9)); }
/** @brief Whether the wide character is white space. @param[in] aCharacter It. @return Non-zero when it is. */
int iswspace(wint_t aCharacter) { return (iswctype(aCharacter, 10)); }
/** @brief Whether the wide character is an upper case letter. @param[in] aCharacter It. @return Non-zero when it is. */
int iswupper(wint_t aCharacter) { return (iswctype(aCharacter, 11)); }
/** @brief Whether the wide character is a hexadecimal digit. @param[in] aCharacter It. @return Non-zero when it is. */
int iswxdigit(wint_t aCharacter) { return (iswctype(aCharacter, 12)); }
/* clang-format on */

/**
 * @brief The lower case of a wide letter of ASCII; any other as it is.
 *
 * @param[in] aCharacter The wide character.
 * @return The character.
 */
wint_t
towlower(wint_t aCharacter)
{
        return (Narrow(aCharacter) < 0 ? aCharacter : (wint_t) tolower((int) aCharacter));
}

/**
 * @brief The upper case of a wide letter of ASCII; any other as it is.
 *
 * @param[in] aCharacter The wide character.
 * @return The character.
 */
wint_t
towupper(wint_t aCharacter)
{
        return (Narrow(aCharacter) < 0 ? aCharacter : (wint_t) toupper((int) aCharacter));
}

/**
 * @brief The mapping of a name: "tolower" or "toupper".
 *
 * @param[in] aIn_pName The name.
 * @return The mapping, or 0.
 */
wctrans_t
wctrans(const char* aIn_pName)
{
        if(strcmp(aIn_pName, "tolower") == 0)
                return (1);
        return (strcmp(aIn_pName, "toupper") == 0 ? 2 : 0);
}

/**
 * @brief Maps a wide character.
 *
 * @param[in] aCharacter The wide character.
 * @param[in] aMapping   The mapping, from wctrans.
 * @return The mapped character; the character itself for mapping 0.
 */
wint_t
towctrans(wint_t aCharacter, wctrans_t aMapping)
{
        if(aMapping == 1)
                return (towlower(aCharacter));
        return (aMapping == 2 ? towupper(aCharacter) : aCharacter);
}

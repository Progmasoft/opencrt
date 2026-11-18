/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <limits.h>
#include <locale.h>
#include <stddef.h>
#include <string.h>

/*
 * The C locale, also named POSIX, is the one locale: setlocale accepts it, and the empty name that
 * asks for the locale of the environment gives it too, as no other is installed.
 */

/** The numeric and monetary conventions of the C locale. */
static struct lconv gsConventions = {
        ".",      "",       "",       "",       "",       "",       "",       "",
        "",       "",       CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
        CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
};

/**
 * @brief Sets or asks for the locale of a category.
 *
 * @param[in] aCategory The category, LC_ALL to LC_MESSAGES.
 * @param[in] aIn_pName "C", "POSIX" or "" to set it, NULL to ask.
 * @return "C", or NULL for a category or a locale that does not exist.
 */
char*
setlocale(int aCategory, const char* aIn_pName)
{
        if(aCategory < LC_ALL || aCategory > LC_MESSAGES)
                return (NULL);
        if(aIn_pName == NULL || aIn_pName[ 0 ] == '\0' || strcmp(aIn_pName, "C") == 0
           || strcmp(aIn_pName, "POSIX") == 0)
        {
                static char sName[] = "C";

                return (sName);
        }
        return (NULL);
}

/**
 * @brief The numeric and monetary conventions of the locale.
 *
 * @return The conventions; a program must not change them.
 */
struct lconv*
localeconv(void)
{
        return (&gsConventions);
}

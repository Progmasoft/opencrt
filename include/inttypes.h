/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

#include <sys/_types/_wchar_t.h>
#include <sys/cdefs.h>

typedef struct
{
        intmax_t quot;
        intmax_t rem;
} imaxdiv_t;

__BEGIN_DECLS
intmax_t  imaxabs(intmax_t) __pure2;
imaxdiv_t imaxdiv(intmax_t, intmax_t) __pure2;
intmax_t
strtoimax(const char* __restrict_arg, char** __restrict_arg, int);
uintmax_t
strtoumax(const char* __restrict_arg, char** __restrict_arg, int);
__END_DECLS

/* LP64: 64-bit types are long, as on Darwin, so their conversions use l. */
#define __PRI8  "hh"
#define __PRI16 "h"
#define __PRI32 ""
#define __PRI64 "ll"

#define PRId8   __PRI8 "d"
#define PRId16  __PRI16 "d"
#define PRId32  __PRI32 "d"
#define PRId64  __PRI64 "d"
#define PRIi8   __PRI8 "i"
#define PRIi16  __PRI16 "i"
#define PRIi32  __PRI32 "i"
#define PRIi64  __PRI64 "i"
#define PRIo8   __PRI8 "o"
#define PRIo16  __PRI16 "o"
#define PRIo32  __PRI32 "o"
#define PRIo64  __PRI64 "o"
#define PRIu8   __PRI8 "u"
#define PRIu16  __PRI16 "u"
#define PRIu32  __PRI32 "u"
#define PRIu64  __PRI64 "u"
#define PRIx8   __PRI8 "x"
#define PRIx16  __PRI16 "x"
#define PRIx32  __PRI32 "x"
#define PRIx64  __PRI64 "x"
#define PRIX8   __PRI8 "X"
#define PRIX16  __PRI16 "X"
#define PRIX32  __PRI32 "X"
#define PRIX64  __PRI64 "X"
#define PRIdMAX "jd"
#define PRIiMAX "ji"
#define PRIoMAX "jo"
#define PRIuMAX "ju"
#define PRIxMAX "jx"
#define PRIXMAX "jX"
#define PRIdPTR "ld"
#define PRIiPTR "li"
#define PRIoPTR "lo"
#define PRIuPTR "lu"
#define PRIxPTR "lx"
#define PRIXPTR "lX"

/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <sys/_types/_wint_t.h>
#include <sys/cdefs.h>

#ifndef WEOF
#    define WEOF ((wint_t) - 1)
#endif

typedef int wctype_t;
typedef int wctrans_t;

__BEGIN_DECLS
int    iswalnum(wint_t);
int    iswalpha(wint_t);
int    iswblank(wint_t);
int    iswcntrl(wint_t);
int    iswdigit(wint_t);
int    iswgraph(wint_t);
int    iswlower(wint_t);
int    iswprint(wint_t);
int    iswpunct(wint_t);
int    iswspace(wint_t);
int    iswupper(wint_t);
int    iswxdigit(wint_t);
wint_t towlower(wint_t);
wint_t towupper(wint_t);
wctype_t
wctype(const char*);
int iswctype(wint_t, wctype_t);
wctrans_t
wctrans(const char*);
wint_t towctrans(wint_t, wctrans_t);
__END_DECLS

/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <sys/_types/_mbstate_t.h>
#include <sys/_types/_null.h>
#include <sys/_types/_size_t.h>
#include <sys/_types/_wchar_t.h>
#include <sys/_types/_wint_t.h>
#include <sys/cdefs.h>

#ifndef WEOF
#    define WEOF ((wint_t) - 1)
#endif
#define WCHAR_MAX __WCHAR_MAX__
#define WCHAR_MIN (-WCHAR_MAX - 1)

__BEGIN_DECLS
size_t
wcslen(const wchar_t*);
wchar_t*
wcscpy(wchar_t* __restrict_arg, const wchar_t* __restrict_arg);
wchar_t*
wcsncpy(wchar_t* __restrict_arg, const wchar_t* __restrict_arg, size_t);
wchar_t*
wcscat(wchar_t* __restrict_arg, const wchar_t* __restrict_arg);
wchar_t*
wcsncat(wchar_t* __restrict_arg, const wchar_t* __restrict_arg, size_t);
int
wcscmp(const wchar_t*, const wchar_t*);
int
wcsncmp(const wchar_t*, const wchar_t*, size_t);
int
wcscoll(const wchar_t*, const wchar_t*);
size_t
wcsxfrm(wchar_t* __restrict_arg, const wchar_t* __restrict_arg, size_t);
wchar_t*
wcschr(const wchar_t*, wchar_t);
wchar_t*
wcsrchr(const wchar_t*, wchar_t);
size_t
wcsspn(const wchar_t*, const wchar_t*);
size_t
wcscspn(const wchar_t*, const wchar_t*);
wchar_t*
wcspbrk(const wchar_t*, const wchar_t*);
wchar_t*
wcsstr(const wchar_t* __restrict_arg, const wchar_t* __restrict_arg);
wchar_t*
wcstok(wchar_t* __restrict_arg, const wchar_t* __restrict_arg, wchar_t** __restrict_arg);
wchar_t*
wmemcpy(wchar_t* __restrict_arg, const wchar_t* __restrict_arg, size_t);
wchar_t*
wmemmove(wchar_t*, const wchar_t*, size_t);
wchar_t*
wmemset(wchar_t*, wchar_t, size_t);
int
wmemcmp(const wchar_t*, const wchar_t*, size_t);
wchar_t*
wmemchr(const wchar_t*, wchar_t, size_t);
size_t
wcsnlen(const wchar_t*, size_t);

wint_t
btowc(int);
int wctob(wint_t);
int
mbsinit(const mbstate_t*);
size_t
mbrlen(const char* __restrict_arg, size_t, mbstate_t* __restrict_arg);
size_t
mbrtowc(wchar_t* __restrict_arg, const char* __restrict_arg, size_t, mbstate_t* __restrict_arg);
size_t
wcrtomb(char* __restrict_arg, wchar_t, mbstate_t* __restrict_arg);
size_t
mbsrtowcs(wchar_t* __restrict_arg, const char** __restrict_arg, size_t, mbstate_t* __restrict_arg);
size_t
wcsrtombs(char* __restrict_arg, const wchar_t** __restrict_arg, size_t, mbstate_t* __restrict_arg);
__END_DECLS

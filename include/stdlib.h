/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <sys/_types/_null.h>
#include <sys/_types/_size_t.h>
#include <sys/_types/_wchar_t.h>
#include <sys/cdefs.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX     0x7FFFFFFF
#define MB_CUR_MAX   ((size_t) 1)

typedef struct
{
        int quot;
        int rem;
} div_t;

typedef struct
{
        long quot;
        long rem;
} ldiv_t;

typedef struct
{
        long long quot;
        long long rem;
} lldiv_t;

__BEGIN_DECLS
int
atoi(const char*);
long
atol(const char*);
long long
atoll(const char*);
long
strtol(const char* __restrict_arg, char** __restrict_arg, int);
long long
strtoll(const char* __restrict_arg, char** __restrict_arg, int);
unsigned long
strtoul(const char* __restrict_arg, char** __restrict_arg, int);
unsigned long long
strtoull(const char* __restrict_arg, char** __restrict_arg, int);

int
abs(int) __pure2;
long
labs(long) __pure2;
long long
llabs(long long) __pure2;
div_t
div(int, int) __pure2;
ldiv_t
ldiv(long, long) __pure2;
lldiv_t
lldiv(long long, long long) __pure2;

void
qsort(void*, size_t, size_t, int (*)(const void*, const void*));
void*
bsearch(const void*, const void*, size_t, size_t, int (*)(const void*, const void*));

int
rand(void);
void
srand(unsigned);

int
mblen(const char*, size_t);
int
mbtowc(wchar_t* __restrict_arg, const char* __restrict_arg, size_t);
int
wctomb(char*, wchar_t);
size_t
mbstowcs(wchar_t* __restrict_arg, const char* __restrict_arg, size_t);
size_t
wcstombs(char* __restrict_arg, const wchar_t* __restrict_arg, size_t);
__END_DECLS

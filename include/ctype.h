/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <sys/cdefs.h>

__BEGIN_DECLS
int
isalnum(int);
int
isalpha(int);
int
isblank(int);
int
iscntrl(int);
int
isdigit(int);
int
isgraph(int);
int
islower(int);
int
isprint(int);
int
ispunct(int);
int
isspace(int);
int
isupper(int);
int
isxdigit(int);
int
tolower(int);
int
toupper(int);
/* POSIX. */
int
isascii(int);
int
toascii(int);
__END_DECLS

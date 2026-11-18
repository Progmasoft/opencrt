/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <sys/_types/_size_t.h>
#include <sys/cdefs.h>

__BEGIN_DECLS
int
strcasecmp(const char*, const char*);
int
strncasecmp(const char*, const char*, size_t);
int
ffs(int);
/* OpenBSD and glibc: memset that the compiler keeps, for secrets. */
void
explicit_bzero(void*, size_t);
__END_DECLS

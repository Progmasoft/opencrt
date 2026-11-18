/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <sys/_types/_null.h>
#include <sys/_types/_size_t.h>
#include <sys/cdefs.h>

#define EOF (-1)

__BEGIN_DECLS
int
snprintf(char* __restrict_arg, size_t, const char* __restrict_arg, ...) __printflike(3, 4);
int
vsnprintf(char* __restrict_arg, size_t, const char* __restrict_arg, __builtin_va_list)
        __printflike(3, 0);
int
sprintf(char* __restrict_arg, const char* __restrict_arg, ...) __printflike(2, 3);
int
vsprintf(char* __restrict_arg, const char* __restrict_arg, __builtin_va_list) __printflike(2, 0);
__END_DECLS

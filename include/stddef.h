/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <sys/_types/_null.h>
#include <sys/_types/_size_t.h>
#include <sys/_types/_wchar_t.h>

typedef __PTRDIFF_TYPE__ ptrdiff_t;

#define offsetof(type, member) __builtin_offsetof(type, member)

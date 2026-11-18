/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

/* The conversion state of a multibyte string, with the size and alignment of Darwin's. */
typedef union
{
        char      __mbstate8[ 128 ];
        long long __mbstateL;
} mbstate_t;

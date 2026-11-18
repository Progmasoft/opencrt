/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

/*
 * The definitions every header of OpenCRT shares, as in the BSDs and Darwin: the C++ linkage block,
 * restrict, and the attributes of the interface.
 */

#pragma once

#ifdef __cplusplus
#    define __BEGIN_DECLS extern "C" {
#    define __END_DECLS   }
#else
#    define __BEGIN_DECLS
#    define __END_DECLS
#endif

#if defined(__cplusplus) || !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#    define __restrict_arg __restrict
#else
#    define __restrict_arg restrict
#endif

#define __dead2            __attribute__((__noreturn__))
#define __pure2            __attribute__((__const__))
#define __printflike(f, a) __attribute__((__format__(__printf__, f, a)))
#define __strftimelike(f)  __attribute__((__format__(__strftime__, f, 0)))

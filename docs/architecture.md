<!-- SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com> -->
<!-- SPDX-License-Identifier: MIT -->

# Architecture

## Headers, `include/`

The headers of C99 and POSIX that OpenCRT has, with Darwin's numbers and layouts (`errno`, `mbstate_t`, `struct tm`,
`struct lconv`). `sys/cdefs.h` gives the attributes the headers use, such as `__printflike`. A type that several
headers declare has one header of its own in `sys/_types/`, as C99 forbids declaring a typedef twice. The
freestanding headers (`stddef.h`, `stdint.h`, `limits.h`, `float.h`, `stdarg.h`) come from the compiler's
predefined macros.

## The library, `src/opencrt/`

| Module | What it has |
| --- | --- |
| `String`, `Strings` | `<string.h>` and `<strings.h>`, `strlcpy`, `strlcat` and `explicit_bzero` included |
| `Ctype`, `Wctype` | The classes of the C locale, narrow and wide |
| `Errno` | `__error()`, Darwin's `errno`, `strerror` and `strerror_r` |
| `Convert`, `Arithmetic`, `Sort` | The conversions of `<stdlib.h>`, `abs` and `div`, `rand`, and `qsort` as an introsort with `bsearch` |
| `FormatSpec`, `Format`, `FormatFloat`, `Printf` | The printf engine: reading a format and its arguments, by position too; writing every conversion, `%n` included; floating point converted exactly; the `snprintf` family over a buffer sink |
| `Multibyte`, `Wide`, `Locale` | The C locale, single-byte, its bytes 0x80 to 0xFF as U+DF80 to U+DFFF; `<wchar.h>`; `setlocale` and `localeconv` |
| `Math`, `Time` | The exact functions of `<math.h>`; `gmtime_r`, `timegm`, `asctime_r`, `strftime` and `difftime` |
| `Ssp` | `__stack_chk_guard` and `__stack_chk_fail`, for Darwin's stack protector |

Names inside the library start with `__OpenCRT_`, so they cannot clash with a program's.

## The dynamic linker

OpenCRT holds the dynamic linker as well, installed as `/usr/lib/dyld` beside `/usr/lib/libSystem.B.dylib`; see
the [README](../README.md).

## Tests, `tests/`

The tests are a program of their own that uses nothing of a host C library: `Test.Start.S` is its `_start`, and
`Test.Harness.c` writes with the `write` system call. Each suite is a file (`StringTests.c`, `StdlibTests.c`,
`PrintfTests.c`, `OtherTests.c`), listed in `Test.Suites.c`. `PrintfCases.inc` holds outputs of the host's printf,
which rounds exactly, written by `tools/printf-cases.c`.

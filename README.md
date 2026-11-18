<!-- SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com> -->
<!-- SPDX-License-Identifier: MIT -->

# OpenCRT

OpenCRT, the Open C Runtime, is the C library of TalaOS: the C standard library and the Unix standard library, on
[OpenTala](https://github.com/Progmasoft/opentala). Unix and POSIX compatibility comes first.

It is written in C99 and built with BSD Make.

## What the standards decide, and what the guidelines decide

The interface of OpenCRT is the one the C standard and POSIX define, and nothing in it is renamed or reshaped:

- the header names, `<stdio.h>`, `<unistd.h>`, `<sys/types.h>`;
- the names, types, macros and signatures of the interface, `printf`, `FILE`, `EOF`, `errno`, `size_t`, `O_RDONLY`;
- the identifiers the standards reserve for the implementation, `_Name` and `__name`, which the headers use for
  everything a program must not see.

The [Progmasoft Core C Coding Guidelines](https://github.com/Progmasoft/progmasoft-cpp) govern everything behind that
interface: the layout, the comments and Doxygen, and the internal code. An internal function or object that a program
could link against is named in the reserved space, `__OpenCRT_<Module>_<Name>`, so it cannot collide with a name of
the program; a `static` one follows the guidelines' names as they are. Where a standard requires a form the guidelines
do not use, the standard wins, and the contributing guide lists each such deviation once code exists.

`long double` follows the Darwin ABI of OpenTala: the x87 80-bit extended format on amd64 and IEEE 754 binary64 on
aarch64. `printf`, `strtold` and the `<math.h>` functions with the `l` suffix use those formats.

## The dynamic linker

OpenCRT also holds the dynamic linker, which the kernel starts for every program that names it with
`LC_LOAD_DYLINKER`: it maps the libraries a program needs, applies their fixups (chained fixups included), sets up
thread-local variables and calls the program's entry point. It is installed as `/usr/lib/dyld`, and the library as
`/usr/lib/libSystem.B.dylib`, the names Darwin's tools write into programs. Keeping both in one repository keeps the
parts they share, such as the layout of thread-local data and the start of a process, from drifting apart.

## What it has

The parts that need no system call: the freestanding headers; `<string.h>`, `<strings.h>`, `<ctype.h>`; the
conversions, sorting and arithmetic of `<stdlib.h>` and `<inttypes.h>`; `errno` and `strerror`; the `snprintf`
family with every conversion, positions (`%1$s`) and exact floating point, `long double` included; `<wchar.h>`,
`<wctype.h>` and the C locale; the exact functions of `<math.h>`; the pure functions of `<time.h>`. File I/O,
`malloc`, signals and processes come with OpenTala's system calls.

Where a choice was open, security came first, then Unix and POSIX, then speed:

- `printf` has every conversion of C99, `%n` included, with every length modifier. `%s` of NULL writes
  `(null)`.
- `qsort` is an introsort: O(n log n) for every input, so no input can make it quadratic.
- The C locale is single-byte, as POSIX has it; the bytes 0x80 to 0xFF map to U+DF80 to U+DFFF, so a byte is never
  taken for a Unicode character.
- `explicit_bzero`, `strlcpy` and `strlcat` are there; `asctime_r` and `gmtime_r` refuse a year that does not fit
  with `EOVERFLOW` instead of overflowing.

## Building and testing

```sh
bmake           # build/libc.a
bmake test      # a test program with no other libc, on Linux amd64
bmake targets   # every source for amd64 and aarch64 Mach-O
bmake format    # clang-format 23
```

The floating-point cases of `tests/PrintfCases.inc` come from the printf of the host C library, which rounds
exactly; `tools/printf-cases.c` writes them.

## License

[MIT](LICENSE.txt), also covered by the [Progmasoft Patent Grant 1.1](PATENTS.md). Participation follows the
[Code of Conduct](CODE_OF_CONDUCT.md).

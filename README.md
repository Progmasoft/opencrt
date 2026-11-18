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

No code is written yet.

## License

[MIT](LICENSE.txt), also covered by the [Progmasoft Patent Grant 1.1](PATENTS.md). Participation follows the
[Code of Conduct](CODE_OF_CONDUCT.md).

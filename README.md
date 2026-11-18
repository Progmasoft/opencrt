<!-- SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com> -->
<!-- SPDX-License-Identifier: MIT -->

# OpenCRT

OpenCRT, the Open C Runtime, is the C library of TalaOS: the C standard library and the Unix standard library, on
[OpenTala](https://github.com/Progmasoft/opentala). Unix and POSIX compatibility comes first.

It is written in C99 under the
[Progmasoft Core C Coding Guidelines](https://github.com/Progmasoft/progmasoft-cpp) and built with BSD Make.

`long double` follows the Darwin ABI of OpenTala: the x87 80-bit extended format on amd64 and IEEE 754 binary64 on
aarch64. `printf`, `strtold` and the `<math.h>` functions with the `l` suffix use those formats.

No code is written yet.

## License

[MIT](LICENSE.txt), also covered by the [Progmasoft Patent Grant 1.1](PATENTS.md). Participation follows the
[Code of Conduct](CODE_OF_CONDUCT.md).

<!-- SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com> -->
<!-- SPDX-License-Identifier: MIT -->

# Development

OpenCRT needs BSD Make (`bmake`) and Clang.

```sh
bmake            # build/libc.a
bmake test       # the test program, on Linux amd64
bmake targets    # every source for amd64 and arm64 Mach-O
bmake format     # clang-format 23
```

The library is C99 with `-pedantic-errors`, built freestanding (`-ffreestanding -fno-builtin -nostdinc`), so it uses
nothing of the host's headers or library. To renew the printf cases:

```sh
cc -o build/printf-cases tools/printf-cases.c -lm && build/printf-cases > tests/PrintfCases.inc
```

CI runs the tests, compiles the Mach-O targets, checks the format, and checks the workflows with zizmor and
actionlint.

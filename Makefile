# SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
# SPDX-License-Identifier: MIT
#
# Builds OpenCRT with BSD Make (bmake on Linux).
#
#   make            build/libc.a for the machine that builds it
#   make test       the tests: a program with no other libc, run on Linux amd64
#   make targets    compiles every source for TalaOS's targets, amd64 and aarch64 (Mach-O)
#   make format     checks the layout with clang-format 23

OPENCRT_CC?=	clang
AR?=		ar
CLANG_FORMAT?=	clang-format

BUILD=		build
LIBRARY=	${BUILD}/libc.a
TESTS=		${BUILD}/opencrt-tests

# C99 without extensions but the attributes and asm the headers and the code spell with __.
WARNINGS=	-std=c99 -pedantic-errors -Wall -Wextra -Werror
LIB_CFLAGS=	${WARNINGS} -ffreestanding -fno-builtin -fno-math-errno -nostdinc -Iinclude -Isrc -O2 -g
# The host has no thread pointer for the canary, so the tests build without the stack protector.
HOST_CFLAGS=	${LIB_CFLAGS} -fno-stack-protector
TEST_CFLAGS=	${HOST_CFLAGS} -Itests
TEST_LDFLAGS=	-nostdlib -static

LIB_SRCS!=	ls src/opencrt/*.c
TEST_SRCS!=	ls tests/*.c
HDRS!=		ls include/*.h include/sys/*.h include/sys/_types/*.h src/opencrt/*.h tests/*.h 2>/dev/null || true

all: ${LIBRARY}

LIB_OBJS=
.for src in ${LIB_SRCS}
LIB_OBJS+=	${BUILD}/${src:R}.o
${BUILD}/${src:R}.o: ${src} ${HDRS}
	@mkdir -p ${.TARGET:H}
	${OPENCRT_CC} ${HOST_CFLAGS} -c ${src} -o ${.TARGET}
.endfor

TEST_OBJS=	${BUILD}/tests/Test.Start.o
${BUILD}/tests/Test.Start.o: tests/Test.Start.S
	@mkdir -p ${.TARGET:H}
	${OPENCRT_CC} -c tests/Test.Start.S -o ${.TARGET}
.for src in ${TEST_SRCS}
TEST_OBJS+=	${BUILD}/${src:R}.o
${BUILD}/${src:R}.o: ${src} ${HDRS}
	@mkdir -p ${.TARGET:H}
	${OPENCRT_CC} ${TEST_CFLAGS} -c ${src} -o ${.TARGET}
.endfor

${LIBRARY}: ${LIB_OBJS}
	rm -f ${.TARGET}
	${AR} rcs ${.TARGET} ${LIB_OBJS}

${TESTS}: ${TEST_OBJS} ${LIBRARY}
	${OPENCRT_CC} ${TEST_LDFLAGS} -o ${.TARGET} ${TEST_OBJS} ${LIBRARY}

test: ${TESTS}
	${TESTS}

targets:
.for target in x86_64-apple-macos11 arm64-apple-macos11
.  for src in ${LIB_SRCS}
	@mkdir -p ${BUILD}/${target}/${src:H}
	${OPENCRT_CC} -target ${target} ${LIB_CFLAGS} -fstack-protector-strong -c ${src} \
		-o ${BUILD}/${target}/${src:R}.o
.  endfor
.endfor

format:
	${CLANG_FORMAT} --style=file --dry-run --Werror ${LIB_SRCS} ${TEST_SRCS} ${HDRS}

clean:
	rm -rf ${BUILD}

.PHONY: all test targets format clean

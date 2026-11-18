/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

/*
 * The entry of the test program on Linux amd64, which has no other libc: align the stack, run the
 * tests, and leave with their status through exit_group.
 */
        .text
        .globl _start
_start:
        xorl %ebp, %ebp
        andq $-16, %rsp
        callq Test_Main
        movl %eax, %edi
        movl $231, %eax
        syscall
        hlt

        .section .note.GNU-stack, "", @progbits

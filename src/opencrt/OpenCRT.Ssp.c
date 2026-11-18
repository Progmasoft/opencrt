/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

/*
 * The stack protector of Darwin's ABI: code compiled with -fstack-protector compares a canary with
 * __stack_chk_guard on return and calls __stack_chk_fail when it changed.
 */

/**
 * The canary. The startup code puts a random value here before main runs; this one stands until
 * then and is never zero, so a zeroed stack cannot match it.
 */
long __stack_chk_guard = 0x2F8A5C31E9D4706BL;

/**
 * @brief Stops the process when a canary changed: the stack was overwritten, so nothing is safe to
 *        run, not even a message.
 */
__attribute__((__noreturn__)) void
__stack_chk_fail(void)
{
        __builtin_trap();
}

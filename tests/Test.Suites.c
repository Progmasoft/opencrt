/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include "Test.h"

extern const Test_tSuite gTest_String;
extern const Test_tSuite gTest_Other;
extern const Test_tSuite gTest_Printf;
extern const Test_tSuite gTest_Stdlib;

/** Every suite, ending with a null pointer. */
const Test_tSuite* const gTest_Suites[] = {
        &gTest_String, &gTest_Stdlib, &gTest_Printf, &gTest_Other, 0,
};

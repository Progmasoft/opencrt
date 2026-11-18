/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

/*
 * A test harness that uses nothing of OpenCRT, so that a broken function cannot make its own test
 * pass: it compares and writes with code of its own, through the write system call of Linux.
 */

/** A test case. */
typedef void (*Test_tFn_Case)(void);

/** A named test case. */
typedef struct Test_Case
{
        const char*   pName;
        Test_tFn_Case function;
} Test_tCase;

/** The test cases of one file. */
typedef struct Test_Suite
{
        const Test_tCase* pCases;
        int               count;
} Test_tSuite;

/** Defines the suite of a test file from an array of cases. */
#define TEST_SUITE(dA_Name, dA_Cases)             \
        const Test_tSuite dA_Name = { (dA_Cases), \
                                      (int) (sizeof(dA_Cases) / sizeof((dA_Cases)[ 0 ])) }

/**
 * @brief Records a failed check and reports it.
 *
 * @param[in] aIn_pFile    The file of the check.
 * @param[in] aLine        The line of the check.
 * @param[in] aIn_pMessage What failed.
 */
void
Test_Fail(const char* aIn_pFile, int aLine, const char* aIn_pMessage);

/**
 * @brief Checks that two strings are equal, and reports both when they are not.
 *
 * @param[in] aIn_pFile     The file of the check.
 * @param[in] aLine         The line of the check.
 * @param[in] aIn_pActual   The string the code gave, or NULL.
 * @param[in] aIn_pExpected The string it should give.
 */
void
Test_CheckString(const char* aIn_pFile,
                 int         aLine,
                 const char* aIn_pActual,
                 const char* aIn_pExpected);

/**
 * @brief Checks that two integers are equal, and reports both when they are not.
 *
 * @param[in] aIn_pFile  The file of the check.
 * @param[in] aLine      The line of the check.
 * @param[in] aIn_pText  The expression checked.
 * @param[in] aActual    The value the code gave.
 * @param[in] aExpected  The value it should give.
 */
void
Test_CheckInteger(const char* aIn_pFile,
                  int         aLine,
                  const char* aIn_pText,
                  long long   aActual,
                  long long   aExpected);

#define TEST_CHECK(dA_Condition)                                      \
        do                                                            \
        {                                                             \
                if(!(dA_Condition))                                   \
                        Test_Fail(__FILE__, __LINE__, #dA_Condition); \
        } while(0)

#define TEST_STRING(dA_Actual, dA_Expected) \
        Test_CheckString(__FILE__, __LINE__, (dA_Actual), (dA_Expected))

#define TEST_INTEGER(dA_Actual, dA_Expected)       \
        Test_CheckInteger(__FILE__,                \
                          __LINE__,                \
                          #dA_Actual,              \
                          (long long) (dA_Actual), \
                          (long long) (dA_Expected))

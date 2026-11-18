/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include "Test.h"

/** How many checks have failed. */
static int gsFailures;

/**
 * @brief Writes bytes to standard error through the Linux system call.
 *
 * @param[in] aIn_pBytes The bytes.
 * @param[in] aCount     How many.
 */
static void
WriteBytes(const char* aIn_pBytes, long aCount)
{
        long result;

        __asm__ volatile("syscall"
                         : "=a"(result)
                         : "0"(1L), "D"(2L), "S"(aIn_pBytes), "d"(aCount)
                         : "rcx", "r11", "memory");
        (void) result;
}

/**
 * @brief Writes a string to standard error.
 *
 * @param[in] aIn_pText The string, or NULL, written as (null).
 */
static void
WriteText(const char* aIn_pText)
{
        long length = 0;

        if(aIn_pText == 0)
                aIn_pText = "(null)";
        while(aIn_pText[ length ] != '\0')
                length++;
        WriteBytes(aIn_pText, length);
}

/**
 * @brief Writes an integer to standard error in decimal.
 *
 * @param[in] aValue The integer.
 */
static void
WriteInteger(long long aValue)
{
        char               digits[ 24 ];
        int                at        = 24;
        unsigned long long magnitude = aValue < 0 ? 0ULL - (unsigned long long) aValue
                                                  : (unsigned long long) aValue;

        do
        {
                digits[ --at ] = (char) ('0' + magnitude % 10);
                magnitude     /= 10;
        } while(magnitude != 0);
        if(aValue < 0)
                digits[ --at ] = '-';
        WriteBytes(digits + at, 24 - at);
}

/**
 * @brief Writes "file:line: " before a failure.
 *
 * @param[in] aIn_pFile The file.
 * @param[in] aLine     The line.
 */
static void
WritePlace(const char* aIn_pFile, int aLine)
{
        WriteText(aIn_pFile);
        WriteText(":");
        WriteInteger(aLine);
        WriteText(": ");
}

/**
 * @brief Records a failed check and reports it.
 *
 * @param[in] aIn_pFile    The file of the check.
 * @param[in] aLine        The line of the check.
 * @param[in] aIn_pMessage What failed.
 */
void
Test_Fail(const char* aIn_pFile, int aLine, const char* aIn_pMessage)
{
        gsFailures++;
        WritePlace(aIn_pFile, aLine);
        WriteText(aIn_pMessage);
        WriteText("\n");
}

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
                 const char* aIn_pExpected)
{
        if(aIn_pActual != 0)
        {
                long index = 0;

                while(aIn_pActual[ index ] == aIn_pExpected[ index ]
                      && aIn_pExpected[ index ] != '\0')
                        index++;
                if(aIn_pActual[ index ] == aIn_pExpected[ index ])
                        return;
        }
        gsFailures++;
        WritePlace(aIn_pFile, aLine);
        WriteText("got \"");
        WriteText(aIn_pActual);
        WriteText("\", expected \"");
        WriteText(aIn_pExpected);
        WriteText("\"\n");
}

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
                  long long   aExpected)
{
        if(aActual == aExpected)
                return;
        gsFailures++;
        WritePlace(aIn_pFile, aLine);
        WriteText(aIn_pText);
        WriteText(" is ");
        WriteInteger(aActual);
        WriteText(", expected ");
        WriteInteger(aExpected);
        WriteText("\n");
}

extern const Test_tSuite* const gTest_Suites[];

/**
 * @brief Runs every test case and reports how many checks failed.
 *
 * @return 0 when every check passed, 1 otherwise.
 */
int
Test_Main(void)
{
        int cases = 0;

        for(int suite = 0; gTest_Suites[ suite ] != 0; suite++)
        {
                for(int index = 0; index < gTest_Suites[ suite ]->count; index++)
                {
                        gTest_Suites[ suite ]->pCases[ index ].function();
                        cases++;
                }
        }
        WriteInteger(cases);
        WriteText(" test cases, ");
        WriteInteger(gsFailures);
        WriteText(" failed checks\n");
        return (gsFailures == 0 ? 0 : 1);
}

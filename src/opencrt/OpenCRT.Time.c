/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * The time functions that need no clock and no time zone data: UTC only, on the proleptic Gregorian
 * calendar, with dates computed by the days-from-civil method, so any date takes the same time.
 */

#define OPENCRT_TIME_SECONDS_PER_DAY 86400L

static const char* const kWeekdays[] = { "Sunday",   "Monday", "Tuesday", "Wednesday",
                                         "Thursday", "Friday", "Saturday" };
static const char* const kMonths[]   = { "January",   "February", "March",    "April",
                                         "May",       "June",     "July",     "August",
                                         "September", "October",  "November", "December" };

/**
 * @brief The days from 1970-01-01 to a date.
 *
 * @param[in] aYear  The year.
 * @param[in] aMonth The month, 1 to 12.
 * @param[in] aDay   The day of the month, from 1.
 * @return The days, negative before 1970.
 */
static long long
DaysFromCivil(long long aYear, int aMonth, int aDay)
{
        aYear              -= aMonth <= 2;
        long long era       = (aYear >= 0 ? aYear : aYear - 399) / 400;
        long long yearOfEra = aYear - era * 400;
        long long dayOfYear = (153 * (aMonth + (aMonth > 2 ? -3 : 9)) + 2) / 5 + aDay - 1;
        long long dayOfEra  = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;

        return (era * 146097 + dayOfEra - 719468);
}

/**
 * @brief The date of a number of days from 1970-01-01.
 *
 * @param[in]  aDays      The days.
 * @param[out] aOut_pYear  The year.
 * @param[out] aOut_pMonth The month, 1 to 12.
 * @param[out] aOut_pDay   The day of the month, from 1.
 */
static void
CivilFromDays(long long aDays, long long* aOut_pYear, int* aOut_pMonth, int* aOut_pDay)
{
        aDays              += 719468;
        long long era       = (aDays >= 0 ? aDays : aDays - 146096) / 146097;
        long long dayOfEra  = aDays - era * 146097;
        long long yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096)
                / 365;
        long long dayOfYear  = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
        long long monthIndex = (5 * dayOfYear + 2) / 153;

        *aOut_pDay   = (int) (dayOfYear - (153 * monthIndex + 2) / 5 + 1);
        *aOut_pMonth = (int) (monthIndex < 10 ? monthIndex + 3 : monthIndex - 9);
        *aOut_pYear  = yearOfEra + era * 400 + (*aOut_pMonth <= 2);
}

/**
 * @brief Whether a year is a leap year.
 *
 * @param[in] aYear The year.
 * @return true when it is.
 */
static bool
IsLeap(long long aYear)
{
        return (aYear % 4 == 0 && (aYear % 100 != 0 || aYear % 400 == 0));
}

/**
 * @brief Splits a time into its UTC date and time.
 *
 * @param[in]  aIn_pTime The seconds since 1970-01-01 00:00:00 UTC.
 * @param[out] aOut_pTm  The fields.
 * @return aOut_pTm, or NULL with EOVERFLOW when the year does not fit an int.
 */
struct tm*
gmtime_r(const time_t* restrict aIn_pTime, struct tm* restrict aOut_pTm)
{
        long long seconds = *aIn_pTime;
        long long days    = seconds / OPENCRT_TIME_SECONDS_PER_DAY;
        long long rest    = seconds % OPENCRT_TIME_SECONDS_PER_DAY;
        long long year;
        int       month;
        int       day;

        if(rest < 0)
        {
                rest += OPENCRT_TIME_SECONDS_PER_DAY;
                days--;
        }
        CivilFromDays(days, &year, &month, &day);
        if(year - 1900 > INT_MAX || year - 1900 < INT_MIN)
        {
                errno = EOVERFLOW;
                return (NULL);
        }
        aOut_pTm->tm_year   = (int) (year - 1900);
        aOut_pTm->tm_mon    = month - 1;
        aOut_pTm->tm_mday   = day;
        aOut_pTm->tm_hour   = (int) (rest / 3600);
        aOut_pTm->tm_min    = (int) (rest / 60 % 60);
        aOut_pTm->tm_sec    = (int) (rest % 60);
        aOut_pTm->tm_wday   = (int) ((days % 7 + 11) % 7);
        aOut_pTm->tm_yday   = (int) (days - DaysFromCivil(year, 1, 1));
        aOut_pTm->tm_isdst  = 0;
        aOut_pTm->tm_gmtoff = 0;
        aOut_pTm->tm_zone   = "UTC";
        return (aOut_pTm);
}

/**
 * @brief Splits a time into its UTC date and time, in storage the next call reuses.
 *
 * @param[in] aIn_pTime The seconds since 1970-01-01 00:00:00 UTC.
 * @return The fields, or NULL with EOVERFLOW.
 */
struct tm*
gmtime(const time_t* aIn_pTime)
{
        static struct tm sTm;

        return (gmtime_r(aIn_pTime, &sTm));
}

/**
 * @brief The time of a UTC date and time; fields out of range carry into the next, as mktime does.
 *
 * @param[in,out] aInOut_pTm The fields; normalized, with tm_wday and tm_yday set.
 * @return The seconds since 1970-01-01 00:00:00 UTC, or (time_t) -1 with EOVERFLOW.
 */
time_t
timegm(struct tm* aInOut_pTm)
{
        long long month = aInOut_pTm->tm_mon;
        long long year  = (long long) aInOut_pTm->tm_year + 1900 + month / 12;

        month %= 12;
        if(month < 0)
        {
                month += 12;
                year--;
        }
        long long days    = DaysFromCivil(year, (int) month + 1, 1) + aInOut_pTm->tm_mday - 1;
        long long seconds = days * OPENCRT_TIME_SECONDS_PER_DAY
                + (long long) aInOut_pTm->tm_hour * 3600 + (long long) aInOut_pTm->tm_min * 60
                + aInOut_pTm->tm_sec;
        time_t result = (time_t) seconds;

        if(gmtime_r(&result, aInOut_pTm) == NULL)
                return ((time_t) -1);
        return (result);
}

/**
 * @brief The seconds from one time to another.
 *
 * @param[in] aEnd   The later time.
 * @param[in] aStart The earlier time.
 * @return aEnd - aStart, computed without integer overflow.
 */
double
difftime(time_t aEnd, time_t aStart)
{
        return ((double) aEnd - (double) aStart);
}

/**
 * @brief Writes a date as "Sun Sep 16 01:03:52 1973\\n".
 *
 * @param[in]  aIn_pTm     The fields.
 * @param[out] aOut_pBuffer At least 26 bytes.
 * @return The buffer, or NULL with EOVERFLOW for fields that do not fit in 26 bytes.
 */
char*
asctime_r(const struct tm* restrict aIn_pTm, char* restrict aOut_pBuffer)
{
        if(aIn_pTm->tm_wday < 0 || aIn_pTm->tm_wday > 6 || aIn_pTm->tm_mon < 0
           || aIn_pTm->tm_mon > 11)
        {
                errno = EOVERFLOW;
                return (NULL);
        }
        int length = snprintf(aOut_pBuffer,
                              26,
                              "%.3s %.3s%3d %.2d:%.2d:%.2d %ld\n",
                              kWeekdays[ aIn_pTm->tm_wday ],
                              kMonths[ aIn_pTm->tm_mon ],
                              aIn_pTm->tm_mday,
                              aIn_pTm->tm_hour,
                              aIn_pTm->tm_min,
                              aIn_pTm->tm_sec,
                              (long) aIn_pTm->tm_year + 1900);

        if(length < 0 || length >= 26)
        {
                errno = EOVERFLOW;
                return (NULL);
        }
        return (aOut_pBuffer);
}

/**
 * @brief Writes a date as asctime_r does, in storage the next call reuses.
 *
 * @param[in] aIn_pTm The fields.
 * @return The text, or NULL with EOVERFLOW.
 */
char*
asctime(const struct tm* aIn_pTm)
{
        static char sBuffer[ 26 ];

        return (asctime_r(aIn_pTm, sBuffer));
}

/** The output of strftime while it is written: what fits, and whether something did not. */
typedef struct Output
{
        char*  pBuffer;
        size_t size;
        size_t used;
        bool   isFull;
} tOutput;

/**
 * @brief Appends text to the output, noting when it does not fit.
 *
 * @param[in,out] aInOut_pOutput The output.
 * @param[in]     aIn_pText      The text.
 */
static void
Append(tOutput* aInOut_pOutput, const char* aIn_pText)
{
        size_t length = strlen(aIn_pText);

        if(aInOut_pOutput->used + length >= aInOut_pOutput->size)
        {
                aInOut_pOutput->isFull = true;
                return;
        }
        memcpy(aInOut_pOutput->pBuffer + aInOut_pOutput->used, aIn_pText, length);
        aInOut_pOutput->used += length;
}

/**
 * @brief The ISO 8601 week-based year and week of a date.
 *
 * @param[in]  aIn_pTm    The fields.
 * @param[out] aOut_pWeek The week, 1 to 53.
 * @return The year.
 */
static long long
IsoWeek(const struct tm* aIn_pTm, int* aOut_pWeek)
{
        long long year    = (long long) aIn_pTm->tm_year + 1900;
        int       weekday = (aIn_pTm->tm_wday + 6) % 7;
        int       week    = (aIn_pTm->tm_yday - weekday + 10) / 7;

        if(week < 1)
        {
                /* The last week of the year before: 52, or 53 when that year has one. */
                year--;
                int previousEnd = aIn_pTm->tm_yday + 365 + IsLeap(year);
                week            = (previousEnd - weekday + 10) / 7;
        } else if(week == 53)
        {
                int daysInYear = 365 + IsLeap(year);

                if(aIn_pTm->tm_yday - weekday + 3 >= daysInYear)
                {
                        week = 1;
                        year++;
                }
        }
        *aOut_pWeek = week;
        return (year);
}

/**
 * @brief Writes one conversion of strftime.
 *
 * @param[in,out] aInOut_pOutput The output.
 * @param[in]     aConversion    The character after %.
 * @param[in]     aIn_pTm        The fields.
 * @return false for a conversion strftime does not have.
 */
static bool
Convert(tOutput* aInOut_pOutput, char aConversion, const struct tm* aIn_pTm);

/**
 * @brief Writes a format of strftime conversions, for those defined by others such as %c.
 *
 * @param[in,out] aInOut_pOutput The output.
 * @param[in]     aIn_pFormat    The format.
 * @param[in]     aIn_pTm        The fields.
 */
static void
Expand(tOutput* aInOut_pOutput, const char* aIn_pFormat, const struct tm* aIn_pTm)
{
        char literal[ 2 ] = { 0, 0 };

        for(; *aIn_pFormat != '\0'; aIn_pFormat++)
        {
                if(*aIn_pFormat == '%' && aIn_pFormat[ 1 ] != '\0')
                        Convert(aInOut_pOutput, *++aIn_pFormat, aIn_pTm);
                else
                {
                        literal[ 0 ] = *aIn_pFormat;
                        Append(aInOut_pOutput, literal);
                }
        }
}

/**
 * @brief Writes a number with at least some digits, zeros or spaces before it.
 *
 * @param[in,out] aInOut_pOutput The output.
 * @param[in]     aValue         The number.
 * @param[in]     aDigits        The fewest digits.
 * @param[in]     aPad           '0' or ' '.
 */
static void
Number(tOutput* aInOut_pOutput, long long aValue, int aDigits, char aPad)
{
        char text[ 32 ];

        snprintf(text, sizeof(text), aPad == '0' ? "%0*lld" : "%*lld", aDigits, aValue);
        Append(aInOut_pOutput, text);
}

/**
 * @brief The name in a table for an index, or "?" for an index out of it.
 *
 * @param[in] aIn_ppNames The names.
 * @param[in] aCount      How many.
 * @param[in] aIndex      The index.
 * @return The name.
 */
static const char*
Name(const char* const* aIn_ppNames, int aCount, int aIndex)
{
        return (aIndex >= 0 && aIndex < aCount ? aIn_ppNames[ aIndex ] : "?");
}

static bool
Convert(tOutput* aInOut_pOutput, char aConversion, const struct tm* aIn_pTm)
{
        char      text[ 8 ];
        int       week;
        long long year = (long long) aIn_pTm->tm_year + 1900;

        switch(aConversion)
        {
        case 'a':
        case 'A':
        case 'b':
        case 'B':
        case 'h':
        {
                bool        isDay = aConversion == 'a' || aConversion == 'A';
                const char* pName = isDay ? Name(kWeekdays, 7, aIn_pTm->tm_wday)
                                          : Name(kMonths, 12, aIn_pTm->tm_mon);

                snprintf(text, sizeof(text), "%.3s", pName);
                Append(aInOut_pOutput, aConversion == 'A' || aConversion == 'B' ? pName : text);
                return (true);
        }
        case 'c':
                Expand(aInOut_pOutput, "%a %b %e %H:%M:%S %Y", aIn_pTm);
                return (true);
        case 'C':
                Number(aInOut_pOutput, year >= 0 ? year / 100 : -((99 - year) / 100), 2, '0');
                return (true);
        case 'd':
                Number(aInOut_pOutput, aIn_pTm->tm_mday, 2, '0');
                return (true);
        case 'D':
                Expand(aInOut_pOutput, "%m/%d/%y", aIn_pTm);
                return (true);
        case 'e':
                Number(aInOut_pOutput, aIn_pTm->tm_mday, 2, ' ');
                return (true);
        case 'F':
                Expand(aInOut_pOutput, "%Y-%m-%d", aIn_pTm);
                return (true);
        case 'g':
                Number(aInOut_pOutput, ((IsoWeek(aIn_pTm, &week) % 100) + 100) % 100, 2, '0');
                return (true);
        case 'G':
                Number(aInOut_pOutput, IsoWeek(aIn_pTm, &week), 1, '0');
                return (true);
        case 'H':
                Number(aInOut_pOutput, aIn_pTm->tm_hour, 2, '0');
                return (true);
        case 'I':
                Number(aInOut_pOutput, (aIn_pTm->tm_hour + 11) % 12 + 1, 2, '0');
                return (true);
        case 'j':
                Number(aInOut_pOutput, aIn_pTm->tm_yday + 1, 3, '0');
                return (true);
        case 'm':
                Number(aInOut_pOutput, aIn_pTm->tm_mon + 1, 2, '0');
                return (true);
        case 'M':
                Number(aInOut_pOutput, aIn_pTm->tm_min, 2, '0');
                return (true);
        case 'n':
                Append(aInOut_pOutput, "\n");
                return (true);
        case 'p':
                Append(aInOut_pOutput, aIn_pTm->tm_hour < 12 ? "AM" : "PM");
                return (true);
        case 'r':
                Expand(aInOut_pOutput, "%I:%M:%S %p", aIn_pTm);
                return (true);
        case 'R':
                Expand(aInOut_pOutput, "%H:%M", aIn_pTm);
                return (true);
        case 'S':
                Number(aInOut_pOutput, aIn_pTm->tm_sec, 2, '0');
                return (true);
        case 't':
                Append(aInOut_pOutput, "\t");
                return (true);
        case 'T':
                Expand(aInOut_pOutput, "%H:%M:%S", aIn_pTm);
                return (true);
        case 'u':
                Number(aInOut_pOutput, aIn_pTm->tm_wday == 0 ? 7 : aIn_pTm->tm_wday, 1, '0');
                return (true);
        case 'U':
                Number(aInOut_pOutput, (aIn_pTm->tm_yday + 7 - aIn_pTm->tm_wday) / 7, 2, '0');
                return (true);
        case 'V':
                IsoWeek(aIn_pTm, &week);
                Number(aInOut_pOutput, week, 2, '0');
                return (true);
        case 'w':
                Number(aInOut_pOutput, aIn_pTm->tm_wday, 1, '0');
                return (true);
        case 'W':
                Number(aInOut_pOutput,
                       (aIn_pTm->tm_yday + 7 - (aIn_pTm->tm_wday + 6) % 7) / 7,
                       2,
                       '0');
                return (true);
        case 'x':
                Expand(aInOut_pOutput, "%m/%d/%y", aIn_pTm);
                return (true);
        case 'X':
                Expand(aInOut_pOutput, "%H:%M:%S", aIn_pTm);
                return (true);
        case 'y':
                Number(aInOut_pOutput, ((year % 100) + 100) % 100, 2, '0');
                return (true);
        case 'Y':
                Number(aInOut_pOutput, year, 1, '0');
                return (true);
        case 'z':
        {
                long offset = aIn_pTm->tm_gmtoff;

                snprintf(text,
                         sizeof(text),
                         "%c%02ld%02ld",
                         offset < 0 ? '-' : '+',
                         labs(offset) / 3600 % 100,
                         labs(offset) / 60 % 60);
                Append(aInOut_pOutput, text);
                return (true);
        }
        case 'Z':
                Append(aInOut_pOutput, aIn_pTm->tm_zone != NULL ? aIn_pTm->tm_zone : "UTC");
                return (true);
        case '%':
                Append(aInOut_pOutput, "%");
                return (true);
        default:
                return (false);
        }
}

/**
 * @brief Writes a date by a format, in the C locale.
 *
 * Every conversion of C99 and POSIX is written; E and O modifiers are taken and ignored, as the C
 * locale has no alternative forms. An unknown conversion is written as it stands.
 *
 * @param[out] aOut_pBuffer The buffer.
 * @param[in]  aSize        Its size.
 * @param[in]  aIn_pFormat  The format.
 * @param[in]  aIn_pTm      The fields.
 * @return The length written, the null character not counted, or 0 when it did not fit.
 */
size_t
strftime(char* restrict aOut_pBuffer,
         size_t aSize,
         const char* restrict aIn_pFormat,
         const struct tm* restrict aIn_pTm)
{
        tOutput output = { aOut_pBuffer, aSize, 0, false };

        for(const char* pAt = aIn_pFormat; *pAt != '\0' && !output.isFull; pAt++)
        {
                if(*pAt != '%' || pAt[ 1 ] == '\0')
                {
                        char literal[ 2 ] = { *pAt, '\0' };

                        Append(&output, literal);
                        continue;
                }
                const char* pStart = pAt;

                pAt++;
                if(*pAt == 'E' || *pAt == 'O')
                        pAt++;
                if(*pAt == '\0' || !Convert(&output, *pAt, aIn_pTm))
                {
                        char unknown[ 4 ] = { 0 };

                        memcpy(unknown, pStart, (size_t) (pAt - pStart) + (*pAt != '\0'));
                        Append(&output, unknown);
                        if(*pAt == '\0')
                                break;
                }
        }
        if(output.isFull || aSize == 0)
                return (0);
        aOut_pBuffer[ output.used ] = '\0';
        return (output.used);
}

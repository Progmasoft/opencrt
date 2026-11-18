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

/* The fields of the numeric conversions. */

/** @brief The century. @param[in] aIn_pTm The fields. @return The value. */
static long long
Century(const struct tm* aIn_pTm)
{
        long long year = (long long) aIn_pTm->tm_year + 1900;

        return (year >= 0 ? year / 100 : -((99 - year) / 100));
}

/** @brief The day of the month. @param[in] aIn_pTm The fields. @return The value. */
static long long
Day(const struct tm* aIn_pTm)
{
        return (aIn_pTm->tm_mday);
}

/** @brief The ISO 8601 week-based year, in two digits. @param[in] aIn_pTm The fields. @return The value. */
static long long
IsoYearShort(const struct tm* aIn_pTm)
{
        int week;

        return (((IsoWeek(aIn_pTm, &week) % 100) + 100) % 100);
}

/** @brief The ISO 8601 week-based year. @param[in] aIn_pTm The fields. @return The value. */
static long long
IsoYear(const struct tm* aIn_pTm)
{
        int week;

        return (IsoWeek(aIn_pTm, &week));
}

/** @brief The ISO 8601 week. @param[in] aIn_pTm The fields. @return The value. */
static long long
IsoWeekNumber(const struct tm* aIn_pTm)
{
        int week;

        IsoWeek(aIn_pTm, &week);
        return (week);
}

/** @brief The hour, 0 to 23. @param[in] aIn_pTm The fields. @return The value. */
static long long
Hour(const struct tm* aIn_pTm)
{
        return (aIn_pTm->tm_hour);
}

/** @brief The hour, 1 to 12. @param[in] aIn_pTm The fields. @return The value. */
static long long
Hour12(const struct tm* aIn_pTm)
{
        return ((aIn_pTm->tm_hour + 11) % 12 + 1);
}

/** @brief The day of the year, from 1. @param[in] aIn_pTm The fields. @return The value. */
static long long
YearDay(const struct tm* aIn_pTm)
{
        return (aIn_pTm->tm_yday + 1);
}

/** @brief The month, from 1. @param[in] aIn_pTm The fields. @return The value. */
static long long
Month(const struct tm* aIn_pTm)
{
        return (aIn_pTm->tm_mon + 1);
}

/** @brief The minute. @param[in] aIn_pTm The fields. @return The value. */
static long long
Minute(const struct tm* aIn_pTm)
{
        return (aIn_pTm->tm_min);
}

/** @brief The second. @param[in] aIn_pTm The fields. @return The value. */
static long long
Second(const struct tm* aIn_pTm)
{
        return (aIn_pTm->tm_sec);
}

/** @brief The weekday, Monday 1 to Sunday 7. @param[in] aIn_pTm The fields. @return The value. */
static long long
IsoWeekday(const struct tm* aIn_pTm)
{
        return (aIn_pTm->tm_wday == 0 ? 7 : aIn_pTm->tm_wday);
}

/** @brief The week of the year, weeks from Sunday. @param[in] aIn_pTm The fields. @return The value. */
static long long
SundayWeek(const struct tm* aIn_pTm)
{
        return ((aIn_pTm->tm_yday + 7 - aIn_pTm->tm_wday) / 7);
}

/** @brief The weekday, Sunday 0 to Saturday 6. @param[in] aIn_pTm The fields. @return The value. */
static long long
Weekday(const struct tm* aIn_pTm)
{
        return (aIn_pTm->tm_wday);
}

/** @brief The week of the year, weeks from Monday. @param[in] aIn_pTm The fields. @return The value. */
static long long
MondayWeek(const struct tm* aIn_pTm)
{
        return ((aIn_pTm->tm_yday + 7 - (aIn_pTm->tm_wday + 6) % 7) / 7);
}

/** @brief The year in two digits. @param[in] aIn_pTm The fields. @return The value. */
static long long
YearShort(const struct tm* aIn_pTm)
{
        return (((((long long) aIn_pTm->tm_year + 1900) % 100) + 100) % 100);
}

/** @brief The year. @param[in] aIn_pTm The fields. @return The value. */
static long long
Year(const struct tm* aIn_pTm)
{
        return ((long long) aIn_pTm->tm_year + 1900);
}

/** A numeric conversion: its letter, its field, its fewest digits and what pads them. */
typedef struct Numeric
{
        char conversion;
        long long (*fn_Field)(const struct tm*);
        int  digits;
        char pad;
} tNumeric;

static const tNumeric kNumerics[] = {
        { 'C', Century, 2, '0' },      { 'd', Day, 2, '0' },           { 'e', Day, 2, ' ' },
        { 'g', IsoYearShort, 2, '0' }, { 'G', IsoYear, 1, '0' },       { 'H', Hour, 2, '0' },
        { 'I', Hour12, 2, '0' },       { 'j', YearDay, 3, '0' },       { 'm', Month, 2, '0' },
        { 'M', Minute, 2, '0' },       { 'S', Second, 2, '0' },        { 'u', IsoWeekday, 1, '0' },
        { 'U', SundayWeek, 2, '0' },   { 'V', IsoWeekNumber, 2, '0' }, { 'w', Weekday, 1, '0' },
        { 'W', MondayWeek, 2, '0' },   { 'y', YearShort, 2, '0' },     { 'Y', Year, 1, '0' },
};

/** A conversion that stands for a format of other conversions, or for fixed text. */
typedef struct Expansion
{
        char        conversion;
        const char* pFormat;
} tExpansion;

static const tExpansion kExpansions[] = {
        { 'c', "%a %b %e %H:%M:%S %Y" },
        { 'D', "%m/%d/%y" },
        { 'F', "%Y-%m-%d" },
        { 'r', "%I:%M:%S %p" },
        { 'R', "%H:%M" },
        { 'T', "%H:%M:%S" },
        { 'x', "%m/%d/%y" },
        { 'X', "%H:%M:%S" },
        { 'n', "\n" },
        { 't', "\t" },
};

/**
 * @brief Writes a conversion of names or zones: a, A, b, B, h, p, z or Z.
 *
 * @param[in,out] aInOut_pOutput The output.
 * @param[in]     aConversion    The character after %.
 * @param[in]     aIn_pTm        The fields.
 * @return false for another conversion.
 */
static bool
ConvertText(tOutput* aInOut_pOutput, char aConversion, const struct tm* aIn_pTm)
{
        char text[ 16 ];

        if(strchr("aA", aConversion) != NULL || strchr("bBh", aConversion) != NULL)
        {
                bool        isDay = aConversion == 'a' || aConversion == 'A';
                const char* pName = isDay ? Name(kWeekdays, 7, aIn_pTm->tm_wday)
                                          : Name(kMonths, 12, aIn_pTm->tm_mon);

                snprintf(text, sizeof(text), "%.3s", pName);
                Append(aInOut_pOutput, aConversion == 'A' || aConversion == 'B' ? pName : text);
                return (true);
        }
        if(aConversion == '%')
                Append(aInOut_pOutput, "%");
        else if(aConversion == 'p')
                Append(aInOut_pOutput, aIn_pTm->tm_hour < 12 ? "AM" : "PM");
        else if(aConversion == 'Z')
                Append(aInOut_pOutput, aIn_pTm->tm_zone != NULL ? aIn_pTm->tm_zone : "UTC");
        else if(aConversion == 'z')
        {
                long offset = labs(aIn_pTm->tm_gmtoff);

                snprintf(text,
                         sizeof(text),
                         "%c%02ld%02ld",
                         aIn_pTm->tm_gmtoff < 0 ? '-' : '+',
                         offset / 3600 % 100,
                         offset / 60 % 60);
                Append(aInOut_pOutput, text);
        } else
                return (false);
        return (true);
}

static bool
Convert(tOutput* aInOut_pOutput, char aConversion, const struct tm* aIn_pTm)
{
        if(aConversion == '\0')
                return (false);
        for(size_t index = 0; index < sizeof(kNumerics) / sizeof(kNumerics[ 0 ]); index++)
        {
                const tNumeric* pNumeric = &kNumerics[ index ];

                if(pNumeric->conversion == aConversion)
                {
                        Number(aInOut_pOutput,
                               pNumeric->fn_Field(aIn_pTm),
                               pNumeric->digits,
                               pNumeric->pad);
                        return (true);
                }
        }
        for(size_t index = 0; index < sizeof(kExpansions) / sizeof(kExpansions[ 0 ]); index++)
        {
                if(kExpansions[ index ].conversion == aConversion)
                {
                        Expand(aInOut_pOutput, kExpansions[ index ].pFormat, aIn_pTm);
                        return (true);
                }
        }
        return (ConvertText(aInOut_pOutput, aConversion, aIn_pTm));
}

/**
 * @brief Writes one conversion of a strftime format, or its text when strftime does not have it.
 *
 * @param[in,out] aInOut_pOutput The output.
 * @param[in]     aIn_pAt        The % that starts it.
 * @param[in]     aIn_pTm        The fields.
 * @return Where the conversion ends, the last character it used.
 */
static const char*
WriteConversion(tOutput* aInOut_pOutput, const char* aIn_pAt, const struct tm* aIn_pTm)
{
        const char* pAt = aIn_pAt + 1;

        /* E and O ask for alternative forms, which the C locale does not have. */
        if(*pAt == 'E' || *pAt == 'O')
                pAt++;
        if(Convert(aInOut_pOutput, *pAt, aIn_pTm))
                return (pAt);
        char unknown[ 4 ] = { 0 };

        memcpy(unknown, aIn_pAt, (size_t) (pAt - aIn_pAt) + (*pAt != '\0'));
        Append(aInOut_pOutput, unknown);
        return (*pAt != '\0' ? pAt : pAt - 1);
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
                if(*pAt == '%' && pAt[ 1 ] != '\0')
                {
                        pAt = WriteConversion(&output, pAt, aIn_pTm);
                        continue;
                }
                char literal[ 2 ] = { *pAt, '\0' };

                Append(&output, literal);
        }
        if(output.isFull || aSize == 0)
                return (0);
        aOut_pBuffer[ output.used ] = '\0';
        return (output.used);
}

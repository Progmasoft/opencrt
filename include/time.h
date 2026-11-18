/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <sys/_types/_clock_t.h>
#include <sys/_types/_null.h>
#include <sys/_types/_size_t.h>
#include <sys/_types/_time_t.h>
#include <sys/cdefs.h>

struct tm
{
        int         tm_sec;
        int         tm_min;
        int         tm_hour;
        int         tm_mday;
        int         tm_mon;
        int         tm_year;
        int         tm_wday;
        int         tm_yday;
        int         tm_isdst;
        long        tm_gmtoff;
        const char* tm_zone;
};

__BEGIN_DECLS
double
difftime(time_t, time_t) __pure2;
struct tm*
gmtime(const time_t*);
struct tm*
gmtime_r(const time_t* __restrict_arg, struct tm* __restrict_arg);
time_t
timegm(struct tm*);
char*
asctime(const struct tm*);
char*
asctime_r(const struct tm* __restrict_arg, char* __restrict_arg);
size_t
strftime(char* __restrict_arg, size_t, const char* __restrict_arg, const struct tm* __restrict_arg)
        __strftimelike(3);
__END_DECLS

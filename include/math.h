/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <sys/cdefs.h>

#define HUGE_VAL  __builtin_huge_val()
#define HUGE_VALF __builtin_huge_valf()
#define HUGE_VALL __builtin_huge_vall()
#define INFINITY  __builtin_inff()
#define NAN       __builtin_nanf("")

/* Darwin's ABI: the functions report through the floating-point exceptions, not errno. */
#define MATH_ERRNO       1
#define MATH_ERREXCEPT   2
#define math_errhandling MATH_ERREXCEPT

#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_NORMAL    3
#define FP_SUBNORMAL 4

#define fpclassify(x) \
        __builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, (x))
#define isfinite(x) __builtin_isfinite(x)
#define isinf(x)    __builtin_isinf_sign(x)
#define isnan(x)    __builtin_isnan(x)
#define isnormal(x) __builtin_isnormal(x)
#define signbit(x)  __builtin_signbit(x)

#define isgreater(x, y)      __builtin_isgreater(x, y)
#define isgreaterequal(x, y) __builtin_isgreaterequal(x, y)
#define isless(x, y)         __builtin_isless(x, y)
#define islessequal(x, y)    __builtin_islessequal(x, y)
#define islessgreater(x, y)  __builtin_islessgreater(x, y)
#define isunordered(x, y)    __builtin_isunordered(x, y)

__BEGIN_DECLS
double
fabs(double) __pure2;
float
fabsf(float) __pure2;
long double
fabsl(long double) __pure2;
double
copysign(double, double) __pure2;
float
copysignf(float, float) __pure2;
long double
copysignl(long double, long double) __pure2;
double
trunc(double) __pure2;
float
truncf(float) __pure2;
long double
truncl(long double) __pure2;
double
floor(double) __pure2;
float
floorf(float) __pure2;
long double
floorl(long double) __pure2;
double
ceil(double) __pure2;
float
ceilf(float) __pure2;
long double
ceill(long double) __pure2;
double
round(double) __pure2;
float
roundf(float) __pure2;
long double
roundl(long double) __pure2;
long
lround(double) __pure2;
long long
llround(double) __pure2;
double
fmin(double, double) __pure2;
double
fmax(double, double) __pure2;
double
fdim(double, double) __pure2;
double
sqrt(double);
float
sqrtf(float);
double
frexp(double, int*);
double
ldexp(double, int);
double
scalbn(double, int);
double
modf(double, double*);
double
fmod(double, double);
double
nan(const char*);
__END_DECLS

/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>

#include "opencrt/OpenCRT.Format.h"

/*
 * Reading printf formats: conversion specifications and their arguments, in order or by position
 * (%2$s) as POSIX has it. A format uses one way only, and every position from 1 to the highest must be
 * used, so that each argument's type is known before it is read.
 */

/** The most positions a format may use: NL_ARGMAX. */
#define OPENCRT_FORMATSPEC_POSITIONS NL_ARGMAX

/** The type an argument is read as. */
typedef enum Kind
{
        kNone,
        kInt,
        kLong,
        kLongLong,
        kIntMax,
        kSize,
        kPtrdiff,
        kDouble,
        kLongDouble,
        kPointer,
        kWideChar,
} tKind;

/** The length modifier of a conversion. */
typedef enum Length
{
        kDefault,
        kChar,
        kShort,
        kLongLength,
        kLongLongLength,
        kIntMaxLength,
        kSizeLength,
        kPtrdiffLength,
        kLongDoubleLength,
} tLength;

/** A value read from the arguments. */
typedef union Value
{
        long long   integer;
        long double floating;
        const void* pPointer;
} tValue;

/** A conversion specification as written: its width and precision may still be arguments. */
typedef struct Raw
{
        __OpenCRT_Format_tSpec spec;
        tLength                length;
        /** The position of the value, or 0 for the next argument. */
        int position;
        /** Whether the width is an argument, and its position or 0. */
        bool hasWidthArgument;
        int  widthPosition;
        /** Whether the precision is an argument, and its position or 0. */
        bool hasPrecisionArgument;
        int  precisionPosition;
} tRaw;

/** The arguments of a format, read in order or by position. */
typedef struct Arguments
{
        va_list list;
        bool    isPositional;
        tValue  values[ OPENCRT_FORMATSPEC_POSITIONS + 1 ];
} tArguments;

/**
 * @brief The type the value of a conversion is read as.
 *
 * @param[in] aIn_pRaw The specification.
 * @return The type, or kNone for a conversion and modifier that do not go together.
 */
tKind
__OpenCRT_FormatSpec_KindOf(const tRaw* aIn_pRaw);

/**
 * @brief Reads one conversion specification, after its %.
 *
 * @param[in,out] aInOut_ppAt Where it starts; moved past it.
 * @param[out]    aOut_pRaw   The specification.
 * @return false for one OpenCRT refuses: an unknown conversion, a bad position or modifier.
 */
bool
__OpenCRT_FormatSpec_Read(const char** aInOut_ppAt, tRaw* aOut_pRaw);

/**
 * @brief Checks every specification of a format and, for positions, reads every argument first.
 *
 * @param[in]     aIn_pFormat       The format.
 * @param[in,out] aInOut_pArguments The arguments, with their list started.
 * @return false for a format OpenCRT refuses.
 */
bool
__OpenCRT_FormatSpec_Prepare(const char* aIn_pFormat, tArguments* aInOut_pArguments);

/**
 * @brief The value of an argument: the next one, or the one at a position.
 *
 * @param[in,out] aInOut_pArguments The arguments.
 * @param[in]     aPosition         The position, or 0.
 * @param[in]     aKind             The type.
 * @return The value.
 */
tValue
__OpenCRT_FormatSpec_Argument(tArguments* aInOut_pArguments, int aPosition, tKind aKind);

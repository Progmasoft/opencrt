/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * The engine of the printf family, inside OpenCRT. Output goes to a sink, so that snprintf, and later
 * the stream functions, share one implementation.
 */

/** Where formatted output goes. */
typedef struct __OpenCRT_Format_Sink
{
        /** Writes bytes; the sink keeps what fits and the engine counts the rest. */
        void (*fn_Put)(struct __OpenCRT_Format_Sink* aInOut_pSink,
                       const char*                   aIn_pBytes,
                       size_t                        aCount);
        /** What fn_Put writes into, for the sink's own use. */
        void* pContext;
        /** How many bytes the output has, written or not. */
        size_t length;
} __OpenCRT_Format_tSink;

/** A conversion specification, once its arguments are read. */
typedef struct __OpenCRT_Format_Spec
{
        bool isLeft;
        bool isPlus;
        bool isSpace;
        bool isAlternate;
        bool isZero;
        /** The width, or 0. */
        size_t width;
        /** The precision, or -1 when it is left out. */
        long precision;
        /** The conversion character: d, i, u, o, x, X, f, F, e, E, g, G, a, A, c, s, p. */
        char conversion;
} __OpenCRT_Format_tSpec;

/**
 * @brief Writes bytes to the sink and counts them.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pBytes   The bytes.
 * @param[in]     aCount       How many.
 */
void
__OpenCRT_Format_Put(__OpenCRT_Format_tSink* aInOut_pSink, const char* aIn_pBytes, size_t aCount);

/**
 * @brief Writes a byte several times.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aByte        The byte.
 * @param[in]     aCount       How many times.
 */
void
__OpenCRT_Format_Repeat(__OpenCRT_Format_tSink* aInOut_pSink, char aByte, size_t aCount);

/**
 * @brief Writes a field: its prefix, zeros, its body, padded to the width.
 *
 * The zeros come between prefix and body; the padding goes on the left, or on the right with the
 * - flag.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pSpec    The specification, for the width and the - flag.
 * @param[in]     aIn_pPrefix  The sign or base prefix.
 * @param[in]     aZeros       The zeros after the prefix.
 * @param[in]     aIn_pBody    The digits or the text.
 * @param[in]     aBodyLength  The length of the body.
 */
void
__OpenCRT_Format_Field(__OpenCRT_Format_tSink*       aInOut_pSink,
                       const __OpenCRT_Format_tSpec* aIn_pSpec,
                       const char*                   aIn_pPrefix,
                       size_t                        aZeros,
                       const char*                   aIn_pBody,
                       size_t                        aBodyLength);

/**
 * @brief Writes a floating-point value for an f, F, e, E, g, G, a or A conversion.
 *
 * @param[in,out] aInOut_pSink The sink.
 * @param[in]     aIn_pSpec    The specification.
 * @param[in]     aValue       The value; a double is widened to long double without change.
 */
void
__OpenCRT_Format_Float(__OpenCRT_Format_tSink*       aInOut_pSink,
                       const __OpenCRT_Format_tSpec* aIn_pSpec,
                       long double                   aValue);

/**
 * @brief Formats the arguments of a printf format into a sink.
 *
 * @param[in,out] aInOut_pSink  The sink.
 * @param[in]     aIn_pFormat   The format.
 * @param[in]     aIn_Arguments The arguments.
 * @return The length of the output, or -1 with errno: EINVAL for a format OpenCRT refuses (an
 *         unknown conversion, positions mixed with plain ones), EILSEQ for a wide character the
 *         locale cannot write, EOVERFLOW for an output longer than INT_MAX.
 */
int
__OpenCRT_Format_Write(__OpenCRT_Format_tSink* aInOut_pSink,
                       const char*             aIn_pFormat,
                       va_list                 aIn_Arguments);

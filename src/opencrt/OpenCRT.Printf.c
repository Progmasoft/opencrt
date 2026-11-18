/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "opencrt/OpenCRT.Format.h"

/** A buffer that keeps what fits, the null character included. */
typedef struct Buffer
{
        char*  pBytes;
        size_t size;
        size_t used;
} tBuffer;

/**
 * @brief Keeps the bytes that fit in the buffer, leaving room for the null character.
 *
 * @param[in,out] aInOut_pSink The sink, whose context is a tBuffer.
 * @param[in]     aIn_pBytes   The bytes.
 * @param[in]     aCount       How many.
 */
static void
PutInBuffer(__OpenCRT_Format_tSink* aInOut_pSink, const char* aIn_pBytes, size_t aCount)
{
        tBuffer* pBuffer = aInOut_pSink->pContext;

        if(pBuffer->size == 0 || pBuffer->used >= pBuffer->size - 1)
                return;
        size_t room = pBuffer->size - 1 - pBuffer->used;
        size_t kept = aCount < room ? aCount : room;

        memcpy(pBuffer->pBytes + pBuffer->used, aIn_pBytes, kept);
        pBuffer->used += kept;
}

/**
 * @brief Formats into a buffer of a given size, always ending it with a null character.
 *
 * @param[out] aOut_pBuffer  The buffer, or NULL when aSize is 0.
 * @param[in]  aSize         Its size.
 * @param[in]  aIn_pFormat   The format.
 * @param[in]  aIn_Arguments The arguments.
 * @return The length the output has whole, or -1 with errno set.
 */
int
vsnprintf(char* restrict aOut_pBuffer,
          size_t aSize,
          const char* restrict aIn_pFormat,
          va_list aIn_Arguments)
{
        tBuffer                buffer = { aOut_pBuffer, aSize, 0 };
        __OpenCRT_Format_tSink sink   = { PutInBuffer, &buffer, 0 };
        int                    length = __OpenCRT_Format_Write(&sink, aIn_pFormat, aIn_Arguments);

        if(aSize > 0)
                aOut_pBuffer[ buffer.used ] = '\0';
        return (length);
}

/**
 * @brief Formats into a buffer of a given size, always ending it with a null character.
 *
 * @param[out] aOut_pBuffer The buffer, or NULL when aSize is 0.
 * @param[in]  aSize        Its size.
 * @param[in]  aIn_pFormat  The format, then its arguments.
 * @return The length the output has whole, or -1 with errno set.
 */
int
snprintf(char* restrict aOut_pBuffer, size_t aSize, const char* restrict aIn_pFormat, ...)
{
        va_list arguments;

        va_start(arguments, aIn_pFormat);
        int length = vsnprintf(aOut_pBuffer, aSize, aIn_pFormat, arguments);

        va_end(arguments);
        return (length);
}

/**
 * @brief Formats into a buffer the caller must make large enough; snprintf is the safe form.
 *
 * @param[out] aOut_pBuffer  The buffer.
 * @param[in]  aIn_pFormat   The format.
 * @param[in]  aIn_Arguments The arguments.
 * @return The length of the output, or -1 with errno set.
 */
int
vsprintf(char* restrict aOut_pBuffer, const char* restrict aIn_pFormat, va_list aIn_Arguments)
{
        return (vsnprintf(aOut_pBuffer, (size_t) -1 / 2, aIn_pFormat, aIn_Arguments));
}

/**
 * @brief Formats into a buffer the caller must make large enough; snprintf is the safe form.
 *
 * @param[out] aOut_pBuffer The buffer.
 * @param[in]  aIn_pFormat  The format, then its arguments.
 * @return The length of the output, or -1 with errno set.
 */
int
sprintf(char* restrict aOut_pBuffer, const char* restrict aIn_pFormat, ...)
{
        va_list arguments;

        va_start(arguments, aIn_pFormat);
        int length = vsprintf(aOut_pBuffer, aIn_pFormat, arguments);

        va_end(arguments);
        return (length);
}

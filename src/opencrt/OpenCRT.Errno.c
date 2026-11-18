/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

/** The value of errno. One thread runs while OpenTala has no threads; then it becomes per thread. */
static int gsErrno;

/**
 * @brief Where errno is, as Darwin's ABI has it.
 *
 * @return The address of errno.
 */
int*
__error(void)
{
        return (&gsErrno);
}

/** The message of each error number, indexed by it. */
static const char* const kMessages[] = {
        [0]               = "Undefined error: 0",
        [EPERM]           = "Operation not permitted",
        [ENOENT]          = "No such file or directory",
        [ESRCH]           = "No such process",
        [EINTR]           = "Interrupted system call",
        [EIO]             = "Input/output error",
        [ENXIO]           = "Device not configured",
        [E2BIG]           = "Argument list too long",
        [ENOEXEC]         = "Exec format error",
        [EBADF]           = "Bad file descriptor",
        [ECHILD]          = "No child processes",
        [EDEADLK]         = "Resource deadlock avoided",
        [ENOMEM]          = "Cannot allocate memory",
        [EACCES]          = "Permission denied",
        [EFAULT]          = "Bad address",
        [ENOTBLK]         = "Block device required",
        [EBUSY]           = "Resource busy",
        [EEXIST]          = "File exists",
        [EXDEV]           = "Cross-device link",
        [ENODEV]          = "Operation not supported by device",
        [ENOTDIR]         = "Not a directory",
        [EISDIR]          = "Is a directory",
        [EINVAL]          = "Invalid argument",
        [ENFILE]          = "Too many open files in system",
        [EMFILE]          = "Too many open files",
        [ENOTTY]          = "Inappropriate ioctl for device",
        [ETXTBSY]         = "Text file busy",
        [EFBIG]           = "File too large",
        [ENOSPC]          = "No space left on device",
        [ESPIPE]          = "Illegal seek",
        [EROFS]           = "Read-only file system",
        [EMLINK]          = "Too many links",
        [EPIPE]           = "Broken pipe",
        [EDOM]            = "Numerical argument out of domain",
        [ERANGE]          = "Result too large",
        [EAGAIN]          = "Resource temporarily unavailable",
        [EINPROGRESS]     = "Operation now in progress",
        [EALREADY]        = "Operation already in progress",
        [ENOTSOCK]        = "Socket operation on non-socket",
        [EDESTADDRREQ]    = "Destination address required",
        [EMSGSIZE]        = "Message too long",
        [EPROTOTYPE]      = "Protocol wrong type for socket",
        [ENOPROTOOPT]     = "Protocol not available",
        [EPROTONOSUPPORT] = "Protocol not supported",
        [ESOCKTNOSUPPORT] = "Socket type not supported",
        [ENOTSUP]         = "Operation not supported",
        [EPFNOSUPPORT]    = "Protocol family not supported",
        [EAFNOSUPPORT]    = "Address family not supported by protocol family",
        [EADDRINUSE]      = "Address already in use",
        [EADDRNOTAVAIL]   = "Can't assign requested address",
        [ENETDOWN]        = "Network is down",
        [ENETUNREACH]     = "Network is unreachable",
        [ENETRESET]       = "Network dropped connection on reset",
        [ECONNABORTED]    = "Software caused connection abort",
        [ECONNRESET]      = "Connection reset by peer",
        [ENOBUFS]         = "No buffer space available",
        [EISCONN]         = "Socket is already connected",
        [ENOTCONN]        = "Socket is not connected",
        [ESHUTDOWN]       = "Can't send after socket shutdown",
        [ETOOMANYREFS]    = "Too many references: can't splice",
        [ETIMEDOUT]       = "Operation timed out",
        [ECONNREFUSED]    = "Connection refused",
        [ELOOP]           = "Too many levels of symbolic links",
        [ENAMETOOLONG]    = "File name too long",
        [EHOSTDOWN]       = "Host is down",
        [EHOSTUNREACH]    = "No route to host",
        [ENOTEMPTY]       = "Directory not empty",
        [EPROCLIM]        = "Too many processes",
        [EUSERS]          = "Too many users",
        [EDQUOT]          = "Disc quota exceeded",
        [ESTALE]          = "Stale NFS file handle",
        [EREMOTE]         = "Too many levels of remote in path",
        [EBADRPC]         = "RPC struct is bad",
        [ERPCMISMATCH]    = "RPC version wrong",
        [EPROGUNAVAIL]    = "RPC prog. not avail",
        [EPROGMISMATCH]   = "Program version wrong",
        [EPROCUNAVAIL]    = "Bad procedure for program",
        [ENOLCK]          = "No locks available",
        [ENOSYS]          = "Function not implemented",
        [EFTYPE]          = "Inappropriate file type or format",
        [EAUTH]           = "Authentication error",
        [ENEEDAUTH]       = "Need authenticator",
        [EPWROFF]         = "Device power is off",
        [EDEVERR]         = "Device error",
        [EOVERFLOW]       = "Value too large to be stored in data type",
        [EBADEXEC]        = "Bad executable (or shared library)",
        [EBADARCH]        = "Bad CPU type in executable",
        [ESHLIBVERS]      = "Shared library version mismatch",
        [EBADMACHO]       = "Malformed Mach-o file",
        [ECANCELED]       = "Operation canceled",
        [EIDRM]           = "Identifier removed",
        [ENOMSG]          = "No message of desired type",
        [EILSEQ]          = "Illegal byte sequence",
        [ENOATTR]         = "Attribute not found",
        [EBADMSG]         = "Bad message",
        [EMULTIHOP]       = "EMULTIHOP (Reserved)",
        [ENODATA]         = "No message available on STREAM",
        [ENOLINK]         = "ENOLINK (Reserved)",
        [ENOSR]           = "No STREAM resources",
        [ENOSTR]          = "Not a STREAM",
        [EPROTO]          = "Protocol error",
        [ETIME]           = "STREAM ioctl timeout",
        [EOPNOTSUPP]      = "Operation not supported on socket",
        [ENOPOLICY]       = "Policy not found",
        [ENOTRECOVERABLE] = "State not recoverable",
        [EOWNERDEAD]      = "Previous owner died",
        [EQFULL]          = "Interface output queue is full",
};

/**
 * @brief Writes "Unknown error: N" into a buffer.
 *
 * @param[out] aOut_pBuffer The buffer, at least 32 bytes.
 * @param[in]  aNumber      The number.
 */
static void
WriteUnknown(char* aOut_pBuffer, int aNumber)
{
        char         digits[ 12 ];
        int          at        = 12;
        unsigned int magnitude = aNumber < 0 ? 0U - (unsigned int) aNumber : (unsigned int) aNumber;

        do
        {
                digits[ --at ] = (char) ('0' + magnitude % 10);
                magnitude     /= 10;
        } while(magnitude != 0);
        if(aNumber < 0)
                digits[ --at ] = '-';
        char* pEnd = stpcpy(aOut_pBuffer, "Unknown error: ");

        memcpy(pEnd, digits + at, (size_t) (12 - at));
        pEnd[ 12 - at ] = '\0';
}

/**
 * @brief The message of an error number, or NULL for a number that has none.
 *
 * @param[in] aNumber The number.
 * @return The message.
 */
static const char*
Message(int aNumber)
{
        if(aNumber < 0 || aNumber > ELAST)
                return (NULL);
        return (kMessages[ aNumber ]);
}

/**
 * @brief The message of an error number; "Unknown error: N" for one it does not know.
 *
 * @param[in] aNumber The number.
 * @return The message, which the next call for an unknown number may overwrite.
 */
char*
strerror(int aNumber)
{
        static char sUnknown[ 32 ];
        const char* pMessage = Message(aNumber);

        if(pMessage != NULL)
                return ((char*) pMessage);
        WriteUnknown(sUnknown, aNumber);
        return (sUnknown);
}

/**
 * @brief Writes the message of an error number into a buffer, cut to fit.
 *
 * @param[in]  aNumber      The number.
 * @param[out] aOut_pBuffer The buffer.
 * @param[in]  aSize        Its size.
 * @return 0; EINVAL for an unknown number, whose "Unknown error: N" is written; ERANGE when the
 *         message was cut.
 */
int
strerror_r(int aNumber, char* aOut_pBuffer, size_t aSize)
{
        char        unknown[ 32 ];
        const char* pMessage = Message(aNumber);
        int         result   = 0;

        if(pMessage == NULL)
        {
                WriteUnknown(unknown, aNumber);
                pMessage = unknown;
                result   = EINVAL;
        }
        if(strlcpy(aOut_pBuffer, pMessage, aSize) >= aSize)
                return (ERANGE);
        return (result);
}

/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <sys/_types/_null.h>
#include <sys/_types/_size_t.h>
#include <sys/cdefs.h>

__BEGIN_DECLS
void*
memcpy(void* __restrict_arg, const void* __restrict_arg, size_t);
void*
memmove(void*, const void*, size_t);
void*
memset(void*, int, size_t);
int
memcmp(const void*, const void*, size_t);
void*
memchr(const void*, int, size_t);
char*
strcpy(char* __restrict_arg, const char* __restrict_arg);
char*
strncpy(char* __restrict_arg, const char* __restrict_arg, size_t);
char*
strcat(char* __restrict_arg, const char* __restrict_arg);
char*
strncat(char* __restrict_arg, const char* __restrict_arg, size_t);
int
strcmp(const char*, const char*);
int
strncmp(const char*, const char*, size_t);
int
strcoll(const char*, const char*);
size_t
strxfrm(char* __restrict_arg, const char* __restrict_arg, size_t);
char*
strchr(const char*, int);
char*
strrchr(const char*, int);
size_t
strspn(const char*, const char*);
size_t
strcspn(const char*, const char*);
char*
strpbrk(const char*, const char*);
char*
strstr(const char*, const char*);
char*
strtok(char* __restrict_arg, const char* __restrict_arg);
size_t
strlen(const char*);
char*
strerror(int);

/* POSIX. */
void*
memccpy(void* __restrict_arg, const void* __restrict_arg, int, size_t);
char*
stpcpy(char* __restrict_arg, const char* __restrict_arg);
char*
stpncpy(char* __restrict_arg, const char* __restrict_arg, size_t);
size_t
strnlen(const char*, size_t);
char*
strtok_r(char* __restrict_arg, const char* __restrict_arg, char** __restrict_arg);
int
strerror_r(int, char*, size_t);

/* The BSDs and Darwin: copies that always end the destination with a null character. */
size_t
strlcpy(char* __restrict_arg, const char* __restrict_arg, size_t);
size_t
strlcat(char* __restrict_arg, const char* __restrict_arg, size_t);
__END_DECLS

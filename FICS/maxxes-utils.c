// SPDX-FileCopyrightText: 2024-2026 Markus Uhlin <maxxe@rpblc.net>
// SPDX-License-Identifier: ISC

#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if __linux__
#include <bsd/string.h>
#endif

#include "maxxes-utils.h"

void
fprintf_logerr(const char *file, const long int line,
    FILE *fp, const char *format, ...)
{
	int	ret;
	va_list	ap;

	if (fp == NULL) {
		warnx("%s:%ld: error: invalid argument (null pointer)",
		    file, line);
		return;
	} else if (fp == stdin || fp == stdout || fp == stderr) {
		warnx("%s:%ld: error: invalid stream (stdin, stdout or stderr)",
		    file, line);
		return;
	}

	va_start(ap, format);
	ret = vfprintf(fp, format, ap);
	va_end(ap);

	if (ret < 0)
		warnx("%s:%ld: warning: vfprintf() error", file, line);
}

bool
is_too_long(const int p_ret, const size_t p_maxlen)
{
	return (p_ret < 0 || (size_t)p_ret >= p_maxlen);
}

void
snprintf_trunc_chk(const char *file, const long int line,
    char *str, size_t size, const char *format, ...)
{
	int ret;
	va_list ap;

	va_start(ap, format);
	ret = vsnprintf(str, size, format, ap);
	va_end(ap);

	if (ret < 0 || (size_t)ret >= size)
		warnx("%s:%ld: warning: vsnprintf() truncated", file, line);
}

void
strlcpy_trunc_chk(char *dst, const char *src, size_t dstsize,
    const char *file,
    const long int line)
{
	if (strlcpy(dst, src, dstsize) >= dstsize)
		warnx("%s:%ld: warning: strlcpy() truncated", file, line);
}

void
strlcat_trunc_chk(char *dst, const char *src, size_t dstsize,
    const char *file,
    const long int line)
{
	if (strlcat(dst, src, dstsize) >= dstsize)
		warnx("%s:%ld: warning: strlcat() truncated", file, line);
}

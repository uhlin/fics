#ifndef MAXXES_UTILITIES_H
#define MAXXES_UTILITIES_H

#include <stddef.h>

#include "common.h"

#define msnprintf(p_str, p_size, ...) \
	snprintf_trunc_chk(__FILE__, __LINE__, (p_str), (p_size), __VA_ARGS__)
#define mstrlcpy(p_dst, p_src, p_dstsize) \
	strlcpy_trunc_chk((p_dst), (p_src), (p_dstsize), __FILE__, __LINE__)
#define mstrlcat(p_dst, p_src, p_dstsize) \
	strlcat_trunc_chk((p_dst), (p_src), (p_dstsize), __FILE__, __LINE__)

void	snprintf_trunc_chk(const char *file, const long int line,
	    char *str, size_t size, const char *format, ...) PRINTFLIKE(5);
void	strlcpy_trunc_chk(char *dst, const char *src, size_t dstsize,
	    const char *, const long int);
void	strlcat_trunc_chk(char *dst, const char *src, size_t dstsize,
	    const char *, const long int);

#endif

/*
 * File: common.h
 * Copyright 1993, Richard V. Nash
 */

/*
    fics - An internet chess server.
    Copyright (C) 1993  Richard V. Nash

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

/* Revision history:
   name		email		yy/mm/dd	Change
   Richard Nash                 93/10/22	Created
   Markus Uhlin                 24/01/03	Added/deleted macros and
						redefined ASSERT().
   Markus Uhlin                 24/01/04	Added PRINTFLIKE().
   Markus Uhlin                 24/04/02	Defined:
						UNUSED_PARAM() and
						UNUSED_VAR().
*/

#ifndef _COMMON_H
#define _COMMON_H

#include "assert_error.h"
#include "legal.h"
#include "vers.h"

#define PUBLIC
#define PRIVATE static

#ifndef NULL
#define NULL ((void *)0)
#endif

#define ARRAY_SIZE(_a) (sizeof((_a)) / sizeof((_a)[0]))

#ifdef __GNUC__
#define PRINTFLIKE(arg_no) __attribute__((format(printf, arg_no, arg_no + 1)))
#else
#define PRINTFLIKE(arg_no)
#endif

#define UNUSED_PARAM(p) ((void) (p))
#define UNUSED_VAR(v) ((void) (v))

#ifdef DEBUG
#define ASSERT(expression) \
	((void) ((expression)||_assert_error(__FILE__, __LINE__)))
#else
#define ASSERT(expression) ((void) 0)
#endif

#endif /* _COMMON_H */

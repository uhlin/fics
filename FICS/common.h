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
   Richard Nash	              	93/10/22	Created
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

#define SWAP(a,b,type) {\
  type tmp; \
  tmp = (a);\
  (a) = (b);\
  (b) = tmp;\
}

#ifdef DEBUG
#define ASSERT(expression) \
	((void) ((expression)||_assert_error(__FILE__, __LINE__)))
#else
#define ASSERT(expression) ((void) 0)
#endif

#endif /* _COMMON_H */

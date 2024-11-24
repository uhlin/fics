/* variable.h
 *
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
   Richard Nash	                93/10/22	Created
   Markus Uhlin                 24/04/01	Revised
*/

#ifndef _VARIABLE_H
#define _VARIABLE_H

#include "common.h"

#define VAR_OK           0
#define VAR_NOSUCH       1
#define VAR_BADVAL       2
#define VAR_AMBIGUOUS    3

#define LANG_ENGLISH     0
#define LANG_SPANISH     1
#define LANG_FRENCH      2
#define LANG_DANISH      3
#define NUM_LANGS        4

typedef struct _var_list {
	char *name;
	int (*var_func)(int, char *, char *);
} var_list;

__FICS_BEGIN_DECLS
extern var_list variables[];

extern const char	*Language(unsigned int);
extern int		 var_set(int, char *, char *, int *);
__FICS_END_DECLS

#endif /* _VARIABLE_H */

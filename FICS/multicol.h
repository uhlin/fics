/* multicol.h
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
   Richard Nash	              	93/10/22	Created
*/

#ifndef _MULTICOL_H
#define _MULTICOL_H

typedef struct _multicol
{
  int arraySize;
  int num;
  char **strArray;
} multicol;

extern multicol *multicol_start(int);
extern int multicol_store(multicol *, char *);
extern int multicol_store_sorted(multicol *, char *);
extern int multicol_pprint(multicol *, int, int, int);
extern int multicol_end(multicol *);

#endif /* _MULTICOL_H */

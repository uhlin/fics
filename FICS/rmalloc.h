/* rmalloc.h
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
   Richard Nash                 93/10/22	Created
   Markus Uhlin                 23/12/20	Revised
*/

#ifndef _RMALLOC_H
#define _RMALLOC_H

extern unsigned int	allocated_size;
extern unsigned int	malloc_count;
extern unsigned int	free_count;

extern void	*rmalloc(int);
extern void	*rrealloc(void *, int);
extern void	 rfree(void *);
extern void	 strfree(char *);

#endif /* _RMALLOC_H */

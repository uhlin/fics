/* matchproc.h
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
   name				yy/mm/dd	Change
   hersco			95/07/24	Created
   Markus Uhlin			24/03/29	Revised
*/

#ifndef _MATCHPROC_H
#define _MATCHPROC_H

#include "command.h"	/* param_list */

extern int	com_accept(int, param_list);
extern int	com_decline(int, param_list);
extern int	com_match(int, param_list);
extern int	com_pending(int, param_list);
extern int	com_withdraw(int, param_list);
extern int	create_new_match(int, int, int, int, int, int, int, char *,
		    char *, int);

#endif /* _MATCHPROC_H */

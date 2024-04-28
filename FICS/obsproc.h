/* obsproc.h
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
   Dave Herscovici		95/11/26	Created
   Markus Uhlin			24/04/28	Revised
*/

#ifndef _OBSPROC_H
#define _OBSPROC_H

#include "command.h" /* param_list */

#define MAX_JOURNAL 10

extern int	GameNumFromParam(int, int *, parameter *);
extern int	com_allobservers(int, param_list);
extern int	com_backward(int, param_list);
extern int	com_examine(int, param_list);
extern int	com_forward(int, param_list);
extern int	com_games(int, param_list);
extern int	com_history(int, param_list);
extern int	com_journal(int, param_list);
extern int	com_jsave(int, param_list);
extern int	com_mailmoves(int, param_list);
extern int	com_mailoldmoves(int, param_list);
extern int	com_mailstored(int, param_list);
extern int	com_mexamine(int, param_list);
extern int	com_moves(int, param_list);
extern int	com_observe(int, param_list);
extern int	com_oldmoves(int, param_list);
extern int	com_revert(int, param_list);
extern int	com_smoves(int, param_list);
extern int	com_sposition(int, param_list);
extern int	com_stored(int, param_list);
extern int	com_unexamine(int, param_list);
extern int	com_unobserve(int, param_list);
extern void	ExamineScratch(int, param_list);
extern void	jsave_history(int, char, int, int, char *);
extern void	unobserveAll(int);

#endif /* _OBSPROC_H */

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
*/

#ifndef _OBSPROC_H
#define _OBSPROC_H

#define MAX_JOURNAL 10

extern int GameNumFromParam();
extern int com_games();
extern int com_observe();
extern void unobserveAll(int);
extern int com_unobserve();
extern int com_allobservers();
extern int com_moves();
extern int com_mailmoves();
extern int com_oldmoves();
extern int com_mailoldmoves();
extern int com_mailstored();
extern int com_stored();
extern int com_smoves();
extern int com_sposition();
extern int com_history();
extern int com_journal();
extern int com_jsave();

extern int com_examine();
extern int com_mexamine();
extern int com_unexamine();
extern int com_forward();
extern int com_backward();
extern int com_revert();

extern void ExamineScratch ();

#endif /* _OBSPROC_H */

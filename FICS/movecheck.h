/* movecheck.h
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
   Richard Nash			93/10/22	Created
   Markus Uhlin			24/05/05	Revised
*/

#ifndef _MOVECHECK_H
#define _MOVECHECK_H

#define MOVE_OK          0
#define MOVE_ILLEGAL     1
#define MOVE_STALEMATE   2
#define MOVE_CHECKMATE   3
#define MOVE_AMBIGUOUS   4
#define MOVE_NOMATERIAL  5

#define MS_NOTMOVE  0
#define MS_COMP     1
#define MS_COMPDASH 2
#define MS_ALG      3
#define MS_KCASTLE  4
#define MS_QCASTLE  5

#define isrank(c)	(((c) <= '8') && ((c) >= '1'))
#define isfile(c)	(((c) >= 'a') && ((c) <= 'h'))

#if !defined(_BOARD_H)
#include "board.h"
#endif

extern int	InitPieceLoop(board_t, int *, int *, int);
extern int	NextPieceLoop(board_t, int *, int *, int);

extern int	backup_move(int, int);
extern int	execute_move(game_state_t *, move_t *, int);
extern int	in_check(game_state_t *);
extern int	is_move(char *);
extern int	legal_andcheck_move(game_state_t *, int, int, int, int);
extern int	legal_move(game_state_t *, int, int, int, int);
extern int	parse_move(char *, game_state_t *, move_t *, int);

#endif /* _MOVECHECK_H */

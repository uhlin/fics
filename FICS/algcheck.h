/* algcheck.h
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

#ifndef _ALGCHECK_H
#define _ALGCHECK_H

#ifndef _BOARD_H
#include "board.h"
#endif

#define DROP_CHAR	'@' // used by algcheck.c and movecheck.c
#define DROP_STR	"@"

extern char	*alg_unparse(game_state_t *, move_t *);
extern int	 alg_is_move(char *);
extern int	 alg_parse_move(char *, game_state_t *, move_t *);

#endif /* _ALGCHECK_H */

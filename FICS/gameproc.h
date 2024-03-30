/* gameproc.h
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

#ifndef _GAMEPROC_H
#define _GAMEPROC_H

#define MAX_SIMPASS 3
#define MAX_JOURNAL 10

#include "command.h" /* param_list */

extern int	com_abort(int, param_list);
extern int	com_adjourn(int, param_list);
extern int	com_boards(int, param_list);
extern int	com_courtesyabort(int, param_list);
extern int	com_courtesyadjourn(int, param_list);
extern int	com_draw(int, param_list);
extern int	com_flag(int, param_list);
extern int	com_goboard(int, param_list);
extern int	com_gonum(int, param_list);
extern int	com_moretime(int, param_list);
extern int	com_pause(int, param_list);
extern int	com_resign(int, param_list);
extern int	com_simabort(int, param_list);
extern int	com_simadjourn(int, param_list);
extern int	com_simallabort(int, param_list);
extern int	com_simalladjourn(int, param_list);
extern int	com_simgames(int, param_list);
extern int	com_simmatch(int, param_list);
extern int	com_simnext(int, param_list);
extern int	com_simpass(int, param_list);
extern int	com_simprev(int, param_list);
extern int	com_switch(int, param_list);
extern int	com_takeback(int, param_list);
extern int	com_time(int, param_list);
extern int	com_unpause(int, param_list);

extern int	pIsPlaying(int);
extern void	game_ended(int, int, int);
extern void	process_move(int, char *);

#endif /* _GAMEPROC_H */

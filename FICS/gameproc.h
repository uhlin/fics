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

extern void game_ended();
extern int pIsPlaying();
extern void process_move();
extern int com_resign();
extern int com_draw();
extern int com_pause();
extern int com_unpause();
extern int com_abort();
extern int com_games();

extern int com_courtesyabort();
extern int com_courtesyadjourn();
extern int com_load();
extern int com_stored();
extern int com_adjourn();
extern int com_flag();
extern int com_takeback();
extern int com_switch();
extern int com_time();
extern int com_boards();

extern int com_simmatch();
extern int com_simnext();
extern int com_simprev();
extern int com_goboard();
extern int com_gonum();
extern int com_simgames();
extern int com_simpass();
extern int com_simabort();
extern int com_simallabort();
extern int com_simadjourn();
extern int com_simalladjourn();

extern int com_moretime();

#endif /* _GAMEPROC_H */

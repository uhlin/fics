/* gamedb.h
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

#ifndef _GAMEDB_H
#define _GAMEDB_H

#include <time.h>

#include "board.h"

extern char	*bstr[];
extern char	*rstr[];

#define GAMEFILE_VERSION	3
#define MAX_GLINE_SIZE		1024

#define REL_GAME         0
#define REL_SPOS         1
#define REL_REFRESH      2
#define REL_EXAMINE      3

#define GAME_EMPTY       0
#define GAME_NEW         1
#define GAME_ACTIVE      2
#define GAME_EXAMINE     3

#define TYPE_UNTIMED     0
#define TYPE_BLITZ       1
#define TYPE_STAND       2
#define TYPE_NONSTANDARD 3
#define TYPE_WILD        4
#define TYPE_LIGHT       5
#define TYPE_BUGHOUSE    6

#ifdef TIMESEAL
#define FLAG_CHECKING	-1
#define FLAG_NONE	0
#define FLAG_CALLED	1
#define FLAG_ABORT	2
#endif

#define END_CHECKMATE         0
#define END_RESIGN            1
#define END_FLAG              2
#define END_AGREEDDRAW        3
#define END_REPETITION        4
#define END_50MOVERULE        5
#define END_ADJOURN           6
#define END_LOSTCONNECTION    7
#define END_ABORT             8
#define END_STALEMATE         9
#define END_NOTENDED          10
#define END_COURTESY          11
#define END_BOTHFLAG          12
#define END_NOMATERIAL        13
#define END_FLAGNOMATERIAL    14
#define END_ADJDRAW           15
#define END_ADJWIN            16
#define END_ADJABORT          17
#define END_COURTESYADJOURN   18

typedef struct _game {
	/*
	 * Saved in the game file.
	 */
	int	 wInitTime, wIncrement;
	int	 bInitTime, bIncrement;
	time_t	 timeOfStart;

#ifdef TIMESEAL
	int		 bLastRealTime;
	int		 bRealTime;
	int		 bTimeWhenMoved;
	int		 bTimeWhenReceivedMove;
	int		 flag_pending;
	int		 wLastRealTime;
	int		 wRealTime;
	int		 wTimeWhenMoved;
	int		 wTimeWhenReceivedMove;
	unsigned long	 flag_check_time;
#endif

	int		 wTime;
	int		 bTime;
	int		 clockStopped;
	int		 rated;
	int		 private;
	int		 type;
	int		 passes;		// For simul's
	int		 numHalfMoves;
	move_t		*moveList;		// Primary movelist
	unsigned char	 FENstartPos[74];	// Save the starting position
	game_state_t	 game_state;
	char		 white_name[18];	// To hold the playername even
						// after he disconnects
	char		 black_name[18];
	int		 white_rating;
	int		 black_rating;

	/*
	 * Not saved in game file
	 */
	int	 revertHalfMove;
	int	 totalHalfMoves;
	int	 white;
	int	 black;
	int	 link;
	int	 status;
	int	 moveListSize;     // Total allocated in '*moveList'
	int	 examHalfMoves;
	move_t	*examMoveList;     // Extra movelist for examine
	int	 examMoveListSize;

	unsigned int startTime;		// The relative time the game started
	unsigned int lastMoveTime;	// Last time a move was made
	unsigned int lastDecTime;	// Last time a players clock was
					// decremented

	int result;
	int winner;
} game;

extern game	*garray;
extern int	 g_num;

extern char	*EndString(int, int);
extern char	*EndSym(int);
extern char	*game_str(int, int, int, int, int, char *, char *);
extern char	*game_time_str(int, int, int, int);
extern char	*movesToString(int, int);
extern int	 CharToPiece(char);
extern int	 PieceToChar(int);
extern int	 ReadGameAttrs();
extern int	 game_clear(int);
extern int	 game_count(void);
extern int	 game_delete(int, int);
extern int	 game_finish(int);
extern int	 game_free(int);
extern int	 game_isblitz(int, int, int, int, char *, char *);
extern int	 game_new(void);
extern int	 game_read(int, int, int);
extern int	 game_remove(int);
extern int	 game_save(int);
extern int	 game_zero(int);
extern int	 got_attr_value();
extern int	 journal_get_info(int, char, char *, int *, char *, int *,
		     char *, int *, int *, char *, char *, char *, char *);
extern int	 pgames(int, int, char *);
extern int	 pjournal(int, int, char *);
extern void	 MakeFENpos(int, char *);
extern void	 RemHist(char *);
extern void	 addjournalitem(int, char, char *, int, char *, int, char *,
		     int, int, char *, char *, char *, char *);
extern void	 game_disconnect(int, int);
extern void	 game_update_time(int);
extern void	 game_update_times(void);
extern void	 game_write_complete(int, int, char *);
extern void	 send_board_to(int, int);
extern void	 send_boards(int);

#endif

/* gamedb.c
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
   Markus Uhlin                 23/12/16	Fixed compiler warnings
   Markus Uhlin                 23/12/17	Reformatted functions
   Markus Uhlin                 23/12/23	Fixed clang warnings
   Markus Uhlin                 24/05/04	Refactored and reformatted all
						functions plus more...
   Markus Uhlin                 24/07/17	Fixed out of bounds array access
						in game_str() which resulted in
						a crash.
   Markus Uhlin                 24/07/18	Return value checking
   Markus Uhlin                 24/08/03	See previous change
   Markus Uhlin                 24/11/23	Added width specifications to
						multiple fscanf() calls.
   Markus Uhlin                 24/11/23	Fixed bugs in movesToString()
   Markus Uhlin                 24/11/25	Null checks
   Markus Uhlin			24/12/02	Fixed bugs and ignored function
						return values.
   Markus Uhlin			25/03/18	Fixed unchecked return values
   Markus Uhlin			25/03/25	ReadGameState: fixed truncated
						stdio return value.
   Markus Uhlin			25/04/01	Fixed call of risky function
*/

#include "stdinclude.h"
#include "common.h"

#include <err.h>
#include <errno.h>
#include <limits.h>

#include "command.h"
#include "config.h"
#include "eco.h"
#include "ficsmain.h"
#include "gamedb.h"
#include "gameproc.h"
#include "maxxes-utils.h"
#include "network.h"
#include "playerdb.h"
#include "rmalloc.h"
#include "utils.h"

#if __linux__
#include <bsd/string.h>
#endif

/*
 * This should be enough to hold any game up to at least 250 moves. If
 * we overwrite this, the server will crash.
 */
#define GAME_STRING_LEN 16000

PUBLIC game	*garray = NULL;
PUBLIC int	 g_num = 0;

PUBLIC const char *bstr[7] = {
	[TYPE_UNTIMED]     = "untimed",
	[TYPE_BLITZ]       = "blitz",
	[TYPE_STAND]       = "standard",
	[TYPE_NONSTANDARD] = "non-standard",
	[TYPE_WILD]        = "wild",
	[TYPE_LIGHT]       = "lightning",
	[TYPE_BUGHOUSE]    = "bughouse"
};
PUBLIC const char *rstr[2] = {
	[TYPE_UNRATED] = "unrated",
	[TYPE_RATED] = "rated"
};

PRIVATE char gameString[GAME_STRING_LEN];

/*
 * This method is awful! How about allocation as we need it and
 * freeing afterwards!
 */
PRIVATE int
get_empty_slot(void)
{
	if (garray != NULL) {
		for (int i = 0; i < g_num; i++) {
			if (garray[i].status == GAME_EMPTY)
				return i;
		}
	}

	g_num++;

	if (!garray) {
		garray = reallocarray(NULL, sizeof(game), g_num);

		if (garray == NULL)
			err(1, "%s: reallocarray", __func__);
		else
			malloc_count++;
	} else {
		garray = reallocarray(garray, sizeof(game), g_num);

		if (garray == NULL)
			err(1, "%s: reallocarray", __func__);
	}	// Yeah great, bet this causes lag! - DAV

	garray[g_num - 1].status = GAME_EMPTY;
	return g_num - 1;
}

PUBLIC int
game_new(void)
{
	int	new = get_empty_slot();

	game_zero(new);

	return new;
}

PUBLIC int
game_zero(int g)
{
	garray[g].white = -1;
	garray[g].black = -1;

	garray[g].link		= -1;
	garray[g].passes	= 0;
	garray[g].private	= 0;
	garray[g].rated		= 0;
	garray[g].result	= END_NOTENDED;
	garray[g].status	= GAME_NEW;
	garray[g].type		= TYPE_UNTIMED;

	board_init(&garray[g].game_state, NULL, NULL);

	garray[g].bIncrement		= 0;
	garray[g].bInitTime		= 300; // 5 minutes
	garray[g].black_name[0]		= '\0';
	garray[g].black_rating		= 0;

	garray[g].wIncrement		= 0;
	garray[g].wInitTime		= 300; // 5 minutes
	garray[g].white_name[0]		= '\0';
	garray[g].white_rating		= 0;

	garray[g].examMoveList		= NULL;
	garray[g].examMoveListSize	= 0;

#ifdef TIMESEAL
	garray[g].flag_check_time	= 0L;
	garray[g].flag_pending		= FLAG_NONE;
#endif

	garray[g].game_state.gameNum	= g;
	garray[g].moveList		= NULL;
	garray[g].moveListSize		= 0;
	garray[g].numHalfMoves		= 0;
	garray[g].revertHalfMove	= 0;

	return 0;
}

PUBLIC int
game_free(int g)
{
	if (garray[g].moveListSize)
		rfree(garray[g].moveList);
	if (garray[g].examMoveListSize)
		rfree(garray[g].examMoveList);

	garray[g].moveListSize = 0;
	garray[g].examMoveListSize = 0;

	return 0;
}

PUBLIC int
game_clear(int g)
{
	game_free(g);
	game_zero(g);
	return 0;
}

PUBLIC int
game_remove(int g)
{
	// Should remove game from players observation list
	game_clear(g);
	garray[g].status = GAME_EMPTY;
	return 0;
}

// old moves not stored now - uses smoves
PUBLIC int
game_finish(int g)
{
	player_game_ended(g);	// Alert playerdb that game ended
	game_remove(g);
	return 0;
}

PUBLIC void
MakeFENpos(int g, char *FEN, size_t size)
{
	mstrlcpy(FEN, boardToFEN(g), size);
}

PUBLIC char *
game_time_str(int wt, int winc, int bt, int binc)
{
	static char	tstr[50] = { '\0' };

	if ((!wt) && (!winc)) { // Untimed
		(void) strlcpy(tstr, "", sizeof tstr);
		return tstr;
	}

	if ((wt == bt) && (winc == binc)) {
		msnprintf(tstr, sizeof tstr, " %d %d", wt, winc);
	} else {
		msnprintf(tstr, sizeof tstr, " %d %d : %d %d",
		    wt, winc,
		    bt, binc);
	}

	return tstr;
}

PUBLIC char *
game_str(int rated, int wt, int winc, int bt, int binc, char *cat, char *board)
{
	static char	tstr[200] = { '\0' };

	if (rated != TYPE_UNRATED &&
	    rated != TYPE_RATED)
		rated = TYPE_UNRATED;

	if (cat && cat[0] && board && board[0] && (strcmp(cat, "standard") ||
	    strcmp(board, "standard"))) {
		msnprintf(tstr, sizeof(tstr), "%s %s%s Loaded from %s/%s",
		    rstr[rated],
		    bstr[game_isblitz(wt / 60, winc, bt / 60, binc, cat, board)],
		    game_time_str(wt / 60, winc, bt / 60, binc),
		    cat,
		    board);
	} else {
		msnprintf(tstr, sizeof(tstr), "%s %s%s",
		    rstr[rated],
		    bstr[game_isblitz(wt / 60, winc, bt / 60, binc, cat, board)],
		    game_time_str(wt / 60, winc, bt / 60, binc));
	}

	return tstr;
}

PUBLIC int
game_isblitz(int wt, int winc, int bt, int binc, char *cat, char *board)
{
	int total;

	if (cat && cat[0] && board && board[0] && (!strcmp(cat, "wild")))
		return TYPE_WILD;
	if (cat && cat[0] && board && board[0] && (strcmp(cat, "standard") ||
	    strcmp(board, "standard")))
		return TYPE_NONSTANDARD;

	if (((wt == 0) && (winc == 0)) || ((bt == 0) && (binc == 0))) {
		/*
		 * nonsense if one is timed and one is not
		 */
		return TYPE_UNTIMED;
	}

	if ((wt != bt) || (winc != binc))
		return TYPE_NONSTANDARD;

	total = wt * 60 + winc * 40;

	if (total < 180)	// 3 minutes
		return TYPE_LIGHT;
	if (total >= 900)	// 15 minutes
		return TYPE_STAND;
	else
		return TYPE_BLITZ;
}

PUBLIC void
send_board_to(int g, int p)
{
	char	*b;
	int	 relation;
	int	 side;

	/*
	 * Since we know 'g' and 'p', figure out our relationship to
	 * this game...
	 */

	side = WHITE;

	if (garray[g].status == GAME_EXAMINE) {
		if (parray[p].game == g) {
			relation = 2;
		} else {
			relation = -2;
		}
	} else {
		if (parray[p].game == g) {
			side = parray[p].side;
			relation = (side == garray[g].game_state.onMove ? 1 :
			    -1);
		} else {
			relation = 0;
		}
	}

	if (parray[p].flip) {	// flip board?
		if (side == WHITE)
			side = BLACK;
		else
			side = WHITE;
	}

	game_update_time(g);
	b = board_to_string(garray[g].white_name, garray[g].black_name,
	    garray[g].wTime,
	    garray[g].bTime,
	    &garray[g].game_state,
	    (garray[g].status == GAME_EXAMINE)
	    ? garray[g].examMoveList
	    : garray[g].moveList,
	    parray[p].style,
	    side,
	    relation,
	    p);

#ifdef TIMESEAL
	if (con[parray[p].socket].timeseal) {
		if (parray[p].bell) {
			pprintf_noformat(p, "\007\n[G]\n%s", b);
		} else {
			pprintf_noformat(p, "\n[G]\n%s", b);
		}
	} else {
		if (parray[p].bell) {
			pprintf_noformat(p, "\007\n%s", b);
		} else {
			pprintf_noformat(p, "\n%s", b);
		}
	}
#else
	if (parray[p].bell) {
		pprintf_noformat(p, "\007\n%s", b);
	} else {
		pprintf_noformat(p, "\n%s", b);
	}
#endif

	if (p != commanding_player)
		pprintf(p, "%s", parray[p].prompt);
}

PUBLIC void
send_boards(int g)
{
	simul_info_t *simInfo = &parray[garray[g].white].simul_info;

	if (simInfo->numBoards == 0 || simInfo->boards[simInfo->onBoard] == g) {
		for (int p = 0; p < p_num; p++) {
			if (parray[p].status == PLAYER_EMPTY)
				continue;
			if (player_is_observe(p, g) || (parray[p].game == g))
				send_board_to(g, p);
		}
	}
}

PUBLIC void
game_update_time(int g)
{
	unsigned int now, timesince;

	if (garray[g].clockStopped)
		return;
	if (garray[g].type == TYPE_UNTIMED)
		return;

	now = tenth_secs();
	timesince = now - garray[g].lastDecTime;

	if (garray[g].game_state.onMove == WHITE) {
		garray[g].wTime -= timesince;
	} else {
		garray[g].bTime -= timesince;
	}

	garray[g].lastDecTime = now;
}

PUBLIC void
game_update_times(void)
{
	for (int g = 0; g < g_num; g++) {
		if (garray[g].status != GAME_ACTIVE)
			continue;
		if (garray[g].clockStopped)
			continue;
		game_update_time(g);
	}
}

PUBLIC char *
EndString(int g, int personal)
{
	/*
	 * personal 0 == White checkmated
	 * personal 1 == loon checkmated
	 */
	char		*blackguy, *whiteguy;
	static char	 blackstr[] = "Black";
	static char	 whitestr[] = "White";
	static char	 endstr[200] = { '\0' };

	blackguy = (personal ? garray[g].black_name : blackstr);
	whiteguy = (personal ? garray[g].white_name : whitestr);

	switch (garray[g].result) {
	case END_CHECKMATE:
		msnprintf(endstr, sizeof endstr, "%s checkmated",
		    garray[g].winner == WHITE ? blackguy : whiteguy);
		break;
	case END_RESIGN:
		msnprintf(endstr, sizeof endstr, "%s resigned",
		    garray[g].winner == WHITE ? blackguy : whiteguy);
		break;
	case END_FLAG:
		msnprintf(endstr, sizeof endstr, "%s ran out of time",
		    garray[g].winner == WHITE ? blackguy : whiteguy);
		break;
	case END_AGREEDDRAW:
		msnprintf(endstr, sizeof endstr, "Game drawn by mutual "
		    "agreement");
		break;
	case END_BOTHFLAG:
		msnprintf(endstr, sizeof endstr, "Game drawn because both "
		    "players ran out of time");
		break;
	case END_REPETITION:
		msnprintf(endstr, sizeof endstr, "Game drawn by repetition");
		break;
	case END_50MOVERULE:
		msnprintf(endstr, sizeof endstr, "Draw by the 50 move rule");
		break;
	case END_ADJOURN:
		msnprintf(endstr, sizeof endstr, "Game adjourned by mutual "
		    "agreement");
		break;
	case END_LOSTCONNECTION:
		msnprintf(endstr, sizeof endstr, "%s lost connection, "
		    "game adjourned",
		    garray[g].winner == WHITE ? whiteguy : blackguy);
		break;
	case END_ABORT:
		msnprintf(endstr, sizeof endstr, "Game aborted by mutual "
		    "agreement");
		break;
	case END_STALEMATE:
		msnprintf(endstr, sizeof endstr, "Stalemate.");
		break;
	case END_NOTENDED:
		msnprintf(endstr, sizeof endstr, "Still in progress");
		break;
	case END_COURTESY:
		msnprintf(endstr, sizeof endstr, "Game courtesyaborted by %s",
		    garray[g].winner == WHITE ? whiteguy : blackguy);
		break;
	case END_COURTESYADJOURN:
		msnprintf(endstr, sizeof endstr, "Game courtesyadjourned by %s",
		    garray[g].winner == WHITE ? whiteguy : blackguy);
		break;
	case END_NOMATERIAL:
		msnprintf(endstr, sizeof endstr, "Game drawn because neither "
		    "player has mating material");
		break;
	case END_FLAGNOMATERIAL:
		msnprintf(endstr, sizeof endstr, "%s ran out of time and %s "
		    "has no material to mate",
		    garray[g].winner == WHITE ? blackguy : whiteguy,
		    garray[g].winner == WHITE ? whiteguy : blackguy);
		break;
	case END_ADJDRAW:
		msnprintf(endstr, sizeof endstr, "Game drawn by adjudication");
		break;
	case END_ADJWIN:
		msnprintf(endstr, sizeof endstr, "%s wins by adjudication",
		    garray[g].winner == WHITE ? whiteguy : blackguy);
		break;
	case END_ADJABORT:
		msnprintf(endstr, sizeof endstr, "Game aborted by "
		    "adjudication");
		break;
	default:
		msnprintf(endstr, sizeof endstr, "???????");
		break;
	}

	return endstr;
}

PUBLIC char *
EndSym(int g)
{
	static char *symbols[] = {
		"1-0",
		"0-1",
		"1/2-1/2",
		"*"
	};

	switch (garray[g].result) {
	case END_CHECKMATE:
	case END_RESIGN:
	case END_FLAG:
	case END_ADJWIN:
		return ((garray[g].winner == WHITE) ? symbols[0] : symbols[1]);
		break;
	case END_AGREEDDRAW:
	case END_BOTHFLAG:
	case END_REPETITION:
	case END_50MOVERULE:
	case END_STALEMATE:
	case END_NOMATERIAL:
	case END_FLAGNOMATERIAL:
	case END_ADJDRAW:
		return (symbols[2]);
		break;
	}

	return (symbols[3]);
}

PUBLIC char *
movesToString(int g, int pgn)
{
	char		*serv_loc = SERVER_LOCATION;
	char		*serv_name = SERVER_NAME;
	char		 tmp[160] = { '\0' };
	int		 i, col;
	int		 wr, br;
	struct tm	*tm_ptr = NULL;
	time_t		 curTime;

	wr = garray[g].white_rating;
	br = garray[g].black_rating;

	curTime = untenths(garray[g].timeOfStart);

	if (pgn) {
		msnprintf(gameString, sizeof gameString,
		    "\n[Event \"%s %s %s game\"]\n"
		    "[Site \"%s, %s\"]\n",
		    serv_name,
		    rstr[garray[g].rated],
		    bstr[garray[g].type],
		    serv_name,
		    serv_loc);

		if ((tm_ptr = localtime(&curTime)) != NULL) {
			strftime(tmp, sizeof(tmp),
			    "[Date \"%Y.%m.%d\"]\n"
			    "[Time \"%H:%M:%S\"]\n",
			    tm_ptr);
			mstrlcat(gameString, tmp, sizeof gameString);
		} else
			warn("%s: localtime", __func__);

		msnprintf(tmp, sizeof tmp,
		    "[Round \"-\"]\n"
		    "[White \"%s\"]\n"
		    "[Black \"%s\"]\n"
		    "[WhiteElo \"%d\"]\n"
		    "[BlackElo \"%d\"]\n",
		    garray[g].white_name,
		    garray[g].black_name,
		    wr, br);
		mstrlcat(gameString, tmp, sizeof gameString);

		msnprintf(tmp, sizeof tmp,
		    "[TimeControl \"%d+%d\"]\n"
		    "[Mode \"ICS\"]\n"
		    "[Result \"%s\"]\n\n",
		    garray[g].wInitTime / 10,
		    garray[g].wIncrement / 10,
		    EndSym(g));
		mstrlcat(gameString, tmp, sizeof gameString);

		col = 0;

		for (i = 0; i < garray[g].numHalfMoves; i++) {
			if (!(i % 2)) {
				if ((col += snprintf(tmp, sizeof tmp, "%d. ",
				    i / 2 + 1)) > 70) {
					mstrlcat(gameString, "\n",
					    sizeof gameString);
					col = 0;
				}

				mstrlcat(gameString, tmp, sizeof gameString);
			}

			if ((col += snprintf(tmp, sizeof tmp, "%s ",
			    (garray[g].status == GAME_EXAMINE)
			    ? garray[g].examMoveList[i].algString
			    : garray[g].moveList[i].algString)) > 70) {
				mstrlcat(gameString, "\n", sizeof gameString);
				col = 0;
			}

			mstrlcat(gameString, tmp, sizeof gameString);
		}

		mstrlcat(gameString, "\n", sizeof gameString);
	} else {
		/*
		 * !pgn
		 */

		msnprintf(gameString, sizeof gameString, "\n%s ",
		    garray[g].white_name);

		if (wr > 0) {
			msnprintf(tmp, sizeof tmp, "(%d) ", wr);
		} else {
			msnprintf(tmp, sizeof tmp, "(UNR) ");
		}

		mstrlcat(gameString, tmp, sizeof gameString);
		msnprintf(tmp, sizeof tmp, "vs. %s ", garray[g].black_name);
		mstrlcat(gameString, tmp, sizeof gameString);

		if (br > 0) {
			msnprintf(tmp, sizeof tmp, "(%d) ", br);
		} else {
			msnprintf(tmp, sizeof tmp, "(UNR) ");
		}

		mstrlcat(gameString, tmp, sizeof gameString);
		mstrlcat(gameString, "--- ", sizeof gameString);

		if ((tm_ptr = localtime(&curTime)) != NULL) {
			strftime(tmp, sizeof tmp, "%Y.%m.%d %H:%M:%S", tm_ptr);
			mstrlcat(gameString, tmp, sizeof gameString);
		} else
			warn("%s: localtime", __func__);

		if (garray[g].rated) {
			mstrlcat(gameString, "\nRated ", sizeof gameString);
		} else {
			mstrlcat(gameString, "\nUnrated ", sizeof gameString);
		}

		if (garray[g].type == TYPE_BLITZ) {
			mstrlcat(gameString, "Blitz ", sizeof(gameString));
		} else if (garray[g].type == TYPE_LIGHT) {
			mstrlcat(gameString, "Lighting ", sizeof(gameString));
		} else if (garray[g].type == TYPE_BUGHOUSE) {
			mstrlcat(gameString, "Bughouse ", sizeof(gameString));
		} else if (garray[g].type == TYPE_STAND) {
			mstrlcat(gameString, "Standard ", sizeof(gameString));
		} else if (garray[g].type == TYPE_WILD) {
			mstrlcat(gameString, "Wild ", sizeof(gameString));
		} else if (garray[g].type == TYPE_NONSTANDARD) {
			mstrlcat(gameString, "Non-standard ",
			    sizeof(gameString));
		} else {
			mstrlcat(gameString, "Untimed ", sizeof(gameString));
		}

		mstrlcat(gameString, "match, initial time: ",
		    sizeof gameString);

		if ((garray[g].bInitTime != garray[g].wInitTime) ||
		    (garray[g].wIncrement != garray[g].bIncrement)) {
			/*
			 * different starting times
			 */

			msnprintf(tmp, sizeof tmp, "%d minutes, increment: %d "
			    "seconds AND %d minutes, increment: %d seconds."
			    "\n\n",
			    garray[g].wInitTime / 600,
			    garray[g].wIncrement / 10,
			    garray[g].bInitTime / 600,
			    garray[g].bIncrement / 10);
		} else {
			msnprintf(tmp, sizeof tmp, "%d minutes, increment: "
			    "%d seconds.\n\n",
			    garray[g].wInitTime / 600,
			    garray[g].wIncrement / 10);
		}

		mstrlcat(gameString, tmp, sizeof gameString);
		msnprintf(tmp, sizeof tmp, "Move  %-19s%-19s\n",
		    garray[g].white_name,
		    garray[g].black_name);
		mstrlcat(gameString, tmp, sizeof gameString);
		mstrlcat(gameString, "----  ----------------   ----------------"
		    "\n", sizeof gameString);

		for (i = 0; i < garray[g].numHalfMoves; i += 2) {
			if (i + 1 < garray[g].numHalfMoves) {
				msnprintf(tmp, sizeof tmp, "%3d.  %-16s   ",
				    i / 2 + 1,
				    (garray[g].status == GAME_EXAMINE)
				    ? move_and_time(&garray[g].examMoveList[i])
				    : move_and_time(&garray[g].moveList[i]));

				mstrlcat(gameString, tmp, sizeof gameString);

				msnprintf(tmp, sizeof tmp, "%-16s\n",
				    (garray[g].status == GAME_EXAMINE)
				    ? move_and_time(&garray[g].examMoveList[i + 1])
				    : move_and_time(&garray[g].moveList[i + 1]));
			} else {
				msnprintf(tmp, sizeof tmp, "%3d.  %-16s\n",
				    i / 2 + 1,
				    (garray[g].status == GAME_EXAMINE)
				    ? move_and_time(&garray[g].examMoveList[i])
				    : move_and_time(&garray[g].moveList[i]));
			}

			mstrlcat(gameString, tmp, sizeof gameString);

			if (strlen(gameString) > GAME_STRING_LEN - 100)
				return gameString;	// Bug out if getting
							// close to filling this
							// string
		}

		mstrlcat(gameString, "      ", sizeof gameString);
	}

	msnprintf(tmp, sizeof tmp, "{%s} %s\n", EndString(g, 0), EndSym(g));
	mstrlcat(gameString, tmp, sizeof gameString);

	return gameString;
}

PUBLIC void
game_disconnect(int g, int p)
{
	game_ended(g, (garray[g].white == p) ? WHITE : BLACK,
	    END_LOSTCONNECTION);
}

PUBLIC int
CharToPiece(char c)
{
	switch (c) {
	case 'P':
		return W_PAWN;
	case 'p':
		return B_PAWN;
	case 'N':
		return W_KNIGHT;
	case 'n':
		return B_KNIGHT;
	case 'B':
		return W_BISHOP;
	case 'b':
		return B_BISHOP;
	case 'R':
		return W_ROOK;
	case 'r':
		return B_ROOK;
	case 'Q':
		return W_QUEEN;
	case 'q':
		return B_QUEEN;
	case 'K':
		return W_KING;
	case 'k':
		return B_KING;
	default:
		return NOPIECE;
	}
}

PUBLIC int
PieceToChar(int piece)
{
	switch (piece) {
	case W_PAWN:
		return 'P';
	case B_PAWN:
		return 'p';
	case W_KNIGHT:
		return 'N';
	case B_KNIGHT:
		return 'n';
	case W_BISHOP:
		return 'B';
	case B_BISHOP:
		return 'b';
	case W_ROOK:
		return 'R';
	case B_ROOK:
		return 'r';
	case W_QUEEN:
		return 'Q';
	case B_QUEEN:
		return 'q';
	case W_KING:
		return 'K';
	case B_KING:
		return 'k';
	default:
		return ' ';
	}
}

/* One line has everything on it */
PRIVATE int
WriteMoves(FILE *fp, move_t *m)
{
	int		 i;
	int		 piece, castle;
	int		 useFile = 0, useRank = 0, check = 0;
	unsigned long	 MoveInfo = (m->color == BLACK);

	castle = (m->moveString[0] == 'o');

	if (castle)
		piece = KING;
	else
		piece = piecetype(CharToPiece(m->moveString[0]));

#define ORIGINAL_CODE 0
#if ORIGINAL_CODE
	MoveInfo	= (MoveInfo <<= 3) | piece;
	MoveInfo	= (MoveInfo <<= 3) | m->fromFile;
	MoveInfo	= (MoveInfo <<= 3) | m->fromRank;
	MoveInfo	= (MoveInfo <<= 3) | m->toFile;
	MoveInfo	= (MoveInfo <<= 3) | m->toRank;
	MoveInfo	= (MoveInfo <<= 3) | (m->pieceCaptured & 7);
	MoveInfo	= (MoveInfo <<= 3) | (m->piecePromotionTo & 7);
	MoveInfo	= (MoveInfo <<= 1) | (m->enPassant != 0);
#else
	MoveInfo <<= 3;
	MoveInfo |= piece;

	MoveInfo <<= 3;
	MoveInfo |= m->fromFile;

	MoveInfo <<= 3;
	MoveInfo |= m->fromRank;

	MoveInfo <<= 3;
	MoveInfo |= m->toFile;

	MoveInfo <<= 3;
	MoveInfo |= m->toRank;

	MoveInfo <<= 3;
	MoveInfo |= (m->pieceCaptured & 7);

	MoveInfo <<= 3;
	MoveInfo |= (m->piecePromotionTo & 7);

	MoveInfo <<= 1;
	MoveInfo |= (m->enPassant != 0);
#endif

	/* Are we using from-file or from-rank in 'algString'? */

	if ((i = strlen(m->algString)) > 0)
		i -= 1;

	if (m->algString[i] == '+') {
		check = 1;
		i--;
	}

	if (piece != PAWN && !castle) {
		i -= 2;

		if (i < 0)
			return -1;
		if (m->algString[i] == 'x')
			i--;
		if (i < 0)
			return -1;
		if (isdigit(m->algString[i])) {
			useRank = 2;
			i--;
		}
		if (i < 0)
			return -1;

		useFile = (islower(m->algString[i]) ? 4 : 0);
	}

	MoveInfo = ((MoveInfo << 3) | useFile | useRank | check);

	fprintf(fp, "%lx %x %x\n", MoveInfo, m->tookTime, m->atTime);
	return 0;
}

PRIVATE int
ReadMove(FILE *fp, move_t *m)
{
	char	line[MAX_GLINE_SIZE] = { '\0' };

	if (fgets(line, sizeof line, fp) == NULL)
		return -1;

	_Static_assert(ARRAY_SIZE(m->moveString) > 7, "'moveString' too small");
	_Static_assert(ARRAY_SIZE(m->algString) > 7,  "'algString' too small");

	if (sscanf(line, "%d %d %d %d %d %d %d %d %d \"%7[^\"]\" \"%7[^\"]\" "
	    "%u %u\n",
	    &m->color,
	    &m->fromFile, &m->fromRank,
	    &m->toFile, &m->toRank,
	    &m->pieceCaptured,
	    &m->piecePromotionTo,
	    &m->enPassant,
	    &m->doublePawn,
	    m->moveString,
	    m->algString,
	    &m->atTime,
	    &m->tookTime) != 13)
		return -1;

	return 0;
}

PRIVATE void
WriteGameState(FILE *fp, game_state_t *gs)
{
	int	i, j;

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++)
			fprintf(fp, "%c", PieceToChar(gs->board[i][j]));
	}

	fprintf(fp, "%d %d %d %d %d %d",
		gs->wkmoved, gs->wqrmoved, gs->wkrmoved,
		gs->bkmoved, gs->bqrmoved, gs->bkrmoved);

	for (i = 0; i < 8; i++) {
		fprintf(fp, " %d %d", gs->ep_possible[0][i],
		    gs->ep_possible[1][i]);
	}

	fprintf(fp, " %d %d %d\n", gs->lastIrreversable, gs->onMove,
	    gs->moveNum);
}

PRIVATE int
ReadGameState(FILE *fp, game_state_t *gs, int version)
{
	int	i, j;
	int	pieceChar;
	int	wkmoved, wqrmoved, wkrmoved, bkmoved, bqrmoved, bkrmoved;

	if (version == 0) {
		for (i = 0; i < 8; i++) {
			for (j = 0; j < 8; j++) {
				if (fscanf(fp, "%d ", &gs->board[i][j]) != 1)
					return -1;
			}
		}
	} else {
		(void) getc(fp);	/* Skip past a newline. */

		for (i = 0; i < 8; i++) {
			for (j = 0; j < 8; j++) {
				if ((pieceChar = getc(fp)) == EOF)
					return -1;
				gs->board[i][j] = CharToPiece(pieceChar);
			}
		}
	}

	if (fscanf(fp, "%d %d %d %d %d %d",
	    &wkmoved, &wqrmoved, &wkrmoved,
	    &bkmoved, &bqrmoved, &bkrmoved) != 6)
		return -1;

	gs->wkmoved	= wkmoved;
	gs->wqrmoved	= wqrmoved;
	gs->wkrmoved	= wkrmoved;

	gs->bkmoved	= bkmoved;
	gs->bqrmoved	= bqrmoved;
	gs->bkrmoved	= bkrmoved;

	for (i = 0; i < 8; i++) {
		if (fscanf(fp, " %d %d", &gs->ep_possible[0][i],
		    &gs->ep_possible[1][i]) != 2)
			return -1;
	}

	if (fscanf(fp, " %d %d %d\n", &gs->lastIrreversable, &gs->onMove,
	    &gs->moveNum) != 3)
		return -1;
	return 0;
}

PUBLIC int
got_attr_value(int g, char *attr, char *value, FILE *fp, char *file)
{
	if (!strcmp(attr, "w_init:")) {
		garray[g].wInitTime = atoi(value);
	} else if (!strcmp(attr, "w_inc:")) {
		garray[g].wIncrement = atoi(value);
	} else if (!strcmp(attr, "b_init:")) {
		garray[g].bInitTime = atoi(value);
	} else if (!strcmp(attr, "b_inc:")) {
		garray[g].bIncrement = atoi(value);
	} else if (!strcmp(attr, "white_name:")) {
		mstrlcpy(garray[g].white_name, value,
		    sizeof(garray[g].white_name));
	} else if (!strcmp(attr, "black_name:")) {
		mstrlcpy(garray[g].black_name, value,
		    sizeof(garray[g].black_name));
	} else if (!strcmp(attr, "white_rating:")) {
		garray[g].white_rating = atoi(value);
	} else if (!strcmp(attr, "black_rating:")) {
		garray[g].black_rating = atoi(value);
	} else if (!strcmp(attr, "result:")) {
		garray[g].result = atoi(value);
	} else if (!strcmp(attr, "timestart:")) {
		garray[g].timeOfStart = atoi(value);
	} else if (!strcmp(attr, "w_time:")) {
		garray[g].wTime = atoi(value);
	} else if (!strcmp(attr, "b_time:")) {
		garray[g].bTime = atoi(value);
	} else if (!strcmp(attr, "clockstopped:")) {
		garray[g].clockStopped = atoi(value);
	} else if (!strcmp(attr, "rated:")) {
		garray[g].rated = atoi(value);
	} else if (!strcmp(attr, "private:")) {
		garray[g].private = atoi(value);
	} else if (!strcmp(attr, "type:")) {
		garray[g].type = atoi(value);
	} else if (!strcmp(attr, "halfmoves:")) {
		garray[g].numHalfMoves = atoi(value);

		if (garray[g].numHalfMoves == 0)
			return 0;

		garray[g].moveListSize = garray[g].numHalfMoves;
		garray[g].moveList = reallocarray(NULL, sizeof(move_t),
		    garray[g].moveListSize);

		if (garray[g].moveList == NULL)
			err(1, "%s: reallocarray", __func__);
		else
			malloc_count++;

		for (int i = 0; i < garray[g].numHalfMoves; i++) {
			if (ReadMove(fp, &garray[g].moveList[i])) {
				fprintf(stderr, "FICS: Trouble reading moves "
				    "from %s.\n", file);
				return -1;
			}
		}
	} else if (!strcmp(attr, "gamestate:")) {	// Value meaningless
		if (garray[g].status != GAME_EXAMINE &&
		    ReadGameState(fp, &garray[g].game_state, 0)) {
			fprintf(stderr, "FICS: Trouble reading game state "
			    "from %s.\n", file);
			return -1;
		}
	} else {
		fprintf(stderr, "FICS: Error bad attribute >%s< from file %s\n",
		    attr, file);
	}

	return 0;
}

PRIVATE int
ReadOneV1Move(FILE *fp, move_t *m)
{
	char		 PieceChar;
	int		 i;
	int		 useFile, useRank, check, piece;
	unsigned long	 MoveInfo;

	if (fscanf(fp, "%lx %x %x", &MoveInfo, &m->tookTime, &m->atTime) != 3)
		return -1;

	check = MoveInfo & 1;
	useRank = MoveInfo & 2;
	useFile = MoveInfo & 4;

	MoveInfo >>= 3;
	m->enPassant = (MoveInfo & 1);		// May have to negate later.

	MoveInfo >>= 1;
	m->piecePromotionTo = (MoveInfo & 7);	// May have to change color.

	MoveInfo >>= 3;
	m->pieceCaptured = (MoveInfo & 7);	// May have to change color.

	MoveInfo >>= 3;
	m->toRank = (MoveInfo & 7);

	MoveInfo >>= 3;
	m->toFile = (MoveInfo & 7);

	MoveInfo >>= 3;
	m->fromRank = (MoveInfo & 7);

	MoveInfo >>= 3;
	m->fromFile = (MoveInfo & 7);

	MoveInfo >>= 3;
	piece = (MoveInfo & 7);

	m->color = ((MoveInfo & 8) ? BLACK : WHITE);

	if (m->pieceCaptured != NOPIECE) {
		if (m->color == BLACK)
			m->pieceCaptured |= WHITE;
		else
			m->pieceCaptured |= BLACK;
	}

	if (piece == PAWN) {
		PieceChar = 'P';

		if ((m->toRank == 3 && m->fromRank == 1) ||
		    (m->toRank == 4 && m->fromRank == 6))
			m->doublePawn = m->toFile;
		else
			m->doublePawn = -1;

		if (m->pieceCaptured) {
			msnprintf(m->algString, sizeof m->algString, "%cx%c%d",
			    ('a' + m->fromFile),
			    ('a' + m->toFile),
			    (m->toRank + 1));
		} else {
			msnprintf(m->algString, sizeof m->algString, "%c%d",
			    ('a' + m->toFile),
			    (m->toRank + 1));
		}

		if (m->piecePromotionTo != 0) {
			if (m->piecePromotionTo == KNIGHT) {
				mstrlcat(m->algString, "=N",
				    sizeof(m->algString));
			} else if (m->piecePromotionTo == BISHOP) {
				mstrlcat(m->algString, "=B",
				    sizeof(m->algString));
			} else if (m->piecePromotionTo == ROOK) {
				mstrlcat(m->algString, "=R",
				    sizeof(m->algString));
			} else if (m->piecePromotionTo == QUEEN) {
				mstrlcat(m->algString, "=Q",
				    sizeof(m->algString));
			}

			m->piecePromotionTo |= m->color;
		}

		if (m->enPassant)
			m->enPassant = (m->toFile - m->fromFile);
	} else {
		m->doublePawn	= -1;
		PieceChar	= PieceToChar(piecetype(piece) | WHITE);

		if (PieceChar == 'K' && m->fromFile == 4 && m->toFile == 6) {
			mstrlcpy(m->algString, "O-O", sizeof(m->algString));
			mstrlcpy(m->moveString, "o-o", sizeof(m->moveString));
		} else if (PieceChar == 'K' &&
		    m->fromFile == 4 &&
		    m->toFile == 2) {
			mstrlcpy(m->algString, "O-O-O", sizeof(m->algString));
			mstrlcpy(m->moveString, "o-o-o", sizeof(m->moveString));
		} else {
			i = 0;
			m->algString[i++] = PieceChar;

			if (useFile)
				m->algString[i++] = 'a' + m->fromFile;
			if (useRank)
				m->algString[i++] = '1' + m->fromRank;
			if (m->pieceCaptured != 0)
				m->algString[i++] = 'x';

			m->algString[i++]	= ('a' + m->toFile);
			m->algString[i++]	= ('1' + m->toRank);
			m->algString[i]		= '\0';
		}
	}

	if (m->algString[0] != 'O') {
		int ret, too_long;

		ret = snprintf(m->moveString, sizeof m->moveString,
		    "%c/%c%d-%c%d",
		    PieceChar,
		    ('a' + m->fromFile),
		    (m->fromRank + 1),
		    ('a' + m->toFile),
		    (m->toRank + 1));

		too_long = (ret < 0 || (size_t)ret >= sizeof m->moveString);

		if (too_long) {
			fprintf(stderr, "FICS: %s: warning: "
			    "snprintf truncated\n", __func__);
		}
	}
	if (check)
		mstrlcat(m->algString, "+", sizeof m->algString);
	return 0;
}

PRIVATE int
ReadV1Moves(game *g, FILE *fp)
{
	g->moveListSize = g->numHalfMoves;
	g->moveList = reallocarray(NULL, sizeof(move_t), g->moveListSize);

	if (g->moveList == NULL)
		err(1, "%s: reallocarray", __func__);
	else
		malloc_count++;

	for (int i = 0; i < g->numHalfMoves; i++) {
		if (ReadOneV1Move(fp, &g->moveList[i]) == -1) {
			warnx("%s: failed to read move %d/%d", __func__, i,
			    g->numHalfMoves);
			return -1;
		}
	}

	return 0;
}

PRIVATE int
ReadV1GameFmt(game *g, FILE *fp, const char *file, int version)
{
	int		ret[3];
	long int	lval;

	_Static_assert(17 < ARRAY_SIZE(g->white_name), "Unexpected array size");
	_Static_assert(17 < ARRAY_SIZE(g->black_name), "Unexpected array size");

	ret[0] = fscanf(fp, "%17s %17s", g->white_name, g->black_name);
	ret[1] = fscanf(fp, "%d %d", &g->white_rating, &g->black_rating);
	ret[2] = fscanf(fp, "%d %d %d %d",
	    &g->wInitTime,
	    &g->wIncrement,
	    &g->bInitTime,
	    &g->bIncrement);
	if (ret[0] != 2 ||
	    ret[1] != 2 ||
	    ret[2] != 4) {
		warnx("%s: fscanf error: %s", __func__, file);
		return -1;
	}

	if (version < 3 && !g->bInitTime)
		g->bInitTime = g->wInitTime;

	if (fscanf(fp, "%ld", &lval) != 1) {
		warnx("%s: %s: failed to get time of start", __func__, file);
		return -1;
	} else
		g->timeOfStart = lval;

	if (fscanf(fp, "%d %d", &g->wTime, &g->bTime) != 2) {
		warnx("%s: %s: failed to get 'wTime' and 'bTime'", __func__,
		    file);
		return -1;
	}

	if (version > 1) {
		if (fscanf(fp, "%d %d", &g->result, &g->winner) != 2) {
			warnx("%s: %s: failed to get 'result' nor 'winner'",
			    __func__, file);
			return -1;
		}
	} else {
		if (fscanf(fp, "%d", &g->result) != 1) {
			warnx("%s: %s: failed to get 'result'",
			    __func__, file);
			return -1;
		}
	}

	ret[0] = fscanf(fp, "%d %d %d %d", &g->private, &g->type, &g->rated,
	    &g->clockStopped);
	ret[1] = fscanf(fp, "%d", &g->numHalfMoves);
	if (ret[0] != 4 || ret[1] != 1) {
		warnx("%s: fscanf error: %s", __func__, file);
		return -1;
	} else if (g->numHalfMoves < 0 || (size_t)g->numHalfMoves >
	    INT_MAX / sizeof(move_t)) {
		warnx("%s: warning: num half moves out-of-bounds (%d)",
		    __func__,
		    g->numHalfMoves);
		return -1;
	}

	if (ReadV1Moves(g, fp) != 0) {
		warnx("%s: failed to read moves: %s", __func__, file);
		return -1;
	}

	if (g->status != GAME_EXAMINE &&
	    ReadGameState(fp, &g->game_state, version)) {
		fprintf(stderr, "FICS: Trouble reading game state from %s.\n",
		    file);
		return -1;
	}

	return 0;
}

PUBLIC int
ReadGameAttrs(FILE *fp, char *fname, int g)
{
	char	*attr, *value;
	char	 line[MAX_GLINE_SIZE] = { '\0' };
	int	 len = 0;
	int	 version = 0;

	if (fgets(line, sizeof line, fp) == NULL) {
		warnx("%s: fgets error", __func__);
		return -1;
	}

	if (line[0] == 'v') {
		if (sscanf(line, "%*c %d", &version) != 1)
			warn("%s: failed to get version", __func__);
	}

	if (version > 0) {
		if (ReadV1GameFmt(&garray[g], fp, fname, version) == -1)
			return -1;
	} else {
		do {
			if ((len = strlen(line)) <= 1) {
				if (fgets(line, sizeof line, fp) == NULL)
					break;
				continue;
			}

			line[len - 1] = '\0';
			attr = eatwhite(line);

			if (attr[0] == '#')
				continue;	// Comment

			value = eatword(attr);

			if (!*value) {
				fprintf(stderr, "FICS: Error reading file %s\n",
				    fname);
				if (fgets(line, sizeof line, fp) == NULL)
					break;
				continue;
			}

			*value = '\0';
			value++;
			value = eatwhite(value);

			if (!*value) {
				fprintf(stderr, "FICS: Error reading file %s\n",
				    fname);
				if (fgets(line, sizeof line, fp) == NULL)
					break;
				continue;
			}

			stolower(attr);

			if (got_attr_value(g, attr, value, fp, fname))
				return -1;

			if (fgets(line, sizeof line, fp) == NULL)
				break;
		} while (!feof(fp));
	}

	if (!(garray[g].bInitTime))
		garray[g].bInitTime = garray[g].wInitTime;
	return 0;
}

PUBLIC int
game_read(int g, int wp, int bp)
{
	FILE	*fp;
	char	 fname[MAX_FILENAME_SIZE] = { '\0' };

	garray[g].white = wp;
	garray[g].black = bp;

#if 0
	garray[g].old_white = -1;
	garray[g].old_black = -1;
#endif

	garray[g].moveListSize = 0;
	garray[g].game_state.gameNum = g;

	mstrlcpy(garray[g].white_name, parray[wp].name,
	    sizeof(garray[g].white_name));
	mstrlcpy(garray[g].black_name, parray[bp].name,
	    sizeof(garray[g].black_name));

	if (garray[g].type == TYPE_BLITZ) {
		garray[g].white_rating = parray[wp].b_stats.rating;
		garray[g].black_rating = parray[bp].b_stats.rating;
	} else if (garray[g].type == TYPE_WILD) {
		garray[g].white_rating = parray[wp].w_stats.rating;
		garray[g].black_rating = parray[bp].w_stats.rating;
	} else if (garray[g].type == TYPE_LIGHT) {
		garray[g].white_rating = parray[wp].l_stats.rating;
		garray[g].black_rating = parray[bp].l_stats.rating;
	} else if (garray[g].type == TYPE_BUGHOUSE) {
		garray[g].white_rating = parray[wp].bug_stats.rating;
		garray[g].black_rating = parray[bp].bug_stats.rating;
	} else {
		garray[g].white_rating = parray[wp].s_stats.rating;
		garray[g].black_rating = parray[bp].s_stats.rating;
	}

	msnprintf(fname, sizeof fname, "%s/%c/%s-%s", adj_dir,
	    parray[wp].login[0], parray[wp].login, parray[bp].login);
	fp = fopen(fname, "r");

	if (!fp) {
		return -1;
	}

	if (ReadGameAttrs(fp, fname, g) < 0) {
		fclose(fp);
		return -1;
	}

	fclose(fp);

	if (garray[g].result == END_ADJOURN || garray[g].result ==
	    END_COURTESYADJOURN)
		garray[g].result = END_NOTENDED;

	garray[g].status = GAME_ACTIVE;
	garray[g].startTime = tenth_secs();
	garray[g].lastMoveTime = garray[g].startTime;
	garray[g].lastDecTime = garray[g].startTime;

	// Need to do notification and pending cleanup
	return 0;
}

PUBLIC int
game_delete(int wp, int bp)
{
	char	fname[MAX_FILENAME_SIZE];
	char	lname[MAX_FILENAME_SIZE];

	msnprintf(fname, sizeof fname, "%s/%c/%s-%s", adj_dir,
	    parray[wp].login[0], parray[wp].login, parray[bp].login);
	msnprintf(lname, sizeof lname, "%s/%c/%s-%s", adj_dir,
	    parray[bp].login[0], parray[wp].login, parray[bp].login);

	unlink(fname);
	unlink(lname);
	return 0;
}

PRIVATE void
WriteGameFile(FILE *fp, int g)
{
	game		*gg = &garray[g];
	long int	 lval;
	player		*bp = &parray[gg->black];
	player		*wp = &parray[gg->white];

	fprintf(fp, "v %d\n", GAMEFILE_VERSION);
	fprintf(fp, "%s %s\n", wp->name, bp->name);
	fprintf(fp, "%d %d\n", gg->white_rating, gg->black_rating);
	fprintf(fp, "%d %d %d %d\n", gg->wInitTime, gg->wIncrement,
	    gg->bInitTime, gg->bIncrement);

	lval = gg->timeOfStart;
	fprintf(fp, "%ld\n", lval);

#ifdef TIMESEAL
	fprintf(fp, "%d %d\n",
	    (con[wp->socket].timeseal ? (gg->wRealTime / 100) : gg->wTime),
	    (con[bp->socket].timeseal ? (gg->bRealTime / 100) : gg->bTime));
#endif

	fprintf(fp, "%d %d\n", gg->result, gg->winner);
	fprintf(fp, "%d %d %d %d\n", gg->private, gg->type, gg->rated,
	    gg->clockStopped);
	fprintf(fp, "%d\n", gg->numHalfMoves);

	for (int i = 0; i < garray[g].numHalfMoves; i++)
		WriteMoves(fp, &garray[g].moveList[i]);

	WriteGameState(fp, &garray[g].game_state);
}

PUBLIC int
game_save(int g)
{
	FILE	*fp;
	char	 fname[MAX_FILENAME_SIZE];
	char	 lname[MAX_FILENAME_SIZE];
	game	*gg = &garray[g];
	player	*wp, *bp;

	wp	= &parray[gg->white];
	bp	= &parray[gg->black];

	msnprintf(fname, sizeof fname, "%s/%c/%s-%s", adj_dir, wp->login[0],
	    wp->login, bp->login);
	msnprintf(lname, sizeof lname, "%s/%c/%s-%s", adj_dir, bp->login[0],
	    wp->login, bp->login);

	fp = fopen(fname, "w");

	if (!fp) {
		fprintf(stderr, "FICS: Problem opening file %s for write\n",
		    fname);
		return -1;
	}

	WriteGameFile(fp, g);

#if 0
	fprintf(fp, "W_Init: %d\n", garray[g].wInitTime);
	fprintf(fp, "W_Inc: %d\n", garray[g].wIncrement);
	fprintf(fp, "B_Init: %d\n", garray[g].bInitTime);
	fprintf(fp, "B_Inc: %d\n", garray[g].bIncrement);
	fprintf(fp, "white_name: %s\n", wp->name);
	fprintf(fp, "black_name: %s\n", bp->name);
	fprintf(fp, "white_rating: %d\n", garray[g].white_rating);
	fprintf(fp, "black_rating: %d\n", garray[g].black_rating);
	fprintf(fp, "result: %d\n", garray[g].result);
	fprintf(fp, "TimeStart: %d\n", (int) garray[g].timeOfStart);
	fprintf(fp, "W_Time: %d\n", garray[g].wTime);
	fprintf(fp, "B_Time: %d\n", garray[g].bTime);
	fprintf(fp, "ClockStopped: %d\n", garray[g].clockStopped);
	fprintf(fp, "Rated: %d\n", garray[g].rated);
	fprintf(fp, "Private: %d\n", garray[g].private);
	fprintf(fp, "Type: %d\n", garray[g].type);
	fprintf(fp, "HalfMoves: %d\n", garray[g].numHalfMoves);

	for (int i = 0; i < garray[g].numHalfMoves; i++)
		WriteMoves(fp, &garray[g].moveList[i]);

	fprintf(fp, "GameState: IsNext\n");
	WriteGameState(fp, &garray[g].game_state);
#endif

	fclose(fp);

	/*
	 * Create link for easier stored game finding
	 */
	if (bp->login[0] != wp->login[0] &&
	    link(fname, lname) != 0)
		warn("%s: link() error", __func__);
	return 0;
}

PRIVATE long int
OldestHistGame(char *login)
{
	FILE		*fp;
	char		 pFile[MAX_FILENAME_SIZE] = { '\0' };
	long int	 when;

	msnprintf(pFile, sizeof pFile, "%s/player_data/%c/%s.%s", stats_dir,
	    login[0], login, STATS_GAMES);

	fp = fopen(pFile, "r");

	if (fp == NULL) {
		msnprintf(pFile, sizeof pFile, "%s/player_data/%c/.rem.%s.%s",
		    stats_dir, login[0], login, STATS_GAMES);
		fp = fopen(pFile, "r");
	}

	if (fp != NULL) {
		if (fscanf(fp, "%*d %*c %*d %*c %*d %*s %*s %*d %*d %*d %*d "
		    "%*s %*s %ld", &when) != 1) {
			warnx("%s: %s: failed to read 'when'", __func__,
			    &pFile[0]);
			fclose(fp);
			return 0L;
		}
		fclose(fp);
		return when;
	} else
		return 0L;
}

PRIVATE void
RemoveHistGame(char *file, int maxlines)
{
	FILE		*fp;
	char		 GameFile[MAX_FILENAME_SIZE] = { '\0' };
	char		 Opponent[MAX_LOGIN_NAME + 1] = { '\0' };
	char		 line[MAX_LINE_SIZE] = { '\0' };
	int		 count = 0;
	long int	 When, oppWhen;

	_Static_assert(20 < ARRAY_SIZE(Opponent), "Not within bounds");
	When = oppWhen = 0;

	if ((fp = fopen(file, "r")) == NULL) {
		return;
	} else if (fgets(line, ARRAY_SIZE(line), fp) == NULL) {
		warnx("%s: fgets error (file: %s)", __func__, file);
		fclose(fp);
		return;
	} else if (sscanf(line, "%*d %*c %*d %*c %*d %20s %*s %*d %*d %*d "
	    "%*d %*s %*s %ld", Opponent, &When) != 2) {
		warnx("%s: unexpected initial line (file: %s)", __func__, file);
		fclose(fp);
		return;
	}

	count++;

	while (fgets(line, ARRAY_SIZE(line), fp) != NULL)
		count++;

	fclose(fp);
	stolower(Opponent);

	if (count > maxlines) {
		truncate_file(file, maxlines);
		oppWhen = OldestHistGame(Opponent);

		if (oppWhen > When || oppWhen <= 0L) {
			msnprintf(GameFile, sizeof GameFile, "%s/%ld/%ld",
			    hist_dir, (When % 100), When);
			unlink(GameFile);
		}
	}
}

PUBLIC void
RemHist(char *who)
{
	FILE		*fp;
	char		 Opp[MAX_LOGIN_NAME] = { '\0' };
	char		 fName[MAX_FILENAME_SIZE] = { '\0' };
	long int	 When, oppWhen;

	msnprintf(fName, sizeof fName, "%s/player_data/%c/%s.%s", stats_dir,
	    who[0], who, STATS_GAMES);

	if ((fp = fopen(fName, "r")) != NULL) {
		long int iter_no = 0;

		while (!feof(fp) && !ferror(fp)) {
			const int ret = fscanf(fp, "%*d %*c %*d %*c %*d %19s "
			    "%*s %*d %*d %*d %*d %*s %*s %ld\n", Opp, &When);
			if (ret != 2) {
				warnx("%s: fscanf() error (%s:%ld)", __func__,
				    fName, iter_no);
//				iter_no++;
				break;
			}

			stolower(Opp);
			oppWhen = OldestHistGame(Opp);

			if (oppWhen > When || oppWhen <= 0L) {
				char histfile[MAX_FILENAME_SIZE] = { '\0' };

				msnprintf(histfile, sizeof histfile,
				    "%s/%ld/%ld", hist_dir, (When % 100), When);
				if (unlink(histfile) != 0) {
					warn("%s: unlink(%s)", __func__,
					    histfile);
				}
			}

			iter_no++;
		}

		fclose(fp);
	}
}

PRIVATE void
write_g_out(int g, char *file, int maxlines, int isDraw, char *EndSymbol,
    char *name, time_t *now)
{
	FILE	*fp;
	char	*goteco;
	char	 cResult;
	char	 tmp[2048] = { '\0' };
	char	*ptmp = tmp;
	char	 type[4];
	int	 count = -1;
	int	 wp, bp;
	int	 wr, br;

	wp	= garray[g].white;
	bp	= garray[g].black;

	if (garray[g].private) {
		type[0] = 'p';
	} else {
		type[0] = ' ';
	}

	if (garray[g].type == TYPE_BLITZ) {
		wr = parray[wp].b_stats.rating;
		br = parray[bp].b_stats.rating;

		type[1] = 'b';
	} else if (garray[g].type == TYPE_WILD) {
		wr = parray[wp].w_stats.rating;
		br = parray[bp].w_stats.rating;

		type[1] = 'w';
	} else if (garray[g].type == TYPE_STAND) {
		wr = parray[wp].s_stats.rating;
		br = parray[bp].s_stats.rating;

		type[1] = 's';
	} else if (garray[g].type == TYPE_LIGHT) {
		wr = parray[wp].l_stats.rating;
		br = parray[bp].l_stats.rating;

		type[1] = 'l';
	} else if (garray[g].type == TYPE_BUGHOUSE) {
		wr = parray[wp].bug_stats.rating;
		br = parray[bp].bug_stats.rating;

		type[1] = 'd';
	} else {
		wr = 0;
		br = 0;

		if (garray[g].type == TYPE_NONSTANDARD)
			type[1] = 'n';
		else
			type[1] = 'u';
	}

	if (garray[g].rated) {
		type[2] = 'r';
	} else {
		type[2] = 'u';
	}

	type[3] = '\0';

	if ((fp = fopen(file, "r")) != NULL) {
		while (fgets(tmp, sizeof tmp, fp) != NULL) {
			/* null */;
		}
		if (sscanf(ptmp, "%d", &count) != 1)
			warnx("%s: failed to read 'count'", __func__);
		fclose(fp);
	}

	count = (count + 1) % 100;

	if ((fp = fopen(file, "a")) == NULL)
		return;
	goteco = getECO(g);

	/*
	 * Counter
	 * Result
	 * MyRating
	 * MyColor
	 * OppRating
	 * OppName [pbr 2 12 2 12]
	 * ECO
	 * End
	 * Date
	 */
	if (name == parray[wp].name) {
		if (isDraw)
			cResult = '=';
		else if (garray[g].winner == WHITE)
			cResult = '+';
		else
			cResult = '-';

		fprintf(fp, "%d %c %d W %d %s %s %d %d %d %d %s %s %ld\n",
		    count, cResult, wr, br, parray[bp].name, type,
		    garray[g].wInitTime, garray[g].wIncrement,
		    garray[g].bInitTime, garray[g].bIncrement,
		    goteco,
		    EndSymbol,
		    (long int) *now);
	} else {
		if (isDraw)
			cResult = '=';
		else if (garray[g].winner == BLACK)
			cResult = '+';
		else
			cResult = '-';

		fprintf(fp, "%d %c %d B %d %s %s %d %d %d %d %s %s %ld\n",
		    count, cResult, br, wr, parray[wp].name, type,
		    garray[g].wInitTime, garray[g].wIncrement,
		    garray[g].bInitTime, garray[g].bIncrement,
		    goteco,
		    EndSymbol,
		    (long int) *now);
	}

	fclose(fp);
	RemoveHistGame(file, maxlines);
}

/*
 * Find from_spot in journal list - return 0 if corrupted
 */
PUBLIC int
journal_get_info(struct JGI_context *ctx, const char *fname)
{
	FILE	*fp;
	char	 count;

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "Corrupt journal file! %s\n", fname);
		pprintf(ctx->p, "The journal file is corrupt! See an admin.\n");
		return 0;
	}

	while (!feof(fp)) {
		_Static_assert(ARRAY_SIZE(ctx->WhiteName) > 20,
		    "'WhiteName' too small");
		_Static_assert(ARRAY_SIZE(ctx->BlackName) > 20,
		    "'BlackName' too small");

		_Static_assert(ARRAY_SIZE(ctx->type) > 99,   "'type' too small");
		_Static_assert(ARRAY_SIZE(ctx->eco) > 99,    "'eco' too small");
		_Static_assert(ARRAY_SIZE(ctx->ending) > 99, "'ending' too small");
		_Static_assert(ARRAY_SIZE(ctx->result) > 99, "'result' too small");

		if (fscanf(fp, "%c %20s %d %20s %d %99s %d %d %99s %99s %99s\n",
		    &count,
		    ctx->WhiteName, &ctx->WhiteRating,
		    ctx->BlackName, &ctx->BlackRating,
		    ctx->type,
		    &ctx->t, &ctx->i,
		    ctx->eco,
		    ctx->ending,
		    ctx->result) != 11) {
			fprintf(stderr, "FICS: Error in journal info format. "
			    "%s\n", fname);
			pprintf(ctx->p, "The journal file is corrupt! Error in "
			    "internal format.\n");
			fclose(fp);
			return 0;
		}

		if (tolower(count) == ctx->from_spot) {
			fclose(fp);
			return 1;
		}
	}

	fclose(fp);
	return 0;
}

PUBLIC void
addjournalitem(int p, char count2, char *WhiteName2, int WhiteRating2,
    char *BlackName2, int BlackRating2, char *type2, int t2, int i2,
    char *eco2, char *ending2, char *result2, char *fname)
{
	FILE	*fp;
	FILE	*fp2;
	char	 BlackName[MAX_LOGIN_NAME + 1] = { '\0' };
	char	 WhiteName[MAX_LOGIN_NAME + 1] = { '\0' };
	char	 count;
	char	 eco[100] = { '\0' };
	char	 ending[100] = { '\0' };
	char	 fname2[MAX_FILENAME_SIZE] = { '\0' };
	char	 result[100] = { '\0' };
	char	 type[100] = { '\0' };
	int	 WhiteRating, BlackRating;
	int	 have_output = 0;
	int	 t, i;

	mstrlcpy(fname2, fname, sizeof fname2);
	mstrlcat(fname2, ".w", sizeof fname2);

	if ((fp2 = fopen(fname2, "w")) == NULL) {
		fprintf(stderr, "FICS: Problem opening file %s for write\n",
		    fname);
		pprintf(p, "Couldn't update journal! Report this to an admin."
		    "\n");
		return;
	}

	if ((fp = fopen(fname, "r")) == NULL) { // Empty?
		fprintf(fp2, "%c %s %d %s %d %s %d %d %s %s %s\n",
		    count2,
		    WhiteName2, WhiteRating2,
		    BlackName2, BlackRating2,
		    type2,
		    t2, i2,
		    eco2,
		    ending2,
		    result2);
		fclose(fp2);
		xrename(__func__, fname2, fname);
		return;
	} else {
		_Static_assert(ARRAY_SIZE(WhiteName) > 19,
		    "'WhiteName' too small");
		_Static_assert(ARRAY_SIZE(BlackName) > 19,
		    "'BlackName' too small");

		_Static_assert(ARRAY_SIZE(type) > 99,   "'type' too small");
		_Static_assert(ARRAY_SIZE(eco) > 99,    "'eco' too small");
		_Static_assert(ARRAY_SIZE(ending) > 99, "'ending' too small");
		_Static_assert(ARRAY_SIZE(result) > 99, "'result' too small");

		while (!feof(fp)) {
			if (fscanf(fp, "%c %19s %d %19s %d %99s %d %d %99s "
			    "%99s %99s\n",
			    &count,
			    WhiteName, &WhiteRating,
			    BlackName, &BlackRating,
			    type,
			    &t, &i,
			    eco,
			    ending,
			    result) != 11) {
				fprintf(stderr, "FICS: Error in journal info "
				    "format - aborting. %s\n", fname);
				fclose(fp);
				fclose(fp2);
				return;
			}

			if ((count >= count2) && (!have_output)) {
				fprintf(fp2, "%c %s %d %s %d %s %d %d %s %s %s\n",
				    count2,
				    WhiteName2, WhiteRating2,
				    BlackName2, BlackRating2,
				    type2,
				    t2, i2,
				    eco2,
				    ending2,
				    result2);
				have_output = 1;
			}

			if (count != count2) {
				fprintf(fp2, "%c %s %d %s %d %s %d %d %s %s %s"
				    "\n",
				    count,
				    WhiteName, WhiteRating,
				    BlackName, BlackRating,
				    type,
				    t, i,
				    eco,
				    ending,
				    result);
			}
		}

		if (!have_output) {	// Haven't written yet
			fprintf(fp2, "%c %s %d %s %d %s %d %d %s %s %s\n",
			    count2,
			    WhiteName2, WhiteRating2,
			    BlackName2, BlackRating2,
			    type2,
			    t2, i2,
			    eco2,
			    ending2,
			    result2);
		}
	}

	fclose(fp);
	fclose(fp2);

	xrename(__func__, fname2, fname);
}

PUBLIC int
pjournal(int p, int p1, char *fname)
{
	FILE	*fp;
	char	 BlackName[MAX_LOGIN_NAME + 1] = { '\0' };
	char	 WhiteName[MAX_LOGIN_NAME + 1] = { '\0' };
	char	 count;
	char	 eco[100] = { '\0' };
	char	 ending[100] = { '\0' };
	char	 result[100] = { '\0' };
	char	 type[100] = { '\0' };
	int	 WhiteRating, BlackRating;
	int	 t, i;

	if ((fp = fopen(fname, "r")) == NULL) {
		pprintf(p, "Sorry, no journal information available.\n");
		return COM_OK;
	}

	pprintf(p, "Journal for %s:\n", parray[p1].name);
	pprintf(p, "   White         Rating  Black         Rating  "
	    "Type         ECO End Result\n");

	_Static_assert(ARRAY_SIZE(WhiteName) > 19, "'WhiteName' too small");
	_Static_assert(ARRAY_SIZE(BlackName) > 19, "'BlackName' too small");

	_Static_assert(ARRAY_SIZE(type) > 99,   "'type' too small");
	_Static_assert(ARRAY_SIZE(eco) > 99,    "'eco' too small");
	_Static_assert(ARRAY_SIZE(ending) > 99, "'ending' too small");
	_Static_assert(ARRAY_SIZE(result) > 99, "'result' too small");

	while (!feof(fp)) {
		if (fscanf(fp, "%c %19s %d %19s %d %99s %d %d %99s %99s %99s\n",
		    &count,
		    WhiteName, &WhiteRating,
		    BlackName, &BlackRating,
		    type,
		    &t, &i,
		    eco,
		    ending,
		    result) != 11) {
			fprintf(stderr, "FICS: Error in journal info format. "
			    "%s\n", fname);
			fclose(fp);
			return COM_OK;
		}

		WhiteName[13]	= '\0'; // only first 13 chars in name
		BlackName[13]	= '\0';

		pprintf(p, "%c: %-13s %4d    %-13s %4d    [%3s%3d%4d] %s %3s "
		    "%-7s\n",
		    count, WhiteName, WhiteRating,
		    BlackName, BlackRating,
		    type, (t / 600), (i / 10), eco, ending,
		    result);
	}

	fclose(fp);
	return COM_OK;
}

PUBLIC int
pgames(int p, int p1, char *fname)
{
	FILE	*fp;
	char	 MyColor[2] = { 0,0 };
	char	 OppName[MAX_LOGIN_NAME + 1] = { '\0' };
	char	 eco[100] = { '\0' };
	char	 ending[100] = { '\0' };
	char	 result[2] = { 0,0 }; // XXX: right size?
	char	 type[100] = { '\0' };
	int	 MyRating, OppRating;
	int	 count;
	int	 wt, wi, bt, bi;
	time_t	 t;

	if ((fp = fopen(fname, "r")) == NULL) {
		pprintf(p, "Sorry, no game information available.\n");
		return COM_OK;
	}

	pprintf(p, "History for %s:\n", parray[p1].name);
	pprintf(p, "                  Opponent      Type         "
	    "ECO End Date\n");

	_Static_assert(ARRAY_SIZE(result) > 1,   "'result' too small");
	_Static_assert(ARRAY_SIZE(MyColor) > 1,  "'MyColor' too small");
	_Static_assert(ARRAY_SIZE(OppName) > 19, "'OppName' too small");
	_Static_assert(ARRAY_SIZE(type) > 99,    "'type' too small");
	_Static_assert(ARRAY_SIZE(eco) > 99,     "'eco' too small");
	_Static_assert(ARRAY_SIZE(ending) > 99,  "'ending' too small");

	while (!feof(fp)) {
		if (fscanf(fp, "%d %1s %d %1s %d %19s %99s %d %d %d %d %99s "
		    "%99s %ld\n",
		    &count, result, &MyRating, MyColor,
		    &OppRating, OppName,
		    type,
		    &wt, &wi,
		    &bt, &bi,
		    eco,
		    ending,
		    (long int *)&t) != 14) {
			fprintf(stderr, "FICS: Error in games info format. "
			    "%s\n", fname);
			fclose(fp);
			return COM_OK;
		}

		OppName[13] = '\0'; // only first 13 chars in name

		pprintf(p, "%2d: %s %4d %s %4d %-13s [%3s%3d%4d] %s %3s %s",
		    count, result, MyRating, MyColor,
		    OppRating, OppName,
		    type, (wt / 600), (wi / 10), eco, ending,
		    ctime(&t));
	}

	fclose(fp);
	return COM_OK;
}

PUBLIC void
game_write_complete(int g, int isDraw, char *EndSymbol)
{
	FILE	*fp = NULL;
	char	 fname[MAX_FILENAME_SIZE] = { '\0' };
	int	 fd = -1;
	int	 wp = garray[g].white, bp = garray[g].black;
	time_t	 now = time(NULL);

	do {
		msnprintf(fname, sizeof fname, "%s/%ld/%ld",
		    hist_dir,
		    (long int)(now % 100),
		    (long int)now);
		errno = 0;
		fd = open(fname, (O_WRONLY | O_CREAT | O_EXCL), 0644);
		if (fd == -1 && errno == EEXIST)
			now++;
	} while (fd == -1 && errno == EEXIST);

	if (fd >= 0) {
		if ((fp = fdopen(fd, "w")) != NULL) {
			WriteGameFile(fp, g);
			fclose(fp);
		} else {
			fprintf(stderr, "Trouble writing history file %s",
			    fname);
		}

		close(fd);
	}

	msnprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s",
	    stats_dir,
	    parray[wp].login[0],
	    parray[wp].login,
	    STATS_GAMES);
	write_g_out(g, fname, 10, isDraw, EndSymbol, parray[wp].name, &now);

	msnprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s",
	    stats_dir,
	    parray[bp].login[0],
	    parray[bp].login,
	    STATS_GAMES);
	write_g_out(g, fname, 10, isDraw, EndSymbol, parray[bp].name, &now);
}

PUBLIC int
game_count(void)
{
	int	g, count = 0;

	for (g = 0; g < g_num; g++) {
		if (garray[g].status == GAME_ACTIVE ||
		    garray[g].status == GAME_EXAMINE)
			count++;
	}

	if (count > game_high)
		game_high = count;
	return count;
}

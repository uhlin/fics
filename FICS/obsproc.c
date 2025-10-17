/* obsproc.c
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
   Markus Uhlin			23/12/13	Fixed bugs
   Markus Uhlin			23/12/13	Reformatted functions
   Markus Uhlin			24/04/28	Completed reformatting
   Markus Uhlin			24/04/28	Replaced unbounded string
						handling functions.
   Markus Uhlin			24/04/29	Fixed compiler warnings
   Markus Uhlin			24/07/07	Fixed unhandled return values of
						fscanf().
   Markus Uhlin			24/12/02	Improved old_mail_moves()
   Markus Uhlin			25/01/18	Fixed -Wshadow
   Markus Uhlin			25/03/15	Fixed possible buffer overflow
						in FindHistory2().
   Markus Uhlin			25/04/06	Fixed Clang Tidy warnings.
   Markus Uhlin			25/10/17	Replaced system() with
						fics_copyfile().
*/

#include "stdinclude.h"
#include "common.h"

#include <err.h>
#include <limits.h>
#include <stdlib.h>

#include "command.h"
#include "comproc.h"
#include "config.h"
#include "copyfile.h"
#include "eco.h"
#include "ficsmain.h"
#include "formula.h"
#include "gamedb.h"
#include "gameproc.h"
#include "matchproc.h"
#include "maxxes-utils.h"
#include "movecheck.h"
#include "network.h"
#include "obsproc.h"
#include "playerdb.h"
#include "ratings.h"
#include "rmalloc.h"
#include "utils.h"

PUBLIC int
GameNumFromParam(int p, int *p1, parameter *param)
{
	if (param->type == TYPE_WORD) {
		*p1 = player_find_part_login(param->val.word);

		if (*p1 < 0) {
			pprintf(p, "No user named \"%s\" is logged in.\n",
			    param->val.word);
			return -1;
		}

		if (parray[*p1].game < 0) {
			pprintf(p, "%s is not playing a game.\n",
			    parray[*p1].name);
		}

		return parray[*p1].game;
	} else { // Must be an integer
		*p1 = -1;

		if (param->val.integer <= 0) {
			pprintf(p, "%d is not a valid game number.\n",
			    param->val.integer);
		}

		return (param->val.integer - 1);
	}
}

PRIVATE int
gamesortfunc(const void *i, const void *j)
{
	const int	x = *(int *)i;
	const int	y = *(int *)j;

	/*
	 * examine mode games moved to top of "games" output
	 */
	return (GetRating(&parray[garray[x].white],
	    garray[x].type) +
	    GetRating(&parray[garray[x].black],
	    garray[x].type) -
	    (garray[x].status == GAME_EXAMINE ? 10000 : 0) -
	    GetRating(&parray[garray[y].white],
	    garray[y].type) -
	    GetRating(&parray[garray[y].black],
	    garray[y].type) +
	    (garray[y].status == GAME_EXAMINE ? 10000 : 0));
}

PUBLIC int
com_games(int p, param_list param)
{
	char	*s = NULL;
	int	*sortedgames;	// for qsort
	int	 count = 0;
	int	 i, j;
	int	 selected = 0;
	int	 slen = 0;
	int	 totalcount;
	int	 wp, bp;
	int	 ws, bs;

	totalcount = game_count();

	if (totalcount == 0) {
		pprintf(p, "There are no games in progress.\n");
	} else {
		// for qsort
		sortedgames = reallocarray(NULL, totalcount, sizeof(int));

		if (sortedgames == NULL)
			err(1, "%s", __func__);
		else
			malloc_count++;

		if (param[0].type == TYPE_WORD) {
			s	= param[0].val.word;
			slen	= strlen(s);

			if ((selected = atoi(s)) < 0)
				selected = 0;
		}

		for (i = 0; i < g_num; i++) {
			if (garray[i].status != GAME_ACTIVE &&
			    garray[i].status != GAME_EXAMINE)
				continue;
			if ((selected) && (selected != i + 1))
				continue;	// not selected game number

			wp	= garray[i].white;
			bp	= garray[i].black;

			UNUSED_VAR(wp);
			UNUSED_VAR(bp);

			if ((!selected) &&
			    s &&
			    strncasecmp(s, garray[i].white_name, slen) &&
			    strncasecmp(s, garray[i].black_name, slen))
				continue;	// player names did not match
			sortedgames[count++] = i;
		} /* for */

		if (!count) {
			pprintf(p, "No matching games were found "
			    "(of %d in progress).\n", totalcount);
		} else {
			qsort(sortedgames, count, sizeof(int), gamesortfunc);
			pprintf(p, "\n");

			for (j = 0; j < count; j++) {
				i = sortedgames[j];

				wp	= garray[i].white;
				bp	= garray[i].black;

				board_calc_strength(&garray[i].game_state,
				    &ws, &bs);

				if (garray[i].status != GAME_EXAMINE) {
					pprintf_noformat(p, "%2d %4s %-11.11s "
					    "%4s %-10.10s [%c%c%c%3d %3d] ",
					    (i + 1),
					    ratstrii(GetRating(&parray[wp],
					    garray[i].type),
					    parray[wp].registered),
					    parray[wp].name,
					    ratstrii(GetRating(&parray[bp],
					    garray[i].type),
					    parray[bp].registered),
					    parray[bp].name,
					    (garray[i].private ? 'p' : ' '),
					    *bstr[garray[i].type],
					    *rstr[garray[i].rated],
					    (garray[i].wInitTime / 600),
					    (garray[i].wIncrement / 10));

					game_update_time(i);

					pprintf_noformat(p, "%6s -",
					    tenth_str((garray[i].wTime > 0 ?
					    garray[i].wTime : 0), 0));

					pprintf_noformat(p, "%6s (%2d-%2d) "
					    "%c: %2d\n",
					    tenth_str((garray[i].bTime > 0 ?
					    garray[i].bTime : 0), 0),
					    ws,
					    bs,
					    (garray[i].game_state.onMove ==
					    WHITE ? 'W' : 'B'),
					    garray[i].game_state.moveNum);
				} else {
					pprintf_noformat(p, "%2d "
					    "(Exam. %4d %-11.11s %4d %-10.10s) "
					    "[%c%c%c%3d %3d] ",
					    (i + 1),
					    garray[i].white_rating,
					    garray[i].white_name,
					    garray[i].black_rating,
					    garray[i].black_name,
					    (garray[i].private ? 'p' : ' '),
					    *bstr[garray[i].type],
					    *rstr[garray[i].rated],
					    (garray[i].wInitTime / 600),
					    (garray[i].wIncrement / 10));

					pprintf_noformat(p, "%c: %2d\n",
					    (garray[i].game_state.onMove ==
					    WHITE ? 'W' : 'B'),
					    garray[i].game_state.moveNum);
				}
			} /* for */

			if (count < totalcount) {
				pprintf(p, "\n  %d game%s displayed "
				    "(of %d in progress).\n",
				    count,
				    (count == 1 ? "" : "s"),
				    totalcount);
			} else {
				pprintf(p, "\n  %d game%s displayed.\n",
				    totalcount, (totalcount == 1 ? "" : "s"));
			}
		} /* else */

		rfree(sortedgames);
	}

	return COM_OK;
}

PRIVATE int
do_observe(int p, int obgame)
{
	if (garray[obgame].private &&
	    parray[p].adminLevel < ADMIN_ADMIN) {
		pprintf(p, "Sorry, game %d is a private game.\n", (obgame + 1));
		return COM_OK;
	}

	if (garray[obgame].white == p ||
	    garray[obgame].black == p) {
		if (garray[obgame].status != GAME_EXAMINE) {
			pprintf(p, "You cannot observe a game that you are "
			    "playing.\n");
			return COM_OK;
		}
	}

	if (player_is_observe(p, obgame)) {
		pprintf(p, "Removing game %d from observation list.\n",
		    (obgame + 1));
		player_remove_observe(p, obgame);
	} else {
		if (!player_add_observe(p, obgame)) {
			pprintf(p, "You are now observing game %d.\n",
			    (obgame + 1));
			send_board_to(obgame, p);
		} else {
			pprintf(p, "You are already observing the maximum "
			    "number of games.\n");
		}
	}

	return COM_OK;
}

PUBLIC void
unobserveAll(int p)
{
	for (int i = 0; i < parray[p].num_observe; i++) {
		pprintf(p, "Removing game %d from observation list.\n",
		    (parray[p].observe_list[i] + 1));
	}

	parray[p].num_observe = 0;
}

PUBLIC int
com_unobserve(int p, param_list param)
{
	int	gNum, p1;

	if (param[0].type == TYPE_NULL) {
		unobserveAll(p);
		return COM_OK;
	}

	if ((gNum = GameNumFromParam(p, &p1, &param[0])) < 0)
		return COM_OK;

	if (!player_is_observe(p, gNum)) {
		pprintf(p, "You are not observing game %d.\n", gNum);
	} else {
		player_remove_observe(p, gNum);
		pprintf(p, "Removing game %d from observation list.\n",
		    (gNum + 1));
	}

	return COM_OK;
}

PUBLIC int
com_observe(int p, param_list param)
{
	int	i;
	int	p1, obgame;

	if (parray[p].game >= 0 && garray[parray[p].game].status ==
	    GAME_EXAMINE) {
		pprintf(p, "You are still examining a game.\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_NULL) {
		unobserveAll(p);
		return COM_OK;
	}

	if ((obgame = GameNumFromParam(p, &p1, &param[0])) < 0)
		return COM_OK;

	if ((obgame >= g_num) ||
	    ((garray[obgame].status != GAME_ACTIVE) &&
	    (garray[obgame].status != GAME_EXAMINE))) {
		pprintf(p, "There is no such game.\n");
		return COM_OK;
	}

	if (p1 >= 0 && parray[p1].simul_info.numBoards) {
		for (i = 0; i < parray[p1].simul_info.numBoards; i++)
			if (parray[p1].simul_info.boards[i] >= 0)
				do_observe(p, parray[p1].simul_info.boards[i]);
	} else {
		do_observe(p, obgame);
	}

	return COM_OK;
}

PUBLIC int
com_allobservers(int p, param_list param)
{
	int	first;
	int	g;
	int	obgame;
	int	p1;
	int	start, end;

	if (param[0].type == TYPE_NULL) {
		obgame = -1;
	} else {
		obgame = GameNumFromParam(p, &p1, &param[0]);

		if (obgame < 0)
			return COM_OK;
	}

	if (obgame == -1) {
		start = 0;
		end = g_num;
	} else if ((obgame >= g_num) ||
	    (garray[obgame].status != GAME_ACTIVE &&
	    garray[obgame].status != GAME_EXAMINE)) {
		pprintf(p, "There is no such game.\n");
		return COM_OK;
	} else {
		start = obgame;
		end = obgame + 1;
	}

	/*
	 * list games being played
	 */
	for (g = start; g < end; g++) {
		if ((garray[g].status == GAME_ACTIVE) &&
		    ((parray[p].adminLevel > 0) ||
		    (garray[g].private == 0))) {
			for (first = 1, p1 = 0; p1 < p_num; p1++) {
				if (parray[p1].status != PLAYER_EMPTY &&
				    player_is_observe(p1, g)) {
					if (first) {
						pprintf(p, "Observing %2d "
						    "[%s vs. %s]:",
						    (g + 1),
						    parray[garray[g].white].name,
						    parray[garray[g].black].name);
						first = 0;
					}

					pprintf(p, " %s%s",
					    (parray[p1].game >= 0 ? "#" : ""),
					    parray[p1].name);
				}
			}

			if (!first)
				pprintf(p, "\n");
		}
	} /* for */

	/*
	 * list games being examined last
	 */
	for (g = start; g < end; g++) {
		if ((garray[g].status == GAME_EXAMINE) &&
		    ((parray[p].adminLevel > 0) ||
		    (garray[g].private == 0))) {
			for (first = 1, p1 = 0; p1 < p_num; p1++) {
				if ((parray[p1].status != PLAYER_EMPTY) &&
				    (player_is_observe(p1, g) ||
				    (parray[p1].game == g))) {
					if (first) {
						if (strcmp(garray[g].white_name,
						    garray[g].black_name)) {
							pprintf(p, "Examining "
							    "%2d [%s vs %s]:",
							    (g + 1),
							    garray[g].white_name,
							    garray[g].black_name);
						} else {
							pprintf(p, "Examining "
							    "%2d (scratch):",
							    (g + 1));
						}

						first = 0;
					}

					pprintf(p, " %s%s",
					    (parray[p1].game == g ? "#" : ""),
					    parray[p1].name);
				}
			}

			if (!first)
				pprintf(p, "\n");
		}
	} /* for */

	return COM_OK;
}

PUBLIC int
com_unexamine(int p, param_list param)
{
	int	g, p1, flag = 0;

	if (parray[p].game < 0 || garray[parray[p].game].status !=
	    GAME_EXAMINE) {
		pprintf(p, "You are not examining any games.\n");
		return COM_OK;
	}

	g = parray[p].game;
	parray[p].game = -1;

	for (p1 = 0; p1 < p_num; p1++) {
		if (parray[p1].status != PLAYER_PROMPT)
			continue;

		if (parray[p1].game == g && p != p1) {
			/*
			 * ok - there are other examiners to take over
			 * the game.
			 */
			flag = 1;
		}

		if (player_is_observe(p1, g) || parray[p1].game == g) {
			pprintf(p1, "%s stopped examining game %d.\n",
			    parray[p].name, (g + 1));
		}
	}

	if (!flag) {
		for (p1 = 0; p1 < p_num; p1++) {
			if (parray[p1].status != PLAYER_PROMPT)
				continue;
			if (player_is_observe(p1, g)) {
				pprintf(p1, "There are no examiners.\n");
				pcommand(p1, "unobserve %d", (g + 1));
			}
		}

		game_remove(g);
	}

	pprintf(p, "You are no longer examining game %d.\n", (g + 1));
	return COM_OK;
}

PUBLIC int
com_mexamine(int p, param_list param)
{
	int	g, p1, p2;

	if (parray[p].game < 0 || garray[parray[p].game].status !=
	    GAME_EXAMINE) {
		pprintf(p, "You are not examining any games.\n");
		return COM_OK;
	}

	if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
		pprintf(p, "No user named \"%s\" is logged in.\n",
		    param[0].val.word);
		return COM_OK;
	}

	g = parray[p].game;

	if (!player_is_observe(p1, g)) {
		pprintf(p, "%s must observe the game you are analysing.\n",
		    parray[p1].name);
		return COM_OK;
	} else {
		if (parray[p1].game >= 0) {
			pprintf(p, "%s is already analysing the game.\n",
			    parray[p1].name);
			return COM_OK;
		}

		/*
		 * If we get here - let's make him examiner of the
		 * game.
		 */
		unobserveAll(p1);	// Fix for Xboard

		player_decline_offers(p1, -1, PEND_MATCH);
		player_withdraw_offers(p1, -1, PEND_MATCH);
		player_withdraw_offers(p1, -1, PEND_SIMUL);

		parray[p1].game = g;
		pprintf(p1, "You are now examiner of game %d.\n", (g + 1));
		send_board_to(g, p1);	// Pos not changed - but fixes Xboard

		for (p2 = 0; p2 < p_num; p2++) {
			if (parray[p2].status != PLAYER_PROMPT)
				continue;
			if (p2 == p1)
				continue;
			if (player_is_observe(p2, g) || parray[p2].game == g) {
				pprintf_prompt(p2, "%s is now examiner of "
				    "game %d.\n", parray[p1].name, (g + 1));
			}
		}
	}

	return COM_OK;
}

PUBLIC int
com_moves(int p, param_list param)
{
	int	g;
	int	p1;

	if (param[0].type == TYPE_NULL) {
		if (parray[p].game >= 0) {
			g = parray[p].game;
		} else if (parray[p].num_observe) {
			for (g = 0; g < parray[p].num_observe; g++) {
				pprintf(p, "%s\n",
				    movesToString(parray[p].observe_list[g],
				    0));
			}

			return COM_OK;
		} else {
			pprintf(p, "You are neither playing, observing nor "
			    "examining a game.\n");
			return COM_OK;
		}
	} else {
		g = GameNumFromParam(p, &p1, &param[0]);

		if (g < 0)
			return COM_OK;
	}

	if ((g < 0) ||
	    (g >= g_num) ||
	    ((garray[g].status != GAME_ACTIVE) &&
	    (garray[g].status != GAME_EXAMINE))) {
		pprintf(p, "There is no such game.\n");
		return COM_OK;
	}

	if ((garray[g].white != p) &&
	    (garray[g].black != p) &&
	    (garray[g].private) &&
	    (parray[p].adminLevel < ADMIN_ADMIN)) {
		pprintf(p, "Sorry, that is a private game.\n");
		return COM_OK;
	}

	pprintf(p, "%s\n", movesToString(g, 0)); // pgn may break interfaces?
	return COM_OK;
}

PUBLIC int
com_mailmoves(int p, param_list param)
{
	char	subj[81] = { '\0' };
	int	g;
	int	p1;

	if (!parray[p].registered) {
		pprintf(p, "Unregistered players cannot use mailmoves.\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_NULL) {
		if (parray[p].game >= 0) {
			g = parray[p].game;
		} else {
			pprintf(p, "You are neither playing, observing nor "
			    "examining a game.\n");
			return COM_OK;
		}
	} else {
		g = GameNumFromParam(p, &p1, &param[0]);

		if (g < 0)
			return COM_OK;
	}

	if ((g < 0) ||
	    (g >= g_num) ||
	    ((garray[g].status != GAME_ACTIVE) &&
	    (garray[g].status != GAME_EXAMINE))) {
		pprintf(p, "There is no such game.\n");
		return COM_OK;
	}

	if ((garray[g].white != p) &&
	    (garray[g].black != p) &&
	    (garray[g].private) &&
	    (parray[p].adminLevel < ADMIN_ADMIN)) {
		pprintf(p, "Sorry, that is a private game.\n");
		return COM_OK;
	}

	msnprintf(subj, sizeof subj, "FICS game report %s vs %s",
	    garray[g].white_name,
	    garray[g].black_name);

	if (mail_string_to_user(p, subj, movesToString(g, parray[p].pgn))) {
		pprintf(p, "Moves NOT mailed, perhaps your address is "
		    "incorrect.\n");
	} else {
		pprintf(p, "Moves mailed.\n");
	}

	return COM_OK;
}

PRIVATE int
old_mail_moves(int p, int mail, param_list param)
{
	FILE	*fp = NULL;
	char	 fname[MAX_FILENAME_SIZE] = { '\0' };
	char	 tmp[2048] = { '\0' };
	char	*ptmp = tmp;
	int	 count = 0;
	int	 p1, connected;

	if (mail && (!parray[p].registered)) {
		pprintf(p, "Unregistered players cannot use mailoldmoves.\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_WORD) {
		if (!FindPlayer(p, param[0].val.word, &p1, &connected))
			return COM_OK;
	} else {
		p1		= p;
		connected	= 1;
	}

	(void) snprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s",
	    stats_dir, parray[p1].login[0], parray[p1].login, STATS_GAMES);
	fp = fopen(fname, "r");	// old moves now looks in history to save mem
				// - DAV

	if (!fp) {
		pprintf(p, "There is no old game for %s.\n", parray[p1].name);

		if (!connected)
			player_remove(p1);
		return COM_OK;
	}

	while (fgets(tmp, sizeof tmp, fp) != NULL) {
		/* null */;
	}

	if (sscanf(ptmp, "%d", &count) != 1) {
		warnx("%s: sscanf() error", __func__);
		fclose(fp);
		return COM_FAILED;
	}

	fclose(fp);
	pprintf(p, "Last game for %s was history game %d.\n", parray[p1].name,
	    count);

	if (mail)
		pcommand(p, "mailstored %s %d", parray[p1].name, count);
	else
		pcommand(p, "smoves %s %d", parray[p1].name, count);

	if (!connected)
		player_remove(p1);
	return COM_OK;
}

PUBLIC int
com_oldmoves(int p, param_list param)
{
	return old_mail_moves(p, 0, param);
}

PUBLIC int
com_mailoldmoves(int p, param_list param)
{
	return old_mail_moves(p, 1, param);
}

PUBLIC void
ExamineScratch(int p, param_list param)
{
	char	*val;
	char	 board[100];
	char	 category[100];
	char	 parsebuf[100];
	int	 confused = 0;
	int	 g = game_new();

	unobserveAll(p);

	player_decline_offers(p, -1, PEND_MATCH);
	player_withdraw_offers(p, -1, PEND_MATCH);
	player_withdraw_offers(p, -1, PEND_SIMUL);

	garray[g].bInitTime = garray[g].bIncrement = 0;
	garray[g].wInitTime = garray[g].wIncrement = 0;

	garray[g].startTime		= tenth_secs();
	garray[g].timeOfStart		= tenth_secs();

	garray[g].clockStopped		= 0;
	garray[g].lastDecTime		= garray[g].startTime;
	garray[g].lastMoveTime		= garray[g].startTime;
	garray[g].rated			= 0;
	garray[g].status		= GAME_EXAMINE;
	garray[g].totalHalfMoves	= 0;
	garray[g].type			= TYPE_UNTIMED;
	garray[g].wTime			= garray[g].bTime = 0;
	garray[g].white			= garray[g].black = p;
	parray[p].game			= g;
	parray[p].side			= WHITE;

	board[0]	= '\0';
	category[0]	= '\0';

	if (param[0].val.string != parray[p].name &&
	    param[1].type == TYPE_WORD) {
		mstrlcpy(category, param[0].val.string, sizeof category);
		mstrlcpy(board, param[1].val.string, sizeof board);
	} else if (param[1].type != TYPE_NULL) {
		val = param[1].val.string;

		while (!confused && sscanf(val, " %99s", parsebuf) == 1) {
			val = eatword(eatwhite(val));

			if (category[0] != '\0' && board[0] == '\0') {
				mstrlcpy(board, parsebuf, sizeof board);
			} else if (isdigit(*parsebuf)) {
				pprintf(p, "You can't specify time controls."
				    "\n");
				return;
			} else if (category[0] == '\0') {
				mstrlcpy(category, parsebuf, sizeof category);
			} else {
				confused = 1;
			}
		}

		if (confused) {
			pprintf(p, "Can't interpret %s in match command.\n",
			    parsebuf);
			return;
		}
	}

	if (category[0] && !board[0]) {
		pprintf(p, "You must specify a board and a category.\n");
		return;
	}

	pprintf(p, "Starting a game in examine (scratch) mode.\n");

	if (category[0]) {
		pprintf(p, "Loading from catagory: %s, board: %s.\n",
		    category,
		    board);
	}
	if (board_init(&garray[g].game_state, category, board)) {
		pprintf(p, "PROBLEM LOADING BOARD. Game Aborted.\n");
		fprintf(stderr, "FICS: PROBLEM LOADING BOARD. Game Aborted.\n");
		return;
	}

	garray[g].game_state.gameNum = g;
	mstrlcpy(garray[g].white_name, parray[p].name,
	    sizeof(garray[g].white_name));
	mstrlcpy(garray[g].black_name, parray[p].name,
	    sizeof(garray[g].black_name));
	garray[g].white_rating = garray[g].black_rating =
	    parray[p].s_stats.rating;
	send_boards(g);
	MakeFENpos(g, (char *)garray[g].FENstartPos,
	    ARRAY_SIZE(garray[g].FENstartPos));
}

PRIVATE int
ExamineStored(FILE *fp, int p, char *filename)
{
	char	 board[100];
	char	 category[100];
	game	*gg;
	int	 g;

	unobserveAll(p);

	player_decline_offers(p, -1, PEND_MATCH);
	player_withdraw_offers(p, -1, PEND_MATCH);
	player_withdraw_offers(p, -1, PEND_SIMUL);

	g	= game_new();
	gg	= &garray[g];

	board[0]	= '\0';
	category[0]	= '\0';

	if (board_init(&gg->game_state, category, board)) {
		pprintf(p, "PROBLEM LOADING BOARD. Game Aborted.\n");
		fprintf(stderr, "FICS: PROBLEM LOADING BOARD %s %s. "
		    "Game Aborted.\n", category, board);
		return -1;
	}

	gg->status = GAME_EXAMINE;

	if (ReadGameAttrs(fp, filename, g) < 0) {
		pprintf(p, "Gamefile is corrupt; please notify an admin.\n");
		return -1;
	}

	gg->startTime		= tenth_secs();
	gg->lastDecTime		= gg->startTime;
	gg->lastMoveTime	= gg->startTime;

	gg->totalHalfMoves	= gg->numHalfMoves;
	gg->numHalfMoves	= 0;
	gg->revertHalfMove	= 0;

	gg->black		= p;
	gg->white		= p;
	gg->game_state.gameNum	= g;

	parray[p].game = g;
	parray[p].side = WHITE;

	send_boards(g);
	MakeFENpos(g, (char *)garray[g].FENstartPos,
	    ARRAY_SIZE(garray[g].FENstartPos));

	return g;
}

PRIVATE void
ExamineAdjourned(int p, int p1, int p2)
{
	FILE	*fp;
	char	*p1Login, *p2Login;
	char	 filename[1024] = { '\0' };
	int	 g;

	p1Login = parray[p1].login;
	p2Login = parray[p2].login;

	(void)snprintf(filename, sizeof filename, "%s/%c/%s-%s",
	    adj_dir, *p1Login, p1Login, p2Login);
	fp = fopen(filename, "r");

	if (!fp) {
		(void)snprintf(filename, sizeof filename, "%s/%c/%s-%s",
		    adj_dir, *p2Login, p1Login, p2Login);
		fp = fopen(filename, "r");

		if (!fp) {
			(void)snprintf(filename, sizeof filename,
			    "%s/%c/%s-%s",
			    adj_dir, *p2Login, p2Login, p1Login);
			fp = fopen(filename, "r");

			if (!fp) {
				(void)snprintf(filename, sizeof filename,
				    "%s/%c/%s-%s",
				    adj_dir, *p1Login, p2Login, p1Login);
				fp = fopen(filename, "r");

				if (!fp) {
					pprintf(p, "No stored game between "
					    "\"%s\" and \"%s\".\n",
					    parray[p1].name,
					    parray[p2].name);
					return;
				}
			}
		}
	}

	g = ExamineStored(fp, p, filename);
	fclose(fp);

	if (g >= 0) {
		if (garray[g].white_name[0] == '\0') {
			mstrlcpy(garray[g].white_name, p1Login,
			    sizeof(garray[g].white_name));
		}
		if (garray[g].black_name[0] == '\0') {
			mstrlcpy(garray[g].black_name, p2Login,
			    sizeof(garray[g].black_name));
		}
	}
}

PRIVATE char *
FindHistory(int p, int p1, int p_game)
{
	FILE		*fpHist;
	int		 index = 0;
	long int	 when = 0;
	static char	 fileName[MAX_FILENAME_SIZE];

	msnprintf(fileName, sizeof fileName, "%s/player_data/%c/%s.%s",
	    stats_dir, parray[p1].login[0], parray[p1].login, STATS_GAMES);

	if ((fpHist = fopen(fileName, "r")) == NULL) {
		pprintf(p, "No games in history for %s.\n", parray[p1].name);
		return NULL;
	}

	do {
		int ret;

		ret = fscanf(fpHist, "%d %*c %*d %*c %*d %*s %*s %*d %*d %*d "
		    "%*d %*s %*s %ld", &index, &when);
		if (ret != 2) {
			warnx("%s: %s: corrupt", __func__, fileName);
			fclose(fpHist);
			return NULL;
		}
	} while (!feof(fpHist) &&
	    !ferror(fpHist) &&
	    index != p_game);

	if (feof(fpHist) || ferror(fpHist)) {
		pprintf(p, "There is no history game %d for %s.\n", p_game,
		    parray[p1].name);
		fclose(fpHist);
		return NULL;
	}

	fclose(fpHist);

	if (when < 0 || when >= LONG_MAX) {
		pprintf(p, "Corrupt history data for %s (invalid timestamp).\n",
		    parray[p1].name);
		return NULL;
	}

	msnprintf(fileName, sizeof fileName, "%s/%ld/%ld", hist_dir,
	    (when % 100), when);
	return (&fileName[0]);
}

PRIVATE char *
FindHistory2(int p, int p1, int p_game, char *End, const size_t End_size)
{
	FILE		*fpHist;
	char		 fmt[80] = { '\0' };
	char		*resolvedPath;
	int		 index = 0;
	long int	 when = 0;
	static char	 fileName[MAX_FILENAME_SIZE];

	msnprintf(fileName, sizeof fileName, "%s/player_data/%c/%s.%s",
	    stats_dir, parray[p1].login[0], parray[p1].login, STATS_GAMES);

	if ((fpHist = fopen(fileName, "r")) == NULL) {
		pprintf(p, "No games in history for %s.\n", parray[p1].name);
		return NULL;
	}

	msnprintf(fmt, sizeof fmt, "%%d %%*c %%*d %%*c %%*d %%*s %%*s %%*d "
	    "%%*d %%*d %%*d %%*s %%%zus %%ld\n", (End_size - 1));

	do {
		if (fscanf(fpHist, fmt, &index, End, &when) != 3) {
			warnx("%s: %s: corrupt", __func__, fileName);
			fclose(fpHist);
			return NULL;
		}
	} while (!feof(fpHist) &&
	    !ferror(fpHist) &&
	    index != p_game);

	if (feof(fpHist) || ferror(fpHist)) {
		pprintf(p, "There is no history game %d for %s.\n", p_game,
		    parray[p1].name);
		fclose(fpHist);
		return NULL;
	}

	fclose(fpHist);

	if (when < 0 || when >= LONG_MAX) {
		pprintf(p, "Invalid history timestamp for %s.\n",
		    parray[p1].name);
		return NULL;
	}

	msnprintf(fileName, sizeof fileName, "%s/%ld/%ld", hist_dir,
	    (when % 100), when);

	// Validate that the resolved path is within hist_dir
	if ((resolvedPath = realpath(fileName, NULL)) == NULL) {
		warn("%s: realpath", __func__);
		return NULL;
	}

	if (strncmp(resolvedPath, hist_dir, strlen(hist_dir)) != 0) {
		warnx("%s: path traversal detected", __func__);
		free(resolvedPath);
		return NULL;
	}

	// Copy 'resolvedPath' back to 'fileName' for return
	mstrlcpy(fileName, resolvedPath, sizeof fileName);
	free(resolvedPath);

	return (&fileName[0]);
}

PRIVATE void
ExamineHistory(int p, int p1, int p_game)
{
	FILE	*fpGame;
	char	*fileName;

	if ((fileName = FindHistory(p, p1, p_game)) != NULL) {
		if ((fpGame = fopen(fileName, "r")) == NULL) {
			pprintf(p, "History game %d not available for %s.\n",
			    p_game, parray[p1].name);
		} else {
			ExamineStored(fpGame, p, fileName);
			fclose(fpGame);
		}
	}
}

PRIVATE void
ExamineJournal(int p, int p1, char slot)
{
	FILE	*fpGame;
	char	*name_from = parray[p1].login;
	char	 fname[MAX_FILENAME_SIZE] = { '\0' };

	if ((parray[p1].jprivate) &&
	    (p != p1) &&
	    (parray[p].adminLevel < ADMIN_ADMIN)) {
		pprintf(p, "Sorry, this journal is private.\n");
		return;
	}

	if (((slot - 'a' - 1) > MAX_JOURNAL) &&
	    (parray[p1].adminLevel < ADMIN_ADMIN) &&
	    (!titled_player(p, parray[p1].login))) {
		pprintf(p, "%s's maximum journal entry is %c\n",
		    parray[p1].name,
		    toupper((char)(MAX_JOURNAL + 'A' - 1)));
		return;
	}

	msnprintf(fname, sizeof fname, "%s/%c/%s.%c", journal_dir, name_from[0],
	    name_from, slot);

	if ((fpGame = fopen(fname, "r")) == NULL) {
		pprintf(p, "Journal entry %c is not available for %s.\n",
		    toupper(slot),
		    parray[p1].name);
	} else {
		ExamineStored(fpGame, p, fname);
		fclose(fpGame);
	}
}

PUBLIC int
com_examine(int p, param_list param)
{
	char	*param2string;
	char	 fname[MAX_FILENAME_SIZE] = { '\0' };
	int	 p1, p2 = p, p1conn, p2conn = 1;

	if (parray[p].game >= 0 && garray[parray[p].game].status ==
	    GAME_EXAMINE) {
		pprintf(p, "You are already examining a game.\n");
	} else if (parray[p].game >= 0) {
		pprintf(p, "You are playing a game.\n");
	} else if (param[0].type == TYPE_NULL) {
		ExamineScratch(p, param);
	} else if (param[0].type == TYPE_WORD) {
		if (param[1].type == TYPE_WORD) {
			msnprintf(fname, sizeof fname, "%s/%s/%s", board_dir,
			    param[0].val.word, param[1].val.word);

			if (file_exists(fname)) {
				ExamineScratch(p, param);
				return COM_OK;
			}
		}

		if (!FindPlayer(p, param[0].val.word, &p1, &p1conn))
			return COM_OK;

		if (param[1].type == TYPE_INT)
			ExamineHistory(p, p1, param[1].val.integer);
		else {
			if (param[1].type == TYPE_WORD) {
				/*
				 * Lets check the journal
				 */
				param2string = param[1].val.word;

				if (strlen(param2string) == 1 &&
				    isalpha(param2string[0])) {
					ExamineJournal(p, p1, param2string[0]);

					if (!p1conn)
						player_remove(p1);
					return COM_OK;
				} else {
					if (!FindPlayer(p, param[1].val.word,
					    &p2, &p2conn)) {
						if (!p1conn)
							player_remove(p1);
						return COM_OK;
					}
				}
			}

			ExamineAdjourned(p, p1, p2);

			if (!p2conn)
				player_remove(p2);
		}

		if (!p1conn)
			player_remove(p1);
	}

	return COM_OK;
}

PUBLIC int
com_stored(int p, param_list param)
{
	DIR		*dirp;
	char		 dname[MAX_FILENAME_SIZE];
	int		 p1, connected;
#ifdef USE_DIRENT
	struct dirent	*dp;
#else
	struct direct	*dp;
#endif

	if (param[0].type == TYPE_WORD) {
		if (!FindPlayer(p, param[0].val.word, &p1, &connected))
			return COM_OK;
	} else {
		p1 = p;
		connected = 1;
	}

	msnprintf(dname, sizeof dname, "%s/%c", adj_dir, parray[p1].login[0]);
	dirp = opendir(dname);

	if (!dirp) {
		pprintf(p, "Player %s has no games stored.\n", parray[p1].name);

		if (!connected)
			player_remove(p1);
		return COM_OK;
	}

	pprintf(p, "Stored games for %s:\n", parray[p1].name);

	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if (file_has_pname(dp->d_name, parray[p1].login)) {
			pprintf(p, "   %s vs. %s\n",
			    file_wplayer(dp->d_name),
			    file_bplayer(dp->d_name));
		}
	}

	closedir(dirp);
	pprintf(p, "\n");

	if (!connected)
		player_remove(p1);
	return COM_OK;
}

PRIVATE void
stored_mail_moves(int p, int mail, param_list param)
{
	FILE	*fpGame;
	char	*fileName;
	char	*name_from;
	char	*param2string = NULL;
	char	 fileName2[MAX_FILENAME_SIZE];
	int	 g = -1;
	int	 wp, wconnected, bp, bconnected, gotit = 0;

	if (mail && !parray[p].registered) {
		pprintf(p, "Unregistered players cannot use mailstored.\n");
		return;
	}

	if (!FindPlayer(p, param[0].val.word, &wp, &wconnected))
		return;

	if (param[1].type == TYPE_INT) { /* look for a game from history */
		if ((fileName = FindHistory(p, wp, param[1].val.integer)) !=
		    NULL) {
			if ((fpGame = fopen(fileName, "r")) == NULL) {
				pprintf(p, "History game %d not available "
				    "for %s.\n",
				    param[1].val.integer,
				    parray[wp].name);
			} else {
				g = game_new();

				if (ReadGameAttrs(fpGame, fileName, g) < 0)
					pprintf(p, "Gamefile is corrupt; "
					    "please notify an admin.\n");
				else
					gotit = 1;

				fclose(fpGame);
			}
		}
	} else {
		/*
		 * Let's test for journal
		 */
		name_from = param[0].val.word;
		param2string = param[1].val.word;

		if (strlen(param2string) == 1 && isalpha(param2string[0])) {
			if (parray[wp].jprivate &&
			    parray[p].adminLevel < ADMIN_ADMIN &&
			    p != wp) {
				pprintf(p, "Sorry, the journal from which you "
				    "are trying to fetch is private.\n");
			} else {
				if ((param2string[0] - 'a' - 1) > MAX_JOURNAL &&
				    parray[wp].adminLevel < ADMIN_ADMIN &&
				    !titled_player(p, parray[wp].login)) {
					pprintf(p, "%s's maximum journal entry "
					    "is %c\n",
					    parray[wp].name,
					    toupper((char)(MAX_JOURNAL +
					    'A' - 1)));
				} else {
					msnprintf(fileName2, sizeof fileName2,
					    "%s/%c/%s.%c",
					    journal_dir,
					    name_from[0],
					    name_from,
					    param2string[0]);

					if ((fpGame = fopen(fileName2, "r")) ==
					    NULL) {
						pprintf(p, "Journal entry %c "
						    "is not available for %s.\n",
						    toupper(param2string[0]),
						    parray[wp].name);
					} else {
						g = game_new();

						/* XXX: was 'fileName' */
						if (ReadGameAttrs(fpGame,
						    fileName2, g) < 0)
							pprintf(p, "Journal "
							    "entry is corrupt; "
							    "please notify an "
							    "admin.\n");
						else
							gotit = 1;

						fclose(fpGame);
					}
				}
			}
		} else {
			/*
			 * look for a stored game between the players
			 */

			if (FindPlayer(p, param[1].val.word, &bp,
			    &bconnected)) {
				g = game_new();

				if (game_read(g, wp, bp) >= 0) {
					gotit = 1;
				} else if (game_read(g, bp, wp) >= 0) {
					gotit = 1;
				} else {
					pprintf(p, "There is no stored game "
					    "%s vs. %s\n",
					    parray[wp].name,
					    parray[bp].name);
				}

				if (!bconnected)
					player_remove(bp);
			}
		}
	}

	if (gotit) {
		if (strcasecmp(parray[p].name, garray[g].white_name) &&
		    strcasecmp(parray[p].name, garray[g].black_name) &&
		    garray[g].private &&
		    parray[p].adminLevel < ADMIN_ADMIN) {
			pprintf(p, "Sorry, that is a private game.\n");
		} else {
			if (mail == 1) { /* Do mailstored */
				char	subj[81];

				if (param[1].type == TYPE_INT) {
					msnprintf(subj, sizeof subj, "FICS "
					    "history game: %s %d",
					    parray[wp].name,
					    param[1].val.integer);
				} else {
					if (param2string == NULL) /* XXX */
						errx(1, "%s: param2string == "
						    "NULL", __func__);
					if (strlen(param2string) == 1 &&
					    isalpha(param2string[0])) {
						msnprintf(subj, sizeof subj,
						    "FICS journal "
						    "game %s vs %s",
						    garray[g].white_name,
						    garray[g].black_name);
					} else {
						msnprintf(subj, sizeof subj,
						    "FICS adjourned "
						    "game %s vs %s",
						    garray[g].white_name,
						    garray[g].black_name);
					}
				}
				if (mail_string_to_user(p, subj, movesToString
				    (g, parray[p].pgn)))
					pprintf(p, "Moves NOT mailed, perhaps "
					    "your address is incorrect.\n");
				else
					pprintf(p, "Moves mailed.\n");
			} else {
				pprintf(p, "%s\n", movesToString(g, 0));
			} /* Do smoves */
		}
	}

	if (!wconnected)
		player_remove(wp);
	if (g != -1)
		game_remove(g);
}

PUBLIC int
com_mailstored(int p, param_list param)
{
	stored_mail_moves(p, 1, param);
	return COM_OK;
}

PUBLIC int
com_smoves(int p, param_list param)
{
	stored_mail_moves(p, 0, param);
	return COM_OK;
}

PUBLIC int
com_sposition(int p, param_list param)
{
	int	g;
	int	wp, wconnected, bp, bconnected, confused = 0;

	if (!FindPlayer(p, param[0].val.word, &wp, &wconnected))
		return (COM_OK);
	if (!FindPlayer(p, param[1].val.word, &bp, &bconnected)) {
		if (!wconnected)
			player_remove(wp);
		return (COM_OK);
	}

	g = game_new();

	if (game_read(g, wp, bp) < 0) {		// if no game white-black,
		if (game_read(g, bp, wp) < 0) {	// look for black-white
			confused = 1;

			pprintf(p, "There is no stored game %s vs. %s\n",
			    parray[wp].name,
			    parray[bp].name);
		} else {
			int	tmp;

			tmp		= wp;
			wp		= bp;
			bp		= tmp;
			tmp		= wconnected;
			wconnected	= bconnected;
			bconnected	= tmp;
		}
	}

	if (!confused) {
		if ((wp != p) &&
		    (bp != p) &&
		    (garray[g].private) &&
		    (parray[p].adminLevel < ADMIN_ADMIN)) {
			pprintf(p, "Sorry, that is a private game.\n");
		} else {
			garray[g].white		= wp;
			garray[g].black		= bp;
			garray[g].startTime	= tenth_secs();
			garray[g].lastMoveTime	= garray[g].startTime;
			garray[g].lastDecTime	= garray[g].startTime;

			pprintf(p, "Position of stored game %s vs. %s\n",
			    parray[wp].name,
			    parray[bp].name);

			send_board_to(g, p);
		}
	}

	game_remove(g);

	if (!wconnected)
		player_remove(wp);
	if (!bconnected)
		player_remove(bp);
	return COM_OK;
}

PUBLIC int
com_forward(int p, param_list param)
{
	int		g, i;
	int		nHalfMoves = 1;
	int		p1;
	unsigned int	now;

	if (!(parray[p].game >= 0 && garray[parray[p].game].status ==
	    GAME_EXAMINE)) {
		pprintf(p, "You are not examining any games.\n");
		return COM_OK;
	}

	g = parray[p].game;

	if (!strcmp(garray[g].white_name, garray[g].black_name)) {
		pprintf(p, "You cannot go forward; no moves are stored.\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_INT)
		nHalfMoves = param[0].val.integer;

	if (garray[g].numHalfMoves > garray[g].revertHalfMove) {
		pprintf(p, "No more moves.\n");
		return COM_OK;
	}

	if (garray[g].numHalfMoves < garray[g].totalHalfMoves) {
		for (p1 = 0; p1 < p_num; p1++) {
			if (parray[p1].status != PLAYER_PROMPT)
				continue;
			if (player_is_observe(p1, g) || parray[p1].game == g) {
				pprintf(p1, "%s goes forward %d move%s.\n",
				    parray[p].name,
				    nHalfMoves,
				    (nHalfMoves == 1 ? "" : "s"));
			}
		}
	}

	for (i = 0; i < nHalfMoves; i++) {
		if (garray[g].numHalfMoves < garray[g].totalHalfMoves) {
			execute_move(&garray[g].game_state,
			    &garray[g].moveList[garray[g].numHalfMoves], 1);

			if (garray[g].numHalfMoves + 1 >
			    garray[g].examMoveListSize) {
				// Allocate 20 moves at a time.
				garray[g].examMoveListSize += 20;

				if (!garray[g].examMoveList) {
					garray[g].examMoveList =
					    reallocarray(NULL,
					    sizeof(move_t),
					    garray[g].examMoveListSize);
					if (garray[g].examMoveList == NULL)
						err(1, "%s", __func__);
					malloc_count++;
				} else {
					garray[g].examMoveList =
					    reallocarray(garray[g].examMoveList,
					    sizeof(move_t),
					    garray[g].examMoveListSize);
					if (garray[g].examMoveList == NULL)
						err(1, "%s", __func__);
				}
			}

			garray[g].examMoveList[garray[g].numHalfMoves] =
			    garray[g].moveList[garray[g].numHalfMoves];
			garray[g].revertHalfMove++;
			garray[g].numHalfMoves++;
		} else {
			for (p1 = 0; p1 < p_num; p1++) {
				if (parray[p1].status != PLAYER_PROMPT)
					continue;
				if (player_is_observe(p1, g) ||
				    parray[p1].game == g)
					pprintf(p1, "End of game.\n");
			}

			break;
		}
	}

	// roll back time
	if (garray[g].game_state.onMove == WHITE) {
		garray[g].wTime += (garray[g].lastDecTime -
				    garray[g].lastMoveTime);
	} else {
		garray[g].bTime += (garray[g].lastDecTime -
				    garray[g].lastMoveTime);
	}

	now = tenth_secs();

	if (garray[g].numHalfMoves == 0)
		garray[g].timeOfStart = now;
	garray[g].lastMoveTime = now;
	garray[g].lastDecTime = now;

	send_boards(g);
	return COM_OK;
}

PUBLIC int
com_backward(int p, param_list param)
{
	int		g, i;
	int		nHalfMoves = 1;
	int		p1;
	unsigned int	now;

	if (!(parray[p].game >= 0 && garray[parray[p].game].status ==
	    GAME_EXAMINE)) {
		pprintf(p, "You are not examining any games.\n");
		return COM_OK;
	}

	g = parray[p].game;

	if (param[0].type == TYPE_INT)
		nHalfMoves = param[0].val.integer;

	if (garray[g].numHalfMoves != 0) {
		for (p1 = 0; p1 < p_num; p1++) {
			if (parray[p1].status != PLAYER_PROMPT)
				continue;
			if (player_is_observe(p1, g) || parray[p1].game == g) {
				pprintf(p1, "%s backs up %d move%s.\n",
				    parray[p].name,
				    nHalfMoves,
				    (nHalfMoves == 1 ? "" : "s"));
			}
		}
	}

	for (i = 0; i < nHalfMoves; i++) {
		if (backup_move(g, REL_EXAMINE) != MOVE_OK) {
			for (p1 = 0; p1 < p_num; p1++) {
				if (parray[p1].status != PLAYER_PROMPT)
					continue;
				if (player_is_observe(p1, g) ||
				    parray[p1].game == g)
					pprintf(p1, "Beginning of game.\n");
			}

			break;
		}
	}

	if (garray[g].numHalfMoves < garray[g].revertHalfMove)
		garray[g].revertHalfMove = garray[g].numHalfMoves;

	// roll back time
	if (garray[g].game_state.onMove == WHITE) {
		garray[g].wTime += (garray[g].lastDecTime -
				    garray[g].lastMoveTime);
	} else {
		garray[g].bTime += (garray[g].lastDecTime -
				    garray[g].lastMoveTime);
	}

	now = tenth_secs();

	if (garray[g].numHalfMoves == 0)
		garray[g].timeOfStart = now;
	garray[g].lastMoveTime = now;
	garray[g].lastDecTime = now;

	send_boards(g);
	return COM_OK;
}

PUBLIC int
com_revert(int p, param_list param)
{
	int		g, i;
	int		nHalfMoves = 1;
	int		p1;
	unsigned int	now;

	if (!(parray[p].game >= 0 && garray[parray[p].game].status ==
	    GAME_EXAMINE)) {
		pprintf(p, "You are not examining any games.\n");
		return COM_OK;
	}

	g = parray[p].game;
	nHalfMoves = garray[g].numHalfMoves - garray[g].revertHalfMove;

	if (nHalfMoves == 0) {
		pprintf(p, "Already at mainline.\n");
		return COM_OK;
	}

	if (nHalfMoves < 0) {	// eek - should NEVER happen!
		fprintf(stderr, "OUCH! in %s: nHalfMoves < 0\n", __func__);
		return COM_OK;
	}

	for (p1 = 0; p1 < p_num; p1++) {
		if (parray[p1].status != PLAYER_PROMPT)
			continue;
		if (player_is_observe(p1, g) || parray[p1].game == g) {
			pprintf(p1, "%s reverts to mainline.\n",
			    parray[p].name);
		}
	}

	for (i = 0; i < nHalfMoves; i++)
		backup_move(g, REL_EXAMINE);

	// roll back time
	if (garray[g].game_state.onMove == WHITE) {
		garray[g].wTime += (garray[g].lastDecTime -
				    garray[g].lastMoveTime);
	} else {
		garray[g].bTime += (garray[g].lastDecTime -
				    garray[g].lastMoveTime);
	}

	now = tenth_secs();

	if (garray[g].numHalfMoves == 0)
		garray[g].timeOfStart = now;
	garray[g].lastMoveTime = now;
	garray[g].lastDecTime = now;

	send_boards(g);
	return COM_OK;
}

PUBLIC int
com_history(int p, param_list param)
{
	char	fname[MAX_FILENAME_SIZE] = { '\0' };
	int	p1, connected;

	if (param[0].type == TYPE_WORD) {
		if (!FindPlayer(p, param[0].val.word, &p1, &connected))
			return COM_OK;
	} else {
		p1 = p;
		connected = 1;
	}

	msnprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s", stats_dir,
	    parray[p1].login[0], parray[p1].login, STATS_GAMES);
	pgames(p, p1, fname);

	if (!connected)
		player_remove(p1);
	return COM_OK;
}

PUBLIC int
com_journal(int p, param_list param)
{
	char	fname[MAX_FILENAME_SIZE];
	int	p1, connected;

	if (param[0].type == TYPE_WORD) {
		if (!FindPlayer(p, param[0].val.word, &p1, &connected))
			return COM_OK;
	} else {
		p1 = p;
		connected = 1;
	}

	if (!parray[p1].registered) {
		pprintf(p, "Only registered players may keep a journal.\n");

		if (!connected)
			player_remove(p1);
		return COM_OK;
	}

	if (parray[p1].jprivate &&
	    p != p1 &&
	    parray[p].adminLevel < ADMIN_ADMIN) {
		pprintf(p, "Sorry, this journal is private.\n");

		if (!connected)
			player_remove(p1);
		return COM_OK;
	}

	msnprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s", stats_dir,
	    parray[p1].login[0], parray[p1].login, STATS_JOURNAL);
	pjournal(p, p1, fname);

	if (!connected)
		player_remove(p1);
	return COM_OK;
}

PRIVATE void
jsave_journalentry(int p, char save_spot, int p1, char from_spot, char *to_file)
{
	FILE			*Game;
	char			*name_from = parray[p1].login;
	char			*name_to = parray[p].login;
	char			 fname[MAX_FILENAME_SIZE];
	char			 fname2[MAX_FILENAME_SIZE];
	struct JGI_context	 ctx;

	msnprintf(fname, sizeof fname, "%s/%c/%s.%c", journal_dir, name_from[0],
	    name_from, from_spot);

	if ((Game = fopen(fname, "r")) == NULL) {
		pprintf(p, "Journal entry %c not available for %s.\n",
		    toupper(from_spot),
		    parray[p1].name);
		return;
	}

	fclose(Game);

	msnprintf(fname2, sizeof fname2, "%s/%c/%s.%c", journal_dir, name_to[0],
	    name_to, save_spot);
	unlink(fname2);

	if (!fics_copyfile(fname, fname2)) {
		pprintf(p, "System command in jsave_journalentry failed!\n");
		pprintf(p, "Please report this to an admin.\n");
		fprintf(stderr, "FICS: System command failed in "
		    "jsave_journalentry\n");
		return;
	}

	msnprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s", stats_dir,
	    name_to[0], name_to, STATS_JOURNAL);

	/*
	 * Init context
	 */
	ctx.p = p;
	ctx.from_spot = from_spot;
	ctx.WhiteRating = 0;
	ctx.BlackRating = 0;
	ctx.t = 0;
	ctx.i = 0;
	memset(ctx.WhiteName, 0, sizeof(ctx.WhiteName));
	memset(ctx.BlackName, 0, sizeof(ctx.BlackName));
	memset(ctx.type, 0, sizeof(ctx.type));
	memset(ctx.eco, 0, sizeof(ctx.eco));
	memset(ctx.ending, 0, sizeof(ctx.ending));
	memset(ctx.result, 0, sizeof(ctx.result));

	if (!journal_get_info(&ctx, fname))
		return;

	addjournalitem(p, toupper(save_spot),
	    ctx.WhiteName, ctx.WhiteRating,
	    ctx.BlackName, ctx.BlackRating,
	    ctx.type,
	    ctx.t, ctx.i,
	    ctx.eco,
	    ctx.ending,
	    ctx.result,
	    to_file);
	pprintf(p, "Journal entry %s %c saved in slot %c in journal.\n",
	    parray[p1].name, toupper(from_spot), toupper(save_spot));
}

PUBLIC void
jsave_history(int p, char save_spot, int p1, int from, char *to_file)
{
	FILE	*Game;
	char	*EndSymbol;
	char	*HistoryFname;
	char	*name_to = parray[p].login;
	char	 End[100] = { '\0' };
	char	 filename[MAX_FILENAME_SIZE + 1] = { '\0' }; // XXX
	char	 jfname[MAX_FILENAME_SIZE] = { '\0' };
	char	 type[4];
	int	 g;

	if ((HistoryFname = FindHistory2(p, p1, from, End, sizeof End)) !=
	    NULL) {
		if ((Game = fopen(HistoryFname, "r")) == NULL) {
			pprintf(p, "History game %d not available for %s.\n",
			    from,
			    parray[p1].name);
		} else {
			msnprintf(jfname, sizeof jfname, "%s/%c/%s.%c",
			    journal_dir,
			    name_to[0],
			    name_to,
			    save_spot);
			unlink(jfname);

			if (!fics_copyfile(HistoryFname, jfname)) {
				pprintf(p, "System command in jsave_history "
				    "failed!\n");
				pprintf(p, "Please report this to an admin.\n");
				fprintf(stderr, "FICS: System command failed "
				    "in jsave_journalentry\n");
				fclose(Game);
				return;
			}

			g = game_new(); // Open a dummy game

			// XXX: is 'filename' right here?
			if (ReadGameAttrs(Game, filename, g) < 0) {
				pprintf(p, "Gamefile is corrupt. Please tell "
				    "an admin.\n");
				game_free(g);
				fclose(Game);
				return;
			}

			fclose(Game);

			if (garray[g].private) {
				type[0] = 'p';
			} else {
				type[0] = ' ';
			}

			if (garray[g].type == TYPE_BLITZ) {
				type[1] = 'b';
			} else if (garray[g].type == TYPE_WILD) {
				type[1] = 'w';
			} else if (garray[g].type == TYPE_STAND) {
				type[1] = 's';
			} else {
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
			EndSymbol = EndSym(g);
			addjournalitem(p, toupper(save_spot),
			    garray[g].white_name, garray[g].white_rating,
			    garray[g].black_name, garray[g].black_rating,
			    type,
			    garray[g].wInitTime,
			    garray[g].wIncrement,
			    getECO(g), End, EndSymbol, to_file);
			game_free(g);
			pprintf(p, "Game %s %d saved in slot %c in journal.\n",
			    parray[p1].name, from, toupper(save_spot));
		}
	}
}

PUBLIC int
com_jsave(int p, param_list param)
{
	char	*from;
	char	*to = param[0].val.word;
	char	 fname[MAX_FILENAME_SIZE] = { '\0' };
	int	 p1, p1conn;

	if (!parray[p].registered) {
		pprintf(p, "Only registered players may keep a journal.\n");
		return COM_OK;
	}

	if (strlen(to) != 1 || !(isalpha(to[0]))) {
		pprintf(p, "Journal entries are referenced by single "
		    "letters.\n");
		return COM_OK;
	}

	if ((to[0] - 'a' - 1) > MAX_JOURNAL &&
	    parray[p].adminLevel < ADMIN_ADMIN &&
	    !titled_player(p, parray[p].login)) {
		pprintf(p, "Your maximum journal entry is %c\n",
		    toupper((char)(MAX_JOURNAL + 'A' - 1)));
		return COM_OK;
	}

	if (!FindPlayer(p, param[1].val.word, &p1, &p1conn))
		return COM_OK;

	if (param[2].type == TYPE_INT) {
		// grab from a history
		msnprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s",
		    stats_dir, parray[p].login[0], parray[p].login,
		    STATS_JOURNAL);
		jsave_history(p, to[0], p1, param[2].val.integer, fname);
	} else {
		from = param[2].val.word;

		if (strlen(from) != 1 || !(isalpha(from[0]))) {
			pprintf(p, "Journal entries are referenced by single "
			    "letters.\n");
			if (!p1conn)
				player_remove(p1);
			return COM_OK;
		}

		if (parray[p1].jprivate &&
		    parray[p].adminLevel < ADMIN_ADMIN &&
		    p != p1) {
			pprintf(p, "Sorry, the journal from which you are "
			    "trying to fetch is private.\n");
			if (!p1conn)
				player_remove(p1);
			return COM_OK;
		}

		if ((to[0] - 'a' - 1) > MAX_JOURNAL &&
		    parray[p1].adminLevel < ADMIN_ADMIN &&
		    !titled_player(p, parray[p1].login)) {
			pprintf(p, "%s's maximum journal entry is %c\n",
			    parray[p1].name,
			    toupper((char)(MAX_JOURNAL + 'A' - 1)));

			if (!p1conn)
				player_remove(p1);
			return COM_OK;
		}

		if ((p == p1) && (to[0] == from[0])) {
			pprintf(p, "Source and destination entries are the "
			    "same.\n");
			return COM_OK;
		}

		// grab from a journal
		msnprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s",
		    stats_dir, parray[p].login[0], parray[p].login,
		    STATS_JOURNAL);
		jsave_journalentry(p, to[0], p1, from[0], fname);
	}

	if (!p1conn)
		player_remove(p1);
	return COM_OK;
}

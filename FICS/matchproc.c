/* matchproc.c
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
   name		email			yy/mm/dd	Change
   hersco dhersco@stmarys-ca.edu        95/07/24	Created
   Markus Uhlin                         23/12/14	Fixed compiler warnings
   Markus Uhlin                         24/03/29	Refactored and
							reformatted all
							functions.
   Markus Uhlin                         24/03/29	Size-bounded string
							handling.
   Markus Uhlin                         24/03/29	Fixed potentially
							insecure format strings.
*/

#include "stdinclude.h"
#include "common.h"

#include <sys/resource.h>

#include <err.h>
#include <string.h>

#include "board.h"
#include "command.h"
#include "comproc.h"
#include "eco.h"
#include "formula.h"
#include "gamedb.h"
#include "lists.h"
#include "matchproc.h"
#include "network.h"
#include "obsproc.h"
#include "playerdb.h"
#include "ratings.h"
#include "rmalloc.h"
#include "talkproc.h"
#include "utils.h"

#if __linux__
#include <bsd/string.h>
#endif

struct print_bh_context {
	int	 pp;
	int	 pp1;

	int	 rated;
	int	 type;
	int	 white;

	char	*board;
	char	*category;

	int	 binc;
	int	 bt;
	int	 winc;
	int	 wt;
};

PUBLIC int
create_new_match(int white_player, int black_player, int wt, int winc, int bt,
    int binc, int rated, char *category, char *board, int white)
{
	char	outStr[1024] = { '\0' };
	int	g, p;
	int	reverse = 0;

	if ((g = game_new()) < 0)
		return COM_FAILED;

	if (white == 0) {
		reverse = 1;
	} else if (white == -1) {
		if (wt == bt && winc == binc) {
			if (parray[white_player].lastColor ==
			    parray[black_player].lastColor) {
				int diff1, diff2;

				diff1 = (parray[white_player].num_white -
					 parray[white_player].num_black);
				diff2 = (parray[black_player].num_white -
					 parray[black_player].num_black);

				if (diff1 > diff2)
					reverse = 1;
			} else if (parray[white_player].lastColor == WHITE)
				reverse = 1;
		} else
			reverse = 1;	// Challenger is always white in
					// unbalanced match
	}

	if (reverse) {
		int tmp = white_player;

		white_player = black_player;
		black_player = tmp;
	}

	player_remove_request(white_player, black_player, PEND_MATCH);
	player_remove_request(black_player, white_player, PEND_MATCH);
	player_remove_request(white_player, black_player, PEND_SIMUL);
	player_remove_request(black_player, white_player, PEND_SIMUL);
	player_decline_offers(white_player, -1, PEND_MATCH);
	player_withdraw_offers(white_player, -1, PEND_MATCH);
	player_decline_offers(black_player, -1, PEND_MATCH);
	player_withdraw_offers(black_player, -1, PEND_MATCH);
	player_withdraw_offers(white_player, -1, PEND_SIMUL);
	player_withdraw_offers(black_player, -1, PEND_SIMUL);

	wt	= (wt * 60); // To Seconds
	bt	= (bt * 60);

	garray[g].white = white_player;
	garray[g].black = black_player;

	strlcpy(garray[g].white_name, parray[white_player].name,
	    sizeof(garray[g].white_name));
	strlcpy(garray[g].black_name, parray[black_player].name,
	    sizeof(garray[g].black_name));

	garray[g].status	= GAME_ACTIVE;
	garray[g].type		= game_isblitz(wt / 60, winc, bt / 60, binc,
	    category, board);

	if (garray[g].type == TYPE_UNTIMED ||
	    garray[g].type == TYPE_NONSTANDARD)
		garray[g].rated = 0;
	else
		garray[g].rated = rated;

	garray[g].private = (parray[white_player].private ||
	    parray[black_player].private);
	garray[g].white = white_player;

	if (garray[g].type == TYPE_BLITZ) {
		garray[g].white_rating = parray[white_player].b_stats.rating;
		garray[g].black_rating = parray[black_player].b_stats.rating;
	} else if (garray[g].type == TYPE_WILD) {
		garray[g].white_rating = parray[white_player].w_stats.rating;
		garray[g].black_rating = parray[black_player].w_stats.rating;
	} else if (garray[g].type == TYPE_LIGHT) {
		garray[g].white_rating = parray[white_player].l_stats.rating;
		garray[g].black_rating = parray[black_player].l_stats.rating;
	} else if (garray[g].type == TYPE_BUGHOUSE) {
		garray[g].white_rating = parray[white_player].bug_stats.rating;
		garray[g].black_rating = parray[black_player].bug_stats.rating;
	} else {
		garray[g].white_rating = parray[white_player].s_stats.rating;
		garray[g].black_rating = parray[black_player].s_stats.rating;
	}

	if (board_init(&garray[g].game_state, category, board)) {
		pprintf(white_player, "PROBLEM LOADING BOARD. Game Aborted.\n");
		pprintf(black_player, "PROBLEM LOADING BOARD. Game Aborted.\n");

		fprintf(stderr, "FICS: PROBLEM LOADING BOARD %s %s. "
		    "Game Aborted.\n", category, board);
	}

	garray[g].game_state.gameNum	= g;
	garray[g].wTime			= (wt * 10);
	garray[g].wInitTime		= (wt * 10);
	garray[g].wIncrement		= (winc * 10);
	garray[g].bTime			= (bt * 10);

	if (garray[g].type != TYPE_UNTIMED) {
		if (wt == 0)
			garray[g].wTime = 100;
		if (bt == 0)
			garray[g].bTime = 100;
	} // 0 x games start with 10 seconds

#ifdef TIMESEAL
	garray[g].wRealTime = garray[g].wTime * 100;
	garray[g].bRealTime = garray[g].bTime * 100;
	garray[g].wTimeWhenReceivedMove = 0;
	garray[g].bTimeWhenReceivedMove = 0;
#endif

	garray[g].bInitTime = bt * 10;
	garray[g].bIncrement = binc * 10;

	if (garray[g].game_state.onMove == BLACK) { // Start with black
		garray[g].numHalfMoves = 1;
		garray[g].moveListSize = 1;
		garray[g].moveList = rmalloc(sizeof(move_t));
		garray[g].moveList[0].fromFile = -1;
		garray[g].moveList[0].fromRank = -1;
		garray[g].moveList[0].toFile = -1;
		garray[g].moveList[0].toRank = -1;
		garray[g].moveList[0].color = WHITE;

		strlcpy(garray[g].moveList[0].moveString, "NONE",
		    sizeof(garray[g].moveList[0].moveString));
		strlcpy(garray[g].moveList[0].algString, "NONE",
		    sizeof(garray[g].moveList[0].algString));
	} else {
		garray[g].numHalfMoves = 0;
		garray[g].moveListSize = 0;
		garray[g].moveList = NULL;
	}

	garray[g].timeOfStart	= tenth_secs();
	garray[g].startTime	= tenth_secs();
	garray[g].lastMoveTime	= garray[g].startTime;
	garray[g].lastDecTime	= garray[g].startTime;
	garray[g].clockStopped	= 0;

	snprintf(outStr, sizeof outStr, "\n{Game %d (%s vs. %s) "
	    "Creating %s %s match.}\n",
	    (g + 1),
	    parray[white_player].name,
	    parray[black_player].name,
	    rstr[garray[g].rated],
	    bstr[garray[g].type]);

	pprintf(white_player, "%s", outStr);
	pprintf(black_player, "%s", outStr);

	for (p = 0; p < p_num; p++) {
		int	gnw, gnb;

		if ((p == white_player) || (p == black_player))
			continue;
		if (parray[p].status != PLAYER_PROMPT)
			continue;
		if (parray[p].i_game)
			pprintf_prompt(p, "%s", outStr);

		gnw = in_list(p, L_GNOTIFY, parray[white_player].login);
		gnb = in_list(p, L_GNOTIFY, parray[black_player].login);

		if (gnw || gnb) {
			pprintf(p, "Game notification: ");

			if (gnw) {
				pprintf_highlight(p, "%s",
				    parray[white_player].name);
			} else {
				pprintf(p, "%s", parray[white_player].name);
			}

			pprintf(p, " (%s) vs. ",
			    ratstr(GetRating(&parray[white_player],
			    garray[g].type)));

			if (gnb) {
				pprintf_highlight(p, "%s",
				    parray[black_player].name);
			} else {
				pprintf(p, "%s", parray[black_player].name);
			}

			pprintf_prompt(p, " (%s) %s %s %d %d\n",
			    ratstr(GetRating(&parray[black_player],
			    garray[g].type)),
			    rstr[garray[g].rated],
			    bstr[garray[g].type],
			    (garray[g].wInitTime / 600),
			    (garray[g].wIncrement / 10));
		}
	}

	parray[white_player].game	= g;
	parray[white_player].opponent	= black_player;
	parray[white_player].side	= WHITE;
	parray[white_player].promote	= QUEEN;
	parray[black_player].game	= g;
	parray[black_player].opponent	= white_player;
	parray[black_player].side	= BLACK;
	parray[black_player].promote	= QUEEN;
	send_boards(g);
	MakeFENpos(g, (char *)garray[g].FENstartPos,
	    ARRAY_SIZE(garray[g].FENstartPos));
	return COM_OK;
}

PRIVATE int
accept_match(int p, int p1)
{
	char	 board[50] = { '\0' };
	char	 category[50] = { '\0' };
	char	 tmp[100] = { '\0' };
	int	 bh = 0, pp, pp1;
	int	 g, adjourned, foo, which;
	int	 wt, winc, bt, binc, rated, white;
	pending	*pend;

	// stop observing when match starts
	unobserveAll(p);
	unobserveAll(p1);

	which	= player_find_pendfrom(p, p1, PEND_MATCH);
	pend	= &parray[p].p_from_list[which];
	wt	= pend->param1;
	winc	= pend->param2;
	bt	= pend->param3;
	binc	= pend->param4;
	rated	= pend->param5;

	strlcpy(category, pend->char1, sizeof category);
	strlcpy(board, pend->char2, sizeof board);

	white = (pend->param6 == -1 ? -1 : (1 - pend->param6));

	pprintf(p, "You accept the challenge of %s.\n", parray[p1].name);
	pprintf(p1, "\n%s accepts your challenge.\n", parray[p].name);

	player_remove_request(p, p1, -1);
	player_remove_request(p1, p, -1);

	while ((which = player_find_pendto(p, -1, -1)) != -1) {
		foo = parray[p].p_to_list[which].whoto;
		pprintf_prompt(foo, "\n%s, who was challenging you, "
		    "has joined a match with %s.\n",
		    parray[p].name,
		    parray[p1].name);
		pprintf(p, "Challenge to %s withdrawn.\n", parray[foo].name);
		player_remove_request(p, foo, -1);
	}

	while ((which = player_find_pendto(p1, -1, -1)) != -1) {
		foo = parray[p1].p_to_list[which].whoto;
		pprintf_prompt(foo, "\n%s, who was challenging you, "
		    "has joined a match with %s.\n",
		    parray[p1].name,
		    parray[p].name);
		pprintf(p1, "Challenge to %s withdrawn.\n", parray[foo].name);
		player_remove_request(p1, foo, -1);
	}

	while ((which = player_find_pendfrom(p, -1, -1)) != -1) {
		foo = parray[p].p_from_list[which].whofrom;
		pprintf_prompt(foo, "\n%s, whom you were challenging, "
		    "has joined a match with %s.\n",
		    parray[p].name,
		    parray[p1].name);
		pprintf(p, "Challenge from %s removed.\n", parray[foo].name);
		player_remove_request(foo, p, -1);
	}

	while ((which = player_find_pendfrom(p1, -1, -1)) != -1) {
		foo = parray[p1].p_from_list[which].whofrom;
		pprintf_prompt(foo, "\n%s, whom you were challenging, "
		    "has joined a match with %s.\n",
		    parray[p1].name,
		    parray[p].name);
		pprintf(p1, "Challenge from %s removed.\n", parray[foo].name);
		player_remove_request(foo, p1, -1);
	}

	if (game_isblitz(wt, winc, bt, binc, category, board) == TYPE_WILD &&
	    strcmp(board, "bughouse") == 0) {
		bh = 1;

		if ((pp = parray[p].partner) >= 0 &&
		    (pp1 = parray[p1].partner) >= 0) {
			// stop observing when match starts
			unobserveAll(pp);
			unobserveAll(pp1);

			pprintf(pp, "\nYour partner accepts the challenge of "
			    "%s.\n", parray[p1].name);
			pprintf(pp1, "\nYour partner %s's challenge was "
			    "accepted.\n", parray[p].name);

			while ((which = player_find_pendto(pp, -1, -1)) != -1) {
				foo = parray[pp].p_to_list[which].whoto;
				pprintf_prompt(foo, "\n%s, who was challenging "
				    "you, has joined a match with %s.\n",
				    parray[pp].name,
				    parray[pp1].name);
				pprintf(pp, "Challenge to %s withdrawn.\n",
				    parray[foo].name);
				player_remove_request(pp, foo, -1);
			}

			while ((which = player_find_pendto(pp1, -1, -1)) !=
			    -1) {
				foo = parray[pp1].p_to_list[which].whoto;
				pprintf_prompt(foo, "\n%s, who was challenging "
				    "you, has joined a match with %s.\n",
				    parray[pp1].name,
				    parray[pp].name);
				pprintf(pp1, "Challenge to %s withdrawn.\n",
				    parray[foo].name);
				player_remove_request(pp1, foo, -1);
			}

			while ((which = player_find_pendfrom(pp, -1, -1)) !=
			    -1) {
				foo = parray[pp].p_from_list[which].whofrom;
				pprintf_prompt(foo, "\n%s, whom you were "
				    "challenging, has joined a match with %s."
				    "\n",
				    parray[pp].name,
				    parray[pp1].name);
				pprintf(pp, "Challenge from %s removed.\n",
				    parray[foo].name);
				player_remove_request(foo, pp, -1);
			}

			while ((which = player_find_pendfrom(pp1, -1, -1)) !=
			    -1) {
				foo = parray[pp1].p_from_list[which].whofrom;
				pprintf_prompt(foo, "\n%s, whom you were "
				    "challenging, has joined a match with %s."
				    "\n",
				    parray[pp1].name,
				    parray[pp].name);
				pprintf(pp1, "Challenge from %s removed.\n",
				    parray[foo].name);
				player_remove_request(foo, pp1, -1);
			}
		} else {
			return COM_OK;
		}
	}

	g = game_new();
	adjourned = 0;

	if (game_read(g, p, p1) >= 0) {
		adjourned = 1;
	} else if (game_read(g, p1, p) >= 0) {
		int	swap;

		adjourned = 1;
		swap = p;
		p = p1;
		p1 = swap;
	}

	if (!adjourned) {	// No adjourned game, so begin a new game.
		game_remove(g);

		if (create_new_match(p, p1, wt, winc, bt, binc, rated, category,
		    board, white) != COM_OK) {
			snprintf(tmp, sizeof tmp, "There was a problem "
			    "creating the new match.\n");
			pprintf(p, "%s", tmp);
			pprintf_prompt(p1, "%s", tmp);
		} else if (bh) {
			white = (parray[p].side == WHITE ? 0 : 1);

			if (create_new_match(pp, pp1, wt, winc, bt, binc, rated,
			    category, board, white) != COM_OK) {
				snprintf(tmp, sizeof tmp, "There was a problem "
				    "creating the new match.\n"); // XXX
				pprintf_prompt(pp, "%s", tmp);
				pprintf_prompt(pp1, "%s", tmp);

				snprintf(tmp, sizeof tmp, "There was a problem "
				    "creating your partner's match.\n");
				pprintf(p, "%s", tmp);
				pprintf_prompt(p1, "%s", tmp);
				// IanO: abort_game()?
			} else {
				int	g1 = parray[p].game;
				int	g2 = parray[pp].game;

				garray[g1].link = g2;
				garray[g2].link = g1;

				snprintf(tmp, sizeof tmp, "\nYour partner is "
				    "playing game %d (%s vs. %s).\n",
				    (g2 + 1),
				    garray[g2].white_name,
				    garray[g2].black_name);
				pprintf(p, "%s", tmp);
				pprintf_prompt(p1, "%s", tmp);

				snprintf(tmp, sizeof tmp, "\nYour partner is "
				    "playing game %d (%s vs. %s).\n",
				    (g1 + 1),
				    garray[g1].white_name,
				    garray[g1].black_name);
				pprintf_prompt(pp, "%s", tmp);
				pprintf_prompt(pp1, "%s", tmp);
			}
		}
	} else {	// resume adjourned game
		game_delete(p, p1);

		// XXX: must be either
		if (garray[g].rated != TYPE_UNRATED &&
		    garray[g].rated != TYPE_RATED) {
			warnx("%s: adjourned game neither rated/unrated",
			    __func__);
			warnx("%s: %s vs %s", __func__,
			    parray[p].name,
			    parray[p1].name);
			garray[g].rated = TYPE_UNRATED;
		}

		snprintf(tmp, sizeof tmp, "{Game %d (%s vs. %s) Continuing "
		    "%s %s match.}\n",
		    (g + 1),
		    parray[p].name,
		    parray[p1].name,
		    rstr[garray[g].rated],
		    bstr[garray[g].type]);

		pprintf(p, "%s", tmp);
		pprintf(p1, "%s", tmp);

		garray[g].white = p;
		garray[g].black = p1;
		garray[g].status = GAME_ACTIVE;
		garray[g].result = END_NOTENDED;
		garray[g].startTime = tenth_secs();
		garray[g].lastMoveTime = garray[g].startTime;
		garray[g].lastDecTime = garray[g].startTime;
		parray[p].game = g;
		parray[p].opponent = p1;
		parray[p].side = WHITE;

		parray[p1].game = g;
		parray[p1].opponent = p;
		parray[p1].side = BLACK;

#ifdef TIMESEAL
		garray[g].wRealTime = garray[g].wTime * 100;
		garray[g].bRealTime = garray[g].bTime * 100;
		garray[g].wTimeWhenReceivedMove = 0;
		garray[g].bTimeWhenReceivedMove = 0;
#endif

		send_boards(g);
	}

	return COM_OK;
}

PRIVATE void
print_bughouse(int p, int p1, struct print_bh_context *ctx, char *colorstr[])
{
	const int pp = ctx->pp;
	const int pp1 = ctx->pp1;

	// pp
	pprintf(pp, "\nYour bughouse partner issuing %s (%s) %s",
	    parray[p].name,
	    ratstrii(GetRating(&parray[p], ctx->type), parray[p].registered),
	    colorstr[ctx->white + 1]);
	pprintf_highlight(pp, "%s", parray[p1].name);
	pprintf(pp, " (%s) %s.\n",
	    ratstrii(GetRating(&parray[p1], ctx->type), parray[p1].registered),
	    game_str(ctx->rated, ctx->wt * 60, ctx->winc,
	    ctx->bt * 60, ctx->binc,
	    ctx->category, ctx->board));

	pprintf(pp, "Your game would be ");
	pprintf_highlight(pp, "%s", parray[pp1].name);
	pprintf_prompt(pp, " (%s) %s%s (%s) %s.\n",
	    ratstrii(GetRating(&parray[pp1], ctx->type), parray[pp1].registered),
	    colorstr[ctx->white + 1],
	    parray[pp].name,
	    ratstrii(GetRating(&parray[pp], ctx->type), parray[pp].registered),
	    game_str(ctx->rated, ctx->wt * 60, ctx->winc,
	    ctx->bt * 60, ctx->binc,
	    ctx->category, ctx->board));

	if (parray[pp].bell == 1)
		pprintf_noformat(pp, "\007");

	// pp1
	pprintf(pp1, "\nYour bughouse partner was challenged ");
	pprintf_highlight(pp1, "%s", parray[p].name);
	pprintf(pp1, " (%s) %s",
	    ratstrii(GetRating(&parray[p], ctx->type), parray[p].registered),
	    colorstr[ctx->white + 1]);
	pprintf(pp1, "%s (%s) %s.\n",
	    parray[p1].name,
	    ratstrii(GetRating(&parray[p1], ctx->type), parray[p1].registered),
	    game_str(ctx->rated, ctx->wt * 60, ctx->winc,
	    ctx->bt * 60, ctx->binc,
	    ctx->category, ctx->board));

	pprintf(pp1, "Your game would be %s (%s) %s",
	    parray[pp1].name,
	    ratstrii(GetRating(&parray[pp1], ctx->type), parray[pp1].registered),
	    colorstr[ctx->white + 1]);
	pprintf_highlight(pp1, "%s", parray[pp].name);
	pprintf_prompt(pp1, " (%s) %s.\n",
	    ratstrii(GetRating(&parray[pp], ctx->type), parray[pp].registered),
	    game_str(ctx->rated, ctx->wt * 60, ctx->winc,
	    ctx->bt * 60, ctx->binc,
	    ctx->category, ctx->board));

	if (parray[pp1].bell == 1)
		pprintf_noformat(pp1, "\007");
}

PUBLIC int
com_match(int p, param_list param)
{
	char		 board[100] = { '\0' };
	char		 category[100] = { '\0' };
	char		 parsebuf[100] = { '\0' };
	char		*adjustr[] = { "", " (adjourned)" };
	char		*colorstr[] = { "", "[black] ", "[white] " };
	char		*val;
	int		 adjourned;	// adjourned game?
	int		 bh = 0, pp, pp1;
	int		 binc = -1;	// black increment
	int		 bt = -1;	// black start time
	int		 confused = 0;
	int		 g;		// more adjourned game junk
	int		 p1;
	int		 pendfrom, pendto;
	int		 ppend, p1pend;
	int		 rated = -1;	// 1 = rated, 0 = unrated
	int		 type;
	int		 white = -1;	// 1 = want white, 0 = want black
	int		 winc = -1;	// white increment
	int		 wt = -1;	// white start time
	textlist	*clauses = NULL;

	if (parray[p].game >= 0 && garray[parray[p].game].status ==
	    GAME_EXAMINE) {
		pprintf(p, "You can't challenge while you are examining a game."
		    "\n");
		return COM_OK;
	}
	if (parray[p].game >= 0) {
		pprintf(p, "You can't challenge while you are playing a game."
		    "\n");
		return COM_OK;
	}

	stolower(param[0].val.word);
	p1 = player_find_part_login(param[0].val.word);

	if (p1 < 0) {
		pprintf(p, "No user named \"%s\" is logged in.\n",
		    param[0].val.word);
		return COM_OK;
	}
	if (p1 == p) {	// Allowing to match yourself to enter analysis mode
		ExamineScratch(p, param);
		return COM_OK;
	}

	if (parray[p].open == 0) {
		parray[p].open = 1;
		pprintf(p, "Setting you open for matches.\n");
	}
	if (!parray[p1].open) {
		pprintf(p, "Player \"%s\" is not open to match requests.\n",
		    parray[p1].name);
		return COM_OK;
	}
	if (parray[p1].game >= 0) {
		pprintf(p, "Player \"%s\" is involved in another game.\n",
		    parray[p1].name);
		return COM_OK;
	}

	// Look for an adjourned game between p and p1
	g = game_new();
	adjourned = (game_read(g, p, p1) >= 0 || game_read(g, p1, p) >= 0);

	if (adjourned) {
		type = garray[g].type;
		wt = garray[g].wInitTime / 600;
		bt = garray[g].bInitTime / 600;
		winc = garray[g].wIncrement / 10;
		binc = garray[g].bIncrement / 10;
		rated = garray[g].rated;
	}

	game_remove(g);
	pendto		= player_find_pendto(p, p1, PEND_MATCH);
	pendfrom	= player_find_pendfrom(p, p1, PEND_MATCH);
	category[0]	= '\0';
	board[0]	= '\0';

	if (!adjourned) {
		if (in_list(p1, L_NOPLAY, parray[p].login)) {
			pprintf(p, "You are on %s's noplay list.\n",
			    parray[p1].name);
			return COM_OK;
		}
		if (player_censored(p1, p)) {
			pprintf(p, "Player \"%s\" is censoring you.\n",
			    parray[p1].name);
			return COM_OK;
		}
		if (player_censored(p, p1)) {
			pprintf(p, "You are censoring \"%s\".\n",
			    parray[p1].name);
			return COM_OK;
		}

		if (param[1].type != TYPE_NULL) {
			int	numba; // temp for atoi()

			val = param[1].val.string;

			while (!confused &&
			    sscanf(val, " %99s", parsebuf) == 1) {

				val = eatword(eatwhite(val));

				if (category[0] != '\0' && board[0] == '\0') {
					strlcpy(board, parsebuf, sizeof board);
				} else if (isdigit(*parsebuf)) {
					if ((numba = atoi(parsebuf)) < 0) {
						pprintf(p, "You can't specify "
						    "negative time controls."
						    "\n");
						return COM_OK;
					} else if (numba > 1000) {
						pprintf(p, "You can't specify "
						    "time or inc above 1000."
						    "\n");
						return COM_OK;
					} else if (wt == -1) {
						wt = numba;
					} else if (winc == -1) {
						winc = numba;
					} else if (bt == -1) {
						bt = numba;
					} else if (binc == -1) {
						binc = numba;
					} else {
						confused = 1;
					}
				} else if (strstr("rated", parsebuf) != NULL) {
					if (rated == -1)
						rated = 1;
					else
						confused = 1;
				} else if (strstr("unrated", parsebuf) != NULL) {
					if (rated == -1)
						rated = 0;
					else
						confused = 1;
				} else if (strstr("white", parsebuf) != NULL) {
					if (white == -1)
						white = 1;
					else
						confused = 1;
				} else if (strstr("black", parsebuf) != NULL) {
					if (white == -1)
						white = 0;
					else
						confused = 1;
				} else if (category[0] == '\0') {
					strlcpy(category, parsebuf,
					    sizeof category);
				} else {
					confused = 1;
				}
			} /* while */

			if (confused) {
				pprintf(p, "Can't interpret %s in match "
				    "command.\n", parsebuf);
				return COM_OK;
			}
		}

		rated = (rated == -1 ? parray[p].rated : rated) &&
		    parray[p1].registered && parray[p].registered;

		if (winc == -1)
			winc = (wt == -1 ? parray[p].d_inc : 0);
		if (wt == -1)
			wt = parray[p].d_time;
		if (bt == -1)
			bt = wt;
		if (binc == -1)
			binc = winc;

		if (!strcmp(category, "bughouse")) {	// save mentioning wild
			strlcpy(board, "bughouse", sizeof board);
			strlcpy(category, "wild", sizeof category);
		}

		if (category[0] && !board[0]) {
			pprintf(p, "You must specify a board and a category."
			    "\n");
			return COM_OK;
		}

		if (category[0]) {
			char	fname[MAX_FILENAME_SIZE] = { '\0' };

			(void) snprintf(fname, sizeof fname, "%s/%s/%s",
			    board_dir, category, board);

			if (!file_exists(fname)) {
				pprintf(p, "No such category/board: %s/%s\n",
				    category, board);
				return COM_OK;
			}
		}

		if (pendfrom < 0 && parray[p1].ropen == 0 && rated !=
		    parray[p1].rated) {
			pprintf(p, "%s only wants to play %s games.\n",
			    parray[p1].name,
			    rstr[parray[p1].rated]);
			pprintf_highlight(p1, "Ignoring");
			pprintf(p1, " %srated match request from %s.\n",
			    (parray[p1].rated ? "un" : ""),
			    parray[p].name);

			return COM_OK;
		}

		type = game_isblitz(wt, winc, bt, binc, category, board);

		if (rated && type == TYPE_WILD && !strcmp(board, "bughouse")) {
			pprintf(p, "Game is bughouse - "
			    "reverting to unrated\n");
			rated = 0;	// will need to kill wild and make
					// TYPE_BUGHOUSE
		}
		if (rated && type == TYPE_NONSTANDARD) {
			pprintf(p, "Game is non-standard - "
			    "reverting to unrated\n");
			rated = 0;
		}
		if (rated && type == TYPE_UNTIMED) {
			pprintf(p, "Game is untimed - "
			    "reverting to unrated\n");
			rated = 0;
		}

		// Now check formula.
		if ((pendfrom < 0 || param[1].type != TYPE_NULL) &&
		    !GameMatchesFormula(p, p1, wt, winc, bt, binc, rated, type,
		    &clauses)) {
			pprintf(p, "Match request does not fit formula for "
			    "%s:\n", parray[p1].name);
			pprintf(p, "%s's formula: %s\n",
			    parray[p1].name,
			    parray[p1].formula);

			ShowClauses(p, p1, clauses);
			ClearTextList(clauses);

			pprintf_highlight(p1, "Ignoring");
			pprintf_prompt(p1, " (formula): %s (%d) %s (%d) %s.\n",
			    parray[p].name,
			    GetRating(&parray[p], type),
			    parray[p1].name,
			    GetRating(&parray[p1], type),
			    game_str(rated, wt * 60, winc, bt * 60, binc,
			    category, board));

			return COM_OK;
		}

		if (type == TYPE_WILD && strcmp(board, "bughouse") == 0) {
			bh	= 1;
			pp	= parray[p].partner;
			pp1	= parray[p1].partner;

			if (pp < 0) {
				pprintf(p, "You have no partner for bughouse."
				    "\n");
				return COM_OK;
			}
			if (pp1 < 0) {
				pprintf(p, "Your opponent has no partner for "
				    "bughouse.\n");
				return COM_OK;
			}
			if (pp == pp1) {
				pprintf(p, "You and your opponent both chose "
				    "the same partner!\n");
				return COM_OK;
			}
			if (pp == p1 || pp1 == p) {
				pprintf(p, "You and your opponent can't choose "
				    "each other as partners!\n");
				return COM_OK;
			}
			if (parray[pp].partner != p) {
				pprintf(p, "Your partner hasn't chosen you as "
				    "his partner!\n");
				return COM_OK;
			}
			if (parray[pp1].partner != p1) {
				pprintf(p, "Your opponent's partner hasn't "
				    "chosen your opponent as his partner!\n");
				return COM_OK;
			}
			if (!parray[pp].open || parray[pp].game >= 0) {
				pprintf(p, "Your partner isn't open to play "
				    "right now.\n");
				return COM_OK;
			}
			if (!parray[pp1].open || parray[pp1].game >= 0) {
				pprintf(p, "Your opponent's partner isn't "
				    "open to play right now.\n");
				return COM_OK;
			}

			/*
			 * Bypass NOPLAY lists, censored lists,
			 * ratedness, privacy, and formula for
			 * now. Active challenger/ee will determine
			 * these.
			 */
		}

		// Ok match offer will be made
	} // !adjourned

	if (pendto >= 0) {
		pprintf(p, "Updating offer already made to \"%s\".\n",
		    parray[p1].name);
	}

	if (pendfrom >= 0) {
		if (pendto >= 0) {
			pprintf(p, "Internal error\n");
			fprintf(stderr, "FICS: This shouldn't happen. "
			    "You can't have a match pending from and to the "
			    "same person.\n");
			return COM_OK;
		}

		if (adjourned ||
		    (wt == parray[p].p_from_list[pendfrom].param1 &&
		    winc == parray[p].p_from_list[pendfrom].param2 &&
		    bt == parray[p].p_from_list[pendfrom].param3 &&
		    binc == parray[p].p_from_list[pendfrom].param4 &&
		    rated == parray[p].p_from_list[pendfrom].param5 &&
		    (white == -1 ||
		    white + parray[p].p_from_list[pendfrom].param6 == 1) &&
		    !strcmp(category, parray[p].p_from_list[pendfrom].char1) &&
		    !strcmp(board, parray[p].p_from_list[pendfrom].char2))) {
			// Identical match - should accept!
			accept_match(p, p1);
			return COM_OK;
		} else {
			player_remove_pendfrom(p, p1, PEND_MATCH);
			player_remove_pendto(p1, p, PEND_MATCH);
		}
	}

	if (pendto < 0) {
		if ((ppend = player_new_pendto(p)) < 0) {
			pprintf(p, "Sorry you can't have any more pending "
			    "matches.\n");
			return COM_OK;
		}

		if ((p1pend = player_new_pendfrom(p1)) < 0) {
			pprintf(p, "Sorry %s can't have any more pending "
			    "matches.\n", parray[p1].name);
			parray[p].num_to = parray[p].num_to - 1;
			return COM_OK;
		}
	} else {
		ppend = pendto;
		p1pend = player_find_pendfrom(p1, p, PEND_MATCH);
	}

	parray[p].p_to_list[ppend].param1 = wt;
	parray[p].p_to_list[ppend].param2 = winc;
	parray[p].p_to_list[ppend].param3 = bt;
	parray[p].p_to_list[ppend].param4 = binc;
	parray[p].p_to_list[ppend].param5 = rated;
	parray[p].p_to_list[ppend].param6 = white;

	strlcpy(parray[p].p_to_list[ppend].char1, category,
	    sizeof(parray[p].p_to_list[ppend].char1));
	strlcpy(parray[p].p_to_list[ppend].char2, board,
	    sizeof(parray[p].p_to_list[ppend].char2));

	parray[p].p_to_list[ppend].type = PEND_MATCH;
	parray[p].p_to_list[ppend].whoto = p1;
	parray[p].p_to_list[ppend].whofrom = p;

	parray[p1].p_from_list[p1pend].param1 = wt;
	parray[p1].p_from_list[p1pend].param2 = winc;
	parray[p1].p_from_list[p1pend].param3 = bt;
	parray[p1].p_from_list[p1pend].param4 = binc;
	parray[p1].p_from_list[p1pend].param5 = rated;
	parray[p1].p_from_list[p1pend].param6 = white;

	strlcpy(parray[p1].p_from_list[p1pend].char1, category,
	    sizeof(parray[p1].p_from_list[p1pend].char1));
	strlcpy(parray[p1].p_from_list[p1pend].char2, board,
	    sizeof(parray[p1].p_from_list[p1pend].char2));

	parray[p1].p_from_list[p1pend].type = PEND_MATCH;
	parray[p1].p_from_list[p1pend].whoto = p1;
	parray[p1].p_from_list[p1pend].whofrom = p;

	if (pendfrom >= 0) {
		pprintf(p, "Declining offer from %s and offering new match "
		    "parameters.\n", parray[p1].name);
		pprintf(p1, "\n%s declines your match offer a match with these "
		    "parameters:", parray[p].name);
	}

	if (pendto >= 0) {
		pprintf(p, "Updating match request to: ");
		pprintf(p1, "\n%s updates the match request.\n", parray[p].name);
	} else {
		pprintf(p, "Issuing: ");
		pprintf(p1, "\n"); // XXX: 'parray[p].name'
	}

	pprintf(p, "%s (%s) %s",
	    parray[p].name,
	    ratstrii(GetRating(&parray[p], type), parray[p].registered),
	    colorstr[white + 1]);
	pprintf_highlight(p, "%s", parray[p1].name);
	pprintf(p, " (%s) %s%s.\n",
	    ratstrii(GetRating(&parray[p1], type), parray[p1].registered),
	    game_str(rated, wt * 60, winc, bt * 60, binc, category, board),
	    adjustr[adjourned]);
	pprintf(p1, "Challenge: ");
	pprintf_highlight(p1, "%s", parray[p].name);
	pprintf(p1, " (%s) %s",
	    ratstrii(GetRating(&parray[p], type), parray[p].registered),
	    colorstr[white + 1]);
	pprintf(p1, "%s (%s) %s%s.\n",
	    parray[p1].name,
	    ratstrii(GetRating(&parray[p1], type), parray[p1].registered),
	    game_str(rated, wt * 60, winc, bt * 60, binc, category, board),
	    adjustr[adjourned]);

	if (parray[p1].bell == 1)
		pprintf_noformat(p1, "\007");

	if (bh) {
		struct print_bh_context ctx = {
			.pp = pp,
			.pp1 = pp1,
			.rated		= rated,
			.type		= type,
			.white		= white,
			.board		= &board[0],
			.category	= &category[0],
			.binc	= binc,
			.bt	= bt,
			.winc	= winc,
			.wt	= wt,
		};

		print_bughouse(p, p1, &ctx, colorstr);
	}

	if (in_list(p, L_COMPUTER, parray[p].name)) {
		pprintf(p1, "--** %s is a ", parray[p].name);
		pprintf_highlight(p1, "computer");
		pprintf(p1, " **--\n");
	}
	if (in_list(p, L_COMPUTER, parray[p1].name)) {
		pprintf(p, "--** %s is a ", parray[p1].name);
		pprintf_highlight(p, "computer");
		pprintf(p, " **--\n");
	}
	if (in_list(p, L_ABUSER, parray[p].name)) {
		pprintf(p1, "--** %s is in the ", parray[p].name);
		pprintf_highlight(p1, "abuser");
		pprintf(p1, " list **--\n");
	}
	if (in_list(p, L_ABUSER, parray[p1].name)) {
		pprintf(p, "--** %s is in the ", parray[p1].name);
		pprintf_highlight(p, "abuser");
		pprintf(p, " list **--\n");
	}

	if (rated) {
		double	newsterr;
		int	win, draw, loss;

		rating_sterr_delta(p1, p, type, time(NULL), RESULT_WIN, &win,
		    &newsterr);
		rating_sterr_delta(p1, p, type, time(NULL), RESULT_DRAW, &draw,
		    &newsterr);
		rating_sterr_delta(p1, p, type, time(NULL), RESULT_LOSS, &loss,
		    &newsterr);

		pprintf(p1, "Your %s rating will change:  "
		    "Win: %s%d,  Draw: %s%d,  Loss: %s%d\n",
		    bstr[type],
		    (win >= 0 ? "+" : ""), win,
		    (draw >= 0 ? "+" : ""), draw,
		    (loss >= 0 ? "+" : ""), loss);
		pprintf(p1, "Your new RD will be %5.1f\n", newsterr);

		rating_sterr_delta(p, p1, type, time(NULL), RESULT_WIN, &win,
		    &newsterr);
		rating_sterr_delta(p, p1, type, time(NULL), RESULT_DRAW, &draw,
		    &newsterr);
		rating_sterr_delta(p, p1, type, time(NULL), RESULT_LOSS, &loss,
		    &newsterr);

		pprintf(p, "Your %s rating will change:  "
		    "Win: %s%d,  Draw: %s%d,  Loss: %s%d\n",
		    bstr[type],
		    (win >= 0 ? "+" : ""), win,
		    (draw >= 0 ? "+" : ""), draw,
		    (loss >= 0 ? "+" : ""), loss);
		pprintf(p, "Your new RD will be %5.1f\n", newsterr);
	}

	pprintf_prompt(p1, "You can \"accept\" or \"decline\", or propose "
	    "different parameters.\n");
	return COM_OK;
}

PUBLIC int
com_accept(int p, param_list param)
{
	int	acceptNum = -1;
	int	from;
	int	p1;
	int	type = -1;

	if (parray[p].num_from == 0) {
		pprintf(p, "You have no offers to accept.\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_NULL) {
		if (parray[p].num_from != 1) {
			pprintf(p, "You have more than one offer to accept.\n"
			    "Use \"pending\" to see them and \"accept n\" to "
			    "choose which one.\n");
			return COM_OK;
		}

		acceptNum = 0;
	} else if (param[0].type == TYPE_INT) {
		if (param[0].val.integer < 1 ||
		    param[0].val.integer > parray[p].num_from) {
			pprintf(p, "Out of range. Use \"pending\" to see "
			    "the list of offers.\n");
			return COM_OK;
		}

		acceptNum = (param[0].val.integer - 1);
	} else if (param[0].type == TYPE_WORD) {
		if (!strcmp(param[0].val.word, "draw")) {
			type = PEND_DRAW;
		} else if (!strcmp(param[0].val.word, "pause")) {
			type = PEND_PAUSE;
		} else if (!strcmp(param[0].val.word, "adjourn")) {
			type = PEND_ADJOURN;
		} else if (!strcmp(param[0].val.word, "abort")) {
			type = PEND_ABORT;
		} else if (!strcmp(param[0].val.word, "takeback")) {
			type = PEND_TAKEBACK;
		} else if (!strcmp(param[0].val.word, "simmatch")) {
			type = PEND_SIMUL;
		} else if (!strcmp(param[0].val.word, "switch")) {
			type = PEND_SWITCH;
		} else if (!strcmp(param[0].val.word, "partner")) {
			type = PEND_PARTNER;
		}

		if (type >= 0) {
			acceptNum = player_find_pendfrom(p, -1, type);

			if (acceptNum < 0) {
				pprintf(p, "There are no pending %s offers.\n",
				    param[0].val.word);
				return COM_OK;
			}
		} else {	// Word must be a name
			p1 = player_find_part_login(param[0].val.word);

			if (p1 < 0) {
				pprintf(p, "No user named \"%s\" is logged "
				    "in.\n", param[0].val.word);
				return COM_OK;
			}

			if ((acceptNum = player_find_pendfrom(p, p1, -1)) < 0) {
				pprintf(p, "There are no pending offers from "
				    "%s.\n", parray[p1].name);
				return COM_OK;
			}
		}
	}

	if (acceptNum < 0 || acceptNum >= ARRAY_SIZE(parray[0].p_from_list)) {
		pprintf(p, "Accept number out-of-bounds!\n");
		return COM_FAILED;
	}

	from = parray[p].p_from_list[acceptNum].whofrom;

	switch (parray[p].p_from_list[acceptNum].type) {
	case PEND_MATCH:
		accept_match(p, from);
		return COM_OK;
	case PEND_DRAW:
		pcommand(p, "draw");
		break;
	case PEND_PAUSE:
		pcommand(p, "pause");
		break;
	case PEND_ABORT:
		pcommand(p, "abort");
		break;
	case PEND_TAKEBACK:
		pcommand(p, "takeback %d",
		    parray[p].p_from_list[acceptNum].param1);
		break;
	case PEND_SIMUL:
		pcommand(p, "simmatch %s", parray[from].name);
		break;
	case PEND_SWITCH:
		pcommand(p, "switch");
		break;
	case PEND_ADJOURN:
		pcommand(p, "adjourn");
		break;
	case PEND_PARTNER:
		pcommand(p, "partner %s", parray[from].name);
		break;
	}

	return COM_OK_NOPROMPT;
}

PRIVATE int
WordToOffer(int p, char *Word, int *type, int *p1)
{
	/*
	 * Convert draw adjourn match takeback abort pause simmatch
	 * switch partner or <name> to offer type.
	 */

	if (!strcmp(Word, "match")) {
		*type = PEND_MATCH;
	} else if (!strcmp(Word, "draw")) {
		*type = PEND_DRAW;
	} else if (!strcmp(Word, "pause")) {
		*type = PEND_PAUSE;
	} else if (!strcmp(Word, "abort")) {
		*type = PEND_ABORT;
	} else if (!strcmp(Word, "takeback")) {
		*type = PEND_TAKEBACK;
	} else if (!strcmp(Word, "adjourn")) {
		*type = PEND_ADJOURN;
	} else if (!strcmp(Word, "switch")) {
		*type = PEND_SWITCH;
	} else if (!strcmp(Word, "simul")) {
		*type = PEND_SIMUL;
	} else if (!strcmp(Word, "partner")) {
		*type = PEND_PARTNER;
	} else if (!strcmp(Word, "all")) {
		;
	} else {
		if ((*p1 = player_find_part_login(Word)) < 0) {
			pprintf(p, "No user named \"%s\" is logged in.\n",
			    Word);
			return 0;
		}
	}

	return 1;
}

PUBLIC int
com_decline(int p, param_list param)
{
	int	count;
	int	declineNum;
	int	p1 = -1;
	int	type = -1;

	if (parray[p].num_from == 0) {
		pprintf(p, "You have no pending offers from other players.\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_NULL) {
		if (parray[p].num_from == 1) {
			p1 = parray[p].p_from_list[0].whofrom;
			type = parray[p].p_from_list[0].type;
		} else {
			pprintf(p, "You have more than one pending offer. "
			    "Please specify which one\nyou wish to decline.\n"
			    "'Pending' will give you the list.\n");
			return COM_OK;
		}
	} else {
		if (param[0].type == TYPE_WORD) {
			if (!WordToOffer(p, param[0].val.word, &type, &p1))
				return COM_OK;
		} else { // Must be an integer
			declineNum = param[0].val.integer - 1;

			if (declineNum >= parray[p].num_from || declineNum < 0) {
				pprintf(p, "Invalid offer number. Must be "
				    "between 1 and %d.\n", parray[p].num_from);
				return COM_OK;
			}

			p1 = parray[p].p_from_list[declineNum].whofrom;
			type = parray[p].p_from_list[declineNum].type;
		}
	}

	if ((count = player_decline_offers(p, p1, type)) != 1)
		pprintf(p, "%d offers declined\n", count);
	return COM_OK;
}

PUBLIC int
com_withdraw(int p, param_list param)
{
	int	count;
	int	p1 = -1;
	int	type = -1;
	int	withdrawNum;

	if (parray[p].num_to == 0) {
		pprintf(p, "You have no pending offers to other players.\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_NULL) {
		if (parray[p].num_to == 1) {
			p1 = parray[p].p_to_list[0].whoto;
			type = parray[p].p_to_list[0].type;
		} else {
			pprintf(p, "You have more than one pending offer. "
			    "Please specify which one\nyou wish to withdraw.\n"
			    "'Pending' will give you the list.\n");
			return COM_OK;
		}
	} else {
		if (param[0].type == TYPE_WORD) {
			if (!WordToOffer(p, param[0].val.word, &type, &p1))
				return COM_OK;
		} else { // Must be an integer
			withdrawNum = param[0].val.integer - 1;

			if (withdrawNum >= parray[p].num_to || withdrawNum < 0) {
				pprintf(p, "Invalid offer number. Must be "
				    "between 1 and %d.\n", parray[p].num_to);
				return COM_OK;
			}

			p1 = parray[p].p_to_list[withdrawNum].whoto;
			type = parray[p].p_to_list[withdrawNum].type;
		}
	}

	if ((count = player_withdraw_offers(p, p1, type)) != 1)
		pprintf(p, "%d offers withdrawn\n", count);
	return COM_OK;
}

PUBLIC int
com_pending(int p, param_list param)
{
	int i;

	if (!parray[p].num_to) {
		pprintf(p, "There are no offers pending TO other players.\n");
	} else {
		pprintf(p, "Offers TO other players:\n");

		for (i = 0; i < parray[p].num_to; i++) {
			pprintf(p, "   ");
			player_pend_print(p, &parray[p].p_to_list[i]);
		}
	}

	if (!parray[p].num_from) {
		pprintf(p, "\nThere are no offers pending FROM other players."
		    "\n");
	} else {
		pprintf(p, "\nOffers FROM other players:\n");

		for (i = 0; i < parray[p].num_from; i++) {
			pprintf(p, " %d: ", i + 1);
			player_pend_print(p, &parray[p].p_from_list[i]);
		}

		pprintf(p, "\nIf you wish to accept any of these offers type "
		    "'accept n'\nor just 'accept' if there is only one offer."
		    "\n");
	}

	return COM_OK;
}

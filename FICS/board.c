/* board.c
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
   Markus Uhlin                 23/12/17	Fixed compiler warnings
   Markus Uhlin                 23/12/23	Reformatted all functions
   Markus Uhlin                 24/04/13	Added usage of the functions
						from 'maxxes-utils.h'.
   Markus Uhlin                 24/06/01	Added and made use of brand().
   Markus Uhlin                 25/04/06	Fixed Clang Tidy warnings.
   Markus Uhlin                 25/09/02	wild_update: fixed file created
						without restricting permissions.
*/

#include "stdinclude.h"
#include "common.h"

#include <err.h>
#include <limits.h>

#include "board.h"
#include "ficsmain.h"
#include "gamedb.h"
#include "maxxes-utils.h"
#include "playerdb.h"
#include "utils.h"

#define WHITE_SQUARE	1
#define BLACK_SQUARE	0
#define ANY_SQUARE	-1

#define SquareColor(f, r) ((f ^ r) & 1)

#define IsMachineStyle(n) (((1 << (n)) & mach_type) != 0)

PUBLIC char	*wpstring[] = {" ", "P", "N", "B", "R", "Q", "K"};
PUBLIC char	*bpstring[] = {" ", "p", "n", "b", "r", "q", "k"};

PUBLIC int pieceValues[7] = {0, 1, 3, 3, 5, 9, 0};

PRIVATE int (*styleFuncs[MAX_STYLES])(game_state_t *, move_t *) = {
	style1,
	style2,
	style3,
	style4,
	style5,
	style6,
	style7,
	style8,
	style9,
	style10,
	style11,
	style12,
	style13
};

PRIVATE char bstring[MAX_BOARD_STRING_LEGTH];

PRIVATE const int mach_type = ((1 << 7) |
    (1 << 8) |
    (1 << 9) |
    (1 << 10) |
    (1 << 11));

/*
 * Globals used for each board
 */
PRIVATE char	*wName, *bName;
PRIVATE int	 wTime, bTime;
PRIVATE int	 orient;
PRIVATE int	 forPlayer;
PRIVATE int	 myTurn;

PRIVATE int
brand(void)
{
#if RAND_MAX < 32767 || RAND_MAX > INT_MAX
#error RAND_MAX unacceptable
#endif
	return ((int) arc4random_uniform(RAND_MAX));
}

PUBLIC int
board_init(game_state_t *b, char *category, char *board)
{
	int	retval;
	int	wval;

	if (category == NULL || board == NULL ||
	    !category[0] || !board[0]) {
		retval = board_read_file("standard", "standard", b);
	} else {
		if (!strcmp(category, "wild")) {
			if (sscanf(board, "%d", &wval) == 1 &&
			    wval >= 1 &&
			    wval <= 4)
				wild_update(wval);
		}

		retval = board_read_file(category, board, b);
	}

	b->gameNum = -1;
	return retval;
}

PUBLIC void
board_calc_strength(game_state_t *b, int *ws, int *bs)
{
	int	*p;
	int	 r, f;

	*ws = *bs = 0;

	for (f = 0; f < 8; f++) {
		for (r = 0; r < 8; r++) {
			if (colorval(b->board[r][f]) == WHITE)
				p = ws;
			else
				p = bs;
			*p += pieceValues[piecetype(b->board[r][f])];
		}
	}

	for (r = PAWN; r <= QUEEN; r++) {
		*ws += (b->holding[0][r - 1] * pieceValues[r]);
		*bs += (b->holding[1][r - 1] * pieceValues[r]);
	}
}

PRIVATE char *
holding_str(int *holding)
{
	int p, i, j;
	static char tmp[30];

	i = 0;

	for (p = PAWN; p <= QUEEN; p++) {
		for (j = 0; j < holding[p - 1]; j++)
			tmp[i++] = wpstring[p][0];
	}

	tmp[i] = '\0';
	return tmp;
}

PRIVATE char *
append_holding_machine(char *buf, const size_t bufsize, int g, int c, int p)
{
	char		 tmp[50];
	game_state_t	*gs = &garray[g].game_state;

	msnprintf(tmp, sizeof tmp, "<b1> game %d white [%s] black [", (g + 1),
	    holding_str(gs->holding[0]));
	mstrlcat(tmp, holding_str(gs->holding[1]), sizeof tmp);
	mstrlcat(buf, tmp, bufsize);

	if (p) {
		msnprintf(tmp, sizeof tmp, "] <- %c%s\n", "WB"[c], wpstring[p]);
		mstrlcat(buf, tmp, bufsize);
	} else
		mstrlcat(buf, "]\n", bufsize);
	return buf;
}

PRIVATE char *
append_holding_display(char *buf, const size_t bufsize, game_state_t *gs,
    int white)
{
	if (white)
		mstrlcat(buf, "White holding: [", bufsize);
	else
		mstrlcat(buf, "Black holding: [", bufsize);
	mstrlcat(buf, holding_str(gs->holding[white ? 0 : 1]), bufsize);
	mstrlcat(buf, "]\n", bufsize);
	return buf;
}

PUBLIC void
update_holding(int g, int pieceCaptured)
{
	char		 tmp1[80];
	char		 tmp2[80];
	game_state_t	*gs = &garray[g].game_state;
	int		 c = colorval(pieceCaptured);
	int		 p = piecetype(pieceCaptured);
	int		 pp, pl;

	if (c == WHITE) {
		c = 0;
		pp = garray[g].white;
	} else {
		c = 1;
		pp = garray[g].black;
	}

	gs->holding[c][p - 1]++;

	tmp1[0] = '\0';
	append_holding_machine(tmp1, sizeof tmp1, g, c, p);

	msnprintf(tmp2, sizeof tmp2, "Game %d %s received: %s -> [%s]\n",
	    (g + 1),
	    parray[pp].name,
	    wpstring[p],
	    holding_str(gs->holding[c]));

	for (pl = 0; pl < p_num; pl++) {
		if (parray[pl].status == PLAYER_EMPTY)
			continue;

		if (player_is_observe(pl, g) || parray[pl].game == g) {
			pprintf_prompt(pl, "%s",
			    (IsMachineStyle(parray[pl].style) ? &tmp1[0] :
			    &tmp2[0]));
		}
	}
}

PUBLIC char *
board_to_string(char *wn, char *bn, int wt, int bt, game_state_t *b, move_t *ml,
    int style, int orientation, int relation, int p)
{
	int bh = (b->gameNum >= 0 && garray[b->gameNum].link >= 0);

	wName		= wn;
	bName		= bn;
	wTime		= wt;
	bTime		= bt;
	orient		= orientation;
	myTurn		= relation;
	forPlayer	= p;

	if (style < 0 || style >= MAX_STYLES)
		return NULL;

	if (style != 11) {    // game header
		msnprintf(bstring, sizeof bstring, "Game %d (%s vs. %s)\n\n",
		    (b->gameNum + 1),
		    garray[b->gameNum].white_name,
		    garray[b->gameNum].black_name);
	} else
		bstring[0] = '\0';

	if (bh && !IsMachineStyle(style)) {
		append_holding_display(bstring, sizeof bstring, b,
		    (orientation == BLACK));
	}
	if (styleFuncs[style](b, ml))
		return NULL;
	if (bh) {
		if (IsMachineStyle(style)) {
			append_holding_machine(bstring, sizeof bstring,
			    b->gameNum, 0, 0);
		} else {
			append_holding_display(bstring, sizeof bstring, b,
			    (orientation == WHITE));
		}
	}

	return bstring;
}

PUBLIC char *
move_and_time(move_t *m)
{
	static char tmp[20];

	msnprintf(tmp, sizeof tmp, "%-7s (%s)", m->algString,
	    tenth_str(m->tookTime, 0));
	return &tmp[0];
}

PRIVATE int
genstyle(game_state_t *b, move_t *ml, char *wp[], char *bp[],
    char *wsqr, char *bsqr, char *top, char *mid, char *start, char *end,
    char *label, char *blabel)
{
	char	tmp[80];
	int	f, r, count;
	int	first, last, inc;
	int	ws, bs;

	board_calc_strength(b, &ws, &bs);

	if (orient == WHITE) {
		first	= 7;
		last	= 0;
		inc	= -1;
	} else {
		first	= 0;
		last	= 7;
		inc	= 1;
	}

	mstrlcat(bstring, top, sizeof bstring);

	for (f = first, count = 7; f != last + inc; f += inc, count--) {
		msnprintf(tmp, sizeof tmp, "     %d  %s", f + 1, start);
		mstrlcat(bstring, tmp, sizeof bstring);

		for (r = last; r != first - inc; r = r - inc) {
			if (square_color(r, f) == WHITE)
				mstrlcat(bstring, wsqr, sizeof bstring);
			else
				mstrlcat(bstring, bsqr, sizeof bstring);

			if (piecetype(b->board[r][f]) == NOPIECE) {
				if (square_color(r, f) == WHITE) {
					mstrlcat(bstring, bp[0],
					    sizeof bstring);
				} else {
					mstrlcat(bstring, wp[0],
					    sizeof bstring);
				}
			} else {
				if (colorval(b->board[r][f]) == WHITE) {
					mstrlcat(bstring,
					    wp[piecetype(b->board[r][f])],
					    sizeof bstring);
				} else {
					mstrlcat(bstring,
					    bp[piecetype(b->board[r][f])],
					    sizeof bstring);
				}
			}
		}

		msnprintf(tmp, sizeof tmp, "%s", end);
		mstrlcat(bstring, tmp, sizeof bstring);

		switch (count) {
		case 7:
			msnprintf(tmp, sizeof tmp, "     Move # : %d (%s)",
			    b->moveNum,
			    CString(b->onMove));
			mstrlcat(bstring, tmp, sizeof bstring);
			break;
		case 6:
			if (garray[b->gameNum].numHalfMoves > 0) {
				// loon: think this fixes the crashing ascii
				// board on takeback bug
				msnprintf(tmp, sizeof tmp, "     %s Moves : "
				    "'%s'",
				    CString(CToggle(b->onMove)), move_and_time
				    (&ml[garray[b->gameNum].numHalfMoves - 1]));
				mstrlcat(bstring, tmp, sizeof bstring);
			}
			break;
		case 5:
			break;
		case 4:
			msnprintf(tmp, sizeof tmp, "     Black Clock : %s",
			    tenth_str((bTime > 0 ? bTime : 0), 1));
			mstrlcat(bstring, tmp, sizeof bstring);
			break;
		case 3:
			msnprintf(tmp, sizeof tmp, "     White Clock : %s",
			    tenth_str((wTime > 0 ? wTime : 0), 1));
			mstrlcat(bstring, tmp, sizeof bstring);
			break;
		case 2:
			msnprintf(tmp, sizeof tmp, "     Black Strength : %d",
			    bs);
			mstrlcat(bstring, tmp, sizeof bstring);
			break;
		case 1:
			msnprintf(tmp, sizeof tmp, "     White Strength : %d",
			    ws);
			mstrlcat(bstring, tmp, sizeof bstring);
			break;
		case 0:
			break;
		} // switch

		mstrlcat(bstring, "\n", sizeof bstring);

		if (count != 0)
			mstrlcat(bstring, mid, sizeof bstring);
		else
			mstrlcat(bstring, top, sizeof bstring);
	} // for

	if (orient == WHITE)
		mstrlcat(bstring, label, sizeof bstring);
	else
		mstrlcat(bstring, blabel, sizeof bstring);
	return 0;
}

/*
 * Experimental ANSI board for colour representation
 */
PUBLIC int
style13(game_state_t *b, move_t *ml)
{
	static char	*wp[] = {
		"   ",
		"\033[37m\033[1m P ",
		"\033[37m\033[1m N ",
		"\033[37m\033[1m B ",
		"\033[37m\033[1m R ",
		"\033[37m\033[1m Q ",
		"\033[37m\033[1m K "
	};
	static char	*bp[] = {
		"   ",
		"\033[21m\033[37m P ",
		"\033[21m\033[37m N ",
		"\033[21m\033[37m B ",
		"\033[21m\033[37m R ",
		"\033[21m\033[37m Q ",
		"\033[21m\033[37m K "
	};
	static char	*wsqr = "\033[40m";
	static char	*bsqr = "\033[45m";
	static char	*top = "\t+------------------------+\n";
	static char	*mid = "";
	static char	*start = "|";
	static char	*end = "\033[0m|";
	static char	*label = "\t  a  b  c  d  e  f  g  h\n";
	static char	*blabel = "\t  h  g  f  e  d  c  b  a\n";

	return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label,
	    blabel);
}

/*
 * Standard ICS
 */
PUBLIC int
style1(game_state_t *b, move_t *ml)
{
	static char	*wp[] = {
		"   |",
		" P |",
		" N |",
		" B |",
		" R |",
		" Q |",
		" K |"
	};
	static char	*bp[] = {
		"   |",
		" *P|",
		" *N|",
		" *B|",
		" *R|",
		" *Q|",
		" *K|"
	};
	static char	*wsqr = "";
	static char	*bsqr = "";
	static char	*top = "\t---------------------------------\n";
	static char	*mid = "\t|---+---+---+---+---+---+---+---|\n";
	static char	*start = "|";
	static char	*end = "";
	static char	*label = "\t  a   b   c   d   e   f   g   h\n";
	static char	*blabel = "\t  h   g   f   e   d   c   b   a\n";

	return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label,
	    blabel);
}

/*
 * USA-Today Sports Center-style board
 */
PUBLIC int
style2(game_state_t *b, move_t *ml)
{
	static char	*wp[] = {
		"+  ",
		"P  ",
		"N  ",
		"B  ",
		"R  ",
		"Q  ",
		"K  "
	};
	static char	*bp[] = {
		"-  ",
		"p' ",
		"n' ",
		"b' ",
		"r' ",
		"q' ",
		"k' "
	};
	static char	*wsqr = "";
	static char	*bsqr = "";
	static char	*top = "";
	static char	*mid = "";
	static char	*start = "";
	static char	*end = "";
	static char	*label = "\ta  b  c  d  e  f  g  h\n";
	static char	*blabel = "\th  g  f  e  d  c  b  a\n";

	return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label,
	    blabel);
}

/*
 * Experimental vt-100 ANSI board for dark backgrounds
 */
PUBLIC int
style3(game_state_t *b, move_t *ml)
{
	static char	*wp[] = {
		"   ",
		" P ",
		" N ",
		" B ",
		" R ",
		" Q ",
		" K "
	};
	static char	*bp[] = {
		"   ",
		" *P",
		" *N",
		" *B",
		" *R",
		" *Q",
		" *K"
	};
	static char	*wsqr = "\033[0m";
	static char	*bsqr = "\033[7m";
	static char	*top = "\t+------------------------+\n";
	static char	*mid = "";
	static char	*start = "|";
	static char	*end = "\033[0m|";
	static char	*label = "\t  a  b  c  d  e  f  g  h\n";
	static char	*blabel = "\t  h  g  f  e  d  c  b  a\n";

	return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label,
	    blabel);
}

/*
 * Experimental vt-100 ANSI board for light backgrounds
 */
PUBLIC int
style4(game_state_t *b, move_t *ml)
{
	static char	*wp[] = {
		"   ",
		" P ",
		" N ",
		" B ",
		" R ",
		" Q ",
		" K "
	};
	static char	*bp[] = {
		"   ",
		" *P",
		" *N",
		" *B",
		" *R",
		" *Q",
		" *K"
	};
	static char	*wsqr = "\033[7m";
	static char	*bsqr = "\033[0m";
	static char	*top = "\t+------------------------+\n";
	static char	*mid = "";
	static char	*start = "|";
	static char	*end = "\033[0m|";
	static char	*label = "\t  a  b  c  d  e  f  g  h\n";
	static char	*blabel = "\t  h  g  f  e  d  c  b  a\n";

	return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label,
	    blabel);
}

/*
 * Style suggested by ajpierce@med.unc.edu
 */
PUBLIC int style5(game_state_t *b, move_t *ml)
{
	static char	*wp[] = {
		"    ",
		"  o ",
		" :N:",
		" <B>",
		" |R|",
		" {Q}",
		" =K="
	};
	static char	*bp[] = {
		"    ",
		"  p ",
		" :n:",
		" <b>",
		" |r|",
		" {q}",
		" =k="
	};
	static char	*wsqr = "";
	static char	*bsqr = "";
	static char	*top = "        .   .   .   .   .   .   .   .   .\n";
	static char	*mid = "        .   .   .   .   .   .   .   .   .\n";
	static char	*start = "";
	static char	*end = "";
	static char	*label = "\t  a   b   c   d   e   f   g   h\n";
	static char	*blabel = "\t  h   g   f   e   d   c   b   a\n";

	return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label,
	    blabel);
}

/*
 * Email Board suggested by Thomas Fought (tlf@rsch.oclc.org)
 */
PUBLIC int
style6(game_state_t *b, move_t *ml)
{
	static char	*wp[] = {
		"    |",
		" wp |",
		" WN |",
		" WB |",
		" WR |",
		" WQ |",
		" WK |"
	};
	static char	*bp[] = {
		"    |",
		" bp |",
		" BN |",
		" BB |",
		" BR |",
		" BQ |",
		" BK |"};
	static char	*wsqr = "";
	static char	*bsqr = "";
	static char	*top = "\t-----------------------------------------\n";
	static char	*mid = "\t-----------------------------------------\n";
	static char	*start = "|";
	static char	*end = "";
	static char	*label = "\t  A    B    C    D    E    F    G    H\n";
	static char	*blabel = "\t  H    G    F    E    D    C    B    A\n";

	return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label,
	    blabel);
}

/*
 * Miniature board
 */
PUBLIC int
style7(game_state_t *b, move_t *ml)
{
	static char	*wp[] = {"  ", " P", " N", " B", " R", " Q", " K"};
	static char	*bp[] = {" -", " p", " n", " b", " r", " q", " k"};
	static char	*wsqr = "";
	static char	*bsqr = "";
	static char	*top = "\t:::::::::::::::::::::\n";
	static char	*mid = "";
	static char	*start = "..";
	static char	*end = " ..";
	static char	*label = "\t   a b c d e f g h\n";
	static char	*blabel = "\t   h g f e d c b a\n";

	return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label,
	    blabel);
}

/*
 * ICS interface maker board  --  raw data dump.
 */
PUBLIC int
style8(game_state_t *b, move_t *ml)
{
	char	tmp[80];
	int	f, r;
	int	ws, bs;

	board_calc_strength(b, &ws, &bs);

	msnprintf(tmp, sizeof tmp, "#@#%03d%-16s%s%-16s%s",
	    (b->gameNum + 1),

	    garray[b->gameNum].white_name,
	    (orient == WHITE ? "*" : ":"),

	    garray[b->gameNum].black_name,
	    (orient == WHITE ? ":" : "*"));

	mstrlcat(bstring, tmp, sizeof bstring);

	for (r = 0; r < 8; r++) {
		for (f = 0; f < 8; f++) {
			if (b->board[f][r] == NOPIECE) {
				mstrlcat(bstring, " ", sizeof bstring);
			} else {
				if (colorval(b->board[f][r]) == WHITE) {
					mstrlcat(bstring, wpstring
					    [piecetype(b->board[f][r])],
					    sizeof bstring);
				} else {
					mstrlcat(bstring, bpstring
					    [piecetype(b->board[f][r])],
					    sizeof bstring);
				}
			}
		}
	}

	msnprintf(tmp, sizeof tmp, "%03d%s%02d%02d%05d%05d%-7s(%s)@#@\n",
	    (garray[b->gameNum].numHalfMoves / 2 + 1),
	    (b->onMove == WHITE ? "W" : "B"),
	    ws,
	    bs,
	    ((wTime + 5) / 10),
	    ((bTime + 5) / 10),

	    (garray[b->gameNum].numHalfMoves
	    ? ml[garray[b->gameNum].numHalfMoves - 1].algString
	    : "none"),

	    (garray[b->gameNum].numHalfMoves
	    ? tenth_str(ml[garray[b->gameNum].numHalfMoves - 1].tookTime, 0)
	    : "0:00"));

	mstrlcat(bstring, tmp, sizeof bstring);
	return 0;
}

/*
 * Last 2 moves only (previous non-verbose mode).
 */
PUBLIC int
style9(game_state_t *b, move_t *ml)
{
	char	tmp[80];
	int	i, count;
	int	startmove;

	msnprintf(tmp, sizeof tmp, "\nMove     %-23s%s\n",
	    garray[b->gameNum].white_name,
	    garray[b->gameNum].black_name);
	mstrlcat(bstring, tmp, sizeof bstring);

	msnprintf(tmp, sizeof tmp, "----     --------------         "
	    "--------------\n");
	mstrlcat(bstring, tmp, sizeof bstring);

	startmove = ((garray[b->gameNum].numHalfMoves - 3) / 2) * 2;

	if (startmove < 0)
		startmove = 0;

	i = startmove;
	count = 0;

	while (i < garray[b->gameNum].numHalfMoves && count < 4) {
		if (!(i & 0x01)) {
			msnprintf(tmp, sizeof tmp, "  %2d     ", (i / 2 + 1));
			mstrlcat(bstring, tmp, sizeof bstring);
		}

		msnprintf(tmp, sizeof tmp, "%-23s", move_and_time(&ml[i]));
		mstrlcat(bstring, tmp, sizeof bstring);

		if (i & 0x01)
			mstrlcat(bstring, "\n", sizeof bstring);
		i++;
		count++;
	}

	if (i & 0x01)
		mstrlcat(bstring, "\n", sizeof bstring);
	return 0;
}

/*
 * Sleator's 'new and improved' raw dump format...
 */
PUBLIC int
style10(game_state_t *b, move_t *ml)
{
	char	 tmp[80];
	int	 f, r;
	int	 ret, too_long;
	int	 ws, bs;

	board_calc_strength(b, &ws, &bs);
	msnprintf(tmp, sizeof tmp, "<10>\n");
	mstrlcat(bstring, tmp, sizeof bstring);

	for (r = 7; r >= 0; r--) {
		mstrlcat(bstring, "|", sizeof bstring);

		for (f = 0; f < 8; f++) {
			if (b->board[f][r] == NOPIECE) {
				mstrlcat(bstring, " ", sizeof bstring);
			} else {
				if (colorval(b->board[f][r]) == WHITE) {
					mstrlcat(bstring, wpstring
					    [piecetype(b->board[f][r])],
					    sizeof bstring);
				} else {
					mstrlcat(bstring, bpstring
					    [piecetype(b->board[f][r])],
					    sizeof bstring);
				}
			}
		}

		mstrlcat(bstring, "|\n", sizeof bstring);
	}

	mstrlcat(bstring, (b->onMove == WHITE ? "W " : "B "), sizeof bstring);

	if (garray[b->gameNum].numHalfMoves) {
		msnprintf(tmp, sizeof tmp, "%d ",
		    ml[garray[b->gameNum].numHalfMoves - 1].doublePawn);
	} else {
		msnprintf(tmp, sizeof tmp, "-1 ");
	}

	mstrlcat(bstring, tmp, sizeof bstring);

	msnprintf(tmp, sizeof tmp, "%d %d %d %d %d\n",
	    !(b->wkmoved || b->wkrmoved),
	    !(b->wkmoved || b->wqrmoved),
	    !(b->bkmoved || b->bkrmoved),
	    !(b->bkmoved || b->bqrmoved),

	    (garray[b->gameNum].numHalfMoves -
	    (b->lastIrreversable == -1 ? 0 : b->lastIrreversable)));

	mstrlcat(bstring, tmp, sizeof bstring);

	ret = snprintf(tmp, sizeof tmp, "%d %s %s %d %d %d %d %d %d %d %d %s "
	    "(%s) %s %d\n",
	    b->gameNum,
	    garray[b->gameNum].white_name,
	    garray[b->gameNum].black_name,
	    myTurn,
	    (garray[b->gameNum].wInitTime / 600),
	    (garray[b->gameNum].wIncrement / 10),
	    ws,
	    bs,
	    ((wTime + 5) / 10),
	    ((bTime + 5) / 10),
	    (garray[b->gameNum].numHalfMoves / 2 + 1),

	    (garray[b->gameNum].numHalfMoves
	    ? ml[garray[b->gameNum].numHalfMoves - 1].moveString
	    : "none"),

	    (garray[b->gameNum].numHalfMoves
	    ? tenth_str(ml[garray[b->gameNum].numHalfMoves - 1].tookTime, 0)
	    : "0:00"),

	    (garray[b->gameNum].numHalfMoves
	    ? ml[garray[b->gameNum].numHalfMoves - 1].algString
	    : "none"),

	    (orient == WHITE ? 0 : 1));

	too_long = (ret < 0 || (size_t)ret >= sizeof tmp);

	if (too_long) {
		fprintf(stderr, "FICS: %s: warning: snprintf truncated\n",
		    __func__);
	}

	mstrlcat(bstring, tmp, sizeof bstring);

	msnprintf(tmp, sizeof tmp, ">10<\n");
	mstrlcat(bstring, tmp, sizeof bstring);

	return 0;
}

/*
 * Same as 8, but with verbose moves ("P/e3-e4", instead of "e4").
 */
PUBLIC int
style11(game_state_t *b, move_t *ml)
{
	char	tmp[80];
	int	f, r;
	int	ws, bs;

	board_calc_strength(b, &ws, &bs);

	msnprintf(tmp, sizeof tmp, "#@#%03d%-16s%s%-16s%s",
	    b->gameNum,

	    garray[b->gameNum].white_name,
	    (orient == WHITE ? "*" : ":"),

	    garray[b->gameNum].black_name,
	    (orient == WHITE ? ":" : "*"));

	mstrlcat(bstring, tmp, sizeof bstring);

	for (r = 0; r < 8; r++) {
		for (f = 0; f < 8; f++) {
			if (b->board[f][r] == NOPIECE) {
				mstrlcat(bstring, " ", sizeof bstring);
			} else {
				if (colorval(b->board[f][r]) == WHITE) {
					mstrlcat(bstring, wpstring
					    [piecetype(b->board[f][r])],
					    sizeof bstring);
				} else {
					mstrlcat(bstring, bpstring
					    [piecetype(b->board[f][r])],
					    sizeof bstring);
				}
			}
		}
	}

	msnprintf(tmp, sizeof tmp, "%03d%s%02d%02d%05d%05d%-7s(%s)@#@\n",
	    (garray[b->gameNum].numHalfMoves / 2 + 1),
	    (b->onMove == WHITE ? "W" : "B"),
	    ws,
	    bs,
	    ((wTime + 5) / 10),
	    ((bTime + 5) / 10),

	    (garray[b->gameNum].numHalfMoves
	    ? ml[garray[b->gameNum].numHalfMoves - 1].moveString
	    : "none"),

	    (garray[b->gameNum].numHalfMoves
	    ? tenth_str(ml[garray[b->gameNum].numHalfMoves - 1].tookTime, 0)
	    : "0:00"));

	mstrlcat(bstring, tmp, sizeof bstring);
	return 0;
}

/*
 * Similar to style 10. See the "style12" help file for information.
 */
PUBLIC int
style12(game_state_t *b, move_t *ml)
{
	char	 tmp[80];
	int	 f, r;
	int	 ret, too_long;
	int	 ws, bs;

	board_calc_strength(b, &ws, &bs);
	msnprintf(bstring, sizeof bstring, "<12> ");

	for (r = 7; r >= 0; r--) {
		for (f = 0; f < 8; f++) {
			if (b->board[f][r] == NOPIECE) {
				mstrlcat(bstring, "-", sizeof bstring);
			} else {
				if (colorval(b->board[f][r]) == WHITE) {
					mstrlcat(bstring, wpstring
					    [piecetype(b->board[f][r])],
					    sizeof bstring);
				} else {
					mstrlcat(bstring, bpstring
					    [piecetype(b->board[f][r])],
					    sizeof bstring);
				}
			}
		}

		mstrlcat(bstring, " ", sizeof bstring);
	}

	mstrlcat(bstring, (b->onMove == WHITE ? "W " : "B "), sizeof bstring);

	if (garray[b->gameNum].numHalfMoves) {
		msnprintf(tmp, sizeof tmp, "%d ",
		    ml[garray[b->gameNum].numHalfMoves - 1].doublePawn);
	} else {
		msnprintf(tmp, sizeof tmp, "-1 ");
	}

	mstrlcat(bstring, tmp, sizeof bstring);

	msnprintf(tmp, sizeof tmp, "%d %d %d %d %d ",
	    !(b->wkmoved || b->wkrmoved),
	    !(b->wkmoved || b->wqrmoved),
	    !(b->bkmoved || b->bkrmoved),
	    !(b->bkmoved || b->bqrmoved),

	    (garray[b->gameNum].numHalfMoves -
	    (b->lastIrreversable == -1 ? 0 : b->lastIrreversable)));
	mstrlcat(bstring, tmp, sizeof bstring);

	ret = snprintf(tmp, sizeof tmp, "%d %s %s %d %d %d %d %d %d %d %d %s "
	    "(%s) %s %d\n",
	    (b->gameNum + 1),
	    garray[b->gameNum].white_name,
	    garray[b->gameNum].black_name,
	    myTurn,
	    (garray[b->gameNum].wInitTime / 600),
	    (garray[b->gameNum].wIncrement / 10),
	    ws,
	    bs,
	    ((wTime + 5) / 10),
	    ((bTime + 5) / 10),
	    (garray[b->gameNum].numHalfMoves / 2 + 1),

	    (garray[b->gameNum].numHalfMoves
	    ? ml[garray[b->gameNum].numHalfMoves - 1].moveString
	    : "none"),

	    (garray[b->gameNum].numHalfMoves
	    ? tenth_str(ml[garray[b->gameNum].numHalfMoves - 1].tookTime, 0)
	    : "0:00"),

	    (garray[b->gameNum].numHalfMoves
	    ? ml[garray[b->gameNum].numHalfMoves - 1].algString
	    : "none"),

	    (orient == WHITE ? 0 : 1));

	too_long = (ret < 0 || (size_t)ret >= sizeof tmp);

	if (too_long) {
		fprintf(stderr, "FICS: %s: warning: snprintf truncated\n",
		    __func__);
	}

	mstrlcat(bstring, tmp, sizeof bstring);

	return 0;
}

PUBLIC int
board_read_file(char *category, char *gname, game_state_t *gs)
{
	FILE	*fp;
	char	 fname[MAX_FILENAME_SIZE + 1];
	int	 c;
	int	 f, r;
	int	 onColor = -1;
	int	 onFile = -1;
	int	 onNewLine = 1;
	int	 onPiece = -1;
	int	 onRank = -1;

	msnprintf(fname, sizeof fname, "%s/%s/%s", board_dir, category, gname);

	if ((fp = fopen(fname, "r")) == NULL)
		return 1;
	for (f = 0; f < 8; f++)
		for (r = 0; r < 8; r++)
			gs->board[f][r] = NOPIECE;
	for (f = 0; f < 2; f++) {
		for (r = 0; r < 8; r++)
			gs->ep_possible[f][r] = 0;
		for (r = PAWN; r <= QUEEN; r++)
			gs->holding[f][r - 1] = 0;
	}

	gs->wkmoved = gs->wqrmoved = gs->wkrmoved = 0;
	gs->bkmoved = gs->bqrmoved = gs->bkrmoved = 0;

	gs->onMove		= -1;
	gs->moveNum		= 1;
	gs->lastIrreversable	= -1;

	while (!feof(fp)) {
		c = fgetc(fp);

		if (onNewLine) {
			if (c == 'W') {
				onColor = WHITE;
				if (gs->onMove < 0)
					gs->onMove = WHITE;
			} else if (c == 'B') {
				onColor = BLACK;
				if (gs->onMove < 0)
					gs->onMove = BLACK;
			} else if (c == '#') {
				while (!feof(fp) && c != '\n')
					c = fgetc(fp); // Comment line
				continue;
			} else { // Skip any line we don't understand
				while (!feof(fp) && c != '\n')
					c = fgetc(fp);
				continue;
			}

			onNewLine = 0;
		} else {
			switch (c) {
			case 'P':
				onPiece = PAWN;
				break;
			case 'R':
				onPiece = ROOK;
				break;
			case 'N':
				onPiece = KNIGHT;
				break;
			case 'B':
				onPiece = BISHOP;
				break;
			case 'Q':
				onPiece = QUEEN;
				break;
			case 'K':
				onPiece = KING;
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
			case 'g':
			case 'h':
				onFile = (c - 'a');
				onRank = -1; // NOLINT: dead store
				break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
				onRank = (c - '1');

				if (onFile >= 0 &&
				    onColor >= 0 &&
				    onPiece >= 0) {
					gs->board[onFile][onRank] =
					    (onPiece | onColor);
				}

				break;
			case '#':
				while (!feof(fp) && c != '\n')
					c = fgetc(fp);	// Comment line
			case '\n':
				onNewLine	= 1;
				onColor		= -1;
				onPiece		= -1;
				onFile		= -1;
				onRank		= -1; // NOLINT: dead store
				break;
			default:
				break;
			} // switch
		}
	} // while

	fclose(fp);
	return 0;
}

PRIVATE void
place_piece(board_t b, int piece, int squareColor)
{
	int	placed = 0;
	int	r, f;

	if (iscolor(piece, BLACK))
		r = 7;
	else
		r = 0;

	while (!placed) {
		if (squareColor == ANY_SQUARE) {
			f = (brand() % 8);
		} else {
			f = (brand() % 4) * 2;

			if (SquareColor(f, r) != squareColor)
				f++;
		}

		if ((b)[f][r] == NOPIECE) {
			(b)[f][r] = piece;
			placed = 1;
		}
	}
}

PUBLIC void
wild_update(int style)
{
	board_t b;
	int f, r, i;

	for (f = 0; f < 8; f++) {
		for (r = 0; r < 8; r++)
			b[f][r] = NOPIECE;
	}

	for (f = 0; f < 8; f++) {
		b[f][1] = W_PAWN;
		b[f][6] = B_PAWN;
	}

	switch (style) {
	case 1:
		if (brand() & 0x01) {
			b[4][0] = W_KING;
			b[3][0] = W_QUEEN;
		} else {
			b[3][0] = W_KING;
			b[4][0] = W_QUEEN;
		}
		if (brand() & 0x01) {
			b[4][7] = B_KING;
			b[3][7] = B_QUEEN;
		} else {
			b[3][7] = B_KING;
			b[4][7] = B_QUEEN;
		}

		b[0][0] = b[7][0] = W_ROOK;
		b[0][7] = b[7][7] = B_ROOK;

		/*
		 * Must do bishops before knights to be sure opposite
		 * colored squares are available.
		 */
		place_piece(b, W_BISHOP, WHITE_SQUARE);
		place_piece(b, W_BISHOP, BLACK_SQUARE);
		place_piece(b, W_KNIGHT, ANY_SQUARE);
		place_piece(b, W_KNIGHT, ANY_SQUARE);
		place_piece(b, B_BISHOP, WHITE_SQUARE);
		place_piece(b, B_BISHOP, BLACK_SQUARE);
		place_piece(b, B_KNIGHT, ANY_SQUARE);
		place_piece(b, B_KNIGHT, ANY_SQUARE);
		break;
	case 2:
		place_piece(b, W_KING, ANY_SQUARE);
		place_piece(b, W_QUEEN, ANY_SQUARE);
		place_piece(b, W_ROOK, ANY_SQUARE);
		place_piece(b, W_ROOK, ANY_SQUARE);
		place_piece(b, W_BISHOP, ANY_SQUARE);
		place_piece(b, W_BISHOP, ANY_SQUARE);
		place_piece(b, W_KNIGHT, ANY_SQUARE);
		place_piece(b, W_KNIGHT, ANY_SQUARE);

		/* Black mirrors White */
		for (i = 0; i < 8; i++)
			b[i][7] = (b[i][0] | BLACK);
		break;
	case 3:
		/*
		 * Generate White king on random square plus random
		 * set of pieces.
		 */
		place_piece(b, W_KING, ANY_SQUARE);

		for (i = 0; i < 8; i++) {
			if (b[i][0] != W_KING) {
				b[i][0] = (brand() % 4) + 2;
			}
		}

		/* Black mirrors White */
		for (i = 0; i < 8; i++)
			b[i][7] = (b[i][0] | BLACK);
		break;
	case 4:
		/*
		 * Generate White king on random square plus random
		 * set of pieces.
		 */
		place_piece(b, W_KING, ANY_SQUARE);

		for (i = 0; i < 8; i++) {
			if (b[i][0] != W_KING)
				b[i][0] = (brand() % 4) + 2;
		}

		/*
		 * Black has the same set of pieces, but randomly
		 * permuted, except that Black must have the same
		 * number of bishops on white squares as White has on
		 * black squares, and vice versa. So we must place
		 * Black's bishops first to be sure there are enough
		 * squares left of the correct color.
		 */

		for (i = 0; i < 8; i++) {
			if (b[i][0] == W_BISHOP)
				place_piece(b, B_BISHOP, !SquareColor(i, 0));
		}

		for (i = 0; i < 8; i++) {
			if (b[i][0] != W_BISHOP)
				place_piece(b, (b[i][0] | BLACK), ANY_SQUARE);
		}
		break;
	default:
		return;
		break;
	}

	{
		FILE	*fp;
		char	 fname[MAX_FILENAME_SIZE + 1];
		int	 fd;
		int	 onPiece;

		msnprintf(fname, sizeof fname, "%s/wild/%d", board_dir, style);

		if ((fd = open(fname, g_open_flags[OPFL_WRITE],
		    g_open_modes)) < 0) {
			warn("%s: can't write file name: %s", __func__, fname);
			return;
		} else if ((fp = fdopen(fd, "w")) == NULL) {
			warn("%s: can't write file name: %s", __func__, fname);
			close(fd);
			return;
		}

		fprintf(fp, "W:");
		onPiece = -1;

		for (r = 1; r >= 0; r--) {
			for (f = 0; f < 8; f++) {
				if (onPiece < 0 || b[f][r] != onPiece) {
					onPiece = b[f][r];

					fprintf(fp, " %s", wpstring
					    [piecetype(b[f][r])]);
				}

				fprintf(fp, " %c%c", (f + 'a'), (r + '1'));
			}
		}

		fprintf(fp, "\nB:");
		onPiece = -1;

		for (r = 6; r < 8; r++) {
			for (f = 0; f < 8; f++) {
				if (onPiece < 0 || b[f][r] != onPiece) {
					onPiece = b[f][r];

					fprintf(fp, " %s", wpstring
					    [piecetype(b[f][r])]);
				}

				fprintf(fp, " %c%c", (f + 'a'), (r + '1'));
			}
		}

		fprintf(fp, "\n");
		fclose(fp);
	}
}

PUBLIC void
wild_init(void)
{
	wild_update(1);
	wild_update(2);
	wild_update(3);
	wild_update(4);
}

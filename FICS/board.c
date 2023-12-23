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
   Markus Uhlin                 23/12/23	Reformatted functions
*/

#include "stdinclude.h"
#include "common.h"

#include "board.h"
#include "gamedb.h"
#include "playerdb.h"
#include "utils.h"

extern int style1();
extern int style2();
extern int style3();
extern int style4();
extern int style5();
extern int style6();
extern int style7();
extern int style8();
extern int style9();
extern int style10();
extern int style11();
extern int style12();
extern int style13();

PUBLIC char *wpstring[] = {" ", "P", "N", "B", "R", "Q", "K"};
PUBLIC char *bpstring[] = {" ", "p", "n", "b", "r", "q", "k"};

PUBLIC int pieceValues[7] = {0, 1, 3, 3, 5, 9, 0};
PUBLIC int (*styleFuncs[MAX_STYLES]) () = {
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

PRIVATE const int mach_type = (1<<7) | (1<<8) | (1<<9) | (1<<10) | (1<<11);
#define IsMachineStyle(n) (((1<<(n)) & mach_type) != 0)

PRIVATE char bstring[MAX_BOARD_STRING_LEGTH];

PUBLIC int board_init(game_state_t *b, char *category, char *board)
{
  int retval;
  int wval;

  if (!category || !board || !category[0] || !board[0])
    retval = board_read_file("standard", "standard", b);
  else {
    if (!strcmp(category, "wild")) {
      if (sscanf(board, "%d", &wval) == 1 && wval >= 1 && wval <= 4)
	wild_update(wval);
    }
    retval = board_read_file(category, board, b);
  }
  b->gameNum = -1;
  return retval;
}

PUBLIC void board_calc_strength(game_state_t *b, int *ws, int *bs)
{
  int r, f;
  int *p;

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
    *ws += b->holding[0][r-1] * pieceValues[r];
    *bs += b->holding[1][r-1] * pieceValues[r];
  }
}

PRIVATE char *holding_str(int *holding)
{
  static char tmp[30];
  int p,i,j;

  i = 0;
  for (p = PAWN; p <= QUEEN; p++)
  {
    for (j = 0; j < holding[p-1]; j++)
    {
      tmp[i++] = wpstring[p][0];
    }
  }
  tmp[i] = '\0';
  return tmp;
}

PRIVATE char *append_holding_machine(char *buf, int g, int c, int p)
{
  game_state_t *gs = &garray[g].game_state;
  char tmp[50];

  sprintf(tmp, "<b1> game %d white [%s] black [", g+1, holding_str(gs->holding[0]));
  strcat(tmp, holding_str(gs->holding[1]));
  strcat(buf, tmp);
  if (p) {
    sprintf(tmp, "] <- %c%s\n", "WB"[c], wpstring[p]);
    strcat(buf, tmp);
  } else
    strcat(buf, "]\n");
  return buf;
}

PRIVATE char *append_holding_display(char *buf, game_state_t *gs, int white)
{
  if (white)
    strcat(buf, "White holding: [");
  else
    strcat(buf, "Black holding: [");
  strcat(buf, holding_str(gs->holding[white ? 0 : 1]));
  strcat(buf, "]\n");
  return buf;
}

PUBLIC void update_holding(int g, int pieceCaptured)
{
  int p = piecetype(pieceCaptured);
  int c = colorval(pieceCaptured);
  game_state_t *gs = &garray[g].game_state;
  int pp, pl;
  char tmp1[80], tmp2[80];

  if (c == WHITE) {
    c = 0;
    pp = garray[g].white;
  } else {
    c = 1;
    pp = garray[g].black;
  }
  gs->holding[c][p-1]++;
  tmp1[0] = '\0';
  append_holding_machine(tmp1, g, c, p);
  sprintf(tmp2, "Game %d %s received: %s -> [%s]\n", g+1,
          parray[pp].name, wpstring[p], holding_str(gs->holding[c]));
  for (pl = 0; pl < p_num; pl++) {
    if (parray[pl].status == PLAYER_EMPTY)
      continue;
    if (player_is_observe(pl, g) || (parray[pl].game == g)) {
      pprintf_prompt(pl, IsMachineStyle(parray[pl].style) ? tmp1 : tmp2);
	}
  }
}

/* Globals used for each board */
PRIVATE char *wName, *bName;
PRIVATE int wTime, bTime;
PRIVATE int orient;
PRIVATE int forPlayer;
PRIVATE int myTurn;		/* 1 = my turn, 0 = observe, -1 = other turn */
 /* 2 = examiner, -2 = observing examiner */
 /* -3 = just send position (spos/refresh) */

PUBLIC char *board_to_string(char *wn, char *bn,
			      int wt, int bt,
			      game_state_t *b, move_t *ml, int style,
			      int orientation, int relation,
			      int p)
{
  int bh = (b->gameNum >= 0 && garray[b->gameNum].link >= 0);
  wName = wn;
  bName = bn;
  wTime = wt;
  bTime = bt;
  orient = orientation;
  myTurn = relation;

  forPlayer = p;
  if ((style < 0) || (style >= MAX_STYLES))
    return NULL;

  if (style != 11) {		/* game header */
    sprintf(bstring, "Game %d (%s vs. %s)\n\n",
	  b->gameNum + 1,
	  garray[b->gameNum].white_name,
	  garray[b->gameNum].black_name);
  } else
    bstring[0] = '\0';
  if (bh && !IsMachineStyle(style))
    append_holding_display(bstring, b, orientation==BLACK);
  if (styleFuncs[style] (b, ml))
    return NULL;
  if (bh) {
    if (IsMachineStyle(style))
      append_holding_machine(bstring, b->gameNum, 0, 0);
    else
      append_holding_display(bstring, b, orientation==WHITE);
  }
  return bstring;
}

PUBLIC char *move_and_time(move_t *m)
{
  static char tmp[20];
  sprintf(tmp, "%-7s (%s)", m->algString, tenth_str(m->tookTime, 0));
  return tmp;
}

/* The following take the game state and whole move list */

PRIVATE int genstyle(game_state_t *b, move_t *ml, char *wp[], char *bp[],
		      char *wsqr, char *bsqr,
		  char *top, char *mid, char *start, char *end, char *label,
		      char *blabel)
{
  int f, r, count;
  char tmp[80];
  int first, last, inc;
  int ws, bs;

  board_calc_strength(b, &ws, &bs);
  if (orient == WHITE) {
    first = 7;
    last = 0;
    inc = -1;
  } else {
    first = 0;
    last = 7;
    inc = 1;
  }
  strcat(bstring, top);
  for (f = first, count = 7; f != last + inc; f += inc, count--) {
    sprintf(tmp, "     %d  %s", f + 1, start);
    strcat(bstring, tmp);
    for (r = last; r != first - inc; r = r - inc) {
      if (square_color(r, f) == WHITE)
	strcat(bstring, wsqr);
      else
	strcat(bstring, bsqr);
      if (piecetype(b->board[r][f]) == NOPIECE) {
	if (square_color(r, f) == WHITE)
	  strcat(bstring, bp[0]);
	else
	  strcat(bstring, wp[0]);
      } else {
	if (colorval(b->board[r][f]) == WHITE)
	  strcat(bstring, wp[piecetype(b->board[r][f])]);
	else
	  strcat(bstring, bp[piecetype(b->board[r][f])]);
      }
    }
    sprintf(tmp, "%s", end);
    strcat(bstring, tmp);
    switch (count) {
    case 7:
      sprintf(tmp, "     Move # : %d (%s)", b->moveNum, CString(b->onMove));
      strcat(bstring, tmp);
      break;
    case 6:
/*    if ((b->moveNum > 1) || (b->onMove == BLACK)) {  */
/* The change from the above line to the one below is a kludge by hersco. */
      if (garray[b->gameNum].numHalfMoves > 0) {
/* loon: think this fixes the crashing ascii board on takeback bug */
	sprintf(tmp, "     %s Moves : '%s'", CString(CToggle(b->onMove)),
		move_and_time(&ml[garray[b->gameNum].numHalfMoves - 1]));
	strcat(bstring, tmp);
      }
      break;
    case 5:
      break;
    case 4:
      sprintf(tmp, "     Black Clock : %s", tenth_str(((bTime > 0) ? bTime : 0), 1));
      strcat(bstring, tmp);
      break;
    case 3:
      sprintf(tmp, "     White Clock : %s", tenth_str(((wTime > 0) ? wTime : 0), 1));
      strcat(bstring, tmp);
      break;
    case 2:
      sprintf(tmp, "     Black Strength : %d", bs);
      strcat(bstring, tmp);
      break;
    case 1:
      sprintf(tmp, "     White Strength : %d", ws);
      strcat(bstring, tmp);
      break;
    case 0:
      break;
    }
    strcat(bstring, "\n");
    if (count != 0)
      strcat(bstring, mid);
    else
      strcat(bstring, top);
  }
  if (orient == WHITE)
    strcat(bstring, label);
  else
    strcat(bstring, blabel);
  return 0;
}

/* Experimental ANSI board for colour representation */
PUBLIC int style13(game_state_t *b, move_t *ml)
{
  static char *wp[] = {"   ", "\033[37m\033[1m P ", "\033[37m\033[1m N ", "\033[37m\033[1m B ", "\033[37m\033[1m R ", "\033[37m\033[1m Q ", "\033[37m\033[1m K "};
  static char *bp[] = {"   ", "\033[21m\033[37m P ", "\033[21m\033[37m N ", "\033[21m\033[37m B ", "\033[21m\033[37m R ", "\033[21m\033[37m Q ", "\033[21m\033[37m K "};
  static char *wsqr = "\033[40m";
  static char *bsqr = "\033[45m";
  static char *top = "\t+------------------------+\n";
  static char *mid = "";
  static char *start = "|";
  static char *end = "\033[0m|";
  static char *label = "\t  a  b  c  d  e  f  g  h\n";
  static char *blabel = "\t  h  g  f  e  d  c  b  a\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* Standard ICS */
PUBLIC int style1(game_state_t *b, move_t *ml)
{
  static char *wp[] = {"   |", " P |", " N |", " B |", " R |", " Q |", " K |"};
  static char *bp[] = {"   |", " *P|", " *N|", " *B|", " *R|", " *Q|", " *K|"};
  static char *wsqr = "";
  static char *bsqr = "";
  static char *top = "\t---------------------------------\n";
  static char *mid = "\t|---+---+---+---+---+---+---+---|\n";
  static char *start = "|";
  static char *end = "";
  static char *label = "\t  a   b   c   d   e   f   g   h\n";
  static char *blabel = "\t  h   g   f   e   d   c   b   a\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* USA-Today Sports Center-style board */
PUBLIC int style2(game_state_t *b, move_t *ml)
{
  static char *wp[] = {"+  ", "P  ", "N  ", "B  ", "R  ", "Q  ", "K  "};
  static char *bp[] = {"-  ", "p' ", "n' ", "b' ", "r' ", "q' ", "k' "};
  static char *wsqr = "";
  static char *bsqr = "";
  static char *top = "";
  static char *mid = "";
  static char *start = "";
  static char *end = "";
  static char *label = "\ta  b  c  d  e  f  g  h\n";
  static char *blabel = "\th  g  f  e  d  c  b  a\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* Experimental vt-100 ANSI board for dark backgrounds */
PUBLIC int style3(game_state_t *b, move_t *ml)
{
  static char *wp[] = {"   ", " P ", " N ", " B ", " R ", " Q ", " K "};
  static char *bp[] = {"   ", " *P", " *N", " *B", " *R", " *Q", " *K"};
  static char *wsqr = "\033[0m";
  static char *bsqr = "\033[7m";
  static char *top = "\t+------------------------+\n";
  static char *mid = "";
  static char *start = "|";
  static char *end = "\033[0m|";
  static char *label = "\t  a  b  c  d  e  f  g  h\n";
  static char *blabel = "\t  h  g  f  e  d  c  b  a\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* Experimental vt-100 ANSI board for light backgrounds */
PUBLIC int style4(game_state_t *b, move_t *ml)
{
  static char *wp[] = {"   ", " P ", " N ", " B ", " R ", " Q ", " K "};
  static char *bp[] = {"   ", " *P", " *N", " *B", " *R", " *Q", " *K"};
  static char *wsqr = "\033[7m";
  static char *bsqr = "\033[0m";
  static char *top = "\t+------------------------+\n";
  static char *mid = "";
  static char *start = "|";
  static char *end = "\033[0m|";
  static char *label = "\t  a  b  c  d  e  f  g  h\n";
  static char *blabel = "\t  h  g  f  e  d  c  b  a\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* Style suggested by ajpierce@med.unc.edu */
PUBLIC int style5(game_state_t *b, move_t *ml)
{
  static char *wp[] = {"    ", "  o ", " :N:", " <B>", " |R|", " {Q}", " =K="};
  static char *bp[] = {"    ", "  p ", " :n:", " <b>", " |r|", " {q}", " =k="};
  static char *wsqr = "";
  static char *bsqr = "";
  static char *top = "        .   .   .   .   .   .   .   .   .\n";
  static char *mid = "        .   .   .   .   .   .   .   .   .\n";
  static char *start = "";
  static char *end = "";
  static char *label = "\t  a   b   c   d   e   f   g   h\n";
  static char *blabel = "\t  h   g   f   e   d   c   b   a\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* Email Board suggested by Thomas Fought (tlf@rsch.oclc.org) */
PUBLIC int style6(game_state_t *b, move_t *ml)
{
  static char *wp[] = {"    |", " wp |", " WN |", " WB |", " WR |", " WQ |", " WK |"};
  static char *bp[] = {"    |", " bp |", " BN |", " BB |", " BR |", " BQ |", " BK |"};
  static char *wsqr = "";
  static char *bsqr = "";
  static char *top = "\t-----------------------------------------\n";
  static char *mid = "\t-----------------------------------------\n";
  static char *start = "|";
  static char *end = "";
  static char *label = "\t  A    B    C    D    E    F    G    H\n";
  static char *blabel = "\t  H    G    F    E    D    C    B    A\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* Miniature board */
PUBLIC int style7(game_state_t *b, move_t *ml)
{
  static char *wp[] = {"  ", " P", " N", " B", " R", " Q", " K"};
  static char *bp[] = {" -", " p", " n", " b", " r", " q", " k"};
  static char *wsqr = "";
  static char *bsqr = "";
  static char *top = "\t:::::::::::::::::::::\n";
  static char *mid = "";
  static char *start = "..";
  static char *end = " ..";
  static char *label = "\t   a b c d e f g h\n";
  static char *blabel = "\t   h g f e d c b a\n";

  return genstyle(b, ml, wp, bp, wsqr, bsqr, top, mid, start, end, label, blabel);
}

/* ICS interface maker board-- raw data dump */
PUBLIC int style8(game_state_t *b, move_t *ml)
{
  char tmp[80];
  int f, r;
  int ws, bs;

  board_calc_strength(b, &ws, &bs);
  sprintf(tmp, "#@#%03d%-16s%s%-16s%s", b->gameNum + 1,
	  garray[b->gameNum].white_name,
	  (orient == WHITE) ? "*" : ":",
	  garray[b->gameNum].black_name,
	  (orient == WHITE) ? ":" : "*");
  strcat(bstring, tmp);
  for (r = 0; r < 8; r++) {
    for (f = 0; f < 8; f++) {
      if (b->board[f][r] == NOPIECE) {
	strcat(bstring, " ");
      } else {
	if (colorval(b->board[f][r]) == WHITE)
	  strcat(bstring, wpstring[piecetype(b->board[f][r])]);
	else
	  strcat(bstring, bpstring[piecetype(b->board[f][r])]);
      }
    }
  }
  sprintf(tmp, "%03d%s%02d%02d%05d%05d%-7s(%s)@#@\n",
	  garray[b->gameNum].numHalfMoves / 2 + 1,
	  (b->onMove == WHITE) ? "W" : "B",
	  ws,
	  bs,
	  (wTime + 5) / 10,
	  (bTime + 5) / 10,
	  garray[b->gameNum].numHalfMoves ?
	  ml[garray[b->gameNum].numHalfMoves - 1].algString :
	  "none",
	  garray[b->gameNum].numHalfMoves ?
	  tenth_str(ml[garray[b->gameNum].numHalfMoves - 1].tookTime, 0) :
	  "0:00");
  strcat(bstring, tmp);
  return 0;
}

/* last 2 moves only (previous non-verbose mode) */
PUBLIC int style9(game_state_t *b, move_t *ml)
{
  int i, count;
  char tmp[80];
  int startmove;

  sprintf(tmp, "\nMove     %-23s%s\n",
	  garray[b->gameNum].white_name,
	  garray[b->gameNum].black_name);
  strcat(bstring, tmp);
  sprintf(tmp, "----     --------------         --------------\n");
  strcat(bstring, tmp);
  startmove = ((garray[b->gameNum].numHalfMoves - 3) / 2) * 2;
  if (startmove < 0)
    startmove = 0;
  for (i = startmove, count = 0;
       i < garray[b->gameNum].numHalfMoves && count < 4;
       i++, count++) {
    if (!(i & 0x01)) {
      sprintf(tmp, "  %2d     ", i / 2 + 1);
      strcat(bstring, tmp);
    }
    sprintf(tmp, "%-23s", move_and_time(&ml[i]));
    strcat(bstring, tmp);
    if (i & 0x01)
      strcat(bstring, "\n");
  }
  if (i & 0x01)
    strcat(bstring, "\n");
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
	sprintf(tmp, "<10>\n");
	strcat(bstring, tmp);

	for (r = 7; r >= 0; r--) {
		strcat(bstring, "|");

		for (f = 0; f < 8; f++) {
			if (b->board[f][r] == NOPIECE) {
				strcat(bstring, " ");
			} else {
				if (colorval(b->board[f][r]) == WHITE) {
					strcat(bstring, wpstring
					    [piecetype(b->board[f][r])]);
				} else {
					strcat(bstring, bpstring
					    [piecetype(b->board[f][r])]);
				}
			}
		}

		strcat(bstring, "|\n");
	}

	strcat(bstring, (b->onMove == WHITE ? "W " : "B "));

	if (garray[b->gameNum].numHalfMoves) {
		sprintf(tmp, "%d ",
		    ml[garray[b->gameNum].numHalfMoves - 1].doublePawn);
	} else {
		sprintf(tmp, "-1 ");
	}

	strcat(bstring, tmp);

	sprintf(tmp, "%d %d %d %d %d\n",
	    !(b->wkmoved || b->wkrmoved),
	    !(b->wkmoved || b->wqrmoved),
	    !(b->bkmoved || b->bkrmoved),
	    !(b->bkmoved || b->bqrmoved),

	    (garray[b->gameNum].numHalfMoves -
	    (b->lastIrreversable == -1 ? 0 : b->lastIrreversable)));

	strcat(bstring, tmp);

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
	strcat(bstring, tmp);

	sprintf(tmp, ">10<\n");
	strcat(bstring, tmp);

	return 0;
}

/* Same as 8, but with verbose moves ("P/e3-e4", instead of "e4") */
PUBLIC int style11(game_state_t *b, move_t *ml)
{
  char tmp[80];
  int f, r;
  int ws, bs;

  board_calc_strength(b, &ws, &bs);
  sprintf(tmp, "#@#%03d%-16s%s%-16s%s", b->gameNum,
	  garray[b->gameNum].white_name,
	  (orient == WHITE) ? "*" : ":",
	  garray[b->gameNum].black_name,
	  (orient == WHITE) ? ":" : "*");
  strcat(bstring, tmp);
  for (r = 0; r < 8; r++) {
    for (f = 0; f < 8; f++) {
      if (b->board[f][r] == NOPIECE) {
	strcat(bstring, " ");
      } else {
	if (colorval(b->board[f][r]) == WHITE)
	  strcat(bstring, wpstring[piecetype(b->board[f][r])]);
	else
	  strcat(bstring, bpstring[piecetype(b->board[f][r])]);
      }
    }
  }
    sprintf(tmp, "%03d%s%02d%02d%05d%05d%-7s(%s)@#@\n",
	    garray[b->gameNum].numHalfMoves / 2 + 1,
	    (b->onMove == WHITE) ? "W" : "B",
	    ws,
	    bs,
	    (wTime + 5) / 10,
	    (bTime + 5) / 10,
	    garray[b->gameNum].numHalfMoves ?
	    ml[garray[b->gameNum].numHalfMoves - 1].moveString :
	    "none",
	    garray[b->gameNum].numHalfMoves ?
	    tenth_str(ml[garray[b->gameNum].numHalfMoves - 1].tookTime, 0) :
	    "0:00");
  strcat(bstring, tmp);
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
	sprintf(bstring, "<12> ");

	for (r = 7; r >= 0; r--) {
		for (f = 0; f < 8; f++) {
			if (b->board[f][r] == NOPIECE) {
				strcat(bstring, "-");
			} else {
				if (colorval(b->board[f][r]) == WHITE) {
					strcat(bstring, wpstring
					    [piecetype(b->board[f][r])]);
				} else {
					strcat(bstring, bpstring
					    [piecetype(b->board[f][r])]);
				}
			}
		}

		strcat(bstring, " ");
	}

	strcat(bstring, (b->onMove == WHITE ? "W " : "B "));

	if (garray[b->gameNum].numHalfMoves) {
		sprintf(tmp, "%d ",
		    ml[garray[b->gameNum].numHalfMoves - 1].doublePawn);
	} else {
		sprintf(tmp, "-1 ");
	}

	strcat(bstring, tmp);

	sprintf(tmp, "%d %d %d %d %d ",
	    !(b->wkmoved || b->wkrmoved),
	    !(b->wkmoved || b->wqrmoved),
	    !(b->bkmoved || b->bkrmoved),
	    !(b->bkmoved || b->bqrmoved),

	    (garray[b->gameNum].numHalfMoves -
	    (b->lastIrreversable == -1 ? 0 : b->lastIrreversable)));
	strcat(bstring, tmp);

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
	strcat(bstring, tmp);

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

	sprintf(fname, "%s/%s/%s", board_dir, category, gname);

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
				onRank = -1;
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
				onRank		= -1;
				break;
			default:
				break;
			} // switch
		}
	} // while

	fclose(fp);
	return 0;
}

#define WHITE_SQUARE 1
#define BLACK_SQUARE 0
#define ANY_SQUARE -1
#define SquareColor(f, r) ((f ^ r) & 1)

PRIVATE void place_piece(board_t b, int piece, int squareColor)
{
  int r, f;
  int placed = 0;

  if (iscolor(piece, BLACK))
    r = 7;
  else
    r = 0;

  while (!placed) {
    if (squareColor == ANY_SQUARE) {
      f = rand() % 8;
    } else {
      f = (rand() % 4) * 2;
      if (SquareColor(f, r) != squareColor)
	f++;
    }
    if ((b)[f][r] == NOPIECE) {
      (b)[f][r] = piece;
      placed = 1;
    }
  }
}

PUBLIC void wild_update(int style)
{
  int f, r, i;
  board_t b;

  for (f = 0; f < 8; f++)
    for (r = 0; r < 8; r++)
      b[f][r] = NOPIECE;
  for (f = 0; f < 8; f++) {
    b[f][1] = W_PAWN;
    b[f][6] = B_PAWN;
  }
  switch (style) {
  case 1:
    if (rand() & 0x01) {
      b[4][0] = W_KING;
      b[3][0] = W_QUEEN;
    } else {
      b[3][0] = W_KING;
      b[4][0] = W_QUEEN;
    }
    if (rand() & 0x01) {
      b[4][7] = B_KING;
      b[3][7] = B_QUEEN;
    } else {
      b[3][7] = B_KING;
      b[4][7] = B_QUEEN;
    }
    b[0][0] = b[7][0] = W_ROOK;
    b[0][7] = b[7][7] = B_ROOK;
    /* Must do bishops before knights to be sure opposite colored squares are
       available. */
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
    for (i = 0; i < 8; i++) {
      b[i][7] = b[i][0] | BLACK;
    }
    break;
  case 3:
    /* Generate White king on random square plus random set of pieces */
    place_piece(b, W_KING, ANY_SQUARE);
    for (i = 0; i < 8; i++) {
      if (b[i][0] != W_KING) {
	b[i][0] = (rand() % 4) + 2;
      }
    }
    /* Black mirrors White */
    for (i = 0; i < 8; i++) {
      b[i][7] = b[i][0] | BLACK;
    }
    break;
  case 4:
    /* Generate White king on random square plus random set of pieces */
    place_piece(b, W_KING, ANY_SQUARE);
    for (i = 0; i < 8; i++) {
      if (b[i][0] != W_KING) {
	b[i][0] = (rand() % 4) + 2;
      }
    }
    /* Black has same set of pieces, but randomly permuted, except that Black
       must have the same number of bishops on white squares as White has on
       black squares, and vice versa.  So we must place Black's bishops first
       to be sure there are enough squares left of the correct color. */
    for (i = 0; i < 8; i++) {
      if (b[i][0] == W_BISHOP) {
	place_piece(b, B_BISHOP, !SquareColor(i, 0));
      }
    }
    for (i = 0; i < 8; i++) {
      if (b[i][0] != W_BISHOP) {
	place_piece(b, b[i][0] | BLACK, ANY_SQUARE);
      }
    }
    break;
  default:
    return;
    break;
  }
  {
    FILE *fp;
    char fname[MAX_FILENAME_SIZE + 1];
    int onPiece;

    sprintf(fname, "%s/wild/%d", board_dir, style);
    fp = fopen(fname, "w");
    if (!fp) {
      fprintf(stderr, "FICS: Can't write file name %s\n", fname);
      return;
    }
    fprintf(fp, "W:");
    onPiece = -1;
    for (r = 1; r >= 0; r--) {
      for (f = 0; f < 8; f++) {
	if (onPiece < 0 || b[f][r] != onPiece) {
	  onPiece = b[f][r];
	  fprintf(fp, " %s", wpstring[piecetype(b[f][r])]);
	}
	fprintf(fp, " %c%c", f + 'a', r + '1');
      }
    }
    fprintf(fp, "\nB:");
    onPiece = -1;
    for (r = 6; r < 8; r++) {
      for (f = 0; f < 8; f++) {
	if (onPiece < 0 || b[f][r] != onPiece) {
	  onPiece = b[f][r];
	  fprintf(fp, " %s", wpstring[piecetype(b[f][r])]);
	}
	fprintf(fp, " %c%c", f + 'a', r + '1');
      }
    }
    fprintf(fp, "\n");
    fclose(fp);
  }
}

PUBLIC void wild_init()
{
  wild_update(1);
  wild_update(2);
  wild_update(3);
  wild_update(4);
}

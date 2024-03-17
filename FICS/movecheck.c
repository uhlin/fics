/* movecheck.c
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
   Markus Uhlin                 23/12/14	Fixed compiler warnings
   Markus Uhlin                 23/12/17	Fixed compiler warnings
   Markus Uhlin                 23/12/24	Fixed dead assignment
   Markus Uhlin                 24/03/16	Refactored and reformatted
						functions.
*/

#include "stdinclude.h"
#include "common.h"

#include "algcheck.h"
#include "board.h"
#include "gamedb.h"
#include "movecheck.h"
#include "network.h"
#include "playerdb.h"
#include "utils.h"

/* Simply tests if the input string is a move or not. */
/* If it matches patterns below */
/* Add to this list as you improve the move parser */
/* MS_COMP e2e4 */
/* MS_COMPDASH e2-e4 */
/* MS_CASTLE o-o, o-o-o */
/* Not done yet */
/* MS_ALG e4, Nd5 Ncd5 */
PUBLIC int is_move(char *mstr)
{
  int len = strlen(mstr);
  if ((len > 3) && (mstr[len - 2] == '='))
    len -= 2;

  if (len == 4) {		/* Test for e2e4 */
    if (isfile(mstr[0]) && isrank(mstr[1]) &&
	isfile(mstr[2]) && isrank(mstr[3])) {
      return MS_COMP;
    }
  }
  if (len == 5) {		/* Test for e2-e4 */
    if (isfile(mstr[0]) && isrank(mstr[1]) &&
	(mstr[2] == '-') &&
	isfile(mstr[3]) && isrank(mstr[4])) {
      return MS_COMPDASH;
    }
  }
  if (len == 3) {		/* Test for o-o */
    if ((mstr[0] == 'o') && (mstr[1] == '-') && (mstr[2] == 'o')) {
      return MS_KCASTLE;
    }
    if ((mstr[0] == 'O') && (mstr[1] == '-') && (mstr[2] == 'O')) {
      return MS_KCASTLE;
    }
    if ((mstr[0] == '0') && (mstr[1] == '-') && (mstr[2] == '0')) {
      return MS_KCASTLE;
    }
  }
  if (len == 2) {		/* Test for oo */
    if ((mstr[0] == 'o') && (mstr[1] == 'o')) {
      return MS_KCASTLE;
    }
    if ((mstr[0] == 'O') && (mstr[1] == 'O')) {
      return MS_KCASTLE;
    }
    if ((mstr[0] == '0') && (mstr[1] == '0')) {
      return MS_KCASTLE;
    }
  }
  if (len == 5) {		/* Test for o-o-o */
    if ((mstr[0] == 'o') && (mstr[1] == '-') && (mstr[2] == 'o') && (mstr[3] == '-') && (mstr[4] == 'o')) {
      return MS_QCASTLE;
    }
    if ((mstr[0] == 'O') && (mstr[1] == '-') && (mstr[2] == 'O') && (mstr[3] == '-') && (mstr[4] == 'O')) {
      return MS_QCASTLE;
    }
    if ((mstr[0] == '0') && (mstr[1] == '-') && (mstr[2] == '0') && (mstr[3] == '-') && (mstr[4] == '0')) {
      return MS_QCASTLE;
    }
  }
  if (len == 3) {		/* Test for ooo */
    if ((mstr[0] == 'o') && (mstr[1] == 'o') && (mstr[2] == 'o')) {
      return MS_QCASTLE;
    }
    if ((mstr[0] == 'O') && (mstr[1] == 'O') && (mstr[2] == 'O')) {
      return MS_QCASTLE;
    }
    if ((mstr[0] == '0') && (mstr[1] == '0') && (mstr[2] == '0')) {
      return MS_QCASTLE;
    }
  }
  return alg_is_move(mstr);
}


PUBLIC int NextPieceLoop(board_t b, int *f, int *r, int color)
{
  while (1) {
    (*r) = (*r) + 1;
    if (*r > 7) {
      *r = 0;
      *f = *f + 1;
      if (*f > 7)
	break;
    }
    if ((b[*f][*r] != NOPIECE) && iscolor(b[*f][*r], color))
      return 1;
  }
  return 0;
}

PUBLIC int InitPieceLoop(board_t b, int *f, int *r, int color)
{
  *f = 0;
  *r = -1;
  return 1;
}

/* All of the routines assume that the obvious problems have been checked */
/* See legal_move() */
PRIVATE int legal_pawn_move( game_state_t *gs, int ff, int fr, int tf, int tr )
{
  if (ff == tf) {
    if (gs->board[tf][tr] != NOPIECE) return 0;
    if (gs->onMove == WHITE) {
      if (tr - fr == 1) return 1;
      if ((fr == 1) && (tr - fr == 2) && gs->board[ff][2]==NOPIECE) return 1;
    } else {
      if (fr - tr == 1) return 1;
      if ((fr == 6) && (fr - tr == 2) && gs->board[ff][5]==NOPIECE) return 1;
    }
    return 0;
  }
  if (ff != tf) { /* Capture ? */
    if ((ff - tf != 1) && (tf - ff != 1)) return 0;
    if ((fr - tr != 1) && (tr - fr != 1)) return 0;
    if (gs->onMove == WHITE) {
      if (fr > tr) return 0;
      if ((gs->board[tf][tr] != NOPIECE) && iscolor(gs->board[tf][tr],BLACK))
        return 1;
      if (gs->ep_possible[0][ff] == 1) {
        if ((tf==ff+1) && (gs->board[ff+1][fr] == B_PAWN)) return 1;
      } else if (gs->ep_possible[0][ff] == -1) {
        if ((tf==ff-1) && (gs->board[ff-1][fr] == B_PAWN)) return 1;
      }
    } else {
      if (tr > fr) return 0;
      if ((gs->board[tf][tr] != NOPIECE) && iscolor(gs->board[tf][tr],WHITE))
        return 1;
      if (gs->ep_possible[1][ff] == 1) {
        if ((tf==ff+1) && (gs->board[ff+1][fr] == W_PAWN)) return 1;
      } else if (gs->ep_possible[1][ff] == -1) {
        if ((tf==ff-1) && (gs->board[ff-1][fr] == W_PAWN)) return 1;
      }
    }
  }
  return 0;
}

PRIVATE int legal_knight_move(game_state_t * gs, int ff, int fr, int tf, int tr)
{
  int dx, dy;

  dx = ff - tf;
  dy = fr - tr;
  if ((dx == 2) || (dx == -2)) {
    if ((dy == -1) || (dy == 1))
      return 1;
  }
  if ((dy == 2) || (dy == -2)) {
    if ((dx == -1) || (dx == 1))
      return 1;
  }
  return 0;
}

PRIVATE int legal_bishop_move(game_state_t * gs, int ff, int fr, int tf, int tr)
{
  int dx, dy, x, y;
  int startx, starty;
  int count;
  int incx, incy;

  if (ff > tf) {
    dx = ff - tf;
    incx = -1;
  } else {
    dx = tf - ff;
    incx = 1;
  }
  startx = ff + incx;
  if (fr > tr) {
    dy = fr - tr;
    incy = -1;
  } else {
    dy = tr - fr;
    incy = 1;
  }
  starty = fr + incy;
  if (dx != dy)
    return 0;			/* Not diagonal */
  if (dx == 1)
    return 1;			/* One square, ok */
  count = dx - 1;
  for (x = startx, y = starty; count; x += incx, y += incy, count--) {
    if (gs->board[x][y] != NOPIECE)
      return 0;
  }
  return 1;
}

PRIVATE int legal_rook_move(game_state_t * gs, int ff, int fr, int tf, int tr)
{
  int i;
  int start, stop;

  if (ff == tf) {
    if (((fr - tr) == 1) || ((tr - fr) == 1))
      return 1;
    if (fr < tr) {
      start = fr + 1;
      stop = tr - 1;
    } else {
      start = tr + 1;
      stop = fr - 1;
    }
    for (i = start; i <= stop; i++) {
      if (gs->board[ff][i] != NOPIECE)
	return 0;
    }
    return 1;
  } else if (fr == tr) {
    if (((ff - tf) == 1) || ((tf - ff) == 1))
      return 1;
    if (ff < tf) {
      start = ff + 1;
      stop = tf - 1;
    } else {
      start = tf + 1;
      stop = ff - 1;
    }
    for (i = start; i <= stop; i++) {
      if (gs->board[i][fr] != NOPIECE)
	return 0;
    }
    return 1;
  } else {
    return 0;
  }
}

PRIVATE int legal_queen_move(game_state_t * gs, int ff, int fr, int tf, int tr)
{
  return legal_rook_move(gs, ff, fr, tf, tr) || legal_bishop_move(gs, ff, fr, tf, tr);
}

/* Ckeck, if square (kf,kr) is attacked by enemy piece.
 * Used in castling from/through check testing.
 */

/* new one from soso: */
PRIVATE int is_square_attacked (game_state_t *gs, int kf, int kr)
{
  game_state_t fakeMove;

  fakeMove = *gs;
  fakeMove.board[4][kr] = NOPIECE;
  fakeMove.board[kf][kr] = KING | fakeMove.onMove;
  fakeMove.onMove = CToggle (fakeMove.onMove);
  if (in_check(&fakeMove)) return 1;
    else return 0;
}

PRIVATE int legal_king_move(game_state_t * gs, int ff, int fr, int tf, int tr)
{
  if (gs->onMove == WHITE) {
    /* King side castling */
    if ((fr == 0) && (tr == 0) && (ff == 4) && (tf == 6) && !gs->wkmoved
	&& (!gs->wkrmoved) && (gs->board[5][0] == NOPIECE) &&
	(gs->board[6][0] == NOPIECE) && (gs->board[7][0] == W_ROOK) &&
	(!is_square_attacked(gs, 4, 0)) && (!is_square_attacked(gs, 5, 0))) {
      return 1;
    }
    /* Queen side castling */
    if ((fr == 0) && (tr == 0) && (ff == 4) && (tf == 2) && !gs->wkmoved
	&& (!gs->wqrmoved) && (gs->board[3][0] == NOPIECE) &&
	(gs->board[2][0] == NOPIECE) && (gs->board[1][0] == NOPIECE) &&
	(gs->board[0][0] == W_ROOK) &&
	(!is_square_attacked(gs, 4, 0)) && (!is_square_attacked(gs, 3, 0))) {
      return 1;
    }
  } else {			/* Black */
    /* King side castling */
    if ((fr == 7) && (tr == 7) && (ff == 4) && (tf == 6) && !gs->bkmoved
	&& (!gs->bkrmoved) && (gs->board[5][7] == NOPIECE) &&
	(gs->board[6][7] == NOPIECE) && (gs->board[7][7] == B_ROOK) &&
	(!is_square_attacked(gs, 4, 7)) && (!is_square_attacked(gs, 5, 7))) {
      return 1;
    }
    /* Queen side castling */
    if ((fr == 7) && (tr == 7) && (ff == 4) && (tf == 2) && (!gs->bkmoved)
	&& (!gs->bqrmoved) && (gs->board[3][7] == NOPIECE) &&
	(gs->board[2][7] == NOPIECE) && (gs->board[1][7] == NOPIECE) &&
	(gs->board[0][7] == B_ROOK) &&
	(!is_square_attacked(gs, 4, 7)) && (!is_square_attacked(gs, 3, 7))) {
      return 1;
    }
  }
  if (((ff - tf) > 1) || ((tf - ff) > 1))
    return 0;
  if (((fr - tr) > 1) || ((tr - fr) > 1))
    return 0;
  return 1;
}

PRIVATE void add_pos(int tof, int tor, int *posf, int *posr, int *numpos)
{
  posf[*numpos] = tof;
  posr[*numpos] = tor;
  (*numpos)++;
}

PRIVATE void possible_pawn_moves(game_state_t * gs,
				  int onf, int onr,
				  int *posf, int *posr, int *numpos)
{
  if (gs->onMove == WHITE) {
    if (gs->board[onf][onr + 1] == NOPIECE) {
      add_pos(onf, onr + 1, posf, posr, numpos);
      if ((onr == 1) && (gs->board[onf][onr + 2] == NOPIECE))
	add_pos(onf, onr + 2, posf, posr, numpos);
    }
    if ((onf > 0) && (gs->board[onf - 1][onr + 1] != NOPIECE) &&
	(iscolor(gs->board[onf - 1][onr + 1], BLACK)))
      add_pos(onf - 1, onr + 1, posf, posr, numpos);
    if ((onf < 7) && (gs->board[onf + 1][onr + 1] != NOPIECE) &&
	(iscolor(gs->board[onf + 1][onr + 1], BLACK)))
      add_pos(onf + 1, onr + 1, posf, posr, numpos);
    if (gs->ep_possible[0][onf] == -1)
      add_pos(onf - 1, onr + 1, posf, posr, numpos);
    if (gs->ep_possible[0][onf] == 1)
      add_pos(onf + 1, onr + 1, posf, posr, numpos);
  } else {
    if (gs->board[onf][onr - 1] == NOPIECE) {
      add_pos(onf, onr - 1, posf, posr, numpos);
      if ((onr == 6) && (gs->board[onf][onr - 2] == NOPIECE))
	add_pos(onf, onr - 2, posf, posr, numpos);
    }
    if ((onf > 0) && (gs->board[onf - 1][onr - 1] != NOPIECE) &&
	(iscolor(gs->board[onf - 1][onr - 1], WHITE)))
      add_pos(onf - 1, onr - 1, posf, posr, numpos);
/* loon: changed what looks like a typo, here's the original line:
      add_pos(onf - 1, onr + 1, posf, posr, numpos);
*/
    if ((onf < 7) && (gs->board[onf + 1][onr - 1] != NOPIECE) &&
	(iscolor(gs->board[onf + 1][onr - 1], WHITE)))
      add_pos(onf + 1, onr - 1, posf, posr, numpos);
    if (gs->ep_possible[1][onf] == -1)
      add_pos(onf - 1, onr - 1, posf, posr, numpos);
    if (gs->ep_possible[1][onf] == 1)
      add_pos(onf + 1, onr - 1, posf, posr, numpos);
  }
}

PRIVATE void possible_knight_moves(game_state_t * gs,
				    int onf, int onr,
				    int *posf, int *posr, int *numpos)
{
  static int knightJumps[8][2] = {{-1, 2}, {1, 2}, {2, -1}, {2, 1},
  {-1, -2}, {1, -2}, {-2, 1}, {-2, -1}};
  int f, r;
  int j;

  for (j = 0; j < 8; j++) {
    f = knightJumps[j][0] + onf;
    r = knightJumps[j][1] + onr;
    if ((f < 0) || (f > 7))
      continue;
    if ((r < 0) || (r > 7))
      continue;
    if ((gs->board[f][r] == NOPIECE) ||
	(iscolor(gs->board[f][r], CToggle(gs->onMove))))
      add_pos(f, r, posf, posr, numpos);
  }
}

PRIVATE void possible_bishop_moves(game_state_t * gs,
				    int onf, int onr,
				    int *posf, int *posr, int *numpos)
{
  int f, r;

  /* Up Left */
  f = onf;
  r = onr;
  while (1) {
    f--;
    r++;
    if ((f < 0) || (f > 7))
      break;
    if ((r < 0) || (r > 7))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
  /* Up Right */
  f = onf;
  r = onr;
  while (1) {
    f++;
    r++;
    if ((f < 0) || (f > 7))
      break;
    if ((r < 0) || (r > 7))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
  /* Down Left */
  f = onf;
  r = onr;
  while (1) {
    f--;
    r--;
    if ((f < 0) || (f > 7))
      break;
    if ((r < 0) || (r > 7))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
  /* Down Right */
  f = onf;
  r = onr;
  while (1) {
    f++;
    r--;
    if ((f < 0) || (f > 7))
      break;
    if ((r < 0) || (r > 7))
      break;
    if ((gs->board[f][r] != NOPIECE) && (iscolor(gs->board[f][r], gs->onMove)))
      break;
    add_pos(f, r, posf, posr, numpos);
    if (gs->board[f][r] != NOPIECE)
      break;
  }
}

PRIVATE void
possible_rook_moves(game_state_t *gs, int onf, int onr, int *posf, int *posr,
    int *numpos)
{
	int f, r;

	/* Left */
	f = onf;
	r = onr;
	while (1) {
		f--;

		if (f < 0 || f > 7)
			break;
		if (r < 0 || r > 7)
			break;
		if (gs->board[f][r] != NOPIECE && iscolor(gs->board[f][r],
		    gs->onMove))
			break;

		add_pos(f, r, posf, posr, numpos);

		if (gs->board[f][r] != NOPIECE)
			break;
	}

	/* Right */
	f = onf;
	r = onr;
	while (1) {
		f++;

		if (f < 0 || f > 7)
			break;
		if (r < 0 || r > 7)
			break;
		if (gs->board[f][r] != NOPIECE && iscolor(gs->board[f][r],
		    gs->onMove))
			break;

		add_pos(f, r, posf, posr, numpos);

		if (gs->board[f][r] != NOPIECE)
			break;
	}

	/* Up */
	f = onf;
	r = onr;
	while (1) {
		r++;

		if (f < 0 || f > 7)
			break;
		if (r < 0 || r > 7)
			break;
		if (gs->board[f][r] != NOPIECE && iscolor(gs->board[f][r],
		    gs->onMove))
			break;

		add_pos(f, r, posf, posr, numpos);

		if (gs->board[f][r] != NOPIECE)
			break;
	}

	/* Down */
	f = onf;
	r = onr;
	while (1) {
		r--;

		if (f < 0 || f > 7)
			break;
		if (r < 0 || r > 7)
			break;
		if (gs->board[f][r] != NOPIECE && iscolor(gs->board[f][r],
		    gs->onMove))
			break;

		add_pos(f, r, posf, posr, numpos);

		if (gs->board[f][r] != NOPIECE)
			break;
	}
}

PRIVATE void
possible_queen_moves(game_state_t *gs, int onf, int onr, int *posf, int *posr,
    int *numpos)
{
	possible_rook_moves(gs, onf, onr, posf, posr, numpos);
	possible_bishop_moves(gs, onf, onr, posf, posr, numpos);
}

PRIVATE void
possible_king_moves(game_state_t *gs, int onf, int onr, int *posf, int *posr,
    int *numpos)
{
	int		f, r;
	int		j;
	static int	kingJumps[8][2] = {
		{-1, -1}, {0, -1}, {1, -1}, {-1, 1}, {0, 1}, {1, 1}, {-1, 0},
		{1, 0}
	};

	for (j = 0; j < 8; j++) {
		f = kingJumps[j][0] + onf;
		r = kingJumps[j][1] + onr;

		if (f < 0 || f > 7)
			continue;
		if (r < 0 || r > 7)
			continue;

		if (gs->board[f][r] == NOPIECE || iscolor(gs->board[f][r],
		    CToggle(gs->onMove)))
			add_pos(f, r, posf, posr, numpos);
	}
}

/* Doesn't check for check */
PUBLIC int
legal_move(game_state_t *gs,
    int fFile, int fRank,
    int tFile, int tRank)
{
	int	legal;
	int	move_piece;

	if (fFile == ALG_DROP) {
		if ((move_piece = fRank) == KING)
			return 0;
		if (gs->holding[gs->onMove == WHITE ? 0 : 1][move_piece - 1]
		    == 0)
			return 0;
		if (gs->board[tFile][tRank] != NOPIECE)
			return 0;
		if (move_piece == PAWN && (tRank == 0 || tRank == 7))
			return 0;
		return 1;
	} else {
		move_piece = piecetype(gs->board[fFile][fRank]);
	}

	if (gs->board[fFile][fRank] == NOPIECE)
		return 0;
	if (!iscolor(gs->board[fFile][fRank], gs->onMove))  // Wrong color
		return 0;
	if (gs->board[tFile][tRank] != NOPIECE &&
	    iscolor(gs->board[tFile][tRank], gs->onMove))   // Can't capture own
		return 0;
	if (fFile == tFile && fRank == tRank)	// Same square
		return 0;

	switch (move_piece) {
	case PAWN:
		legal = legal_pawn_move(gs, fFile, fRank, tFile, tRank);
		break;
	case KNIGHT:
		legal = legal_knight_move(gs, fFile, fRank, tFile, tRank);
		break;
	case BISHOP:
		legal = legal_bishop_move(gs, fFile, fRank, tFile, tRank);
		break;
	case ROOK:
		legal = legal_rook_move(gs, fFile, fRank, tFile, tRank);
		break;
	case QUEEN:
		legal = legal_queen_move(gs, fFile, fRank, tFile, tRank);
		break;
	case KING:
		legal = legal_king_move(gs, fFile, fRank, tFile, tRank);
		break;
	default:
		return 0;
	}

	return legal;
}

/*
 * This fills in the rest of the mt structure once it is determined
 * that. (Returns 'MOVE_ILLEGAL' if move leaves you in check.)
 */
PRIVATE int
move_calculate(game_state_t *gs, move_t *mt, int promote)
{
	game_state_t	fakeMove;
	int		ret, too_long;

	mt->pieceCaptured	= gs->board[mt->toFile][mt->toRank];
	mt->enPassant		= 0;	// Don't know yet,
					// let execute move take care of it

	if (mt->fromFile == ALG_DROP) {
		mt->piecePromotionTo = NOPIECE;
		ret = snprintf(mt->moveString, sizeof mt->moveString,
		    "%s/%c%c-%c%d",
		    wpstring[mt->fromRank],
		    DROP_CHAR,
		    DROP_CHAR,
		    (mt->toFile + 'a'),
		    (mt->toRank + 1));

		too_long = (ret < 0 || (size_t)ret >= sizeof mt->moveString);

		if (too_long) { /* XXX */
			fprintf(stderr, "FICS: %s: warning: "
			    "snprintf truncated\n", __func__);
		}
	} else {
		if (piecetype(gs->board[mt->fromFile][mt->fromRank]) == PAWN &&
		    (mt->toRank == 0 || mt->toRank == 7)) {
			mt->piecePromotionTo = (promote |
			    colorval(gs->board[mt->fromFile][mt->fromRank]));
		} else {
			mt->piecePromotionTo = NOPIECE;
		}

		if (piecetype(gs->board[mt->fromFile][mt->fromRank]) == PAWN &&
		    (mt->fromRank - mt->toRank == 2 ||
		    mt->toRank - mt->fromRank == 2)) {
			mt->doublePawn = mt->fromFile;
		} else {
			mt->doublePawn = -1;
		}

		if (piecetype(gs->board[mt->fromFile][mt->fromRank]) == KING &&
		    mt->fromFile == 4 &&
		    mt->toFile == 2) {
			sprintf(mt->moveString, "o-o-o");
		} else if (piecetype(gs->board[mt->fromFile][mt->fromRank]) == KING &&
		    mt->fromFile == 4 &&
		    mt->toFile == 6) {
			sprintf(mt->moveString, "o-o");
		} else {
			ret = snprintf(mt->moveString, sizeof mt->moveString,
			    "%s/%c%d-%c%d",
			    wpstring[piecetype(gs->board[mt->fromFile][mt->fromRank])],
			    (mt->fromFile + 'a'),
			    (mt->fromRank + 1),
			    (mt->toFile + 'a'),
			    (mt->toRank + 1));

			too_long = (ret < 0 || (size_t)ret >= sizeof mt->moveString);

			if (too_long) { /* XXX */
				fprintf(stderr, "FICS: %s: warning: "
				    "snprintf truncated\n", __func__);
			}
		}
	}

	// Replace this with an algabraic de-parser
	snprintf(mt->algString, sizeof mt->algString, "%s",
	    alg_unparse(gs, mt));
	fakeMove = *gs;
	execute_move(&fakeMove, mt, 0); // Calculates enPassant also

	// Does making this move leave ME in check?
	if (in_check(&fakeMove))
		return MOVE_ILLEGAL;
	// IanO: bughouse variants: drop cannot be check/checkmate

	return MOVE_OK;
}

PUBLIC int
legal_andcheck_move(game_state_t *gs, int fFile, int fRank,
    int tFile, int tRank)
{
	move_t	mt;

	if (!legal_move(gs, fFile, fRank, tFile, tRank))
		return 0;

	mt.color	= gs->onMove;
	mt.fromFile	= fFile;
	mt.fromRank	= fRank;
	mt.toFile	= tFile;
	mt.toRank	= tRank;

	if (move_calculate(gs, &mt, QUEEN) == MOVE_OK)
		return 1;
	return 0;
}

PUBLIC int
in_check(game_state_t *gs)
{
	int	f, r;
	int	kf = -1, kr = -1;

	/* Find the king */
	if (gs->onMove == WHITE) {
		for (f = 0; f < 8 && kf < 0; f++) {
			for (r = 0; r < 8 && kf < 0; r++) {
				if (gs->board[f][r] == B_KING) {
					kf = f;
					kr = r;
				}
			}
		}
	} else {
		for (f = 0; f < 8 && kf < 0; f++) {
			for (r = 0; r < 8 && kf < 0; r++) {
				if (gs->board[f][r] == W_KING) {
					kf = f;
					kr = r;
				}
			}
		}
	}

	if (kf < 0) {
		fprintf(stderr, "FICS: Error game with no king!\n");
		return 0;
	}

	for (InitPieceLoop(gs->board, &f, &r, gs->onMove);
	     NextPieceLoop(gs->board, &f, &r, gs->onMove);) {
		if (legal_move(gs, f, r, kf, kr))	// In Check?
			return 1;
	}

	return 0;
}

PRIVATE int
has_legal_move(game_state_t *gs)
{
	int	f, r;
	int	i;
	int	kf = 0, kr = -1;
	int	kf_and_kr_set = 0;
	int	numpossible = 0;
	int	possiblef[500];
	int	possibler[500];

	for (InitPieceLoop(gs->board, &f, &r, gs->onMove);
	     NextPieceLoop(gs->board, &f, &r, gs->onMove);) {
		switch (piecetype(gs->board[f][r])) {
		case PAWN:
			possible_pawn_moves(gs, f, r, possiblef, possibler,
			    &numpossible);
			break;
		case KNIGHT:
			possible_knight_moves(gs, f, r, possiblef, possibler,
			    &numpossible);
			break;
		case BISHOP:
			possible_bishop_moves(gs, f, r, possiblef, possibler,
			    &numpossible);
			break;
		case ROOK:
			possible_rook_moves(gs, f, r, possiblef, possibler,
			    &numpossible);
			break;
		case QUEEN:
			possible_queen_moves(gs, f, r, possiblef, possibler,
			    &numpossible);
			break;
		case KING:
			kf = f;
			kr = r;
			kf_and_kr_set = 1;
			possible_king_moves(gs, f, r, possiblef, possibler,
			    &numpossible);
			break;
		}
		if (numpossible >= 500)
			fprintf(stderr, "FICS: Possible move overrun\n");
		for (i = 0; i < numpossible; i++) {
			if (legal_andcheck_move(gs, f, r, possiblef[i],
			    possibler[i]))
				return 1;
		}
	}

	if (!kf_and_kr_set && kf == 0 && kr == -1) {
		fprintf(stderr, "FICS: %s: 'kf_and_kr_set' is 0\n", __func__);
		return 0;
	}

	// IanO: if we got here, then kf and kr must be set
	if (gs->gameNum >= 0 && garray[gs->gameNum].link >= 0) {
		// bughouse: potential drops as check interpositions
		gs->holding[gs->onMove == WHITE ? 0 : 1][QUEEN - 1]++;
		for (f = kf - 1; f <= kf + 1; f++) {
			for (r = kr - 1; r <= kr + 1; r++) {
				if (f >= 0 &&
				    f < 8 &&
				    r >= 0 &&
				    r < 8 &&
				    gs->board[f][r] == NOPIECE) {
					// try a drop next to the king
					if (legal_andcheck_move(gs, ALG_DROP,
					    QUEEN, f, r)) {
						gs->holding[gs->onMove == WHITE
						    ? 0 : 1][QUEEN - 1]--;
						return 1;
					}
				}
			}
		}

		gs->holding[gs->onMove == WHITE ? 0 : 1][QUEEN - 1]--;
	}

	fprintf(stderr, "FICS: NO LEGAL MOVE!\n");
	return 0;
}

PUBLIC int
parse_move(char *mstr, game_state_t *gs, move_t *mt, int promote)
{
	int	result;
	int	type = is_move(mstr);

	mt->color = gs->onMove;

	switch (type) {
	case MS_NOTMOVE:
		return MOVE_ILLEGAL;
	case MS_COMP:
		mt->fromFile = mstr[0] - 'a';
		mt->fromRank = mstr[1] - '1';
		mt->toFile = mstr[2] - 'a';
		mt->toRank = mstr[3] - '1';
		break;
	case MS_COMPDASH:
		mt->fromFile = mstr[0] - 'a';
		mt->fromRank = mstr[1] - '1';
		mt->toFile = mstr[3] - 'a';
		mt->toRank = mstr[4] - '1';
		break;
	case MS_KCASTLE:
		mt->fromFile = 4;
		mt->toFile = 6;

		if (gs->onMove == WHITE) {
			mt->fromRank = 0;
			mt->toRank = 0;
		} else {
			mt->fromRank = 7;
			mt->toRank = 7;
		}

		break;
	case MS_QCASTLE:
		mt->fromFile = 4;
		mt->toFile = 2;

		if (gs->onMove == WHITE) {
			mt->fromRank = 0;
			mt->toRank = 0;
		} else {
			mt->fromRank = 7;
			mt->toRank = 7;
		}

		break;
	case MS_ALG:
		/* Fills in the mt structure */
		if ((result = alg_parse_move(mstr, gs, mt)) != MOVE_OK)
			return result;
		break;
	default:
		return MOVE_ILLEGAL;
	}

	if (!legal_move(gs, mt->fromFile, mt->fromRank, mt->toFile, mt->toRank))
		return MOVE_ILLEGAL;

	return move_calculate(gs, mt, promote);
}

/*
 * Returns 'MOVE_OK', 'MOVE_NOMATERIAL', 'MOVE_CHECKMATE', or
 * 'MOVE_STALEMATE'.
 *
 * ('check_game_status' prevents recursion.)
 */
PUBLIC int
execute_move(game_state_t *gs, move_t *mt, int check_game_status)
{
	int	i, j, foobar;
	int	movedPiece;
	int	tookPiece;

	if (mt->fromFile == ALG_DROP) {
		movedPiece	= mt->fromRank;
//		tookPiece	= NOPIECE;

		gs->holding[gs->onMove == WHITE ? 0 : 1][movedPiece-1]--;
		gs->board[mt->toFile][mt->toRank] = (movedPiece | gs->onMove);

		if (gs->gameNum >= 0)
			gs->lastIrreversable = garray[gs->gameNum].numHalfMoves;
	} else {
		movedPiece	= gs->board[mt->fromFile][mt->fromRank];
		tookPiece	= gs->board[mt->toFile][mt->toRank];

		if (mt->piecePromotionTo == NOPIECE) {
			gs->board[mt->toFile][mt->toRank] =
			    gs->board[mt->fromFile][mt->fromRank];
		} else {
			gs->board[mt->toFile][mt->toRank] =
			    (mt->piecePromotionTo | gs->onMove);
		}

		gs->board[mt->fromFile][mt->fromRank] = NOPIECE;

		/*
		 * Check if irreversable
		 */
		if (piecetype(movedPiece) == PAWN || tookPiece != NOPIECE) {
			if (gs->gameNum >= 0) {
				gs->lastIrreversable =
				    garray[gs->gameNum].numHalfMoves;
			}
		}

		/*
		 * Check if this move is en-passant
		 */
		if (piecetype(movedPiece) == PAWN &&
		    mt->fromFile != mt->toFile &&
		    tookPiece == NOPIECE) {
			if (gs->onMove == WHITE)
				mt->pieceCaptured = B_PAWN;
			else
				mt->pieceCaptured = W_PAWN;

			if (mt->fromFile > mt->toFile)
				mt->enPassant = -1;
			else
				mt->enPassant = 1;
			gs->board[mt->toFile][mt->fromRank] = NOPIECE;
		}

		/*
		 * Check en-passant flags for next moves
		 */
		for (i = 0; i < 8; i++) {
			gs->ep_possible[0][i] = 0;
			gs->ep_possible[1][i] = 0;
		}

		if (piecetype(movedPiece) == PAWN &&
		    (mt->fromRank == (mt->toRank + 2) ||
		    (mt->fromRank + 2) == mt->toRank)) {
			/*
			 * Should turn on enpassent flag if possible.
			 */

			if (gs->onMove == WHITE) {
				if (mt->toFile < 7 &&
				    gs->board[mt->toFile + 1][3] == B_PAWN)
					gs->ep_possible[1][mt->toFile + 1] = -1;
				if ((mt->toFile - 1) >= 0 &&
				    gs->board[mt->toFile - 1][3] == B_PAWN)
					gs->ep_possible[1][mt->toFile - 1] = 1;
			} else {
				if (mt->toFile < 7 &&
				    gs->board[mt->toFile + 1][4] == W_PAWN)
					gs->ep_possible[0][mt->toFile + 1] = -1;
				if ((mt->toFile - 1) >= 0 &&
				    gs->board[mt->toFile - 1][4] == W_PAWN)
					gs->ep_possible[0][mt->toFile - 1] = 1;
			}
		}

		if (piecetype(movedPiece) == ROOK && mt->fromFile == 0) {
			if (mt->fromRank == 0 && gs->onMove == WHITE)
				gs->wqrmoved = 1;
			if (mt->fromRank == 7 && gs->onMove == BLACK)
				gs->bqrmoved = 1;
		}

		if (piecetype(movedPiece) == ROOK && mt->fromFile == 7) {
			if (mt->fromRank == 0 && gs->onMove == WHITE)
				gs->wkrmoved = 1;
			if (mt->fromRank == 7 && gs->onMove == BLACK)
				gs->bkrmoved = 1;
		}

		if (piecetype(movedPiece) == KING) {
			if (gs->onMove == WHITE)
				gs->wkmoved = 1;
			else
				gs->bkmoved = 1;
		}

		if (piecetype(movedPiece) == KING &&
		    (mt->fromFile == 4 && mt->toFile == 6)) {	// Check for KS
								// castling
			gs->board[5][mt->toRank] = gs->board[7][mt->toRank];
			gs->board[7][mt->toRank] = NOPIECE;
		}

		if (piecetype(movedPiece) == KING &&
		    (mt->fromFile == 4 && mt->toFile == 2)) {	// Check for QS
								// castling
			gs->board[3][mt->toRank] = gs->board[0][mt->toRank];
			gs->board[0][mt->toRank] = NOPIECE;
		}
	}

	if (gs->onMove == BLACK)
		gs->moveNum++;

	if (check_game_status) {
		/*
		 * Does this move result in check?
		 */

		if (in_check(gs)) {
			/*
			 * Check for checkmate
			 */

			gs->onMove = CToggle(gs->onMove);

			if (!has_legal_move(gs))
				return MOVE_CHECKMATE;
		} else {
			/*
			 * Check for stalemate
			 */

			gs->onMove = CToggle(gs->onMove);

			if (!has_legal_move(gs))
				return MOVE_STALEMATE;

			/*
			 * loon: check for insufficient mating material
			 */

			foobar = 0;

			for (i = 0; i < 8; i++) {
				for (j = 0; j < 8; j++) {
					switch (piecetype(gs->board[i][j])) {
					case KNIGHT:
					case BISHOP:
						foobar++;
						break;
					case KING:
					case NOPIECE:
						break;
					default:
						foobar = 2;
						break;
					}
				}
			}

			if (foobar < 2)
				return MOVE_NOMATERIAL;
		}
	} else {
		gs->onMove = CToggle(gs->onMove);
	}

	return MOVE_OK;
}

PRIVATE void
piecetype_rook(int g, int mode, game_state_t *gs, move_t *m, move_t **m1)
{
	int i;

	if (m->color == WHITE) {
		if (m->fromFile == 0 && m->fromRank == 0) {
			for (i = 2; i < garray[g].numHalfMoves - 1; i += 2) {
				*m1 = ((mode == REL_GAME)
				       ? &garray[g].moveList[i]
				       : &garray[g].examMoveList[i]);

				if ((*m1)->fromFile == 0 &&
				    (*m1)->fromRank == 0)
					break;
			}

			if (i == (garray[g].numHalfMoves - 1))
				gs->wqrmoved = 0;
		}

		if (m->fromFile == 7 && m->fromRank == 0) {
			for (i = 2; i < garray[g].numHalfMoves - 1; i += 2) {
				*m1 = ((mode == REL_GAME)
				       ? &garray[g].moveList[i]
				       : &garray[g].examMoveList[i]);

				if ((*m1)->fromFile == 7 &&
				    (*m1)->fromRank == 0)
					break;
			}

			if (i == (garray[g].numHalfMoves - 1))
				gs->wkrmoved = 0;
		}
	} else {
		if (m->fromFile == 0 && m->fromRank == 7) {
			for (i = 3; i < garray[g].numHalfMoves - 1; i += 2) {
				*m1 = ((mode == REL_GAME)
				       ? &garray[g].moveList[i]
				       : &garray[g].examMoveList[i]);

				if ((*m1)->fromFile == 0 &&
				    (*m1)->fromRank == 0)
					break;
			}

			if (i == (garray[g].numHalfMoves - 1))
				gs->bqrmoved = 0;
		}

		if (m->fromFile == 7 && m->fromRank == 7) {
			for (i = 3; i < garray[g].numHalfMoves - 1; i += 2) {
				*m1 = ((mode == REL_GAME)
				       ? &garray[g].moveList[i]
				       : &garray[g].examMoveList[i]);

				if ((*m1)->fromFile == 7 &&
				    (*m1)->fromRank == 0)
					break;
			}

			if (i == (garray[g].numHalfMoves - 1))
				gs->bkrmoved = 0;
		}
	}
}

#ifdef TIMESEAL
PRIVATE void
backup_move_timeseal_block(int g, move_t *m)
{
	if (m->color == WHITE) {
		if (con[parray[garray[g].white].socket].timeseal) {
			garray[g].wRealTime += (m->tookTime * 100);
			garray[g].wRealTime -= (garray[g].wIncrement * 100);
			garray[g].wTime = (garray[g].wRealTime / 100);

			if (con[parray[garray[g].black].socket].timeseal) {
				garray[g].bTime = (garray[g].bRealTime / 100);
			} else {   // Opp has no timeseal
				garray[g].bTime += (garray[g].lastDecTime -
						    garray[g].lastMoveTime);
			}
		} else { // White has no timeseal
			garray[g].wTime += m->tookTime;
			garray[g].wTime -= garray[g].wIncrement;

			if (con[parray[garray[g].black].socket].timeseal) {
				garray[g].bTime = (garray[g].bRealTime / 100);
			} else {   // Opp has no timeseal
				garray[g].bTime += (garray[g].lastDecTime -
						    garray[g].lastMoveTime);
			}
		}
	} else {
		if (con[parray[garray[g].black].socket].timeseal) {
			garray[g].bRealTime += (m->tookTime * 100);
			garray[g].bRealTime -= (garray[g].wIncrement * 100);
			garray[g].bTime = (garray[g].bRealTime / 100);

			if (con[parray[garray[g].white].socket].timeseal) {
				garray[g].wTime = (garray[g].wRealTime / 100);
			} else {   // Opp has no timeseal
				garray[g].wTime += (garray[g].lastDecTime -
						    garray[g].lastMoveTime);
			}
		} else { // Black has no timeseal
			garray[g].bTime += m->tookTime;

			if (!garray[g].bIncrement)
				garray[g].bTime -= garray[g].wIncrement;
			else
				garray[g].bTime -= garray[g].bIncrement;

			if (con[parray[garray[g].white].socket].timeseal) {
				garray[g].wTime = (garray[g].wRealTime / 100);
			} else {   // Opp has no timeseal
				garray[g].wTime += (garray[g].lastDecTime -
						    garray[g].lastMoveTime);
			}
		}
	}
}
#endif // TIMESEAL

PRIVATE void
update_enpassant_array(int g, int mode, game_state_t *gs)
{
	move_t *m1;

	if (!(garray[g].numHalfMoves > 0))
		return;

	m1 = ((mode == REL_GAME)
	      ? &garray[g].moveList[garray[g].numHalfMoves - 1]
	      : &garray[g].examMoveList[garray[g].numHalfMoves - 1]);

	if (piecetype(gs->board[m1->toFile][m1->toRank]) == PAWN) {
		if ((m1->toRank - m1->fromRank) == 2) {
			if (m1->toFile < 7 &&
			    gs->board[m1->toFile + 1][3] == B_PAWN)
				gs->ep_possible[1][m1->toFile + 1] = -1;
			if (m1->toFile - 1 >= 0 &&
			    gs->board[m1->toFile - 1][3] == B_PAWN)
				gs->ep_possible[1][m1->toFile - 1] = 1;
		}

		if ((m1->toRank - m1->fromRank) == -2) {
			if (m1->toFile < 7 &&
			    gs->board[m1->toFile + 1][4] == W_PAWN)
				gs->ep_possible[0][m1->toFile + 1] = -1;
			if (m1->toFile - 1 >= 0 &&
			    gs->board[m1->toFile - 1][4] == W_PAWN)
				gs->ep_possible[0][m1->toFile - 1] = 1;
		}
	}
}

PRIVATE void
clean_up_doit(int g, int mode, game_state_t *gs, move_t *m)
{
	unsigned int now;

	if (garray[g].status != GAME_EXAMINE)
		game_update_time(g);

	garray[g].numHalfMoves--;

	if (garray[g].status != GAME_EXAMINE) {
		if (garray[g].wInitTime) {	// Don't update times in untimed
						// games.
			now = tenth_secs();
#ifdef TIMESEAL
			backup_move_timeseal_block(g, m);
#else
			if (m->color == WHITE) {
				garray[g].wTime   += m->tookTime;
				garray[g].wTime   = (garray[g].wTime -
						     garray[g].wIncrement);
				garray[g].bTime   +=
				    (garray[g].lastDecTime -
				     garray[g].lastMoveTime);
			} else {
				garray[g].bTime += m->tookTime;

				if (!garray[g].bIncrement) {
					garray[g].bTime =
					    (garray[g].bTime -
					     garray[g].wIncrement);
				} else {
					garray[g].bTime =
					    (garray[g].bTime -
					     garray[g].bIncrement);
				}

				garray[g].wTime += (garray[g].lastDecTime -
						    garray[g].lastMoveTime);
			}
#endif

			if (garray[g].numHalfMoves == 0)
				garray[g].timeOfStart = now;
			garray[g].lastMoveTime = now;
			garray[g].lastDecTime = now;
		}
	}

	if (gs->onMove == BLACK) {
		gs->onMove = WHITE;
	} else {
		gs->onMove = BLACK;
		gs->moveNum--;
	}

	/*
	 * Take back of last move is done already. It's time to update
	 * enpassant array...
	 */
	update_enpassant_array(g, mode, gs);
}

PUBLIC int
backup_move(int g, int mode)
{
	game_state_t	*gs;
	int		 i;
	move_t		*m, *m1;

	if (garray[g].link >= 0)   // Not implemented for bughouse yet.
		return MOVE_ILLEGAL;
	if (garray[g].numHalfMoves < 1)
		return MOVE_ILLEGAL;

	gs = &garray[g].game_state;
	m = ((mode == REL_GAME)
	     ? &garray[g].moveList[garray[g].numHalfMoves - 1]
	     : &garray[g].examMoveList[garray[g].numHalfMoves - 1]);

	if (m->toFile < 0)
		return MOVE_ILLEGAL;

	gs->board[m->fromFile][m->fromRank] = gs->board[m->toFile][m->toRank];

	if (m->piecePromotionTo != NOPIECE) {
		gs->board[m->fromFile][m->fromRank] = (PAWN |
		    colorval(gs->board[m->fromFile][m->fromRank]));
	}

	/*
	 * When take back a _first_ move of rook, the ??rmoved
	 * variable must be cleared. To check if the move is first we
	 * should the scan move list.
	 */
	if (piecetype(gs->board[m->fromFile][m->fromRank]) == ROOK)
		piecetype_rook(g, mode, gs, m, &m1);

	if (piecetype(gs->board[m->fromFile][m->fromRank]) == KING) {
		gs->board[m->toFile][m->toRank] = m->pieceCaptured;

		if (m->toFile - m->fromFile == 2) {
			gs->board[7][m->fromRank] = (ROOK |
			    colorval(gs->board[m->fromFile][m->fromRank]));
			gs->board[5][m->fromRank] = NOPIECE;

			/*
			 * If take back a castling, the appropriates
			 * ??moved variables must be cleared.
			 */
			if (m->color == WHITE) {
				gs->wkmoved = 0;
				gs->wkrmoved = 0;
			} else {
				gs->bkmoved = 0;
				gs->bkrmoved = 0;
			}

			goto cleanupMove;
		}

		if (m->fromFile - m->toFile == 2) {
			gs->board[0][m->fromRank] = (ROOK |
			    colorval(gs->board[m->fromFile][m->fromRank]));
			gs->board[3][m->fromRank] = NOPIECE;

			/*
			 * If take back a castling, the appropriate
			 * ??moved variables must be cleared.
			 */
			if (m->color == WHITE) {
				gs->wkmoved = 0;
				gs->wqrmoved = 0;
			} else {
				gs->bkmoved = 0;
				gs->bqrmoved = 0;
			}

			goto cleanupMove;
		}

		/*
		 * When take back a _first_ move of king (not the
		 * castling), the ?kmoved variable must be cleared. To
		 * check if the move is first we should scan the move
		 * list.
		 */
		if (m->color == WHITE) {
			if (m->fromFile == 4 && m->fromRank == 0) {
				for (i = 2;
				    i < garray[g].numHalfMoves - 1;
				    i += 2) {
					m1 = ((mode == REL_GAME)
					      ? &garray[g].moveList[i]
					      : &garray[g].examMoveList[i]);

					if (m1->fromFile == 4 &&
					    m1->fromRank == 0)
						break;
				}

				if (i == (garray[g].numHalfMoves - 1))
					gs->wkmoved = 0;
			}
		} else {
			if (m->fromFile == 4 && m->fromRank == 7) {
				for (i = 3;
				    i < garray[g].numHalfMoves - 1;
				    i += 2) {
					m1 = ((mode == REL_GAME)
					      ? &garray[g].moveList[i]
					      : &garray[g].examMoveList[i]);

					if (m1->fromFile == 4 &&
					    m1->fromRank == 7)
						break;
				}

				if (i == (garray[g].numHalfMoves - 1))
					gs->bkmoved = 0;
			}
		}
	}

	if (m->enPassant) {
		if (colorval(gs->board[m->fromFile][m->fromRank]) == WHITE)
			gs->board[m->toFile][m->fromRank] = (PAWN | BLACK);
		else
			gs->board[m->toFile][m->fromRank] = (PAWN | WHITE);

		gs->board[m->toFile][m->toRank] = NOPIECE;

		/*
		 * Should set the enpassant array. But I don't care
		 * right now.
		 */
		goto cleanupMove;
	}

	gs->board[m->toFile][m->toRank] = m->pieceCaptured;

  cleanupMove:

	clean_up_doit(g, mode, gs, m);

	return MOVE_OK;
}

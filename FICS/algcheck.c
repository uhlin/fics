/* algcheck.c
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

#include "stdinclude.h"
#include "common.h"

#include <err.h>

#include "algcheck.h"
#include "board.h"
#include "maxxes-utils.h"
#include "movecheck.h"
#include "utils.h"

#define ALG_UNKNOWN -1

/*
 * Well, lets see if I can list the possibilities:
 *
 * Piece moves:
 * Ne4
 * Nxe4
 * Nce4
 * Ncxe4
 * R2f3
 * R2xf3
 *
 * Special pawn moves:
 * e4
 * ed
 * exd
 * exd5
 * ed5
 *
 * Drop moves (bughouse, board edit):
 * P@f7 P*f7
 * #f7 #Nf7
 * (o-o, o-o-o)
 * Castling is handled earlier, so don't worry about that. Of course
 * any of these can have a + or ++ or = string on the end, just cut
 * that off.
 */

/*
 * f - file
 * r - rank
 * p - piece
 * x - x
 * @ - drop character (bughouse)
 */
PRIVATE char *alg_list[] = {
	"fxfr", "pxfr",	// These two get confused in case of bishop
	"ffr", "pfr",	// These two get confused in case of bishop
	"pffr",
	"pfxfr",
	"prfr",
	"prxfr",
	"fr",
	"ff",
	"fxf",
	"p@fr",
	"#fr",
	"#pfr",
	NULL
};

PRIVATE int
get_move_info(char *str, int *piece, int *ff, int *fr, int *tf, int *tr,
    int *bishconfusion)
{
	char	*s;
	char	 c;
	char	 tmp[1024] = { '\0' };
	int	 i, j, len;
	int	 lpiece, lff, lfr, ltf, ltr;
	int	 matchVal = -1;

	*bishconfusion = 0;
	mstrlcpy(tmp, str, sizeof tmp);

	if ((s = strchr(tmp, '+'))) // Cut off any check marks
		*s = '\0';
	if ((s = strchr(tmp, '='))) // Cut off any promotion marks
		*s = '\0';

	*piece = *ff = *fr = *tf = *tr = ALG_UNKNOWN;
	len = strlen(tmp);

	for (i = 0; alg_list[i]; i++) {
		lpiece = lff = lfr = ltf = ltr = ALG_UNKNOWN;

		if (strlen(alg_list[i]) != (size_t)len)
			continue;

		for (j = len - 1; j >= 0; j--) {
			switch (alg_list[i][j]) {
			case 'f':
				if ((tmp[j] < 'a') || (tmp[j] > 'h'))
					goto nomatch;
				if (ltf == ALG_UNKNOWN)
					ltf = tmp[j] - 'a';
				else
					lff = tmp[j] - 'a';
				break;
			case 'r':
				if ((tmp[j] < '1') || (tmp[j] > '8'))
					goto nomatch;
				if (ltr == ALG_UNKNOWN)
					ltr = tmp[j] - '1';
				else
					lfr = tmp[j] - '1';
				break;
			case 'p':
				if (isupper(tmp[j]))
					c = tolower(tmp[j]);
				else
					c = tmp[j];
				if (c == 'k')
					lpiece = KING;
				else if (c == 'q')
					lpiece = QUEEN;
				else if (c == 'r')
					lpiece = ROOK;
				else if (c == 'b')
					lpiece = BISHOP;
				else if (c == 'n')
					lpiece = KNIGHT;
				else if (c == 'p')
					lpiece = PAWN;
				else
					goto nomatch;
				break;
			case 'x':
				if ((tmp[j] != 'x') && (tmp[j] != 'X'))
					goto nomatch;
				break;
			case '@':
				if (tmp[j] != '@' && tmp[j] != '*')
					goto nomatch;
				lff = lfr = ALG_DROP;
				break;
			case '#':
				if (tmp[j] != '#')
					goto nomatch;
				lff = lfr = ALG_DROP;
				break;
			default:
				fprintf(stderr, "Unknown character in "
				    "algebraic parsing\n");
				break;
			}
		}

		if (lpiece == ALG_UNKNOWN)
			lpiece = PAWN;
		if (lpiece == PAWN && (lfr == ALG_UNKNOWN)) { // ffr or ff
			if (lff != ALG_UNKNOWN) {
				if (lff == ltf)
					goto nomatch;
				if ((lff - ltf != 1) && (ltf - lff != 1))
					goto nomatch;
			}
		}

		*piece = lpiece; // We have a match
		*tf = ltf;
		*tr = ltr;
		*ff = lff;
		*fr = lfr;

		if (matchVal != -1) {
			/*
			 * We have two matches, it must be that Bxc4
			 * vs. bxc4 problem. Or it could be the Bc4 vs
			 * bc4 problem.
			 */
			*bishconfusion = 1;
		}

		matchVal = i;

	  nomatch:;
	} /* for */

	if (matchVal != -1)
		return MS_ALG;
	else
		return MS_NOTMOVE;
}

PUBLIC int
alg_is_move(char *mstr)
{
	int piece, ff, fr, tf, tr, bc;

	return get_move_info(mstr, &piece, &ff, &fr, &tf, &tr, &bc);
}

/*
 * We already know it is algebraic - get the move squares.
 */
PUBLIC int
alg_parse_move(char *mstr, game_state_t *gs, move_t *mt)
{
	int	f, r, tmpr, posf, posr, posr2;
	int	piece, ff, fr, tf, tr, bc;

	if (get_move_info(mstr, &piece, &ff, &fr, &tf, &tr, &bc) != MS_ALG) {
		fprintf(stderr, "FICS: Shouldn't try to algebraically parse "
		    "non-algebraic move string.\n");
		return MOVE_ILLEGAL;
	}

	// Resolve ambiguities in to-ness
	if (tf == ALG_UNKNOWN)
		return MOVE_AMBIGUOUS;	// Must always know to file
	if (tr == ALG_UNKNOWN) {
		posr = posr2 = ALG_UNKNOWN;

		if (piece != PAWN)
			return MOVE_AMBIGUOUS;
		if (ff == ALG_UNKNOWN)
			return MOVE_AMBIGUOUS;

		/*
		 * Need to find pawn on ff that can take to tf and
		 * fill in ranks.
		 */
		for (InitPieceLoop(gs->board, &f, &r, gs->onMove);
		     NextPieceLoop(gs->board, &f, &r, gs->onMove);) {
			if ((ff != ALG_UNKNOWN) && (ff != f))
				continue;

			if (f < 0 || f >= 8 ||
			    r < 0 || r >= 8) {
				warnx("%s: out-of-bounds array read/write: "
				    "f:%d r:%d", __func__, f, r);
				return MOVE_AMBIGUOUS;
			}
			if (piecetype(gs->board[f][r]) != piece)
				continue;

			if (gs->onMove == WHITE) {
				tmpr = r + 1;
			} else {
				tmpr = r - 1;
			}

			if (gs->board[tf][tmpr] == NOPIECE) {
				if ((gs->ep_possible[((gs->onMove == WHITE) ?
				    0 : 1)][ff]) != (tf - ff))
					continue;
			} else {
				if (iscolor(gs->board[tf][tmpr], gs->onMove))
					continue;
			}

			if (legal_andcheck_move(gs, f, r, tf, tmpr)) {
				if ((posr != ALG_UNKNOWN) &&
				    (posr2 != ALG_UNKNOWN))
					return MOVE_AMBIGUOUS;
				posr = tmpr;
				posr2 = r;
			}
		}

		tr = posr;
		fr = posr2;
	} else if (bc) {	// Could be bxc4 or Bxc4, tr is known.
		ff = ALG_UNKNOWN;
		fr = ALG_UNKNOWN;

		for (InitPieceLoop(gs->board, &f, &r, gs->onMove);
		     NextPieceLoop(gs->board, &f, &r, gs->onMove);) {
			if ((piecetype(gs->board[f][r]) != PAWN) &&
			    (piecetype(gs->board[f][r]) != BISHOP))
				continue;
			if (legal_andcheck_move(gs, f, r, tf, tr)) {
				if ((piecetype(gs->board[f][r]) == PAWN) &&
				    (f != 1))
					continue;
				if ((ff != ALG_UNKNOWN) && (fr != ALG_UNKNOWN))
					return (MOVE_AMBIGUOUS);
				ff = f;
				fr = r;
			}
		}
	} else {	// The from position is unknown
		posf = ALG_UNKNOWN;
		posr = ALG_UNKNOWN;

		if ((ff == ALG_UNKNOWN) || (fr == ALG_UNKNOWN)) {
			/*
			 * Need to find a piece that can go to tf, tr.
			 */
			for (InitPieceLoop(gs->board, &f, &r, gs->onMove);
			     NextPieceLoop(gs->board, &f, &r, gs->onMove);) {
				if ((ff != ALG_UNKNOWN) && (ff != f))
					continue;
				if ((fr != ALG_UNKNOWN) && (fr != r))
					continue;
				if (piecetype(gs->board[f][r]) != piece)
					continue;
				if (legal_andcheck_move(gs, f, r, tf, tr)) {
					if ((posf != ALG_UNKNOWN) &&
					    (posr != ALG_UNKNOWN))
						return MOVE_AMBIGUOUS;
					posf = f;
					posr = r;
				}
			}
		} else if (ff == ALG_DROP) {
			if (legal_andcheck_move(gs, ALG_DROP, piece, tf, tr)) {
				posf = ALG_DROP;
				posr = piece;
			}
		}

		ff = posf;
		fr = posr;
	}

	if ((tf == ALG_UNKNOWN) || (tr == ALG_UNKNOWN) ||
	    (ff == ALG_UNKNOWN) || (fr == ALG_UNKNOWN))
		return MOVE_ILLEGAL;

	mt->fromFile = ff;
	mt->fromRank = fr;
	mt->toFile = tf;
	mt->toRank = tr;

	return MOVE_OK;
}

/*
 * A assumes the move has yet to be made on the board. (This is the
 * old stupid function, we are testing one from soso...)
 */
#if 0
PUBLIC char *
alg_unparse(game_state_t *gs, move_t *mt)
{
	static char	mStr[20] = { '\0' };

	if ((piecetype(gs->board[mt->fromFile][mt->fromRank]) == KING) &&
	    ((mt->fromFile == 4) && (mt->toFile == 6)))
		return "o-o";
	if ((piecetype(gs->board[mt->fromFile][mt->fromRank]) == KING) &&
	    ((mt->fromFile == 4) && (mt->toFile == 2)))
		return "o-o-o";

	msnprintf(mStr, sizeof mStr, "%c%d%c%d",
	    mt->fromFile + 'a',
	    mt->fromRank + 1,
	    mt->toFile + 'a',
	    mt->toRank + 1);
	return mStr;
}
#endif

/*
 * Soso: Rewrote the alg_unparse() function.
 *
 * Algebraic deparser - sets the 'mStr' variable with move description
 * in short notation. Used in last move report and in 'moves' command.
 */
PUBLIC char *
alg_unparse(game_state_t *gs, move_t *mt)
{
	char		tmp[20] = { '\0' };
	game_state_t	fakeMove;
	int		ambig, r_ambig, f_ambig;
	int		piece, f, r;
	static char	mStr[20] = { '\0' };

	if (mt->fromFile == ALG_DROP) {
		piece = mt->fromRank;
	} else {
		piece = piecetype(gs->board[mt->fromFile][mt->fromRank]);
	}

	if ((piece == KING) && ((mt->fromFile == 4) && (mt->toFile == 6))) {
		mstrlcpy(mStr, "O-O", sizeof mStr);
		goto check;
	}
	if ((piece == KING) && ((mt->fromFile == 4) && (mt->toFile == 2))) {
		mstrlcpy(mStr, "O-O-O", sizeof mStr);
		goto check;
	}

	mstrlcpy(mStr, "", sizeof mStr);

	switch (piece) {
	case PAWN:
		if (mt->fromFile == ALG_DROP) {
			mstrlcpy(mStr, "P", sizeof mStr);
		} else if (mt->fromFile != mt->toFile) {
			msnprintf(tmp, sizeof tmp, "%c", mt->fromFile + 'a');
			mstrlcpy(mStr, tmp, sizeof mStr);
		}
		break;
	case KNIGHT:
		mstrlcpy(mStr, "N", sizeof mStr);
		break;
	case BISHOP:
		mstrlcpy(mStr, "B", sizeof mStr);
		break;
	case ROOK:
		mstrlcpy(mStr, "R", sizeof mStr);
		break;
	case QUEEN:
		mstrlcpy(mStr, "Q", sizeof mStr);
		break;
	case KING:
		mstrlcpy(mStr, "K", sizeof mStr);
		break;
	default:
		mstrlcpy(mStr, "", sizeof mStr);
		break;
	}

	if (mt->fromFile == ALG_DROP) {
		mstrlcat(mStr, DROP_STR, sizeof mStr);
	} else {
		/*
		 * Checks for ambiguity in short notation (Ncb3, R8e8
		 * or so).
		 */
		if (piece != PAWN) {
			ambig = r_ambig = f_ambig = 0;

			for (r = 0; r < 8; r++) {
				for (f = 0; f < 8; f++) {
					if ((gs->board[f][r] != NOPIECE) &&
					    iscolor(gs->board[f][r], gs->onMove) &&
					    (piecetype(gs->board[f][r]) == piece) &&
					    ((f != mt->fromFile) ||
					    (r != mt->fromRank))) {
						if (legal_move(gs, f, r,
						    mt->toFile, mt->toRank)) {
							fakeMove = *gs;
							fakeMove.board[f][r] =
							    NOPIECE;
							fakeMove.onMove =
							    CToggle(fakeMove.onMove);
							gs->onMove =
							    CToggle(gs->onMove);

							/*
							 * New
							 * bracketing
							 * below to
							 * try to fix
							 * 'ambiguous
							 * move' bug.
							 */
							if ((in_check(gs)) ||
							    !in_check(&fakeMove))
								ambig++;

							if (f == mt->fromFile) {
								f_ambig++;
								ambig++;
							}
							if (r == mt->fromRank) {
								r_ambig++;
								ambig++;
							}

							gs->onMove =
							    CToggle(gs->onMove);
						}
					}
				} /* for */
			} /* for */

			if (ambig > 0) {
				/*
				 * Ambiguity in short notation, need
				 * to add file, rank or _both_ in
				 * notation.
				 */

				if (f_ambig == 0) {
					msnprintf(tmp, sizeof tmp, "%c",
					    mt->fromFile + 'a');
					mstrlcat(mStr, tmp, sizeof mStr);
				} else if (r_ambig == 0) {
					msnprintf(tmp, sizeof tmp, "%d",
					    mt->fromRank + 1);
					mstrlcat(mStr, tmp, sizeof mStr);
				} else {
					msnprintf(tmp, sizeof tmp, "%c%d",
					    mt->fromFile + 'a',
					    mt->fromRank + 1);
					mstrlcat(mStr, tmp, sizeof mStr);
				}
			}
		}

		if ((gs->board[mt->toFile][mt->toRank] != NOPIECE) ||
		    ((piece == PAWN) && (mt->fromFile != mt->toFile))) {
			mstrlcat(mStr, "x", sizeof mStr);
		}
	}

	msnprintf(tmp, sizeof tmp, "%c%d", mt->toFile + 'a', mt->toRank + 1);
	mstrlcat(mStr, tmp, sizeof mStr);

	if ((piece == PAWN) && (mt->piecePromotionTo != NOPIECE)) {
		mstrlcat(mStr, "=", sizeof mStr); // = before promoting piece

		switch (piecetype(mt->piecePromotionTo)) {
		case KNIGHT:
			mstrlcat(mStr, "N", sizeof mStr);
			break;
		case BISHOP:
			mstrlcat(mStr, "B", sizeof mStr);
			break;
		case ROOK:
			mstrlcat(mStr, "R", sizeof mStr);
			break;
		case QUEEN:
			mstrlcat(mStr, "Q", sizeof mStr);
			break;
		default:
			break;
		}
	}

  check:;

	fakeMove = *gs;
	execute_move(&fakeMove, mt, 0);
	fakeMove.onMove = CToggle(fakeMove.onMove);

	if (in_check(&fakeMove))
		mstrlcat(mStr, "+", sizeof mStr);
	return mStr;
}

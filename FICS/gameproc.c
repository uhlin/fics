/* gameproc.c
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
   Dave Herscovici		95/11/26	Split into two files;
   						Second is 'obsproc.c'.
   Markus Uhlin			23/12/16	Fixed compiler warnings
   Markus Uhlin			24/01/04	Fixed pprintf_prompt() calls
*/

#include "stdinclude.h"
#include "common.h"

#include "command.h"
#include "comproc.h"
#include "config.h"
#include "eco.h"
#include "ficsmain.h"
#include "gamedb.h"
#include "gameproc.h"
#include "lists.h"
#include "matchproc.h"
#include "movecheck.h"
#include "network.h"
#include "obsproc.h"
#include "playerdb.h"
#include "ratings.h"
#include "rmalloc.h"
#include "utils.h"

PUBLIC void game_ended(int g, int winner, int why)
{
  char outstr[200];
  char tmp[200];
  int p;
  int gl = garray[g].link;
  int rate_change = 0;
  int isDraw = 0;
  int whiteResult;
  char winSymbol[10];
  char EndSymbol[10];
  char *NameOfWinner, *NameOfLoser;
  int beingplayed = 0;		/* i.e. it wasn't loaded for adjudication */

  beingplayed = (parray[garray[g].black].game == g);

  sprintf(outstr, "\n{Game %d (%s vs. %s) ", g + 1,
	  parray[garray[g].white].name,
	  parray[garray[g].black].name);
  garray[g].result = why;
  garray[g].winner = winner;
  if (winner == WHITE) {
    whiteResult = RESULT_WIN;
    strcpy(winSymbol, "1-0");
    NameOfWinner = parray[garray[g].white].name;
    NameOfLoser = parray[garray[g].black].name;
  } else {
    whiteResult = RESULT_LOSS;
    strcpy(winSymbol, "0-1");
    NameOfWinner = parray[garray[g].black].name;
    NameOfLoser = parray[garray[g].white].name;
  }
  switch (why) {
  case END_CHECKMATE:
    sprintf(tmp, "%s checkmated} %s\n", NameOfLoser, winSymbol);
    strcpy(EndSymbol, "Mat");
    rate_change = 1;
    break;
  case END_RESIGN:
    sprintf(tmp, "%s resigns} %s\n", NameOfLoser, winSymbol);
    strcpy(EndSymbol, "Res");
    rate_change = 1;
    break;
  case END_FLAG:
    sprintf(tmp, "%s forfeits on time} %s\n", NameOfLoser, winSymbol);
    strcpy(EndSymbol, "Fla");
    rate_change = 1;
    break;
  case END_STALEMATE:
    sprintf(tmp, "Game drawn by stalemate} 1/2-1/2\n");
    isDraw = 1;
    strcpy(EndSymbol, "Sta");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_AGREEDDRAW:
    sprintf(tmp, "Game drawn by mutual agreement} 1/2-1/2\n");
    isDraw = 1;
    strcpy(EndSymbol, "Agr");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_BOTHFLAG:
    sprintf(tmp, "Game drawn because both players ran out of time} 1/2-1/2\n");
    isDraw = 1;
    strcpy(EndSymbol, "Fla");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_REPETITION:
    sprintf(tmp, "Game drawn by repetition} 1/2-1/2\n");
    isDraw = 1;
    strcpy(EndSymbol, "Rep");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_50MOVERULE:
    sprintf(tmp, "Game drawn by the 50 move rule} 1/2-1/2\n");
    isDraw = 1;
    strcpy(EndSymbol, "50");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_ADJOURN:
    if (gl >= 0) {
      sprintf(tmp, "Bughouse game aborted.} *\n");
      whiteResult = RESULT_ABORT;
    } else {
    sprintf(tmp, "Game adjourned by mutual agreement} *\n");
    game_save(g);
    }
    break;
  case END_LOSTCONNECTION:
    sprintf(tmp, "%s lost connection; game ", NameOfWinner);
    if (parray[garray[g].white].registered && parray[garray[g].black].registered
        && gl < 0) {
      sprintf(tmp, "adjourned} *\n");
      game_save(g);
    } else
      sprintf(tmp, "aborted} *\n");
    whiteResult = RESULT_ABORT;
    break;
  case END_ABORT:
    sprintf(tmp, "Game aborted by mutual agreement} *\n");
    whiteResult = RESULT_ABORT;
    break;
  case END_COURTESY:
    sprintf(tmp, "Game courtesyaborted by %s} *\n", NameOfWinner);
    whiteResult = RESULT_ABORT;
    break;
  case END_COURTESYADJOURN:
    if (gl >= 0) {
      sprintf(tmp, "Bughouse game courtesyaborted by %s.} *\n", NameOfWinner);
      whiteResult = RESULT_ABORT;
    } else {
    sprintf(tmp, "Game courtesyadjourned by %s} *\n", NameOfWinner);
    game_save(g);
    }
    break;
  case END_NOMATERIAL:
    /* Draw by insufficient material (e.g., lone K vs. lone K) */
    sprintf(tmp, "Neither player has mating material} 1/2-1/2\n");
    isDraw = 1;
    strcpy(EndSymbol, "NM ");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_FLAGNOMATERIAL:
    sprintf(tmp, "%s ran out of time and %s has no material to mate} 1/2-1/2\n",
	    NameOfLoser, NameOfWinner);
    isDraw = 1;
    strcpy(EndSymbol, "TM ");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_ADJWIN:
    sprintf(tmp, "%s wins by adjudication} %s\n", NameOfWinner, winSymbol);
    strcpy(EndSymbol, "Adj");
    rate_change = 1;
    break;
  case END_ADJDRAW:
    sprintf(tmp, "Game drawn by adjudication} 1/2-1/2\n");
    isDraw = 1;
    strcpy(EndSymbol, "Adj");
    rate_change = 1;
    whiteResult = RESULT_DRAW;
    break;
  case END_ADJABORT:
    sprintf(tmp, "Game aborted by adjudication} *\n");
    whiteResult = RESULT_ABORT;
    break;
  default:
    sprintf(tmp, "Hmm, the game ended and I don't know why} *\n");
    break;
  }
  strcat(outstr, tmp);
  if (beingplayed) {
    pprintf_noformat(garray[g].white, outstr);
    pprintf_noformat(garray[g].black, outstr);
    if (parray[garray[g].white].bell)
      pprintf (garray[g].white, "\007");
    if (parray[garray[g].black].bell)
      pprintf (garray[g].black, "\007");

    garray[g].link = -1;		/*IanO: avoids recursion */
    if (gl >= 0 && garray[gl].link >= 0) {
      pprintf_noformat(garray[gl].white, outstr);
      pprintf_noformat(garray[gl].black, outstr);
      game_ended(gl, CToggle(winner), why);
    }

    for (p = 0; p < p_num; p++) {
      if ((p == garray[g].white) || (p == garray[g].black))
	continue;
      if (parray[p].status != PLAYER_PROMPT)
	continue;
      if (!parray[p].i_game && !player_is_observe(p, g))
	continue;
      pprintf_noformat(p, outstr);
      pprintf_prompt(p, "%s", "");
    }
  }
  if ((garray[g].rated) && (rate_change)) {
    /* Adjust ratings */
    rating_update(g);

  } else {
    if (beingplayed) {
      pprintf(garray[g].white, "No ratings adjustment done.\n");
      pprintf(garray[g].black, "No ratings adjustment done.\n");
    }
  }
  if (rate_change && gl < 0)
    game_write_complete(g, isDraw, EndSymbol);
  /* Mail off the moves */
  if (parray[garray[g].white].automail) {
    pcommand(garray[g].white, "mailmoves");
  }
  if (parray[garray[g].black].automail) {
    pcommand(garray[g].black, "mailmoves");
  }
  parray[garray[g].white].num_white++;
  parray[garray[g].white].lastColor = WHITE;
  parray[garray[g].black].num_black++;
  parray[garray[g].black].lastColor = BLACK;
  parray[garray[g].white].last_opponent = garray[g].black;
  parray[garray[g].black].last_opponent = garray[g].white;
  if (beingplayed) {
    parray[garray[g].white].game = -1;
    parray[garray[g].black].game = -1;
    parray[garray[g].white].opponent = -1;
    parray[garray[g].black].opponent = -1;
    if (garray[g].white != commanding_player)
      pprintf_prompt(garray[g].white, "%s", "");
    if (garray[g].black != commanding_player)
      pprintf_prompt(garray[g].black, "%s", "");
    if (parray[garray[g].white].simul_info.numBoards) {
      player_simul_over(garray[g].white, g, whiteResult);
    }
  }
  game_finish(g);
}

PRIVATE int
was_promoted(game *g, int f, int r)
{
#define BUGHOUSE_PAWN_REVERT 1
#if BUGHOUSE_PAWN_REVERT
	for (int i = g->numHalfMoves-2; i > 0; i -= 2) {
		if (g->moveList[i].toFile == f &&
		    g->moveList[i].toRank == r) {
			if (g->moveList[i].piecePromotionTo)
				return 1;
			if (g->moveList[i].fromFile == ALG_DROP)
				return 0;
			f = g->moveList[i].fromFile;
			r = g->moveList[i].fromRank;
		}
	}
#endif
	return 0;
}

PUBLIC int
pIsPlaying(int p)
{
	int	g = parray[p].game;
	int	p1 = parray[p].opponent;

	if (g < 0 || garray[g].status == GAME_EXAMINE) {
		pprintf(p, "You are not playing a game.\n");
		return 0;
	} else if (garray[g].white != p && garray[g].black != p) {
		/*
		 * oh oh; big bad game bug.
		 */

		fprintf(stderr, "BUG: Player %s playing game %d according to "
		    "parray, but not according to garray.\n",
		    parray[p].name, (g + 1));
		pprintf(p, "Disconnecting you from game number %d.\n", (g + 1));
		parray[p].game = -1;

		if (p1 >= 0 &&
		    parray[p1].game == g &&
		    garray[g].white != p1 &&
		    garray[g].black != p1) {
			pprintf(p1, "Disconnecting you from game number %d.\n",
			    (g + 1));
			parray[p1].game = -1;
		}

		return 0;
	} else
		return 1;
}

PUBLIC void
process_move(int p, char *command)
{
	int		 g;
	int		 i;
	int		 len;
	int		 result;
	move_t		 move;
	unsigned int	 now;

	if (parray[p].game < 0) {
		pprintf(p, "You are not playing or examining a game.\n");
		return;
	}

	player_decline_offers(p, -1, -PEND_SIMUL);
	g = parray[p].game;

	if (garray[g].status != GAME_EXAMINE) {
		if (!pIsPlaying(p)) // XXX
			return;
		if (parray[p].side != garray[g].game_state.onMove) {
			pprintf(p, "It is not your move.\n");
			return;
		}
		if (garray[g].clockStopped) {
			pprintf(p, "Game clock is paused, use \"unpause\" "
			    "to resume.\n");
			return;
		}
	}

	if ((len = strlen(command)) > 1) {
		if (command[len - 2] == '=') {
			switch (tolower(command[strlen(command) - 1])) {
			case 'n':
				parray[p].promote = KNIGHT;
				break;
			case 'b':
				parray[p].promote = BISHOP;
				break;
			case 'r':
				parray[p].promote = ROOK;
				break;
			case 'q':
				parray[p].promote = QUEEN;
				break;
			default:
				pprintf(p, "Don't understand that move.\n");
				return;
				break;
			}
		}
	}

	switch (parse_move(command, &garray[g].game_state, &move,
			   parray[p].promote)) {
	case MOVE_ILLEGAL:
		pprintf(p, "Illegal move.\n");
		return;
		break;
	case MOVE_AMBIGUOUS:
		pprintf(p, "Ambiguous move.\n");
		return;
		break;
	default:
		break;
	}

	if (garray[g].status == GAME_EXAMINE) {
		garray[g].numHalfMoves++;

		if (garray[g].numHalfMoves > garray[g].examMoveListSize) {
			size_t size;

			garray[g].examMoveListSize += 20;	// Allocate 20
								// moves at a
								// time
			size = (sizeof(move_t) * garray[g].examMoveListSize);

			if (!garray[g].examMoveList) {
				garray[g].examMoveList = rmalloc(size);
			} else {
				garray[g].examMoveList =
				    rrealloc(garray[g].examMoveList,
				    (sizeof(move_t) *
				    garray[g].examMoveListSize));
			}
		}

		now = tenth_secs();

		result		= execute_move(&garray[g].game_state, &move, 1);
		move.atTime	= now; // XXX
		move.tookTime	= 0;
		MakeFENpos(g, (char *)move.FENpos);
		garray[g].examMoveList[garray[g].numHalfMoves - 1] = move;

		/*
		 * roll back time
		 */

		if (garray[g].game_state.onMove == WHITE) {
			garray[g].wTime +=
			    (garray[g].lastDecTime - garray[g].lastMoveTime);
		} else {
			garray[g].bTime +=
			    (garray[g].lastDecTime - garray[g].lastMoveTime);
		}

		// XXX: 'now' was assigned here
		// <--

		if (garray[g].numHalfMoves == 0)
			garray[g].timeOfStart = now;

		garray[g].lastMoveTime = now;
		garray[g].lastDecTime = now;
	} else { // real game
		i = parray[p].opponent;

		if (parray[i].simul_info.numBoards &&
		    parray[i].simul_info.boards[parray[i].simul_info.onBoard] !=
		    g) {
			pprintf(p, "It isn't your turn: wait until the simul "
			    "giver is at your board.\n");
			return;
		}

#ifdef TIMESEAL
		if (con[parray[p].socket].timeseal) { // Does he use timeseal?
			if (parray[p].side == WHITE) {
				int diff;

				garray[g].wLastRealTime = garray[g].wRealTime;
				garray[g].wTimeWhenMoved =
				    con[parray[p].socket].time;

				diff = (garray[g].wTimeWhenMoved -
					garray[g].wTimeWhenReceivedMove);

				if (diff < 0 ||
				    garray[g].wTimeWhenReceivedMove == 0) {
					/*
					 * Might seem weird - but
					 * could be caused by a person
					 * moving BEFORE he receives
					 * the board pos (this is
					 * possible due to lag) but
					 * it's safe to say he moved
					 * in 0 secs.
					 */
					garray[g].wTimeWhenReceivedMove =
					    garray[g].wTimeWhenMoved;
				} else {
					garray[g].wRealTime -=
					    (garray[g].wTimeWhenMoved -
					    garray[g].wTimeWhenReceivedMove);
				}
			} else if (parray[p].side == BLACK) {
				int diff;

				garray[g].bLastRealTime = garray[g].bRealTime;
				garray[g].bTimeWhenMoved =
				    con[parray[p].socket].time;

				diff = (garray[g].bTimeWhenMoved -
					garray[g].bTimeWhenReceivedMove);

				if (diff < 0 ||
				    garray[g].bTimeWhenReceivedMove == 0) {
					/*
					 * Might seem weird - but
					 * could be caused by a person
					 * moving BEFORE he receives
					 * the board pos (this is
					 * possible due to lag) but
					 * it's safe to say he moved
					 * in 0 secs.
					 */
					garray[g].bTimeWhenReceivedMove =
					    garray[g].bTimeWhenMoved;
				} else {
					garray[g].bRealTime -=
					    (garray[g].bTimeWhenMoved -
					    garray[g].bTimeWhenReceivedMove);
				}
			}
		}

		/*
		 * We need to reset the opp's time for receiving the
		 * board since the timeseal decoder only alters the
		 * time if it's 0. Otherwise the time would be changed
		 * if the player did a refresh which would screw up
		 * the timings.
		 */
		if (parray[p].side == WHITE)
			garray[g].bTimeWhenReceivedMove = 0;
		else
			garray[g].wTimeWhenReceivedMove = 0;
#endif

		game_update_time(g);

		/*
		 * XXX: Maybe add autoflag here in the future?
		 */

#ifdef TIMESEAL
		if (con[parray[p].socket].timeseal) {  // Does he use timeseal?
			if (parray[p].side == WHITE) {
				garray[g].wRealTime +=
				    (garray[g].wIncrement * 100);
				garray[g].wTime = (garray[g].wRealTime / 100);
			} else if (parray[p].side == BLACK) {
				garray[g].bRealTime +=
				    (garray[g].bIncrement * 100);
				garray[g].bTime = (garray[g].bRealTime / 100);
			}
		} else {
			if (garray[g].game_state.onMove == BLACK)
				garray[g].bTime += garray[g].bIncrement;
			if (garray[g].game_state.onMove == WHITE)
				garray[g].wTime += garray[g].wIncrement;
		}
#else
		if (garray[g].game_state.onMove == BLACK)
			garray[g].bTime += garray[g].bIncrement;
		if (garray[g].game_state.onMove == WHITE)
			garray[g].wTime += garray[g].wIncrement;
#endif

		/*
		 * Do the move.
		 */

		garray[g].numHalfMoves++;

		if (garray[g].numHalfMoves > garray[g].moveListSize) {
			garray[g].moveListSize += 20;	// Allocate 20 moves at
							// a time

			if (!garray[g].moveList) {
				garray[g].moveList =
				    rmalloc(sizeof(move_t) *
				    garray[g].moveListSize);
			} else {
				garray[g].moveList =
				    rrealloc(garray[g].moveList,
				    (sizeof(move_t) * garray[g].moveListSize));
			}
		}

		result = execute_move(&garray[g].game_state, &move, 1);

		if (result == MOVE_OK &&
		    garray[g].link >= 0 &&
		    move.pieceCaptured != NOPIECE) {
			/*
			 * Transfer captured piece to partner.
			 * Check if piece reverts to a pawn.
			 */
			if (was_promoted(&garray[g], move.toFile, move.toRank)) {
				update_holding(garray[g].link,
				    colorval(move.pieceCaptured) | PAWN);
			} else {
				update_holding(garray[g].link,
				    move.pieceCaptured);
			}
		}

		now = tenth_secs();
		move.atTime = now;

		if (garray[g].numHalfMoves > 1)
			move.tookTime = (move.atTime - garray[g].lastMoveTime);
		else
			move.tookTime = (move.atTime - garray[g].startTime);

		garray[g].lastMoveTime = now;
		garray[g].lastDecTime = now;

#ifdef TIMESEAL
		if (con[parray[p].socket].timeseal) { // Does he use timeseal?
			if (parray[p].side == WHITE) {
				move.tookTime =
				    ((garray[parray[p].game].wTimeWhenMoved -
				    garray[parray[p].game].wTimeWhenReceivedMove) /
				    100);
			} else {
				move.tookTime =
				    ((garray[parray[p].game].bTimeWhenMoved -
				    garray[parray[p].game].bTimeWhenReceivedMove) /
				    100);
			}
		}
#endif

		MakeFENpos(g, (char *)move.FENpos);
		garray[g].moveList[garray[g].numHalfMoves - 1] = move;
	}

	send_boards(g);

	if (result == MOVE_ILLEGAL)
		pprintf(p, "Internal error, illegal move accepted!\n");
	if (result == MOVE_OK && garray[g].status == GAME_EXAMINE) {
		for (int p1 = 0; p1 < p_num; p1++) {
			if (parray[p1].status != PLAYER_PROMPT)
				continue;
			if (player_is_observe(p1, g) || parray[p1].game == g) {
				pprintf_prompt(p1, "%s moves: %s\n",
				    parray[p].name, move.algString);
			}
		}
	}

	if (result == MOVE_CHECKMATE) {
		if (garray[g].status == GAME_EXAMINE) {
			for (int p1 = 0; p1 < p_num; p1++) {
				if (parray[p1].status != PLAYER_PROMPT)
					continue;
				if (player_is_observe(p1, g) ||
				    parray[p1].game == g) {
					pprintf(p1, "%s has been checkmated.\n",
					    (CToggle(garray[g].game_state.onMove)
					    == BLACK ? "White" : "Black"));
				}
			}
		} else {
			game_ended(g, CToggle(garray[g].game_state.onMove),
			    END_CHECKMATE);
		}
	}

	if (result == MOVE_STALEMATE) {
		if (garray[g].status == GAME_EXAMINE) {
			for (int p1 = 0; p1 < p_num; p1++) {
				if (parray[p1].status != PLAYER_PROMPT)
					continue;
				if (player_is_observe(p1, g) ||
				    parray[p1].game == g)
					pprintf(p1, "Stalemate.\n");
			}
		} else {
			game_ended(g, CToggle(garray[g].game_state.onMove),
			    END_STALEMATE);
		}
	}

	if (result == MOVE_NOMATERIAL) {
		if (garray[g].status == GAME_EXAMINE) {
			for (int p1 = 0; p1 < p_num; p1++) {
				if (parray[p1].status != PLAYER_PROMPT)
					continue;
				if (player_is_observe(p1, g) ||
				    parray[p1].game == g)
					pprintf(p1, "No mating material.\n");
			}
		} else {
			game_ended(g, CToggle(garray[g].game_state.onMove),
			    END_NOMATERIAL);
		}
	}
}

PUBLIC int
com_resign(int p, param_list param)
{
	int	g, o, oconnected;

	if (param[0].type == TYPE_NULL) {
		g = parray[p].game;

		if (!pIsPlaying(p))
			return COM_OK;
		else {
			player_decline_offers(p, -1, -1);
			game_ended(g, (garray[g].white == p ? BLACK : WHITE),
			    END_RESIGN);
		}
	} else if (FindPlayer(p, param[0].val.word, &o, &oconnected)) {
		g = game_new();

		if (game_read(g, p, o) < 0) {
			if (game_read(g, o, p) < 0) {
				pprintf(p, "You have no stored game with %s\n",
				    parray[o].name);
				if (!oconnected)
					player_remove(o);
				return COM_OK;
			} else {
				garray[g].white = o;
				garray[g].black = p;
			}
		} else {
			garray[g].white = p;
			garray[g].black = o;
		}

		pprintf(p, "You resign your stored game with %s\n",
		    parray[o].name);

		game_delete(garray[g].white, garray[g].black);
		game_ended(g, (garray[g].white == p ? BLACK : WHITE),
		    END_RESIGN);

		pcommand(p, "message %s I have resigned our stored game "
		    "\"%s vs. %s.\"",
		    parray[o].name,
		    parray[garray[g].white].name,
		    parray[garray[g].black].name);

		if (!oconnected)
			player_remove(o);
	}

	return COM_OK;
}

PRIVATE int
Check50MoveRule(int p, int g)
{
	int	num_reversible = garray[g].numHalfMoves;

	if (garray[g].game_state.lastIrreversable >= 0)
		num_reversible -= garray[g].game_state.lastIrreversable;

	if (num_reversible > 99) {
		game_ended(g, (garray[g].white == p ? BLACK : WHITE),
		    END_50MOVERULE);
		return 1;
	}

	return 0;
}

PRIVATE char *
GetFENpos(int g, int half_move)
{
	if (half_move < 0)
		return ((char *)garray[g].FENstartPos);
	return ((char *)garray[g].moveList[half_move].FENpos);
}

PRIVATE int
CheckRepetition(int p, int g)
{
	char	*pos1 = GetFENpos(g, garray[g].numHalfMoves - 1);
	char	*pos2 = GetFENpos(g, garray[g].numHalfMoves);
	char	*pos;
	int	 flag1 = 1, flag2 = 1;
	int	 move_num;

	if (garray[g].numHalfMoves < 8) // Can't have three repeats any quicker.
		return 0;

	for (move_num = garray[g].game_state.lastIrreversable;
	    move_num < garray[g].numHalfMoves - 1;
	    move_num++) {
		pos = GetFENpos(g, move_num);

		if (strlen(pos1) == strlen(pos) && !strcmp(pos1, pos))
			flag1++;
		if (strlen(pos2) == strlen(pos) && !strcmp(pos2, pos))
			flag2++;
	}

	if (flag1 >= 3 || flag2 >= 3) {
		if (player_find_pendfrom(p, parray[p].opponent, PEND_DRAW)
		    >= 0) {
			player_remove_request(parray[p].opponent, p, PEND_DRAW);
			player_decline_offers(p, -1, -1);
		}

		game_ended(g, (garray[g].white == p ? BLACK : WHITE),
		    END_REPETITION);
		return 1;
	} else
		return 0;
}

PUBLIC int
com_draw(int p, param_list param)
{
	int p1, g = parray[p].game;

	ASSERT(param[0].type == TYPE_NULL);

	if (!pIsPlaying(p))
		return COM_OK;
	if (Check50MoveRule(p, g) || CheckRepetition(p, g))
		return COM_OK;

	p1 = parray[p].opponent;

	if (parray[p1].simul_info.numBoards &&
	    parray[p1].simul_info.boards[parray[p1].simul_info.onBoard] != g) {
		pprintf(p, "You can only make requests when the simul player "
		    "is at your board.\n");
		return COM_OK;
	}

	if (player_find_pendfrom(p, parray[p].opponent, PEND_DRAW) >= 0) {
		player_remove_request(parray[p].opponent, p, PEND_DRAW);
		player_decline_offers(p, -1, -1);
		game_ended(g, (garray[g].white == p ? BLACK : WHITE),
		    END_AGREEDDRAW);
	} else {
		pprintf(parray[p].opponent, "\n");
		pprintf_highlight(parray[p].opponent, "%s", parray[p].name);
		pprintf_prompt(parray[p].opponent, " offers you a draw.\n");
		pprintf(p, "Draw request sent.\n");
		player_add_request(p, parray[p].opponent, PEND_DRAW, 0);
	}

	return COM_OK;
}

PUBLIC int
com_pause(int p, param_list param)
{
	int	g;
	int	now;

	ASSERT(param[0].type == TYPE_NULL);

	if (!pIsPlaying(p))
		return COM_OK;

	g = parray[p].game;

	if (garray[g].wTime == 0) {
		pprintf(p, "You can't pause untimed games.\n");
		return COM_OK;
	}

	if (garray[g].clockStopped) {
		pprintf(p, "Game is already paused, "
		    "use \"unpause\" to resume.\n");
		return COM_OK;
	}

	if (player_find_pendfrom(p, parray[p].opponent, PEND_PAUSE) >= 0) {
		player_remove_request(parray[p].opponent, p, PEND_PAUSE);
		garray[g].clockStopped = 1;

		// Roll back the time
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

		pprintf_prompt(parray[p].opponent, "\n%s accepted pause. "
		    "Game clock paused.\n", parray[p].name);
		pprintf(p, "Game clock paused.\n");
	} else {
		pprintf(parray[p].opponent, "\n");
		pprintf_highlight(parray[p].opponent, "%s", parray[p].name);
		pprintf_prompt(parray[p].opponent, " requests to pause the "
		    "game.\n");
		pprintf(p, "Pause request sent.\n");
		player_add_request(p, parray[p].opponent, PEND_PAUSE, 0);
	}

	return COM_OK;
}

PUBLIC int
com_unpause(int p, param_list param)
{
	int	g;
	int	now;

	ASSERT(param[0].type == TYPE_NULL);

	if (!pIsPlaying(p))
		return COM_OK;

	g = parray[p].game;

	if (!garray[g].clockStopped) {
		pprintf(p, "Game is not paused.\n");
		return COM_OK;
	}

	garray[g].clockStopped = 0;
	now = tenth_secs();

	if (garray[g].numHalfMoves == 0)
		garray[g].timeOfStart = now;

	garray[g].lastMoveTime = now;
	garray[g].lastDecTime = now;

	send_boards(g);

	pprintf(p, "Game clock resumed.\n");
	pprintf_prompt(parray[p].opponent, "\nGame clock resumed.\n");

	return COM_OK;
}

PUBLIC int
com_abort(int p, param_list param)
{
	int courtesyOK = 1;
	int p1, g, myColor, yourColor, myGTime, yourGTime;

	ASSERT(param[0].type == TYPE_NULL);

	g = parray[p].game;

	if (!pIsPlaying(p))
		return COM_OK;

	p1 = parray[p].opponent;

	if (p == garray[g].white) {
		myColor		= WHITE;
		yourColor	= BLACK;
		myGTime		= garray[g].wTime;
		yourGTime	= garray[g].bTime;
	} else {
		myColor		= BLACK;
		yourColor	= WHITE;
		myGTime		= garray[g].bTime;
		yourGTime	= garray[g].wTime;
	}

	if (parray[p1].simul_info.numBoards &&
	    parray[p1].simul_info.boards[parray[p1].simul_info.onBoard] != g) {
		pprintf(p, "You can only make requests when the simul player "
		    "is at your board.\n");
		return COM_OK;
	}

	if (player_find_pendfrom(p, p1, PEND_ABORT) >= 0) {
		player_remove_request(p1, p, PEND_ABORT);
		player_decline_offers(p, -1, -1);
		game_ended(g, yourColor, END_ABORT);
	} else {
		game_update_time(g);

#ifdef TIMESEAL
		if (con[parray[p].socket].timeseal &&
		    garray[g].game_state.onMove == myColor &&
		    garray[g].flag_pending == FLAG_ABORT) {
			/*
			 * It's my move, opponent has asked for abort;
			 * I lagged out, my timeseal prevented
			 * courtesyabort, and I sent an abort request
			 * before acknowledging (and processing) my
			 * opponent's courtesyabort. OK, let's abort
			 * already :-).
			 */

			player_decline_offers(p, -1, -1);
			game_ended(g, yourColor, END_ABORT);
		}

		if (con[parray[p1].socket].timeseal) { // Opp uses timeseal?
			int yourRealTime = (myColor == WHITE ?
			    garray[g].bRealTime : garray[g].wRealTime);

			if (myGTime > 0 && yourGTime <= 0 && yourRealTime > 0) {
				/*
				 * Override courtesyabort; opponent
				 * still has time. Check for lag.
				 */
				courtesyOK = 0;

				if (garray[g].game_state.onMove != myColor &&
				    garray[g].flag_pending != FLAG_CHECKING) {
					// Opponent may be lagging; let's ask.
					garray[g].flag_pending = FLAG_ABORT;
					garray[g].flag_check_time = time(0);

					pprintf(p, "Opponent has timeseal; "
					    "trying to courtesyabort.\n");
					pprintf(p1, "\n[G]\n");

					return COM_OK;
				}
			}
		}
#endif

		if (myGTime > 0 && yourGTime <= 0 && courtesyOK) {
			/*
			 * Player wants to abort + opponent is out of
			 * time = courtesyabort.
			 */

			pprintf(p, "Since you have time, and your opponent "
			    "has none, the game has been aborted.");
			pprintf(p1, "Your opponent has aborted the game "
			    "rather than calling your flag.");
			player_decline_offers(p, -1, -1);
			game_ended(g, myColor, END_COURTESY);
		} else {
			pprintf(p1, "\n");
			pprintf_highlight(p1, "%s", parray[p].name);
			pprintf(p1, " would like to abort the game; ");
			pprintf_prompt(p1, "type \"abort\" to accept.\n");
			pprintf(p, "Abort request sent.\n");
			player_add_request(p, p1, PEND_ABORT, 0);
		}
	}

	return COM_OK;
}

PUBLIC int
com_courtesyabort(int p, param_list param)
{
	pprintf(p, "Courtesyabort is obsolete; use \"abort\" instead.\n");
	return COM_OK;
}

PUBLIC int
com_courtesyadjourn(int p, param_list param)
{
	pprintf(p, "Use \"adjourn\" to courtesyadjourn a game.\n");
	return COM_OK;
}

PRIVATE int
player_has_mating_material(game_state_t *gs, int color)
{
	int	i, j;
	int	minor_pieces = 0;
	int	piece;

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			piece = gs->board[i][j];

			switch (piecetype(piece)) {
			case BISHOP:
			case KNIGHT:
				if (iscolor(piece, color))
					minor_pieces++;
				break;
			case KING:
			case NOPIECE:
				break;
			default:
				if (iscolor(piece, color))
					return 1;
			}
		}
	}

	return (minor_pieces > 1 ? 1 : 0);
}

PUBLIC int
com_flag(int p, param_list param)
{
	int	g;
	int	myColor;

	ASSERT(param[0].type == TYPE_NULL);

	if (!pIsPlaying(p))
		return COM_OK;

	g = parray[p].game;
	myColor = (p == garray[g].white ? WHITE : BLACK);

	if (garray[g].type == TYPE_UNTIMED) {
		pprintf(p, "You can't flag an untimed game.\n");
		return COM_OK;
	}

	if (garray[g].numHalfMoves < 2) {
		pprintf(p, "You cannot flag before both players have moved.\n"
		    "Use abort instead.\n");
		return COM_OK;
	}

	game_update_time(g);

#ifdef TIMESEAL
	{
		int	myTime, yourTime, serverTime;
		int	opp = parray[p].opponent;

		if (con[parray[p].socket].timeseal) {	// Does the caller use
							// timeseal?
			myTime = (myColor == WHITE ? garray[g].wRealTime :
			    garray[g].bRealTime);
		} else {
			myTime = (myColor == WHITE ? garray[g].wTime :
			    garray[g].bTime);
		}

		serverTime = (myColor == WHITE ? garray[g].bTime :
		    garray[g].wTime);

		if (con[parray[opp].socket].timeseal) { // Opp uses timeseal?
			yourTime = (myColor == WHITE ? garray[g].bRealTime :
			    garray[g].wRealTime);
		} else {
			yourTime = serverTime;
		}

		// The clocks to compare are now in 'myTime' and 'yourTime'.
		if (myTime <= 0 && yourTime <= 0) {
			player_decline_offers(p, -1, -1);
			game_ended(g, myColor, END_BOTHFLAG);
			return COM_OK;
		}

		if (yourTime > 0) {
			/*
			 * Opponent still has time, but if that's only
			 * because s/he may be lagging, we should ask
			 * for an acknowledgement and then try to call
			 * the flag.
			 */

			if (serverTime <= 0 &&
			    garray[g].game_state.onMove != myColor &&
			    garray[g].flag_pending != FLAG_CHECKING) {
				garray[g].flag_pending = FLAG_CALLED;
				garray[g].flag_check_time = time(0);

				pprintf(p, "Opponent has timeseal; "
				    "checking if (s)he's lagging.\n");
				pprintf(opp, "\n[G]\n");

				return COM_OK;
			}

			/*
			 * If we're here, it means:
			 * 1) The server agrees opponent has time, whether
			 *    lagging or not.
			 * 2) Opponent has timeseal (if yourTime != serverTime),
			 *    had time left after the last move (yourTime > 0),
			 *    and it's still your move.
			 * 3) We're currently checking a flag call after having
			 *    receiving acknowledgement from the other timeseal
			 *    (and would have reset 'yourTime' if the flag were
			 *    down).
			 */
			pprintf(p, "Your opponent is not out of time!\n");
			return COM_OK;
		}
	}
#else	// !defined(TIMESEAL)
	if (garray[g].wTime <= 0 && garray[g].bTime <= 0) {
		player_decline_offers(p, -1, -1);
		game_ended(g, myColor, END_BOTHFLAG);
		return COM_OK;
	}

	if (myColor == WHITE) {
		if (garray[g].bTime > 0) {
			pprintf(p, "Your opponent is not out of time!\n");
			return COM_OK;
		}
	} else {
		if (garray[g].wTime > 0) {
			pprintf(p, "Your opponent is not out of time!\n");
			return COM_OK;
		}
	}
#endif

	player_decline_offers(p, -1, -1);

	if (player_has_mating_material(&garray[g].game_state, myColor))
		game_ended(g, myColor, END_FLAG);
	else
		game_ended(g, myColor, END_FLAGNOMATERIAL);

	return COM_OK;
}

PUBLIC int
com_adjourn(int p, param_list param)
{
	int	p1, g, myColor, yourColor;

	ASSERT(param[0].type == TYPE_NULL);

	if (!pIsPlaying(p))
		return COM_OK;

	p1 = parray[p].opponent;
	g = parray[p].game;

	if (!(parray[p].registered && parray[p1].registered)) {
		pprintf(p, "Both players must be registered to adjorn a game. "
		    "Use \"abort\".\n");
		return COM_OK;
	}

	if (garray[g].link >= 0) {
		pprintf(p, "Bughouse games cannot be adjourned.\n");
		return COM_OK;
	}

	myColor		= (p == garray[g].white ? WHITE : BLACK);
	yourColor	= (myColor == WHITE ? BLACK : WHITE);

	if (player_find_pendfrom(p, p1, PEND_ADJOURN) >= 0) {
		player_remove_request(p1, p, PEND_ADJOURN);
		player_decline_offers(p, -1, -1);
		game_ended(parray[p].game, yourColor, END_ADJOURN);
	} else {
		game_update_time(g);

		if ((myColor == WHITE &&
		    garray[g].wTime > 0 &&
		    garray[g].bTime <= 0) ||
		    (myColor == BLACK &&
		    garray[g].bTime > 0 &&
		    garray[g].wTime <= 0)) {
			/*
			 * Player wants to adjourn + opponent is out
			 * of time = courtesyadjourn.
			 */

			pprintf(p, "Since you have time, and your opponent "
			    "has none, the game has been adjourned.");
			pprintf(p1, "Your opponent has adjourned the game "
			    "rather than calling your flag.");
			player_decline_offers(p, -1, -1);
			game_ended(g, myColor, END_COURTESYADJOURN);
		} else {
			pprintf(p1, "\n");
			pprintf_highlight(p1, "%s", parray[p].name);
			pprintf(p1, " would like to adjourn the game; ");
			pprintf_prompt(p1, "type \"adjourn\" to accept.\n");
			pprintf(p, "Adjourn request sent.\n");
			player_add_request(p, p1, PEND_ADJOURN, 0);
		}
	}

	return COM_OK;
}

PUBLIC int
com_takeback(int p, param_list param)
{
	int	from;
	int	g, i;
	int	nHalfMoves = 1;
	int	p1;

	if (!pIsPlaying(p))
		return COM_OK;

	p1 = parray[p].opponent;

	if (parray[p1].simul_info.numBoards &&
	    parray[p1].simul_info.boards[parray[p1].simul_info.onBoard] !=
	    parray[p].game) {
		pprintf(p, "You can only make requests when the simul player "
		    "is at your board.\n");
		return COM_OK;
	}

	g = parray[p].game;

	if (garray[g].link >= 0) {
		pprintf(p, "Takeback not implemented for bughouse games yet."
		    "\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_INT)
		nHalfMoves = param[0].val.integer;

	if ((from = player_find_pendfrom(p, parray[p].opponent, PEND_TAKEBACK))
	    >= 0) {
		player_remove_request(parray[p].opponent, p, PEND_TAKEBACK);

		if (parray[p].p_from_list[from].param1 == nHalfMoves) {
			// Doing the takeback
			player_decline_offers(p, -1, -PEND_SIMUL);

			for (i = 0; i < nHalfMoves; i++) {
				if (backup_move(g, REL_GAME) != MOVE_OK) {
					pprintf(garray[g].white, "Can only "
					    "backup %d moves\n", i);
					pprintf(garray[g].black, "Can only "
					    "backup %d moves\n", i);
					break;
				}
			}

#ifdef TIMESEAL
			garray[g].wTimeWhenReceivedMove = 0;
			garray[g].bTimeWhenReceivedMove = 0;
#endif

			send_boards(g);
		} else {
			if (garray[g].numHalfMoves < nHalfMoves) {
				pprintf(p, "There are only %d half moves in "
				    "your game.\n", garray[g].numHalfMoves);
				pprintf_prompt(parray[p].opponent, "\n%s has "
				    "declined the takeback request.\n",
				    parray[p].name);
				return COM_OK;
			}

			pprintf(p, "You disagree on the number of half-moves "
			    "to takeback.\n");
			pprintf(p, "Alternate takeback request sent.\n");
			pprintf_prompt(parray[p].opponent, "\n%s proposes a "
			    "different number (%d) of half-move(s).\n",
			    parray[p].name,
			    nHalfMoves);
			player_add_request(p, parray[p].opponent, PEND_TAKEBACK,
			    nHalfMoves);
		}
	} else {
		if (garray[g].numHalfMoves < nHalfMoves) {
			pprintf(p, "There are only %d half moves in your game."
			    "\n", garray[g].numHalfMoves);
			return COM_OK;
		}

		pprintf(parray[p].opponent, "\n");
		pprintf_highlight(parray[p].opponent, "%s", parray[p].name);
		pprintf_prompt(parray[p].opponent, " would like to take back "
		    "%d half move(s).\n", nHalfMoves);
		pprintf(p, "Takeback request sent.\n");
		player_add_request(p, parray[p].opponent, PEND_TAKEBACK,
		    nHalfMoves);
	}

	return COM_OK;
}

PUBLIC int
com_switch(int p, param_list param)
{
	char	*strTmp;
	int	 g = parray[p].game;
	int	 p1;
	int	 tmp, now;

	if (!pIsPlaying(p))
		return COM_OK;

	p1 = parray[p].opponent;

	if (parray[p1].simul_info.numBoards &&
	    parray[p1].simul_info.boards[parray[p1].simul_info.onBoard] != g) {
		pprintf(p, "You can only make requests when the simul player "
		    "is at your board.\n");
		return COM_OK;
	}

	if (garray[g].link >= 0) {
		pprintf(p, "Switch not implemented for bughouse games.\n");
		return COM_OK;
	}

	if (player_find_pendfrom(p, parray[p].opponent, PEND_SWITCH) >= 0) {
		player_remove_request(parray[p].opponent, p, PEND_SWITCH);
		player_decline_offers(p, -1, -PEND_SIMUL);

		tmp = garray[g].white;
		garray[g].white = garray[g].black;
		garray[g].black = tmp;
		parray[p].side = (parray[p].side == WHITE ? BLACK : WHITE);

		strTmp = xstrdup(garray[g].white_name);
		strcpy(garray[g].white_name, garray[g].black_name);
		strcpy(garray[g].black_name, strTmp);
		strfree(strTmp);

		parray[parray[p].opponent].side =
		    (parray[parray[p].opponent].side == WHITE ? BLACK : WHITE);

		// Roll back the time
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

	if (garray[g].rated && garray[g].numHalfMoves > 0) {
		pprintf(p, "You cannot switch sides once a rated game is "
		    "underway.\n");
		return COM_OK;
	}

	pprintf(parray[p].opponent, "\n");
	pprintf_highlight(parray[p].opponent, "%s", parray[p].name);
	pprintf_prompt(parray[p].opponent, " would like to switch sides.\n"
	    "Type \"accept\" to switch sides, or \"decline\" to refuse.\n");
	pprintf(p, "Switch request sent.\n");
	player_add_request(p, parray[p].opponent, PEND_SWITCH, 0);

	return COM_OK;
}

PUBLIC int
com_time(int p, param_list param)
{
	int	p1, g;

	if (param[0].type == TYPE_NULL) {
		g = parray[p].game;

		if (!pIsPlaying(p))
			return COM_OK;
	} else {
		if ((g = GameNumFromParam(p, &p1, &param[0])) < 0)
			return COM_OK;
	}

	if (g < 0 || g >= g_num || garray[g].status != GAME_ACTIVE) {
		pprintf(p, "There is no such game.\n");
		return COM_OK;
	}

	game_update_time(g);

	pprintf(p, "White (%s) : %d mins, %d secs\n",
		parray[garray[g].white].name,
		garray[g].wTime / 600,
		(garray[g].wTime - ((garray[g].wTime / 600) * 600)) / 10);
	pprintf(p, "Black (%s) : %d mins, %d secs\n",
		parray[garray[g].black].name,
		garray[g].bTime / 600,
		(garray[g].bTime - ((garray[g].bTime / 600) * 600)) / 10);

	return COM_OK;
}

PUBLIC int
com_boards(int p, param_list param)
{
	DIR		*dirp;
	char		*category = NULL;
	char		 dname[MAX_FILENAME_SIZE] = { '\0' };
#ifdef USE_DIRENT
	struct dirent	*dp;
#else
	struct direct	*dp;
#endif

	if (param[0].type == TYPE_WORD)
		category = param[0].val.word;

	if (category) {
		pprintf(p, "Boards Available For Category %s:\n", category);
		snprintf(dname, sizeof dname, "%s/%s", board_dir, category);
	} else {
		pprintf(p, "Categories Available:\n");
		snprintf(dname, sizeof dname, "%s", board_dir);
	}

	if ((dirp = opendir(dname)) == NULL) {
		pprintf(p, "No such category %s, try \"boards\".\n", category);
		return COM_OK;
	}

	// YUK! What a mess, how about printing an ordered directory? - DAV
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if (!strcmp(dp->d_name, "."))
			continue;
		if (!strcmp(dp->d_name, ".."))
			continue;
		pprintf(p, "%s\n", dp->d_name);
	}

	closedir(dirp);
	return COM_OK;
}

PUBLIC int
com_simmatch(int p, param_list param)
{
	char	tmp[100] = { '\0' };
	int	num;
	int	p1, g, adjourned;

	if (parray[p].game >= 0 && garray[parray[p].game].status ==
	    GAME_EXAMINE) {
		pprintf(p, "You are still examining a game.\n");
		return COM_OK;
	}

	p1 = player_find_part_login(param[0].val.word);

	if (p1 < 0) {
		pprintf(p, "No user named \"%s\" is logged in.\n",
		    param[0].val.word);
		return COM_OK;
	}

	if (p == p1) {
		pprintf(p, "You can't simmatch yourself!\n");
		return COM_OK;
	}

	if (player_find_pendfrom(p, p1, PEND_SIMUL) >= 0) {
		player_remove_request(p, p1, PEND_MATCH);
		player_remove_request(p1, p, PEND_MATCH);
		player_remove_request(p, p1, PEND_SIMUL);
		player_remove_request(p1, p, PEND_SIMUL);
		player_withdraw_offers(p, -1, PEND_SIMUL);
		player_decline_offers(p1, -1, PEND_SIMUL);
		player_withdraw_offers(p1, -1, PEND_SIMUL);
		player_decline_offers(p, -1, PEND_MATCH);
		player_withdraw_offers(p, -1, PEND_MATCH);
		player_decline_offers(p1, -1, PEND_MATCH);
		player_withdraw_offers(p1, -1, PEND_MATCH);
		player_decline_offers(p, -1, PEND_PARTNER);
		player_withdraw_offers(p, -1, PEND_PARTNER);
		player_decline_offers(p1, -1, PEND_PARTNER);
		player_withdraw_offers(p1, -1, PEND_PARTNER);

		if (parray[p].simul_info.numBoards >= MAX_SIMUL) {
			pprintf(p, "You are already playing the maximum of %d "
			    "boards.\n", MAX_SIMUL);
			pprintf(p1, "Simul request removed, boards filled.\n");
			return COM_OK;
		}

		// stop observing when match starts
		unobserveAll(p);
		unobserveAll(p1);

		g = game_new();
		adjourned = 0;

		if (game_read(g, p, p1) >= 0)
			adjourned = 1;

		if (!adjourned) { // no adjourned game - so begin a new game.
			game_remove(g);

			if (create_new_match(p, p1, 0, 0, 0, 0, 0, "standard",
			    "standard", 1)) {
				pprintf(p, "There was a problem creating the "
				    "new match.\n");
				pprintf_prompt(p1, "There was a problem "
				    "creating the new match.\n");
				return COM_OK;
			}
		} else { // resume adjourned game
			game_delete(p, p1);

			snprintf(tmp, sizeof tmp, "{Game %d (%s vs. %s) "
			    "Continuing %s %s simul.}\n",
			    (g + 1),
			    parray[p].name,
			    parray[p1].name,
			    rstr[garray[g].rated],
			    bstr[garray[g].type]);

			pprintf(p, tmp);
			pprintf(p1, tmp);

			garray[g].white = p;
			garray[g].black = p1;
			garray[g].status = GAME_ACTIVE;
			garray[g].startTime = tenth_secs();
			garray[g].lastMoveTime = garray[g].startTime;
			garray[g].lastDecTime = garray[g].startTime;
			parray[p].game = g;
			parray[p].opponent = p1;
			parray[p].side = WHITE;
			parray[p1].game = g;
			parray[p1].opponent = p;
			parray[p1].side = BLACK;

			send_boards(g);
		}

		num = parray[p].simul_info.numBoards;

		parray[p].simul_info.results[num] = -1;
		parray[p].simul_info.boards[num] = parray[p].game;
		parray[p].simul_info.numBoards++;

		if (parray[p].simul_info.numBoards > 1 &&
		    parray[p].simul_info.onBoard >= 0)
			player_goto_board(p, parray[p].simul_info.onBoard);
		else
			parray[p].simul_info.onBoard = 0;
		return COM_OK;
	}

	if (player_find_pendfrom(p, -1, PEND_SIMUL) >= 0) {
		pprintf(p, "You cannot be the simul giver and request to join "
		    "another simul.\nThat would just be too confusing for me "
		    "and you.\n");
		return COM_OK;
	}

	if (parray[p].simul_info.numBoards) {
		pprintf(p, "You cannot be the simul giver and request to join "
		    "another simul.\nThat would just be too confusing for me "
		    "and you.\n");
		return COM_OK;
	}

	if (parray[p].game >= 0) {
		pprintf(p, "You are already playing a game.\n");
		return COM_OK;
	}

	if (!parray[p1].sopen) {
		pprintf_highlight(p, "%s", parray[p1].name);
		pprintf(p, " is not open to receiving simul requests.\n");
		return COM_OK;
	}

	if (parray[p1].simul_info.numBoards >= MAX_SIMUL) {
		pprintf_highlight(p, "%s", parray[p1].name);
		pprintf(p, " is already playing the maximum of %d boards.\n",
		    MAX_SIMUL);
		return COM_OK;
	}

	// loon: checking for some crazy situations we can't allow :)
	if (parray[p1].game >= 0 && parray[p1].simul_info.numBoards == 0) {
		pprintf_highlight(p, "%s", parray[p1].name);

		if (parray[garray[parray[p1].game].white].simul_info.numBoards) {
			pprintf(p, " is playing in ");
			pprintf_highlight(p, "%s",
			    parray[parray[p1].opponent].name);
			pprintf(p, "'s simul, and can't accept.\n");
		} else {
			pprintf(p, " can't begin a simul while playing a "
			    "non-simul game.\n");
		}

		return COM_OK;
	}

	g = game_new();
	adjourned = ((game_read(g, p, p1) < 0 && game_read(g, p1, p) < 0)
	    ? 0 : 1);

	if (adjourned) {
		if (!(garray[g].type == TYPE_UNTIMED))
			adjourned = 0;
	}

	game_remove(g);

	if (player_add_request(p, p1, PEND_SIMUL, 0)) {
		pprintf(p, "Maximum number of pending actions reached. "
		    "Your request was not sent.\nTry again later.\n");
		return COM_OK;
	} else {

		pprintf(p1, "\n");
		pprintf_highlight(p1, "%s", parray[p].name);

		if (adjourned) {
			pprintf_prompt(p1, " requests to continue an "
			    "adjourned simul game.\n");
			pprintf(p, "Request to resume simul sent. "
			    "Adjourned game found.\n");
		} else {
			pprintf_prompt(p1, " requests to join a simul match "
			    "with you.\n");
			pprintf(p, "Simul match request sent.\n");
		}
	}

	return COM_OK;
}

PUBLIC int
com_goboard(int p, param_list param)
{
	int on, g, p1;

	if (!parray[p].simul_info.numBoards) {
		pprintf(p, "You are not giving a simul.\n");
		return COM_OK;
	}

	p1 = player_find_part_login(param[0].val.word);

	if (p1 < 0) {
		pprintf(p, "No user named \"%s\" is logged in.\n",
		    param[0].val.word);
		return COM_OK;
	}

	if (p == p1) {
		pprintf(p, "You can't goboard yourself!\n");
		return COM_OK;
	}

	on = parray[p].simul_info.onBoard;
	g = parray[p].simul_info.boards[on];

	if (p1 == garray[g].black) {
		pprintf(p, "You are already at that board!\n");
		return COM_OK;
	}

	if (parray[p].simul_info.numBoards > 1) {
		player_decline_offers(p, -1, -PEND_SIMUL);

		if (player_goto_simulgame_bynum(p, parray[p1].game) != -1) {
			if (g >= 0) {
				pprintf(garray[g].black, "\n");
				pprintf_highlight(garray[g].black, "%s",
				    parray[p].name);
				pprintf_prompt(garray[g].black, " has moved "
				    "away from your board.\n");
			}
		}
	} else
		pprintf(p, "You are only playing one board!\n");
	return COM_OK;
}

PUBLIC int
com_gonum(int p, param_list param)
{
	int on, g, gamenum;

	if (!parray[p].simul_info.numBoards) {
		pprintf(p, "You are not giving a simul.\n");
		return COM_OK;
	}

	on = parray[p].simul_info.onBoard;
	g = parray[p].simul_info.boards[on];
	gamenum = param[0].val.integer - 1;

	if (gamenum < 0)
		gamenum = 0;

	if (on == gamenum) {
		pprintf(p, "You are already at that board!\n");
		return COM_OK;
	}

	if (parray[p].simul_info.numBoards > 1) {
		player_decline_offers(p, -1, -PEND_SIMUL);

		if (player_goto_simulgame_bynum(p, gamenum) != -1) {
			if (g >= 0) {
				pprintf(garray[g].black, "\n");
				pprintf_highlight(garray[g].black, "%s",
				    parray[p].name);
				pprintf_prompt(garray[g].black, " has moved "
				    "away from your board.\n");
			}
		}
	} else
		pprintf(p, "You are only playing one board!\n");
	return COM_OK;
}

PUBLIC int
com_simnext(int p, param_list param)
{
	int on, g;

	if (!parray[p].simul_info.numBoards) {
		pprintf(p, "You are not giving a simul.\n");
		return COM_OK;
	}

	if (parray[p].simul_info.numBoards > 1) {
		player_decline_offers(p, -1, -PEND_SIMUL);
		on = parray[p].simul_info.onBoard;
		g = parray[p].simul_info.boards[on];

		if (g >= 0) {
			pprintf(garray[g].black, "\n");
			pprintf_highlight(garray[g].black, "%s",
			    parray[p].name);
			pprintf_prompt(garray[g].black, " is moving away from "
			    "your board.\n");
			player_goto_next_board(p);
		}
	} else
		pprintf(p, "You are only playing one board!\n");
	return COM_OK;
}

PUBLIC int
com_simprev(int p, param_list param)
{
	int on, g;

	if (!parray[p].simul_info.numBoards) {
		pprintf(p, "You are not giving a simul.\n");
		return COM_OK;
	}

	if (parray[p].simul_info.numBoards > 1) {
		player_decline_offers(p, -1, -PEND_SIMUL);
		on = parray[p].simul_info.onBoard;
		g = parray[p].simul_info.boards[on];

		if (g >= 0) {
			pprintf(garray[g].black, "\n");
			pprintf_highlight(garray[g].black, "%s",
			    parray[p].name);
			pprintf_prompt(garray[g].black, " is moving back to "
			    "the previous board.\n");
		}

		player_goto_prev_board(p);
	} else
		pprintf(p, "You are only playing one board!\n");
	return COM_OK;
}

PUBLIC int
com_simgames(int p, param_list param)
{
	int p1 = p;

	if (param[0].type == TYPE_WORD) {
		if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
			pprintf(p, "No player named %s is logged in.\n",
			    param[0].val.word);
			return COM_OK;
		}
	}

	if (p1 == p) {
		pprintf(p, "You are playing %d simultaneous games.\n",
		    player_num_active_boards(p1));
	} else {
		pprintf(p, "%s is playing %d simultaneous games.\n",
		    parray[p1].name, player_num_active_boards(p1));
	}

	return COM_OK;
}

PUBLIC int
com_simpass(int p, param_list param)
{
	int	g, p1, on;

	if (!pIsPlaying(p))
		return COM_OK;

	g	= parray[p].game;
	p1	= garray[g].white;

	if (!parray[p1].simul_info.numBoards) {
		pprintf(p, "You are not participating in a simul.\n");
		return COM_OK;
	}

	if (p == p1) {
		pprintf(p, "You are the simul holder and cannot pass!\n");
		return COM_OK;
	}

	if (player_num_active_boards(p1) == 1) {
		pprintf(p, "This is the only game, so passing is futile.\n");
		return COM_OK;
	}

	on = parray[p1].simul_info.onBoard;

	if (parray[p1].simul_info.boards[on] != g) {
		pprintf(p, "You cannot pass until the simul holder arrives!\n");
		return COM_OK;
	}

	if (garray[g].passes >= MAX_SIMPASS) {
		if (parray[p].bell)
			pprintf(p, "\a");
		pprintf(p, "You have reached your maximum of %d pass(es).\n",
		    MAX_SIMPASS);
		pprintf(p, "Please move IMMEDIATELY!\n");
		pprintf_highlight(p1, "%s", parray[p].name);
		pprintf_prompt(p1, " tried to pass, but is out of passes.\n");
		return COM_OK;
	}

	player_decline_offers(p, -1, -PEND_SIMUL);
	garray[g].passes++;

	pprintf(p, "You have passed and have %d pass(es) left.\n",
	    (MAX_SIMPASS - garray[g].passes));
	pprintf_highlight(p1, "%s", parray[p].name);
	pprintf_prompt(p1, " has decided to pass and has %d pass(es) left.\n",
	    (MAX_SIMPASS - garray[g].passes));
	player_goto_next_board(p1);

	return COM_OK;
}

PUBLIC int
com_simabort(int p, param_list param)
{
	if (!parray[p].simul_info.numBoards) {
		pprintf(p, "You are not giving a simul.\n");
		return COM_OK;
	}

	player_decline_offers(p, -1, -PEND_SIMUL);
	game_ended(parray[p].simul_info.boards[parray[p].simul_info.onBoard],
	    WHITE, END_ABORT);

	return COM_OK;
}

PUBLIC int
com_simallabort(int p, param_list param)
{
	if (!parray[p].simul_info.numBoards) {
		pprintf(p, "You are not giving a simul.\n");
		return COM_OK;
	}

	player_decline_offers(p, -1, -PEND_SIMUL);

	for (int i = 0; i < parray[p].simul_info.numBoards; i++) {
		if (parray[p].simul_info.boards[i] >= 0) {
			game_ended(parray[p].simul_info.boards[i], WHITE,
			    END_ABORT);
		}
	}

	return COM_OK;
}

PUBLIC int
com_simadjourn(int p, param_list param)
{
	if (!parray[p].simul_info.numBoards) {
		pprintf(p, "You are not giving a simul.\n");
		return COM_OK;
	}

	player_decline_offers(p, -1, -PEND_SIMUL);
	game_ended(parray[p].simul_info.boards[parray[p].simul_info.onBoard],
	    WHITE, END_ADJOURN);

	return COM_OK;
}

PUBLIC int
com_simalladjourn(int p, param_list param)
{
	if (!parray[p].simul_info.numBoards) {
		pprintf(p, "You are not giving a simul.\n");
		return COM_OK;
	}

	player_decline_offers(p, -1, -PEND_SIMUL);

	for (int i = 0; i < parray[p].simul_info.numBoards; i++) {
		if (parray[p].simul_info.boards[i] >= 0) {
			game_ended(parray[p].simul_info.boards[i], WHITE,
			    END_ADJOURN);
		}
	}

	return COM_OK;
}

PUBLIC int
com_moretime(int p, param_list param)
{
	int	g, increment;

	ASSERT(param[0].type == TYPE_INT);

	if (parray[p].game >= 0 && garray[parray[p].game].status ==
	    GAME_EXAMINE) {
		pprintf(p, "You cannot use moretime in an examined game.\n");
		return COM_OK;
	}

	if ((increment = param[0].val.integer) <= 0) {
		pprintf(p, "Moretime requires an integer value greater than "
		    "zero.\n");
		return COM_OK;
	}

	if (!pIsPlaying(p))
		return COM_OK;

	if (increment > 600) {
		pprintf(p, "Moretime has a maximum limit of 600 seconds.\n");
		increment = 600;
	}

	g = parray[p].game;

	if (garray[g].white == p) {
		garray[g].bTime += increment * 10;
#ifdef TIMESEAL
		garray[g].bRealTime += increment * 10 * 100;
#endif
		pprintf(p, "%d seconds were added to your opponents clock\n",
		    increment);
		pprintf_prompt(parray[p].opponent, "\nYour opponent has "
		    "added %d seconds to your clock.\n", increment);
	}

	if (garray[g].black == p) {
		garray[g].wTime += increment * 10;;
#ifdef TIMESEAL
		garray[g].wRealTime += increment * 10 * 100;
#endif
		pprintf(p, "%d seconds were added to your opponents clock\n",
		    increment);
		pprintf_prompt(parray[p].opponent, "\nYour opponent has "
		    "added %d seconds to your clock.\n", increment);
	}

	return COM_OK;
}

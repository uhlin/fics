/* playerdb.c
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
   Markus Uhlin                 23/12/13	Fixed compiler warnings
   Markus Uhlin                 23/12/17	Usage of 'time_t'
   Markus Uhlin                 23/12/19	Usage of 'time_t'
   Markus Uhlin                 23/12/25	Reformatted functions
*/

#include "stdinclude.h"
#include "common.h"

#include "command.h"
#include "comproc.h"
#include "config.h"
#include "ficsmain.h"
#include "gamedb.h"
#include "lists.h"
#include "network.h"
#include "playerdb.h"
#include "ratings.h"
#include "rmalloc.h"
#include "talkproc.h"
#include "utils.h"

PUBLIC player	 parray[PARRAY_SIZE];
PUBLIC int	 p_num = 0;

PRIVATE int get_empty_slot(void)
{
  int i;

  for (i = 0; i < p_num; i++) {
    if (parray[i].status == PLAYER_EMPTY) {
/***  fprintf(stderr,"New player put in parray[%d/%d]\n", i, p_num-1);*/
      return i;
    }
  }

  p_num++;

  if (p_num+1 >= PARRAY_SIZE) {
    fprintf(stderr, "*** Bogus attempt to get_empty_slot() past end of parray ***\n");
  }

/*** fprintf(stderr,"New player added in parray[%d]\n", p_num-1); */
  parray[p_num - 1].status = PLAYER_EMPTY;
  return p_num - 1;
}

PUBLIC void player_array_init()
{
  int i;

  for (i = 0; i < PARRAY_SIZE; i++)
    parray[i].status = PLAYER_EMPTY;
}

PUBLIC void player_init(int startConsole)
{
  int p;

  if (startConsole) {
    net_addConnection(0, 0);
    p = player_new();
    parray[p].login = xstrdup("console");
    parray[p].name = xstrdup("console");
    parray[p].passwd = xstrdup("*");
    parray[p].fullName = xstrdup("The Operator");
    parray[p].emailAddress = NULL;
    parray[p].prompt = xstrdup("fics%");
    parray[p].adminLevel = ADMIN_GOD;
    parray[p].socket = 0;
    parray[p].busy[0] = '\0';
    pprintf_prompt(p, "\nLogged in on console.\n");
  }
}

PUBLIC int player_new()
{
  int new;

  new = get_empty_slot();
  player_zero(new);
  return new;
}

#define INVALID ((char *) -42)

PUBLIC int player_zero(int p)
{
  int i;

  parray[p].name = NULL;
  parray[p].login = NULL;
  parray[p].fullName = NULL;
  parray[p].emailAddress = NULL;
  parray[p].prompt = def_prompt;
  parray[p].partner = -1;
  parray[p].passwd = NULL;
  parray[p].socket = -1;
  parray[p].registered = 0;
  parray[p].status = PLAYER_NEW;
  parray[p].s_stats.num = 0;
  parray[p].s_stats.win = 0;
  parray[p].s_stats.los = 0;
  parray[p].s_stats.dra = 0;
  parray[p].s_stats.rating = 0;
  parray[p].s_stats.sterr = 350.0;
  parray[p].s_stats.ltime = 0;
  parray[p].s_stats.best = 0;
  parray[p].s_stats.whenbest = 0;
  parray[p].b_stats.num = 0;
  parray[p].b_stats.win = 0;
  parray[p].b_stats.los = 0;
  parray[p].b_stats.dra = 0;
  parray[p].b_stats.rating = 0;
  parray[p].b_stats.sterr = 350.0;
  parray[p].b_stats.ltime = 0;
  parray[p].b_stats.best = 0;
  parray[p].b_stats.whenbest = 0;
  parray[p].w_stats.num = 0;
  parray[p].w_stats.win = 0;
  parray[p].w_stats.los = 0;
  parray[p].w_stats.dra = 0;
  parray[p].w_stats.rating = 0;
  parray[p].w_stats.sterr = 350.0;
  parray[p].w_stats.ltime = 0;
  parray[p].w_stats.best = 0;
  parray[p].w_stats.whenbest = 0;
  parray[p].l_stats.num = 0;
  parray[p].l_stats.win = 0;
  parray[p].l_stats.los = 0;
  parray[p].l_stats.dra = 0;
  parray[p].l_stats.rating = 0;
  parray[p].l_stats.sterr = 350.0;
  parray[p].l_stats.ltime = 0;
  parray[p].l_stats.best = 0;
  parray[p].l_stats.whenbest = 0;
  parray[p].bug_stats.num = 0;
  parray[p].bug_stats.win = 0;
  parray[p].bug_stats.los = 0;
  parray[p].bug_stats.dra = 0;
  parray[p].bug_stats.rating = 0;
  parray[p].bug_stats.sterr = 350.0;
  parray[p].bug_stats.ltime = 0;
  parray[p].bug_stats.best = 0;
  parray[p].bug_stats.whenbest = 0;
  parray[p].d_time = 2;
  parray[p].d_inc = 12;
  parray[p].d_height = 24;
  parray[p].d_width = 79;
  parray[p].language = LANG_DEFAULT;
  parray[p].last_file = NULL;
  parray[p].last_file_byte = 0L;
  parray[p].open = 1;
  parray[p].rated = 0;
  parray[p].ropen = 1;
  parray[p].bell = 0;
  parray[p].timeOfReg = 0;
  parray[p].totalTime = 0;
  parray[p].pgn = 0;
  parray[p].notifiedby = 0;
  parray[p].i_login = 0;
  parray[p].i_game = 0;
  parray[p].i_shout = 1;
  parray[p].i_cshout = 1;
  parray[p].i_tell = 1;
  parray[p].i_kibitz = 1;
  parray[p].kiblevel = 0;
  parray[p].private = 0;
  parray[p].jprivate = 0;
  parray[p].automail = 0;
  parray[p].i_mailmess = 0;
  parray[p].style = 0;
  parray[p].promote = QUEEN;
  parray[p].game = -1;
  parray[p].last_tell = -1;
  parray[p].last_channel = -1;
  parray[p].logon_time = 0;
  parray[p].last_command_time = 0;
  parray[p].num_from = 0;
  parray[p].num_to = 0;
  parray[p].adminLevel = 0;
  parray[p].i_admin = 1;
/*  parray[p].computer = 0; */
  parray[p].num_plan = 0;
  for (i = 0; i < MAX_PLAN; i++)
    parray[p].planLines[i] = INVALID;
  parray[p].num_formula = 0;
  for (i = 0; i < MAX_FORMULA; i++)
    parray[p].formulaLines[i] = NULL;
  parray[p].formula = NULL;
/*  parray[p].nochannels = 0; */
  parray[p].num_white = 0;
  parray[p].num_black = 0;
  parray[p].num_observe = 0;
/*  parray[p].uscfRating = 0; */
/*  parray[p].network_player = 0; */
  parray[p].thisHost = 0;
  parray[p].lastHost = 0;
  parray[p].lastColor = WHITE;
  parray[p].numAlias = 0;
  for (i = 0; i < MAX_ALIASES; i++) {
    parray[p].alias_list[i].comm_name = INVALID;
    parray[p].alias_list[i].alias = INVALID;
  }
  parray[p].opponent = -1;
  parray[p].last_opponent = -1;
  parray[p].highlight = 0;
/*  parray[p].query_log = NULL;  */
  parray[p].lastshout_a = 0;
  parray[p].lastshout_b = 0;
  parray[p].sopen = 0;
  parray[p].simul_info.numBoards = 0;
  parray[p].num_comments = 0;
  parray[p].flip = 0;
  parray[p].lists = NULL;
  return 0;
}

PUBLIC int player_free(int p)
{
  int i;

  strfree(parray[p].login);
  strfree(parray[p].name);
  strfree(parray[p].passwd);
  strfree(parray[p].fullName);
  strfree(parray[p].emailAddress);
  if (parray[p].prompt != def_prompt)
    strfree(parray[p].prompt);
/*  strfree(parray[p].partner); */
  for (i = 0; i < parray[p].num_plan; i++)
    strfree(parray[p].planLines[i]);
  for (i = 0; i < parray[p].num_formula; i++)
    strfree(parray[p].formulaLines[i]);
  strfree(parray[p].formula);
  list_free(parray[p].lists);
  for (i = 0; i < parray[p].numAlias; i++) {
    strfree(parray[p].alias_list[i].comm_name);
    strfree(parray[p].alias_list[i].alias);
  }
/*
  if (parray[p].query_log != NULL)
    tl_free(parray[p].query_log);
*/
  return 0;
}

PUBLIC int player_clear(int p)
{
  player_free(p);
  player_zero(p);
  return 0;
}

PUBLIC int player_remove(int p)
{
  int i;

  player_decline_offers(p, -1, -1);
  player_withdraw_offers(p, -1, -1);
  if (parray[p].simul_info.numBoards) {	/* Player disconnected in middle of
					   simul */
    for (i = 0; i < parray[p].simul_info.numBoards; i++) {
      if (parray[p].simul_info.boards[i] >= 0) {
	game_disconnect(parray[p].simul_info.boards[i], p);
      }
    }
  }

  if (parray[p].game >=0) {  /* Player disconnected in the middle of a
				       game! */
    pprintf(parray[p].opponent, "Your opponent has lost contact or quit.");
    game_disconnect(parray[p].game, p);
  }
/*  ReallyRemoveOldGamesForPlayer(p); */
  for (i = 0; i < p_num; i++) {
    if (parray[i].status == PLAYER_EMPTY)
      continue;
    if (parray[i].last_tell == p)
      parray[i].last_tell = -1;
    if (parray[i].last_opponent == p)
      parray[i].last_opponent = -1;
    if (parray[i].partner == p) {
      pprintf_prompt (i, "Your partner has disconnected.\n");
      player_withdraw_offers(i, -1, PEND_BUGHOUSE);
      player_decline_offers(i, -1, PEND_BUGHOUSE);
      parray[i].partner = -1;
    }
  }
  player_clear(p);
  parray[p].status = PLAYER_EMPTY;
/***  fprintf(stderr, "Removed parray[%d/%d]\n", p, p_num-1);*/
  return 0;
}

void
ReadV1PlayerFmt(int p, player *pp, FILE *fp, char *file, int version)
{
	char	*tmp;
	char	 tmp2[MAX_STRING_LENGTH];
	int	 bs, ss, ws, ls, bugs;
	int	 i, size_cens, size_noplay, size_not, size_gnot, size_chan, len;

	/*
	 * Name
	 */
	fgets(tmp2, sizeof tmp2, fp);
	if (strcmp(tmp2, "NONE\n")) {
		tmp2[strlen(tmp2) - 1] = '\0';
		pp->name = xstrdup(tmp2);
	} else {
		pp->name = NULL;
	}

	/*
	 * Full name
	 */
	fgets(tmp2, sizeof tmp2, fp);
	if (strcmp(tmp2, "NONE\n")) {
		tmp2[strlen(tmp2) - 1] = '\0';
		pp->fullName = xstrdup(tmp2);
	} else {
		pp->fullName = NULL;
	}

	/*
	 * Password
	 */
	fgets(tmp2, sizeof tmp2, fp);
	if (strcmp(tmp2, "NONE\n")) {
		tmp2[strlen(tmp2) - 1] = '\0';
		pp->passwd = xstrdup(tmp2);
	} else {
		pp->passwd = NULL;
	}

	/*
	 * Email
	 */
	fgets(tmp2, sizeof tmp2, fp);
	if (strcmp(tmp2, "NONE\n")) {
		tmp2[strlen(tmp2) - 1] = '\0';
		pp->emailAddress = xstrdup(tmp2);
	} else {
		pp->emailAddress = NULL;
	}

	if (fscanf(fp, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u "
	    "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u "
	    "%u %u %u %u %u %d\n",
	    &pp->s_stats.num, &pp->s_stats.win, &pp->s_stats.los,
	    &pp->s_stats.dra, &pp->s_stats.rating, &ss,
	    &pp->s_stats.ltime, &pp->s_stats.best, &pp->s_stats.whenbest,

	    &pp->b_stats.num, &pp->b_stats.win, &pp->b_stats.los,
	    &pp->b_stats.dra, &pp->b_stats.rating, &bs,
	    &pp->b_stats.ltime, &pp->b_stats.best, &pp->b_stats.whenbest,

	    &pp->w_stats.num, &pp->w_stats.win, &pp->w_stats.los,
	    &pp->w_stats.dra, &pp->w_stats.rating, &ws,
	    &pp->w_stats.ltime, &pp->w_stats.best, &pp->w_stats.whenbest,

	    &pp->l_stats.num, &pp->l_stats.win, &pp->l_stats.los,
	    &pp->l_stats.dra, &pp->l_stats.rating, &ls,
	    &pp->l_stats.ltime, &pp->l_stats.best, &pp->l_stats.whenbest,

	    &pp->bug_stats.num, &pp->bug_stats.win, &pp->bug_stats.los,
	    &pp->bug_stats.dra, &pp->bug_stats.rating, &bugs,
	    &pp->bug_stats.ltime, &pp->bug_stats.best, &pp->bug_stats.whenbest,

	    &pp->lastHost) != 46) {
		fprintf(stderr, "Player %s is corrupt\n", parray[p].name);
		return;
	}

	pp->b_stats.sterr	= (bs / 10.0);
	pp->s_stats.sterr	= (ss / 10.0);
	pp->w_stats.sterr	= (ws / 10.0);
	pp->l_stats.sterr	= (ls / 10.0);
	pp->bug_stats.sterr	= (bugs / 10.0);

	fgets(tmp2, sizeof tmp2, fp);
	tmp2[strlen(tmp2) - 1] = '\0';
	pp->prompt = xstrdup(tmp2);

	if (fscanf(fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
	    "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
	    &pp->open, &pp->rated, &pp->ropen, &pp->timeOfReg, &pp->totalTime,
	    &pp->bell, &pp->pgn, &pp->notifiedby, &pp->i_login, &pp->i_game,
	    &pp->i_shout, &pp->i_cshout, &pp->i_tell, &pp->i_kibitz,
	    &pp->private, &pp->jprivate, &pp->automail, &pp->i_mailmess,
	    &pp->style, &pp->d_time, &pp->d_inc, &pp->d_height, &pp->d_width,
	    &pp->language, &pp->adminLevel, &pp->num_white, &pp->num_black,
	    &pp->highlight, &pp->num_comments, &pp->num_plan, &pp->num_formula,
	    &size_cens, &size_not, &size_noplay, &size_gnot, &pp->numAlias,
	    &size_chan) != 37) {
		fprintf(stderr, "Player %s is corrupt\n", parray[p].name);
		return;
	}

	if (pp->num_plan > 0) {
		for (i = 0; i < pp->num_plan; i++) {
			fgets(tmp2, sizeof tmp2, fp);

			if (!(len = strlen(tmp2))) {
				fprintf(stderr, "FICS: Error bad plan in "
				    "file %s\n", file);
				i--;
				pp->num_plan--;
			} else {
				tmp2[len - 1] = '\0'; // Get rid of '\n'.

				pp->planLines[i] = (len > 1 ? xstrdup(tmp2) :
				    NULL);
			}
		}
	}

	if (pp->num_formula > 0) {
		for (i = 0; i < pp->num_formula; i++) {
			fgets(tmp2, sizeof tmp2, fp);

			if (!(len = strlen(tmp2))) {
				fprintf(stderr, "FICS: Error bad formula in "
				    "file %s\n", file);
				i--;
				pp->num_formula--;
			} else {
				tmp2[len - 1] = '\0'; // Get rid of '\n'.

				pp->formulaLines[i] = (len > 1 ? xstrdup(tmp2) :
				    NULL);
			}
		}
	}

	fgets(tmp2, sizeof tmp2, fp);
	tmp2[strlen(tmp2) - 1] = '\0';

	if (!strcmp(tmp2, "NONE"))
		pp->formula = NULL;
	else
		pp->formula = xstrdup(tmp2);

	if (pp->numAlias > 0) {
		for (i = 0; i < pp->numAlias; i++) {
			fgets(tmp2, sizeof tmp2, fp);

			if (!(len = strlen(tmp2))) {
				fprintf(stderr, "FICS: Error bad alias in "
				    "file %s\n", file);
				i--;
				pp->numAlias--;
			} else {
				tmp2[len - 1] = '\0'; // Get rid of '\n'.
				tmp = tmp2;
				tmp = eatword(tmp2);
				*tmp = '\0';
				tmp++;
				tmp = eatwhite(tmp);

				pp->alias_list[i].comm_name = xstrdup(tmp2);
				pp->alias_list[i].alias = xstrdup(tmp);
			}
		}
	}

	while (size_cens--) {
		fscanf(fp, "%s", tmp2);
		list_add(p, L_CENSOR, tmp2);
	}
	while (size_not--) {
		fscanf(fp, "%s", tmp2);
		list_add(p, L_NOTIFY, tmp2);
	}
	while (size_noplay--) {
		fscanf(fp, "%s", tmp2);
		list_add(p, L_NOPLAY, tmp2);
	}
	while (size_gnot--) {
		fscanf(fp, "%s", tmp2);
		list_add(p, L_GNOTIFY, tmp2);
	}
	while (size_chan--) {
		fscanf(fp, "%s", tmp2);
		list_add(p, L_CHANNEL, tmp2);
	}
}

PRIVATE int got_attr_value_player(int p, char *attr, char *value, FILE * fp, char *file)
{
  int i, len;
  char tmp[MAX_LINE_SIZE], *tmp1;

  if (!strcmp(attr, "name:")) {
    parray[p].name = xstrdup(value);
  } else if (!strcmp(attr, "password:")) {
    parray[p].passwd = xstrdup(value);
  } else if (!strcmp(attr, "fullname:")) {
    parray[p].fullName = xstrdup(value);
  } else if (!strcmp(attr, "email:")) {
    parray[p].emailAddress = xstrdup(value);
  } else if (!strcmp(attr, "prompt:")) {
    parray[p].prompt = xstrdup(value);
  } else if (!strcmp(attr, "s_num:")) {
    parray[p].s_stats.num = atoi(value);
  } else if (!strcmp(attr, "s_win:")) {
    parray[p].s_stats.win = atoi(value);
  } else if (!strcmp(attr, "s_loss:")) {
    parray[p].s_stats.los = atoi(value);
  } else if (!strcmp(attr, "s_draw:")) {
    parray[p].s_stats.dra = atoi(value);
  } else if (!strcmp(attr, "s_rating:")) {
    parray[p].s_stats.rating = atoi(value);
  } else if (!strcmp(attr, "s_sterr:")) {
    parray[p].s_stats.sterr = (atoi(value) / 10.0);
  } else if (!strcmp(attr, "s_ltime:")) {
    parray[p].s_stats.ltime = atoi(value);
  } else if (!strcmp(attr, "s_best:")) {
    parray[p].s_stats.best = atoi(value);
  } else if (!strcmp(attr, "s_wbest:")) {
    parray[p].s_stats.whenbest = atoi(value);
  } else if (!strcmp(attr, "b_num:")) {
    parray[p].b_stats.num = atoi(value);
  } else if (!strcmp(attr, "b_win:")) {
    parray[p].b_stats.win = atoi(value);
  } else if (!strcmp(attr, "b_loss:")) {
    parray[p].b_stats.los = atoi(value);
  } else if (!strcmp(attr, "b_draw:")) {
    parray[p].b_stats.dra = atoi(value);
  } else if (!strcmp(attr, "b_rating:")) {
    parray[p].b_stats.rating = atoi(value);
  } else if (!strcmp(attr, "b_sterr:")) {
    parray[p].b_stats.sterr = (atoi(value) / 10.0);
  } else if (!strcmp(attr, "b_ltime:")) {
    parray[p].b_stats.ltime = atoi(value);
  } else if (!strcmp(attr, "b_best:")) {
    parray[p].b_stats.best = atoi(value);
  } else if (!strcmp(attr, "b_wbest:")) {
    parray[p].b_stats.whenbest = atoi(value);
  } else if (!strcmp(attr, "w_num:")) {
    parray[p].w_stats.num = atoi(value);
  } else if (!strcmp(attr, "w_win:")) {
    parray[p].w_stats.win = atoi(value);
  } else if (!strcmp(attr, "w_loss:")) {
    parray[p].w_stats.los = atoi(value);
  } else if (!strcmp(attr, "w_draw:")) {
    parray[p].w_stats.dra = atoi(value);
  } else if (!strcmp(attr, "w_rating:")) {
    parray[p].w_stats.rating = atoi(value);
  } else if (!strcmp(attr, "w_sterr:")) {
    parray[p].w_stats.sterr = (atoi(value) / 10.0);
  } else if (!strcmp(attr, "w_ltime:")) {
    parray[p].w_stats.ltime = atoi(value);
  } else if (!strcmp(attr, "w_best:")) {
    parray[p].w_stats.best = atoi(value);
  } else if (!strcmp(attr, "w_wbest:")) {
    parray[p].w_stats.whenbest = atoi(value);
  } else if (!strcmp(attr, "open:")) {
    parray[p].open = atoi(value);
  } else if (!strcmp(attr, "rated:")) {
    parray[p].rated = atoi(value);
  } else if (!strcmp(attr, "ropen:")) {
    parray[p].ropen = atoi(value);
  } else if (!strcmp(attr, "bell:")) {
    parray[p].bell = atoi(value);
  } else if (!strcmp(attr, "pgn:")) {
    parray[p].pgn = atoi(value);
  } else if (!strcmp(attr, "timeofreg:")) {
    parray[p].timeOfReg = atoi(value);
  } else if (!strcmp(attr, "totaltime:")) {
    parray[p].totalTime = atoi(value);
  } else if (!strcmp(attr, "notifiedby:")) {
    parray[p].notifiedby = atoi(value);
  } else if (!strcmp(attr, "i_login:")) {
    parray[p].i_login = atoi(value);
  } else if (!strcmp(attr, "i_game:")) {
    parray[p].i_game = atoi(value);
  } else if (!strcmp(attr, "i_shout:")) {
    parray[p].i_shout = atoi(value);
  } else if (!strcmp(attr, "i_cshout:")) {
    parray[p].i_cshout = atoi(value);
  } else if (!strcmp(attr, "i_tell:")) {
    parray[p].i_tell = atoi(value);
  } else if (!strcmp(attr, "i_kibitz:")) {
    parray[p].i_kibitz = atoi(value);
  } else if (!strcmp(attr, "kiblevel:")) {
    parray[p].kiblevel = atoi(value);
  } else if (!strcmp(attr, "private:")) {
    parray[p].private = atoi(value);
  } else if (!strcmp(attr, "jprivate:")) {
    parray[p].jprivate = atoi(value);
  } else if (!strcmp(attr, "automail:")) {
    parray[p].automail = atoi(value);
  } else if (!strcmp(attr, "i_mailmess:")) {
    parray[p].i_mailmess = atoi(value);
  } else if (!strcmp(attr, "style:")) {
    parray[p].style = atoi(value);
  } else if (!strcmp(attr, "d_time:")) {
    parray[p].d_time = atoi(value);
  } else if (!strcmp(attr, "d_inc:")) {
    parray[p].d_inc = atoi(value);
  } else if (!strcmp(attr, "d_height:")) {
    parray[p].d_height = atoi(value);
  } else if (!strcmp(attr, "d_width:")) {
    parray[p].d_width = atoi(value);
  } else if (!strcmp(attr, "language:")) {
    parray[p].language = atoi(value);
  } else if (!strcmp(attr, "admin_level:")) {
    parray[p].adminLevel = atoi(value);
    if (parray[p].adminLevel >= ADMIN_ADMIN)
      parray[p].i_admin = 1;
  } else if (!strcmp(attr, "i_admin:")) {
/*    parray[p].i_admin = atoi(value);  */
  } else if (!strcmp(attr, "computer:")) {
/*    parray[p].computer = atoi(value); */
  } else if (!strcmp(attr, "black_games:")) {
    parray[p].num_black = atoi(value);
  } else if (!strcmp(attr, "white_games:")) {
    parray[p].num_white = atoi(value);
  } else if (!strcmp(attr, "uscf:")) {
/*    parray[p].uscfRating = atoi(value); */
  } else if (!strcmp(attr, "muzzled:")) {	/* ignore these: obsolete */
  } else if (!strcmp(attr, "cmuzzled:")) {	/* ignore these: obsolete */
  } else if (!strcmp(attr, "highlight:")) {
    parray[p].highlight = atoi(value);
  } else if (!strcmp(attr, "network:")) {
/*    parray[p].network_player = atoi(value); */
  } else if (!strcmp(attr, "lasthost:")) {
    parray[p].lastHost = atoi(value);
  } else if (!strcmp(attr, "channel:")) {
    list_addsub(p,"channel",value, 1);
  } else if (!strcmp(attr, "num_comments:")) {
    parray[p].num_comments = atoi(value);
  } else if (!strcmp(attr, "num_plan:")) {
    parray[p].num_plan = atoi(value);
    if (parray[p].num_plan > 0) {
      for (i = 0; i < parray[p].num_plan; i++) {
	fgets(tmp, MAX_LINE_SIZE, fp);
	if (!(len = strlen(tmp))) {
	  fprintf(stderr, "FICS: Error bad plan in file %s\n", file);
          i--;
          parray[p].num_plan--;
	} else {
	  tmp[len - 1] = '\0';	/* Get rid of '\n' */
	  parray[p].planLines[i] = (len > 1) ? xstrdup(tmp) : NULL;
	}
      }
    }
  } else if (!strcmp(attr, "num_formula:")) {
    parray[p].num_formula = atoi(value);
    if (parray[p].num_formula > 0) {
      for (i = 0; i < parray[p].num_formula; i++) {
	fgets(tmp, MAX_LINE_SIZE, fp);
	if (!(len = strlen(tmp))) {
	  fprintf(stderr, "FICS: Error bad formula in file %s\n", file);
          i--;
          parray[p].num_formula--;
	} else {
	  tmp[len - 1] = '\0';	/* Get rid of '\n' */
	  parray[p].formulaLines[i] = (len > 1) ? xstrdup(tmp) : NULL;
	}
      }
    }
  } else if (!strcmp(attr, "formula:")) {
    parray[p].formula = xstrdup(value);
  } else if (!strcmp(attr, "num_alias:")) {
    parray[p].numAlias = atoi(value);
    if (parray[p].numAlias > 0) {
      for (i = 0; i < parray[p].numAlias; i++) {
	fgets(tmp, MAX_LINE_SIZE, fp);
	if (!(len = strlen(tmp))) {
	  fprintf(stderr, "FICS: Error bad alias in file %s\n", file);
	  i--;
	  parray[p].numAlias--;
	} else {
	  tmp[len - 1] = '\0';	/* Get rid of '\n' */
	  tmp1 = tmp;
	  tmp1 = eatword(tmp1);
	  *tmp1 = '\0';
	  tmp1++;
	  tmp1 = eatwhite(tmp1);
	  parray[p].alias_list[i].comm_name = xstrdup(tmp);
	  parray[p].alias_list[i].alias = xstrdup(tmp1);
	}
      }
    }
  } else if (!strcmp(attr, "num_censor:")) {
    i = atoi(value);
    while (i--) {
      fgets(tmp, MAX_LINE_SIZE, fp);
      if ((!(len = strlen(tmp))) || (len == 1)) { /* blank lines do occur!! */
        fprintf(stderr, "FICS: Error bad censor in file %s\n", file);
      } else {
        tmp[len - 1] = '\0';    /* Get rid of '\n' */
        list_add(p, L_CENSOR, tmp);
      }
    }
  } else if (!strcmp(attr, "num_notify:")) {
    i = atoi(value);
    while(i--) {
      fgets(tmp, MAX_LINE_SIZE, fp);
      if ((!(len = strlen(tmp))) || (len == 1)) { /* blank lines do occur!! */
        fprintf(stderr, "FICS: Error bad notify in file %s\n", file);
      } else {
        tmp[len - 1] = '\0';    /* Get rid of '\n' */
        list_add(p, L_NOTIFY, tmp);
      }
    }
  } else if (!strcmp(attr, "num_noplay:")) {
    i = atoi(value);
    while(i--) {
      fgets(tmp, MAX_LINE_SIZE, fp);
      if ((!(len = strlen(tmp))) || (len == 1)) { /* blank lines do occur!! */
        fprintf(stderr, "FICS: Error bad noplay in file %s\n", file);
      } else {
        tmp[len - 1] = '\0';    /* Get rid of '\n' */
        list_add(p, L_NOPLAY, tmp);
      }
    }
  } else if (!strcmp(attr, "num_gnotify:")) {
    i = atoi(value);
    while(i--) {
      fgets(tmp, MAX_LINE_SIZE, fp);
      if ((!(len = strlen(tmp)))  || (len == 1)) { /* blank lines do occur!! */
        fprintf(stderr, "FICS: Error bad gnotify in file %s\n", file);
      } else {
        tmp[len - 1] = '\0';    /* Get rid of '\n' */
        list_add(p, L_GNOTIFY, tmp);
      }
    }
  } else {
    fprintf(stderr, "FICS: Error bad attribute >%s< from file %s\n", attr, file);
  }
  return 0;
}

PUBLIC int player_read(int p, char *name)
{
  char fname[MAX_FILENAME_SIZE];
  char line[MAX_LINE_SIZE];
  char *attr, *value;
  FILE *fp;
  int len;
  int version = 0;

  parray[p].login = stolower(xstrdup(name));

  sprintf(fname, "%s/%c/%s", player_dir, parray[p].login[0], parray[p].login);
  fp = fopen(fname, "r");

  if (!fp) { /* unregistered player */
    parray[p].name = xstrdup(name);
    parray[p].registered = 0;
    return -1;
  }

  parray[p].registered = 1; /* lets load the file */

  fgets(line, MAX_LINE_SIZE, fp); /* ok so which version file? */

  if (line[0] == 'v') {
    sscanf(line, "%*c %d", &version);
  }

  if (version > 0) {
    ReadV1PlayerFmt(p,&parray[p], fp, fname, version); /* Quick method */
  }
  else /* do it the old SLOW way */
   do {
    if (feof(fp))
      break;
    if ((len = strlen(line)) <= 1)
      continue;
    line[len - 1] = '\0';
    attr = eatwhite(line);
    if (attr[0] == '#')
      continue;			/* Comment */
    value = eatword(attr);
    if (!*value) {
      fprintf(stderr, "FICS: Error reading file %s\n", fname);
      continue;
    }
    *value = '\0';
    value++;
    value = eatwhite(value);
    stolower(attr);
    got_attr_value_player(p, attr, value, fp, fname);
    fgets(line, MAX_LINE_SIZE, fp);
   } while (!feof(fp));

  fclose(fp);

  if (version == 0)
   player_save (p); /* ensure old files are quickly converted eg when someone
						fingers */
  if (!parray[p].name) {
    parray[p].name = xstrdup(name);
    pprintf(p, "\n*** WARNING: Your Data file is corrupt. Please tell an admin ***\n");
  }
  return 0;
}

PUBLIC int player_delete(int p)
{
  char fname[MAX_FILENAME_SIZE];

  if (!parray[p].registered) {	/* Player must not be registered */
    return -1;
  }
/*  if (iamserver)
    sprintf(fname, "%s.server/%c/%s", player_dir, parray[p].login[0], parray[p].login);
  else */
  sprintf(fname, "%s/%c/%s", player_dir, parray[p].login[0], parray[p].login);
  unlink(fname);
  return 0;
}

PUBLIC int player_markdeleted(int p)
{
  FILE *fp;
  char fname[MAX_FILENAME_SIZE], fname2[MAX_FILENAME_SIZE];

  if (!parray[p].registered) {	/* Player must not be registered */
    return -1;
  }
/*  if (iamserver) {
    sprintf(fname, "%s.server/%c/%s", player_dir, parray[p].login[0], parray[p].login);
    sprintf(fname2, "%s.server/%c/%s.delete", player_dir, parray[p].login[0], parray[p].login);
  } else {*/
  sprintf(fname, "%s/%c/%s", player_dir, parray[p].login[0], parray[p].login);
  sprintf(fname2, "%s/%c/%s.delete", player_dir, parray[p].login[0], parray[p].login);
/*  } */
  rename(fname, fname2);
  fp = fopen(fname2, "a");	/* Touch the file */
  if (fp) {
    fprintf(fp, "\n");
    fclose(fp);
  }
  return 0;
}

void
WritePlayerFile(FILE *fp, int p)
{
	int	 i;
	player	*pp = &parray[p];

	fprintf(fp, "v %d\n", PLAYER_VERSION);

	fprintf(fp, "%s\n", (pp->name ? pp->name : "NONE"));
	fprintf(fp, "%s\n", (pp->fullName ? pp->fullName : "NONE"));
	fprintf(fp, "%s\n", (pp->passwd ? pp->passwd : "NONE"));
	fprintf(fp, "%s\n", (pp->emailAddress ? pp->emailAddress : "NONE"));

	fprintf(fp, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u "
	    "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u "
	    "%u %u %u %u %d\n",
	    pp->s_stats.num, pp->s_stats.win, pp->s_stats.los,
	    pp->s_stats.dra, pp->s_stats.rating,
	    (int)(pp->s_stats.sterr * 10.0),
	    pp->s_stats.ltime, pp->s_stats.best, pp->s_stats.whenbest,

	    pp->b_stats.num, pp->b_stats.win, pp->b_stats.los,
	    pp->b_stats.dra, pp->b_stats.rating,
	    (int)(pp->b_stats.sterr * 10.0),
	    pp->b_stats.ltime, pp->b_stats.best, pp->b_stats.whenbest,

	    pp->w_stats.num, pp->w_stats.win, pp->w_stats.los,
	    pp->w_stats.dra, pp->w_stats.rating,
	    (int)(pp->w_stats.sterr * 10.0),
	    pp->w_stats.ltime, pp->w_stats.best, pp->w_stats.whenbest,

	    pp->l_stats.num, pp->l_stats.win, pp->l_stats.los,
	    pp->l_stats.dra, pp->l_stats.rating,
	    (int)(pp->l_stats.sterr * 10.0),
	    pp->l_stats.ltime, pp->l_stats.best, pp->l_stats.whenbest,

	    pp->bug_stats.num, pp->bug_stats.win, pp->bug_stats.los,
	    pp->bug_stats.dra, pp->bug_stats.rating,
	    (int)(pp->bug_stats.sterr * 10.0),
	    pp->bug_stats.ltime, pp->bug_stats.best, pp->bug_stats.whenbest,

	    pp->lastHost); /* fprintf() */

	fprintf(fp, "%s\n", pp->prompt);

	fprintf(fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
	    "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
	    pp->open, pp->rated, pp->ropen, pp->timeOfReg, pp->totalTime,
	    pp->bell, pp->pgn, pp->notifiedby, pp->i_login, pp->i_game,
	    pp->i_shout, pp->i_cshout, pp->i_tell, pp->i_kibitz, pp->private,
	    pp->jprivate, pp->automail, pp->i_mailmess, pp->style, pp->d_time,
	    pp->d_inc, pp->d_height, pp->d_width, pp->language, pp->adminLevel,
	    pp->num_white, pp->num_black, pp->highlight, pp->num_comments,
	    pp->num_plan, pp->num_formula,

	    list_size(p, L_CENSOR),
	    list_size(p, L_NOTIFY),
	    list_size(p, L_NOPLAY),
	    list_size(p, L_GNOTIFY),
	    pp->numAlias,
	    list_size(p, L_CHANNEL));

	for (i = 0; i < pp->num_plan; i++)
		fprintf(fp, "%s\n", (pp->planLines[i] ? pp->planLines[i] : ""));
	for (i = 0; i < pp->num_formula; i++) {
		fprintf(fp, "%s\n", (pp->formulaLines[i] ? pp->formulaLines[i] :
		    ""));
	}

	if (parray[p].formula != NULL)
		fprintf(fp, "%s\n", pp->formula);
	else
		fprintf(fp, "NONE\n");

	for (i = 0; i < pp->numAlias; i++) {
		fprintf(fp, "%s %s\n", pp->alias_list[i].comm_name,
		    pp->alias_list[i].alias);
	}

	list_print(fp, p, L_CENSOR);
	list_print(fp, p, L_NOTIFY);
	list_print(fp, p, L_NOPLAY);
	list_print(fp, p, L_GNOTIFY);
	list_print(fp, p, L_CHANNEL);
}

PUBLIC int player_save(int p)
{
  FILE *fp;
  char fname[MAX_FILENAME_SIZE];

  if (!parray[p].registered) {  /* Player must not be registered */
    return -1;
  }
  if (parray[p].name == NULL) { /* fixes a bug if name is null */
	    pprintf(p, "WARNING: Your player file could not be updated, due to corrupt data.\n");
    return -1;
   }
  if (strcasecmp(parray[p].login, parray[p].name)) {
    pprintf(p, "WARNING: Your player file could not be updated, due to corrupt data.\n");
    return -1;
  }

  sprintf(fname, "%s/%c/%s", player_dir, parray[p].login[0], parray[p].login);
  fp = fopen(fname, "w");
  if (!fp) {
    fprintf(stderr, "FICS: Problem opening file %s for write\n", fname);
    return -1;
  }

  WritePlayerFile(fp,p);
  fclose(fp);
  return 0;
}

PUBLIC int player_find(int fd)
{
  int i;

  for (i = 0; i < p_num; i++) {
    if (parray[i].status == PLAYER_EMPTY)
      continue;
    if (parray[i].socket == fd)
      return i;
  }
  return -1;
}

PUBLIC int player_find_bylogin(char *name)
{
  int i;

  for (i = 0; i < p_num; i++) {
    if ((parray[i].status == PLAYER_EMPTY) ||
	(parray[i].status == PLAYER_LOGIN) ||
	(parray[i].status == PLAYER_PASSWORD))
      continue;
    if (!parray[i].login)
      continue;
    if (!strcmp(parray[i].login, name))
      return i;
  }
  return -1;
}

PUBLIC int player_find_part_login(char *name)
{
  int i;
  int found = -1;

  i = player_find_bylogin(name);
  if (i >= 0)
    return i;
  for (i = 0; i < p_num; i++) {
    if ((parray[i].status == PLAYER_EMPTY) ||
	(parray[i].status == PLAYER_LOGIN) ||
	(parray[i].status == PLAYER_PASSWORD))
      continue;
    if (!parray[i].login)
      continue;
    if (!strncmp(parray[i].login, name, strlen(name))) {
      if (found >= 0) {		/* Ambiguous */
	return -2;
      }
      found = i;
    }
  }
  return found;
}

PUBLIC int player_censored(int p, int p1)
{
  if (in_list(p, L_CENSOR, parray[p1].login))
    return 1;
  else
    return 0;
}

/* is p1 on p's notify list? */
PUBLIC int player_notified(int p, int p1)
{
  if (!parray[p1].registered)
    return 0;

  /* possible bug: p has just arrived! */
  if (!parray[p].name)
    return 0;

  return (in_list(p, L_NOTIFY, parray[p1].login));
}

PUBLIC void player_notify_departure(int p)
/* Notify those with notifiedby set on a departure */
{
  int p1;

  if (!parray[p].registered)
    return;
  for (p1 = 0; p1 < p_num; p1++) {
    if (parray[p1].notifiedby && !player_notified(p1, p) && player_notified(p, p1)
          && (parray[p1].status == PLAYER_PROMPT)) {
      if (parray[p1].bell)
	pprintf_noformat(p1, "\007");
      pprintf(p1, "\nNotification: ");
      pprintf_highlight(p1, "%s", parray[p].name);
      pprintf_prompt(p1, " has departed and isn't on your notify list.\n");
    }
  }
}

PUBLIC int player_notify_present(int p)
/* output Your arrival was notified by..... */
/* also notify those with notifiedby set if necessary */
{
  int p1;
  int count = 0;

  if (!parray[p].registered)
    return count;
  for (p1 = 0; p1 < p_num; p1++) {
    if ((player_notified(p, p1)) && (parray[p1].status == PLAYER_PROMPT)) {
      if (!count) {
	pprintf(p, "Present company includes:");
      }
      count++;
      pprintf(p, " %s", parray[p1].name);
      if ((parray[p1].notifiedby) && (!player_notified(p1, p))
                         && (parray[p1].status == PLAYER_PROMPT)) {
	if (parray[p1].bell)
	  pprintf_noformat(p1, "\007");
	pprintf(p1, "\nNotification: ");
	pprintf_highlight(p1, "%s", parray[p].name);
	pprintf_prompt(p1, " has arrived and isn't on your notify list.\n");
      }
    }
  }
  if (count)
    pprintf(p, ".\n");
  return count;
}

PUBLIC int player_notify(int p, char *note1, char *note2)
/* notify those interested that p has arrived/departed */
{
  int p1;
  int count = 0;

  if (!parray[p].registered)
    return count;
  for (p1 = 0; p1 < p_num; p1++) {
    if ((player_notified(p1, p)) && (parray[p1].status == PLAYER_PROMPT)) {
      if (parray[p1].bell)
	pprintf_noformat(p1, "\007");
      pprintf(p1, "\nNotification: ");
      pprintf_highlight(p1, "%s", parray[p].name);
      pprintf_prompt(p1, " has %s.\n", note1);
      if (!count) {
	pprintf(p, "Your %s was noted by:", note2);
      }
      count++;
      pprintf(p, " %s", parray[p1].name);
    }
  }
  if (count)
    pprintf(p, ".\n");
  return count;
}

/* Show adjourned games upon logon. connex/sous@ipp.tu-clausthal.de
24.10.1995 */
PUBLIC int showstored(int p)
{
  DIR *dirp;
#ifdef USE_DIRENT
  struct dirent *dp;
#else
  struct direct *dp;
#endif
  int c=0,p1;
  char dname[MAX_FILENAME_SIZE];
  multicol *m = multicol_start(50); /* Limit to 50, should be enough*/

  sprintf(dname, "%s/%c", adj_dir, parray[p].login[0]);
  dirp = opendir(dname);
  if (!dirp) {
    multicol_end(m);
    return COM_OK;
  }
  for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
    if (file_has_pname(dp->d_name, parray[p].login)) {
      if (strcmp(file_wplayer(dp->d_name),parray[p].login) != 0) {
      	p1=player_find_bylogin(file_wplayer(dp->d_name));
      } else {
      	p1=player_find_bylogin(file_bplayer(dp->d_name));
      }
      if (p1>=0) {
      	if (c<50)
      		multicol_store(m,parray[p1].name);
      	pprintf(p1,"\nNotification: ");
      	pprintf_highlight(p1,"%s",parray[p].name);
      	pprintf_prompt(p1,", who has an adjourned game with you, has arrived.\n");
      	c++;
      }
    }
  }
  closedir(dirp);
  if (c == 1) {
        pprintf(p, "1 player, who has an adjourned game with you, is online:\007");
  } else if (c > 1) {
  	pprintf(p, "\n%d players, who have an adjourned game with you, are online:\007",c);
  }
  if (c != 0)
  	multicol_pprint(m,p,parray[p].d_width,2);
  multicol_end(m);
  return COM_OK;
}


PUBLIC int player_count(int CountAdmins)
{
  int count;
  int i;

  for (count = 0, i = 0; i < p_num; i++) {
    if ((parray[i].status == PLAYER_PROMPT) &&
        (CountAdmins || !in_list(i, L_ADMIN, parray[i].name)))
      count++;
  }
  if (count > player_high)
    player_high = count;

  return count;
}

PUBLIC int player_idle(int p)
{
  if (parray[p].status != PLAYER_PROMPT)
    return time(0) - parray[p].logon_time;
  else
    return time(0) - parray[p].last_command_time;
}

PUBLIC int player_ontime(int p)
{
  return time(0) - parray[p].logon_time;
}

PRIVATE void write_p_inout(int inout, int p, char *file, int maxlines)
{
  FILE *fp;

  fp = fopen(file, "a");
  if (!fp)
    return;
  fprintf(fp, "%d %s %d %d %s\n", inout, parray[p].name, (int) time(0),
	  parray[p].registered, dotQuad(parray[p].thisHost));
  fclose(fp);
  if (maxlines)
    truncate_file(file, maxlines);
}

PUBLIC void player_write_login(int p)
{
  char fname[MAX_FILENAME_SIZE];

  if (parray[p].registered) {
    sprintf(fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0], parray[p].login, STATS_LOGONS);
    write_p_inout(P_LOGIN, p, fname, 8);
  }
  sprintf(fname, "%s/%s", stats_dir, STATS_LOGONS);
  write_p_inout(P_LOGIN, p, fname, 30);
  /* added complete login/logout log to "logons.log" file */
  sprintf(fname, "%s/%s", stats_dir, "logons.log");
  write_p_inout(P_LOGIN, p, fname, 0);
}

PUBLIC void player_write_logout(int p)
{
  char fname[MAX_FILENAME_SIZE];

  if (parray[p].registered) {
    sprintf(fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0], parray[p].login, STATS_LOGONS);
    write_p_inout(P_LOGOUT, p, fname, 8);
  }
  sprintf(fname, "%s/%s", stats_dir, STATS_LOGONS);
  write_p_inout(P_LOGOUT, p, fname, 30);
  /* added complete login/logout log to "logons.log" file */
  sprintf(fname, "%s/%s", stats_dir, "logons.log");
  write_p_inout(P_LOGOUT, p, fname, 0);
}

PUBLIC time_t
player_lastconnect(int p)
{
	FILE		*fp;
	char		 fname[MAX_FILENAME_SIZE];
	char		 ipstr[20];
	char		 loginName[MAX_LOGIN_NAME];
	int		 inout, registered;
	long int	 lval;
	time_t		 last = 0;

	sprintf(fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0],
	    parray[p].login, STATS_LOGONS);

	if ((fp = fopen(fname, "r")) == NULL)
		return 0;

	inout = 1;

	while (!feof(fp)) {
		if (inout == P_LOGIN)
			last = lval;

		if (fscanf(fp, "%d %s %ld %d %s\n", &inout, loginName, &lval,
		    &registered, ipstr) != 5) {
			fprintf(stderr, "FICS: Error in login info format. %s"
			    "\n", fname);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);
	return last;
}

PUBLIC time_t
player_lastdisconnect(int p)
{
	FILE		*fp;
	char		 fname[MAX_FILENAME_SIZE];
	char		 ipstr[20];
	char		 loginName[MAX_LOGIN_NAME];
	int		 inout, registered;
	long int	 lval;
	time_t		 last = 0;

	sprintf(fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0],
	    parray[p].login, STATS_LOGONS);

	if ((fp = fopen(fname, "r")) == NULL)
		return 0;

	while (!feof(fp)) {
		if (fscanf(fp, "%d %s %ld %d %s\n", &inout, loginName, &lval,
		    &registered, ipstr) != 5) {
			fprintf(stderr, "FICS: Error in login info format. %s"
			    "\n", fname);
			fclose(fp);
			return 0;
		}

		if (inout == P_LOGOUT)
			last = lval;
	}

	fclose(fp);
	return last;
}

PUBLIC void player_pend_print(int p, pending *pend)
{
  char outstr[200];
  char tmp[200];

  if (p == pend->whofrom) {
    sprintf(outstr, "You are offering ");
  } else {
    sprintf(outstr, "%s is offering ", parray[pend->whofrom].name);
  }

  if (p == pend->whoto) {
    strcpy(tmp, "");
  } else {
    sprintf(tmp, "%s ", parray[pend->whoto].name);
  }

  strcat(outstr, tmp);
  switch (pend->type) {
  case PEND_MATCH:
    sprintf(tmp, "%s.", game_str(pend->param5, pend->param1 * 60, pend->param2, pend->param3 * 60, pend->param4, pend->char1, pend->char2));
    break;
  case PEND_DRAW:
    sprintf(tmp, "a draw.\n");
    break;
  case PEND_PAUSE:
    sprintf(tmp, "to pause the clock.\n");
    break;
  case PEND_ABORT:
    sprintf(tmp, "to abort the game.\n");
    break;
  case PEND_TAKEBACK:
    sprintf(tmp, "to takeback the last %d half moves.\n", pend->param1);
    break;
  case PEND_SIMUL:
    sprintf(tmp, "to play a simul match.\n");
    break;
  case PEND_SWITCH:
    sprintf(tmp, "to switch sides.\n");
    break;
  case PEND_ADJOURN:
    sprintf(tmp, "an adjournment.\n");
    break;
  case PEND_PARTNER:
    sprintf(tmp, "to be bughouse partners.\n");
    break;
  }
  strcat(outstr, tmp);
  pprintf(p, "%s\n", outstr);
}

PUBLIC int player_find_pendto(int p, int p1, int type)
{
  int i;

  for (i = 0; i < parray[p].num_to; i++) {
    if (parray[p].p_to_list[i].whoto != p1 && p1 != -1)
      continue;
    if (type < 0 || parray[p].p_to_list[i].type == type)
      return i;
    if (type == PEND_BUGHOUSE
        && parray[p].p_to_list[i].type == PEND_MATCH
        && !strcmp(parray[p].p_to_list[i].char2, "bughouse"))
      return i;
  }
  return -1;
}

PUBLIC int player_new_pendto(int p)
{
  if (parray[p].num_to >= MAX_PENDING)
    return -1;
  parray[p].num_to++;
  return parray[p].num_to - 1;
}

PUBLIC int player_remove_pendto(int p, int p1, int type)
{
  int w;
  if ((w = player_find_pendto(p, p1, type)) < 0)
    return -1;
  for (; w < parray[p].num_to - 1; w++)
    parray[p].p_to_list[w] = parray[p].p_to_list[w + 1];
  parray[p].num_to = parray[p].num_to - 1;
  return 0;
}

PUBLIC int player_find_pendfrom(int p, int p1, int type)
{
  int i;

  for (i = 0; i < parray[p].num_from; i++) {
    if (parray[p].p_from_list[i].whofrom != p1 && p1 != -1)
      continue;
    if (type == PEND_ALL || parray[p].p_from_list[i].type == type)
      return i;
    if (type < 0 && parray[p].p_from_list[i].type != -type)
      return i;
    /* The above "if" allows a type of -PEND_SIMUL to match every request
       EXCEPT simuls, for example.  I'm doing this because Heringer does
       not want to decline simul requests when he makes a move in a sumul.
       -- hersco. */
    if (type == PEND_BUGHOUSE
        && parray[p].p_from_list[i].type == PEND_MATCH
        && !strcmp(parray[p].p_from_list[i].char2, "bughouse"))
      return i;
  }
  return -1;
}

PUBLIC int player_new_pendfrom(int p)
{
  if (parray[p].num_from >= MAX_PENDING)
    return -1;
  parray[p].num_from++;
  return parray[p].num_from - 1;
}

PUBLIC int player_remove_pendfrom(int p, int p1, int type)
{
  int w;
  if ((w = player_find_pendfrom(p, p1, type)) < 0)
    return -1;
  for (; w < parray[p].num_from - 1; w++)
    parray[p].p_from_list[w] = parray[p].p_from_list[w + 1];
  parray[p].num_from = parray[p].num_from - 1;
  return 0;
}

PUBLIC int player_add_request(int p, int p1, int type, int param)
{
  int pendt;
  int pendf;

  if (player_find_pendto(p, p1, type) >= 0)
    return -1;			/* Already exists */
  pendt = player_new_pendto(p);
  if (pendt == -1) {
    return -1;
  }
  pendf = player_new_pendfrom(p1);
  if (pendf == -1) {
    parray[p].num_to--;		/* Remove the pendto we allocated */
    return -1;
  }
  parray[p].p_to_list[pendt].type = type;
  parray[p].p_to_list[pendt].whoto = p1;
  parray[p].p_to_list[pendt].whofrom = p;
  parray[p].p_to_list[pendt].param1 = param;
  parray[p1].p_from_list[pendf].type = type;
  parray[p1].p_from_list[pendf].whoto = p1;
  parray[p1].p_from_list[pendf].whofrom = p;
  parray[p1].p_from_list[pendf].param1 = param;
  return 0;
}

PUBLIC int player_remove_request(int p, int p1, int type)
{
  int to = 0, from = 0;

  while (to != -1) {
    to = player_find_pendto(p, p1, type);
    if (to != -1) {
      for (; to < parray[p].num_to - 1; to++)
	parray[p].p_to_list[to] = parray[p].p_to_list[to + 1];
      parray[p].num_to = parray[p].num_to - 1;
    }
  }
  while (from != -1) {
    from = player_find_pendfrom(p1, p, type);
    if (from != -1) {
      for (; from < parray[p1].num_from - 1; from++)
	parray[p1].p_from_list[from] = parray[p1].p_from_list[from + 1];
      parray[p1].num_from = parray[p1].num_from - 1;
    }
  }
  if ((type == PEND_ALL || type == PEND_MATCH)
      && parray[p].partner >= 0)
    player_remove_request (parray[p].partner, p1, PEND_BUGHOUSE);
  return 0;
}


PUBLIC int player_decline_offers(int p, int p1, int offerType)
{
  int offer;
  int type, p2;
  int count = 0;
  int part, p2part;
  char *pName = parray[p].name, *p2Name;

  /* First get rid of bughouse offers from partner. */
  if ((offerType == PEND_MATCH || offerType == PEND_ALL)
      && parray[p].partner >= 0 && parray[parray[p].partner].partner == p)
    count += player_decline_offers(parray[p].partner, p1, PEND_BUGHOUSE);

  while ((offer = player_find_pendfrom(p, p1, offerType)) >= 0) {
    type = parray[p].p_from_list[offer].type;
    p2 = parray[p].p_from_list[offer].whofrom;
    p2Name = parray[p2].name;

    part = parray[p].partner;
    if (part >= 0 && parray[part].partner != p)
      part = -1;
    p2part = parray[p2].partner;
    if (p2part >= 0 && parray[p2part].partner != p2)
      p2part = -1;

    switch (type) {
    case PEND_MATCH:
      pprintf_prompt(p2, "\n%s declines the match offer.\n", pName);
      pprintf(p, "You decline the match offer from %s.\n", p2Name);
      if (!strcmp(parray[p].p_from_list[offer].char2, "bughouse")) {
        if (part >= 0)
          pprintf_prompt(part,
            "Your partner declines the bughouse offer from %s.\n",
            parray[p2].name);
        if (p2part >= 0)
          pprintf_prompt(p2part,
            "%s declines the bughouse offer from your partner.\n",
            parray[p].name);
      }
      break;
    case PEND_DRAW:
      pprintf_prompt(p2, "\n%s declines draw request.\n", pName);
      pprintf(p, "You decline the draw request from %s.\n", p2Name);
      break;
    case PEND_PAUSE:
      pprintf_prompt(p2, "\n%s declines pause request.\n", pName);
      pprintf(p, "You decline the pause request from %s.\n", p2Name);
      break;
    case PEND_ABORT:
      pprintf_prompt(p2, "\n%s declines abort request.\n", pName);
      pprintf(p, "You decline the abort request from %s.\n", p2Name);
      break;
    case PEND_TAKEBACK:
      pprintf_prompt(p2, "\n%s declines the takeback request.\n", pName);
      pprintf(p, "You decline the takeback request from %s.\n", p2Name);
      break;
    case PEND_ADJOURN:
      pprintf_prompt(p2, "\n%s declines the adjourn request.\n", pName);
      pprintf(p, "You decline the adjourn request from %s.\n", p2Name);
      break;
    case PEND_SWITCH:
      pprintf_prompt(p2, "\n%s declines the switch sides request.\n", pName);
      pprintf(p, "You decline the switch sides request from %s.\n", p2Name);
      break;
    case PEND_SIMUL:
      pprintf_prompt(p2, "\n%s declines the simul offer.\n", pName);
      pprintf(p, "You decline the simul offer from %s.\n", p2Name);
      break;
    case PEND_PARTNER:
      pprintf_prompt(p2, "\n%s declines your partnership request.\n", pName);
      pprintf(p, "You decline the partnership request from %s.\n", p2Name);
      break;
    }
    player_remove_request(p2, p, type);
    count++;
  }
  return count;
}


PUBLIC int player_withdraw_offers(int p, int p1, int offerType)
{
  int offer;
  int type, p2;
  int count = 0;
  int part, p2part;
  char *pName = parray[p].name, *p2Name;

  /* First get rid of bughouse offers from partner. */
  if ((offerType == PEND_MATCH || offerType == PEND_ALL)
      && parray[p].partner >= 0 && parray[parray[p].partner].partner == p)
    count += player_withdraw_offers(parray[p].partner, p1, PEND_BUGHOUSE);

  while ((offer = player_find_pendto(p, p1, offerType)) >= 0) {
    type = parray[p].p_to_list[offer].type;
    p2 = parray[p].p_to_list[offer].whoto;
    p2Name = parray[p2].name;

    part = parray[p].partner;
    if (part >= 0 && parray[part].partner != p)
      part = -1;
    p2part = parray[p2].partner;
    if (p2part >= 0 && parray[p2part].partner != p2)
      p2part = -1;

    switch (type) {
    case PEND_MATCH:
      pprintf_prompt(p2, "\n%s withdraws the match offer.\n", pName);
      pprintf(p, "You withdraw the match offer to %s.\n", p2Name);
      if (!strcmp(parray[p].p_to_list[offer].char2, "bughouse")) {
        if (part >= 0)
          pprintf_prompt(part,
            "Your partner withdraws the bughouse offer to %s.\n",
            parray[p2].name);
        if (p2part >= 0)
          pprintf_prompt(p2part,
            "%s withdraws the bughouse offer to your partner.\n",
            parray[p].name);
      }
      break;
    case PEND_DRAW:
      pprintf_prompt(p2, "\n%s withdraws draw request.\n", pName);
      pprintf(p, "You withdraw the draw request to %s.\n", p2Name);
      break;
    case PEND_PAUSE:
      pprintf_prompt(p2, "\n%s withdraws pause request.\n", pName);
      pprintf(p, "You withdraw the pause request to %s.\n", p2Name);
      break;
    case PEND_ABORT:
      pprintf_prompt(p2, "\n%s withdraws abort request.\n", pName);
      pprintf(p, "You withdraw the abort request to %s.\n", p2Name);
      break;
    case PEND_TAKEBACK:
      pprintf_prompt(p2, "\n%s withdraws the takeback request.\n", pName);
      pprintf(p, "You withdraw the takeback request to %s.\n", p2Name);
      break;
    case PEND_ADJOURN:
      pprintf_prompt(p2, "\n%s withdraws the adjourn request.\n", pName);
      pprintf(p, "You withdraw the adjourn request to %s.\n", p2Name);
      break;
    case PEND_SWITCH:
      pprintf_prompt(p2, "\n%s withdraws the switch sides request.\n", pName);
      pprintf(p, "You withdraw the switch sides request to %s.\n", p2Name);
      break;
    case PEND_SIMUL:
      pprintf_prompt(p2, "\n%s withdraws the simul offer.\n", pName);
      pprintf(p, "You withdraw the simul offer to %s.\n", p2Name);
      break;
    case PEND_PARTNER:
      pprintf_prompt(p2, "\n%s withdraws partnership request.\n", pName);
      pprintf(p, "You withdraw the partnership request to %s.\n", p2Name);
      break;
    }
    player_remove_request(p, p2, type);
    count++;
  }
  return count;
}


PUBLIC int player_is_observe(int p, int g)
{
  int i;

  for (i = 0; i < parray[p].num_observe; i++) {
    if (parray[p].observe_list[i] == g)
      break;
  }
  if (i == parray[p].num_observe)
    return 0;
  else
    return 1;
}

PUBLIC int player_add_observe(int p, int g)
{
  if (parray[p].num_observe == MAX_OBSERVE)
    return -1;
  parray[p].observe_list[parray[p].num_observe] = g;
  parray[p].num_observe++;
  return 0;
}

PUBLIC int player_remove_observe(int p, int g)
{
  int i;

  for (i = 0; i < parray[p].num_observe; i++) {
    if (parray[p].observe_list[i] == g)
      break;
  }
  if (i == parray[p].num_observe)
    return -1;			/* Not found! */
  for (; i < parray[p].num_observe - 1; i++) {
    parray[p].observe_list[i] = parray[p].observe_list[i + 1];
  }
  parray[p].num_observe--;
  return 0;
}

PUBLIC int player_game_ended(int g)
{
  int p;

  for (p = 0; p < p_num; p++) {
    if (parray[p].status == PLAYER_EMPTY)
      continue;
    player_remove_observe(p, g);
  }
  player_remove_request(garray[g].white, garray[g].black, -1);
  player_remove_request(garray[g].black, garray[g].white, -1);
  player_save(garray[g].white);	/* Hawk: Added to save finger-info after each
				   game */
  player_save(garray[g].black);
  return 0;
}

PUBLIC int player_goto_board(int p, int board_num)
{
  int start, count = 0, on, g;

  if (board_num < 0 || board_num >= parray[p].simul_info.numBoards)
    return -1;
  if (parray[p].simul_info.boards[board_num] < 0)
    return -1;
  parray[p].simul_info.onBoard = board_num;
  parray[p].game = parray[p].simul_info.boards[board_num];
  parray[p].opponent = garray[parray[p].game].black;
  if (parray[p].simul_info.numBoards == 1)
    return 0;
  send_board_to(parray[p].game, p);
  start = parray[p].game;
  on = parray[p].simul_info.onBoard;
  do {
    g = parray[p].simul_info.boards[on];
    if (g >= 0) {
      if (count == 0) {
	if (parray[garray[g].black].bell) {
	  pprintf(garray[g].black, "\007");
	}
	pprintf(garray[g].black, "\n");
	pprintf_highlight(garray[g].black, "%s", parray[p].name);
	pprintf_prompt(garray[g].black, " is at your board!\n");
      } else if (count == 1) {
	if (parray[garray[g].black].bell) {
	  pprintf(garray[g].black, "\007");
	}
	pprintf(garray[g].black, "\n");
	pprintf_highlight(garray[g].black, "%s", parray[p].name);
	pprintf_prompt(garray[g].black, " will be at your board NEXT!\n");
      } else {
	pprintf(garray[g].black, "\n");
	pprintf_highlight(garray[g].black, "%s", parray[p].name);
	pprintf_prompt(garray[g].black, " is %d boards away.\n", count);
      }
      count++;
    }
    on++;
    if (on >= parray[p].simul_info.numBoards)
      on = 0;
  } while (start != parray[p].simul_info.boards[on]);
  return 0;
}

PUBLIC int player_goto_next_board(int p)
{
  int on;
  int start;
  int g;

  on = parray[p].simul_info.onBoard;
  start = on;
  g = -1;
  do {
    on++;
    if (on >= parray[p].simul_info.numBoards)
      on = 0;
    g = parray[p].simul_info.boards[on];
    if (g >= 0)
      break;
  } while (start != on);
  if (g == -1) {
    pprintf(p, "\nMajor Problem! Can't find your next board.\n");
    return -1;
  }
  return player_goto_board(p, on);
}

PUBLIC int player_goto_prev_board(int p)
{
  int on;
  int start;
  int g;

  on = parray[p].simul_info.onBoard;
  start = on;
  g = -1;
  do {
    --on;
    if (on < 0)
      on = (parray[p].simul_info.numBoards) - 1;
    g = parray[p].simul_info.boards[on];
    if (g >= 0)
      break;
  } while (start != on);
  if (g == -1) {
    pprintf(p, "\nMajor Problem! Can't find your previous board.\n");
    return -1;
  }
  return player_goto_board(p, on);
}

PUBLIC int player_goto_simulgame_bynum(int p, int num)
{
  int on;
  int start;
  int g;

  on = parray[p].simul_info.onBoard;
  start = on;
  do {
    on++;
    if (on >= parray[p].simul_info.numBoards)
      on = 0;
    g = parray[p].simul_info.boards[on];
    if (g == num)
      break;
  } while (start != on);
  if (g != num) {
    pprintf(p, "\nYou aren't playing that game!!\n");
    return -1;
  }
  return player_goto_board(p, on);
}

PUBLIC int player_num_active_boards(int p)
{
  int count = 0, i;

  if (!parray[p].simul_info.numBoards)
    return 0;
  for (i = 0; i < parray[p].simul_info.numBoards; i++)
    if (parray[p].simul_info.boards[i] >= 0)
      count++;
  return count;
}

PUBLIC int player_num_results(int p, int result)
{
  int count = 0, i;

  if (!parray[p].simul_info.numBoards)
    return 0;
  for (i = 0; i < parray[p].simul_info.numBoards; i++)
    if (parray[p].simul_info.results[i] == result)
      count++;
  return count;
}

PUBLIC int player_simul_over(int p, int g, int result)
{
  int on, ong, p1, which = -1, won;
  char tmp[1024];

  for (won = 0; won < parray[p].simul_info.numBoards; won++) {
    if (parray[p].simul_info.boards[won] == g) {
      which = won;
      break;
    }
  }
  if (which == -1) {
    pprintf(p, "I can't find that game!\n");
    return -1;
  }
  pprintf(p, "\nBoard %d has completed.\n", won + 1);
  on = parray[p].simul_info.onBoard;
  ong = parray[p].simul_info.boards[on];
  parray[p].simul_info.boards[won] = -1;
  parray[p].simul_info.results[won] = result;
  if (player_num_active_boards(p) == 0) {
    sprintf(tmp, "\n{Simul (%s vs. %d) is over.}\nResults: %d Wins, %d Losses, %d Draws, %d Aborts\n",
	    parray[p].name,
	    parray[p].simul_info.numBoards,
	    player_num_results(p, RESULT_WIN),
	    player_num_results(p, RESULT_LOSS),
	    player_num_results(p, RESULT_DRAW),
	    player_num_results(p, RESULT_ABORT));
    for (p1 = 0; p1 < p_num; p1++) {
      if (parray[p].status != PLAYER_PROMPT)
	continue;
      if (!parray[p1].i_game && !player_is_observe(p1, g) && (p1 != p))
	continue;
      pprintf_prompt(p1, "%s", tmp);
    }
    parray[p].simul_info.numBoards = 0;
    pprintf_prompt(p, "\nThat was the last board, thanks for playing.\n");
    return 0;
  }
  if (ong == g) {		/* This game is over */
    player_goto_next_board(p);
  } else {
    player_goto_board(p, parray[p].simul_info.onBoard);
  }
  pprintf_prompt(p, "\nThere are %d boards left.\n",
		 player_num_active_boards(p));
  return 0;
}

PRIVATE void GetMsgFile (int p, char *fName)
{
 sprintf(fName, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0],
         parray[p].login, STATS_MESSAGES);
 }

PUBLIC int player_num_messages(int p)
{
  char fname[MAX_FILENAME_SIZE];

  if (!parray[p].registered)
    return 0;
  GetMsgFile (p, fname);
  return lines_file(fname);
}

PUBLIC int
player_add_message(int top, int fromp, char *message)
{
	FILE	*fp;
	char	 fname[MAX_FILENAME_SIZE];
	char	 messbody[1024];
	char	 subj[256];
	time_t	 t = time(NULL);

	if (!parray[top].registered)
		return -1;
	if (!parray[fromp].registered)
		return -1;

	GetMsgFile(top, fname);

	if (lines_file(fname) >= MAX_MESSAGES && parray[top].adminLevel == 0)
		return -1;

	if ((fp = fopen(fname, "a")) == NULL)
		return -1;
	fprintf(fp, "%s at %s: %s\n", parray[fromp].name, strltime(&t),
	    message);
	fclose(fp);

	pprintf(fromp, "\nThe following message was sent ");

	if (parray[top].i_mailmess) {
		sprintf(subj, "FICS message from %s at FICS %s "
		    "(Do not reply by mail)", parray[fromp].name,
		    fics_hostname);
		sprintf(messbody, "%s at %s: %s\n", parray[fromp].name,
		    strltime(&t), message);
		mail_string_to_user(top, subj, messbody);
		pprintf(fromp, "(and emailed) ");
	}

	pprintf(fromp, "to %s: \n   %s\n", parray[top].name, message);
	return 0;
}

PUBLIC void SaveTextListEntry(textlist **Entry, char *string, int n)
{
  *Entry = (textlist *) rmalloc(sizeof(textlist));
  (*Entry)->text = xstrdup(string);
  (*Entry)->index = n;
  (*Entry)->next = NULL;
}

PUBLIC textlist *ClearTextListEntry(textlist *entry)
{
  textlist *ret = entry->next;
  strfree(entry->text);
  rfree(entry);
  return ret;
}

PUBLIC void ClearTextList(textlist *head)
{
  textlist *cur;

  for (cur = head; cur != NULL; cur = ClearTextListEntry(cur));
}

/* if which=0 load all messages; if it's (p1+1) load messages only from
   p1; if it's -(p1+1) load all messages EXCEPT those from p1. */
PRIVATE int SaveThisMsg (int which, char *line)
{
  char Sender[MAX_LOGIN_NAME];
  int p1;

  if (which == 0) return 1;

  sscanf (line, "%s", Sender);
  if (which < 0) {
    p1 = -which - 1;
    return strcmp(Sender, parray[p1].name);
  }
  else {
    p1 = which - 1;
    return !strcmp(Sender, parray[p1].name);
  }
}

PRIVATE int LoadMsgs(int p, int which, textlist **Head)
  /* which=0 to load all messages; it's (p1+1) to load messages only from
     p1, and it's -(p1+1) to load all messages EXCEPT those from p1. */
{
  FILE *fp;
  textlist **Cur = Head;
  char fName[MAX_FILENAME_SIZE];
  char line[MAX_LINE_SIZE];
  int n=0, nSave=0;

  *Head = NULL;
  GetMsgFile (p, fName);
  fp = fopen(fName, "r");
  if (fp == NULL) {
    return -1;
  }
  while (!feof(fp)) {
    fgets(line, MAX_LINE_SIZE, fp);
    if (feof(fp))
      break;
    if (SaveThisMsg(which, line)) {
      SaveTextListEntry(Cur, line, ++n);
      Cur = &(*Cur)->next;
      nSave++;
    }
    else n++;
  }
  fclose (fp);
  return nSave;
}

/* start > 0 and end > start (or end = 0) to save messages in range;
   start < 0 and end < start (or end = 0) to clear messages in range;
   if end = 0, range goes to end of file (not tested yet). */
PRIVATE int LoadMsgRange(int p, int start, int end, textlist **Head)
{
  FILE *fp;
  char fName[MAX_FILENAME_SIZE];
  char line[MAX_LINE_SIZE];
  textlist **Cur = Head;
  int n=1, nSave=0, nKill=0;

  *Head = NULL;
  GetMsgFile (p, fName);
  fp = fopen  (fName, "r");
  if (fp == NULL) {
    pprintf (p, "You have no messages.\n");
    return -1;
  }
  for (n=1; n <= end || end <= 0; n++) {
    fgets (line, MAX_LINE_SIZE, fp);
    if (feof(fp))
      break;
    if ((start < 0 && (n < -start || n > -end))
        || (start >= 0 && n >= start)) {
      SaveTextListEntry (Cur, line, n);
      Cur = &(*Cur)->next;
      nSave++;
    }
    else nKill++;
  }
  fclose (fp);
  if (start < 0) {
    if (n <= -start)
      pprintf (p, "You do not have a message %d.\n", -start);
    return nKill;
  } else {
    if (n <= start)
      pprintf (p, "You do not have a message %d.\n", start);
    return nSave;
  }
}

PRIVATE int WriteMsgFile (int p, textlist *Head)
{
  char fName[MAX_FILENAME_SIZE];
  FILE *fp;
  textlist *Cur;

  GetMsgFile (p, fName);
  fp = fopen(fName, "w");
  if (fp == NULL)
    return 0;
  for (Cur = Head; Cur != NULL; Cur = Cur->next)
    fprintf(fp, "%s", Cur->text);
  fclose(fp);
  return 1;
}

PUBLIC int ClearMsgsBySender(int p, param_list param)
{
  textlist *Head;
  int p1, connected;
  int nFound;

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return -1;

  nFound = LoadMsgs(p, -(p1+1), &Head);
  if (nFound < 0) {
    pprintf(p, "You have no messages.\n");
  } else if (nFound == 0) {
    pprintf(p, "You have no messages from %s.\n", parray[p1].name);
  } else {
    if (WriteMsgFile (p, Head))
      pprintf(p, "Messages from %s cleared.\n", parray[p1].name);
    else {
      pprintf(p, "Problem writing message file; please contact an admin.\n");
      fprintf(stderr, "Problem writing message file for %s.\n", parray[p].name);
    }
    ClearTextList(Head);
  }
  if (!connected)
    player_remove(p1);
  return nFound;
}

PRIVATE void ShowTextList (int p, textlist *Head, int ShowIndex)
{
  textlist *CurMsg;

  if (ShowIndex) {
    for (CurMsg = Head; CurMsg != NULL; CurMsg = CurMsg->next)
      pprintf(p, "%2d. %s", CurMsg->index, CurMsg->text);
  }
  else {
    for (CurMsg = Head; CurMsg != NULL; CurMsg = CurMsg->next)
      pprintf(p, "%s", CurMsg->text);
  }
}

PUBLIC int player_show_messages(int p)
{
  textlist *Head;
  int n;

  n = LoadMsgs (p, 0, &Head);
  if (n <= 0) {
    pprintf (p, "You have no messages.\n");
    return -1;
  } else {
    pprintf (p, "Messages:\n");
    ShowTextList (p, Head, 1);
    ClearTextList (Head);
    return 0;
  }
}

PUBLIC int
ShowMsgsBySender(int p, param_list param)
{
	int		 nFrom, nTo;
	int		 p1, connected;
	textlist	*Head;

	if (!FindPlayer(p, param[0].val.word, &p1, &connected))
		return -1;

	if (!parray[p1].registered) {
		pprintf(p, "Player \"%s\" is unregistered and cannot send or "
		    "receive messages.\n", parray[p1].name);
		return -1; /* no need to disconnect */
	}

	nFrom = nTo = -1;

	if (p != p1) {
		if ((nTo = LoadMsgs(p1, p + 1, &Head)) <= 0) {
			pprintf(p, "%s has no messages from you.\n",
			    parray[p1].name);
		} else {
			pprintf(p, "Messages to %s:\n", parray[p1].name);
			ShowTextList(p, Head, 0);
			ClearTextList(Head);
		}
	}

	if ((nFrom = LoadMsgs(p, p1 + 1, &Head)) <= 0) {
		pprintf(p, "\nYou have no messages from %s.\n",
		    parray[p1].name);
	} else {
		pprintf(p, "Messages from %s:\n", parray[p1].name);
		ShowTextList(p, Head, 1);
		ClearTextList(Head);
	}

	if (!connected)
		player_remove(p1);
	return (nFrom > 0 || nTo > 0);
}

PUBLIC int ShowMsgRange (int p, int start, int end)
{
  textlist *Head;
  int n;

  n = LoadMsgRange (p, start, end, &Head);
  if (n > 0) {
    ShowTextList (p, Head, 1);
    ClearTextList (Head);
  }
  return n;
}

PUBLIC int ClrMsgRange (int p, int start, int end)
{
  textlist *Head;
  int n;

  n = LoadMsgRange (p, -start, -end, &Head);
  if (n > 0) {
/* You can use ShowTextList to see what's left after the real code. */
/*  ShowTextList (p, Head, 1);  */
    if (WriteMsgFile (p, Head))
      pprintf (p, "Message %d cleared.\n", start);
  }
  if (n >= 0)
    ClearTextList (Head);
  return n;
}

PUBLIC int player_clear_messages(int p)
{
  char fname[MAX_FILENAME_SIZE];

  if (!parray[p].registered)
    return -1;
  GetMsgFile (p, fname);
  unlink(fname);
  return 0;
}

PUBLIC int player_search(int p, char *name)
/*
 * Find player matching the given string. First looks for exact match
 *  with a logged in player, then an exact match with a registered player,
 *  then a partial unique match with a logged in player, then a partial
 *  match with a registered player.
 *  Returns player number if the player is connected, negative (player number)
 *  if the player had to be connected, and 0 if no player was found
 */
{
  int p1, count;
  char *buffer[1000];
  char pdir[MAX_FILENAME_SIZE];

  /* exact match with connected player? */
  if ((p1 = player_find_bylogin(name)) >= 0) {
    return p1 + 1;
  }
  /* exact match with registered player? */
  sprintf(pdir, "%s/%c", player_dir, name[0]);
  count = search_directory(pdir, name, buffer, 1000);
  if (count > 0 && !strcmp(name, *buffer)) {
    goto ReadPlayerFromFile;	/* found an unconnected registered player */
  }
  /* partial match with connected player? */
  if ((p1 = player_find_part_login(name)) >= 0) {
    return p1 + 1;
  } else if (p1 == -2) {
    /* ambiguous; matches too many connected players. */
    pprintf (p, "Ambiguous name '%s'; matches more than one player.\n", name);
    return 0;
  }
  /* partial match with registered player? */
  if (count < 1) {
    pprintf(p, "There is no player matching that name.\n");
    return 0;
  }
  if (count > 1) {
    pprintf(p, "-- Matches: %d names --", count);
    display_directory(p, buffer, count);
    return(0);
  }
ReadPlayerFromFile:
  p1 = player_new();
  if (player_read(p1, *buffer)) {
    player_remove(p1);
    pprintf(p, "ERROR: a player named %s was expected but not found!\n",
	    *buffer);
    pprintf(p, "Please tell an admin about this incident. Thank you.\n");
    return 0;
  }
  return (-p1) - 1;		/* negative to indicate player was not
				   connected */
}


PUBLIC int player_kill(char *name)
{
  char fname[MAX_FILENAME_SIZE], fname2[MAX_FILENAME_SIZE];

  sprintf(fname, "%s/%c/%s", player_dir, name[0], name);
  sprintf(fname2, "%s/%c/.rem.%s", player_dir, name[0], name);
  rename(fname, fname2);
  RemHist (name);
  sprintf(fname, "%s/player_data/%c/%s.games", stats_dir, name[0], name);
  sprintf(fname2, "%s/player_data/%c/.rem.%s.games", stats_dir, name[0], name);
  rename(fname, fname2);
  sprintf(fname, "%s/player_data/%c/%s.comments", stats_dir, name[0], name);
  sprintf(fname2, "%s/player_data/%c/.rem.%s.comments", stats_dir, name[0], name);
  rename(fname, fname2);

  sprintf(fname, "%s/player_data/%c/%s.logons", stats_dir, name[0], name);
  sprintf(fname2, "%s/player_data/%c/.rem.%s.logons", stats_dir, name[0], name);
  rename(fname, fname2);
  sprintf(fname, "%s/player_data/%c/%s.messages", stats_dir, name[0], name);
  sprintf(fname2, "%s/player_data/%c/.rem.%s.messages", stats_dir, name[0], name);
  rename(fname, fname2);
  return 0;
}

PUBLIC int player_rename(char *name, char *newname)
{
  char fname[MAX_FILENAME_SIZE], fname2[MAX_FILENAME_SIZE];

  sprintf(fname, "%s/%c/%s", player_dir, name[0], name);
  sprintf(fname2, "%s/%c/%s", player_dir, newname[0], newname);
  rename(fname, fname2);
  sprintf(fname, "%s/player_data/%c/%s.games", stats_dir, name[0], name);
  sprintf(fname2, "%s/player_data/%c/%s.games", stats_dir, newname[0], newname);
  rename(fname, fname2);
  sprintf(fname, "%s/player_data/%c/%s.comments", stats_dir, name[0], name);
  sprintf(fname2, "%s/player_data/%c/%s.comments", stats_dir, newname[0], newname);
  rename(fname, fname2);
  sprintf(fname, "%s/player_data/%c/%s.logons", stats_dir, name[0], name);
  sprintf(fname2, "%s/player_data/%c/%s.logons", stats_dir, newname[0], newname);
  rename(fname, fname2);
  sprintf(fname, "%s/player_data/%c/%s.messages", stats_dir, name[0], name);
  sprintf(fname2, "%s/player_data/%c/%s.messages", stats_dir, newname[0], newname);
  rename(fname, fname2);
  return 0;
}

PUBLIC int player_raise(char *name)
{
  char fname[MAX_FILENAME_SIZE], fname2[MAX_FILENAME_SIZE];

  sprintf(fname, "%s/%c/%s", player_dir, name[0], name);
  sprintf(fname2, "%s/%c/.rem.%s", player_dir, name[0], name);
  rename(fname2, fname);
  sprintf(fname, "%s/player_data/%c/%s.games", stats_dir, name[0], name);
  sprintf(fname2, "%s/player_data/%c/.rem.%s.games", stats_dir, name[0], name);
  rename(fname2, fname);
  sprintf(fname, "%s/player_data/%c/%s.comments", stats_dir, name[0], name);
  sprintf(fname2, "%s/player_data/%c/.rem.%s.comments", stats_dir, name[0], name);
  rename(fname2, fname);
  sprintf(fname, "%s/player_data/%c/%s.logons", stats_dir, name[0], name);
  sprintf(fname2, "%s/player_data/%c/.rem.%s.logons", stats_dir, name[0], name);
  rename(fname2, fname);
  sprintf(fname, "%s/player_data/%c/%s.messages", stats_dir, name[0], name);
  sprintf(fname2, "%s/player_data/%c/.rem.%s.messages", stats_dir, name[0], name);
  rename(fname2, fname);
  return 0;
}

PUBLIC int
player_reincarn(char *name, char *newname)
{
	char	fname[MAX_FILENAME_SIZE];
	char	fname2[MAX_FILENAME_SIZE];

	snprintf(fname, sizeof fname, "%s/%c/%s", player_dir,
	    newname[0], newname);
	snprintf(fname2, sizeof fname2, "%s/%c/.rem.%s", player_dir,
	    name[0], name);
	rename(fname2, fname);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.games",
	    stats_dir, newname[0], newname);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.games",
	    stats_dir, name[0], name);
	rename(fname2, fname);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.comments",
	    stats_dir, newname[0], newname);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.comments",
	    stats_dir, name[0], name);
	rename(fname2, fname);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.logons",
	    stats_dir, newname[0], newname);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.logons",
	    stats_dir, name[0], name);
	rename(fname2, fname);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.messages",
	    stats_dir, newname[0], newname);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.messages",
	    stats_dir, name[0], name);
	rename(fname2, fname);

	return 0;
}

PUBLIC int
player_num_comments(int p)
{
	char fname[MAX_FILENAME_SIZE];

	if (!parray[p].registered)
		return 0;
	sprintf(fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0],
	    parray[p].login, "comments");
	return lines_file(fname);
}

PUBLIC int
player_add_comment(int p_by, int p_to, char *comment)
{
	FILE	*fp;
	char	 fname[MAX_FILENAME_SIZE];
	time_t	 t = time(NULL);

	if (!parray[p_to].registered)
		return -1;

	sprintf(fname, "%s/player_data/%c/%s.%s", stats_dir,
	    parray[p_to].login[0], parray[p_to].login, "comments");

	if ((fp = fopen(fname, "a")) == NULL)
		return -1;

	fprintf(fp, "%s at %s: %s\n", parray[p_by].name, strltime(&t), comment);
	fclose(fp);
	parray[p_to].num_comments = player_num_comments(p_to);
	return 0;
}

PUBLIC int
player_show_comments(int p, int p1)
{
	char fname[MAX_FILENAME_SIZE];

	sprintf(fname, "%s/player_data/%c/%s.%s", stats_dir,
	    parray[p1].login[0], parray[p1].login, "comments");
	psend_file(p, NULL, fname);
	return 0;
}

/*
 * Returns 1 if player is head admin and 0 otherwise.
 */
PUBLIC int
player_ishead(int p)
{
	return (!strcasecmp(parray[p].name, hadmin_handle));
}

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
   Markus Uhlin                 23/12/31	Completed reformation of all
						functions and much more...
   Markus Uhlin                 24/03/16	Replaced unbounded string
						handling functions and added
						truncation checks.
   Markus Uhlin                 24/08/04	Fixed multiple possible buffer
						overflows.
   Markus Uhlin                 24/08/13	Handled function return values
   Markus Uhlin                 24/11/24	Fixed incorrect format strings
   Markus Uhlin                 24/12/02	Made many improvements
   Markus Uhlin                 24/12/04	Added player number checks
   Markus Uhlin                 25/02/11	Calc string length once
   Markus Uhlin                 25/03/22	Fixed overflowed return value in
						player_search().
   Markus Uhlin                 25/03/23	Fixed overflowed array index
						read/write.
   Markus Uhlin                 25/03/29	player_remove_request:
						fixed overflowed array index
						read/write.
   Markus Uhlin                 25/04/02	add_to_list: added an upper
						limit for the list size.
   Markus Uhlin                 25/04/06	Fixed Clang Tidy warnings.
   Markus Uhlin                 25/07/28	Restricted file permissions upon
						creation.
   Markus Uhlin                 25/07/30	Usage of 'int64_t'.
   Markus Uhlin                 25/11/02	Added overflow checks for array
						indices.
*/

#include "stdinclude.h"
#include "common.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>

#include "command.h"
#include "comproc.h"
#include "config.h"
#include "ficslim.h"
#include "ficsmain.h"
#include "gamedb.h"
#include "lists.h"
#include "maxxes-utils.h"
#include "network.h"
#include "playerdb.h"
#include "ratings.h"
#include "rmalloc.h"
#include "settings.h"
#include "talkproc.h"
#include "utils.h"

#if __linux__
#include <bsd/string.h>
#endif

PUBLIC player	 parray[PARRAY_SIZE];
PUBLIC int	 p_num = 0;

/*
 * Checks if a player number is within bounds.
 */
PUBLIC bool
player_num_ok_chk(const int num)
{
	return (num >= 0 && num <= p_num &&
	    num < (int)ARRAY_SIZE(parray));
}

PUBLIC void
xrename(const char *fn, const char *name1, const char *name2)
{
	if (fn == NULL || name1 == NULL || name2 == NULL) {
		errno = EINVAL;
		warn("%s", __func__);
		return;
	}

	errno = 0;

	if (rename(name1, name2) != 0)
		warn("%s: '%s' -> '%s'", fn, name1, name2);
}

PRIVATE int
get_empty_slot(void)
{
	for (int i = 0; i < p_num; i++) {
		if (parray[i].status == PLAYER_EMPTY)
			return i;
	}

	p_num++;

	if ((p_num + 1) >= PARRAY_SIZE) {
		fprintf(stderr, "*** Bogus attempt to %s() past end of parray "
		    "***\n", __func__);
	}

	parray[p_num - 1].status = PLAYER_EMPTY;

	return (p_num - 1);
}

PUBLIC void
player_array_init(void)
{
	for (int i = 0; i < PARRAY_SIZE; i++)
		parray[i].status = PLAYER_EMPTY;
}

PUBLIC void
player_init(int startConsole)
{
	int p;

	if (startConsole) {
		net_addConnection(0, 0);
		p = player_new();

		parray[p].login		= xstrdup("console");
		parray[p].name		= xstrdup("console");
		parray[p].passwd	= xstrdup("*");
		parray[p].fullName	= xstrdup("The Operator");
		parray[p].emailAddress	= NULL;
		parray[p].prompt	= xstrdup("fics%");
		parray[p].adminLevel	= ADMIN_GOD;
		parray[p].socket	= 0;
		parray[p].busy[0]	= '\0';

		pprintf_prompt(p, "\nLogged in on console.\n");
	}
}

PUBLIC int
player_new(void)
{
	int new;

	new = get_empty_slot();
	player_zero(new);
	return new;
}

PUBLIC int
player_zero(int p)
{
#define INVALID ((char *)-42)
	int i;

	parray[p].name		= NULL;
	parray[p].emailAddress	= NULL;
	parray[p].fullName	= NULL;
	parray[p].passwd	= NULL;
	parray[p].prompt	= def_prompt;

	parray[p].adminLevel = 0;
	parray[p].automail = 0;

	for (i = 0; i < MAX_ALIASES; i++) {
		parray[p].alias_list[i].comm_name = INVALID;
		parray[p].alias_list[i].alias = INVALID;
	}

	parray[p].bell		= 0;
	parray[p].d_height	= 24;
	parray[p].d_inc		= 12;
	parray[p].d_time	= 2;
	parray[p].d_width	= 79;

	parray[p].flip		= 0;
	parray[p].formula	= NULL;

	for (i = 0; i < MAX_FORMULA; i++)
		parray[p].formulaLines[i] = NULL;

	parray[p].game		= -1;
	parray[p].highlight	= 0;
	parray[p].i_admin	= 1;
	parray[p].i_cshout	= 1;
	parray[p].i_game	= 0;
	parray[p].i_kibitz	= 1;
	parray[p].i_login	= 0;
	parray[p].i_mailmess	= 0;
	parray[p].i_shout	= 1;
	parray[p].i_tell	= 1;
	parray[p].jprivate	= 0;
	parray[p].kiblevel	= 0;
	parray[p].language	= LANG_DEFAULT;

	parray[p].lastColor		= WHITE;
	parray[p].lastHost		= 0;
	parray[p].last_channel		= -1;
	parray[p].last_command_time	= 0;
	parray[p].last_file		= NULL;
	parray[p].last_file_byte	= 0L;
	parray[p].last_opponent		= -1;
	parray[p].last_tell		= -1;
	parray[p].lastshout_a		= 0;
	parray[p].lastshout_b		= 0;

	parray[p].lists		= NULL;
	parray[p].login		= NULL;
	parray[p].logon_time	= 0;
	parray[p].notifiedby	= 0;
	parray[p].numAlias	= 0;
	parray[p].num_black	= 0;
	parray[p].num_comments	= 0;
	parray[p].num_formula	= 0;
	parray[p].num_from	= 0;
	parray[p].num_observe	= 0;
	parray[p].num_plan	= 0;
	parray[p].num_to	= 0;
	parray[p].num_white	= 0;
	parray[p].open		= 1;
	parray[p].opponent	= -1;

	parray[p].partner = -1;
	parray[p].pgn = 0;
	for (i = 0; i < MAX_PLAN; i++)
		parray[p].planLines[i] = INVALID;
	parray[p].private = 0;
	parray[p].promote = QUEEN;

	parray[p].rated			= 0;
	parray[p].registered		= 0;
	parray[p].ropen			= 1;
	parray[p].seek			= 0;
	parray[p].simul_info.numBoards	= 0;
	parray[p].socket		= -1;
	parray[p].sopen			= 0;
	parray[p].status		= PLAYER_NEW;
	parray[p].style			= 0;
	parray[p].thisHost		= 0;
	parray[p].timeOfReg		= 0;
	parray[p].totalTime		= 0;

	parray[p].b_stats.best		= 0;
	parray[p].b_stats.dra		= 0;
	parray[p].b_stats.los		= 0;
	parray[p].b_stats.ltime		= 0;
	parray[p].b_stats.num		= 0;
	parray[p].b_stats.rating	= 0;
	parray[p].b_stats.sterr		= 350.0;
	parray[p].b_stats.whenbest	= 0;
	parray[p].b_stats.win = 0;

	parray[p].bug_stats.best	= 0;
	parray[p].bug_stats.dra		= 0;
	parray[p].bug_stats.los		= 0;
	parray[p].bug_stats.ltime	= 0;
	parray[p].bug_stats.num		= 0;
	parray[p].bug_stats.rating	= 0;
	parray[p].bug_stats.sterr	= 350.0;
	parray[p].bug_stats.whenbest	= 0;
	parray[p].bug_stats.win		= 0;

	parray[p].l_stats.best		= 0;
	parray[p].l_stats.dra		= 0;
	parray[p].l_stats.los		= 0;
	parray[p].l_stats.ltime		= 0;
	parray[p].l_stats.num		= 0;
	parray[p].l_stats.rating	= 0;
	parray[p].l_stats.sterr		= 350.0;
	parray[p].l_stats.whenbest	= 0;
	parray[p].l_stats.win		= 0;

	parray[p].s_stats.best		= 0;
	parray[p].s_stats.dra		= 0;
	parray[p].s_stats.los		= 0;
	parray[p].s_stats.ltime		= 0;
	parray[p].s_stats.num		= 0;
	parray[p].s_stats.rating	= 0;
	parray[p].s_stats.sterr		= 350.0;
	parray[p].s_stats.whenbest	= 0;
	parray[p].s_stats.win		= 0;

	parray[p].w_stats.best		= 0;
	parray[p].w_stats.dra		= 0;
	parray[p].w_stats.los		= 0;
	parray[p].w_stats.ltime		= 0;
	parray[p].w_stats.num		= 0;
	parray[p].w_stats.rating	= 0;
	parray[p].w_stats.sterr		= 350.0;
	parray[p].w_stats.whenbest	= 0;
	parray[p].w_stats.win		= 0;

	// Used to store the player's interface.
	// For example: xboard 4.9.1
	(void) memset(&(parray[p].interface[0]), 0,
	    ARRAY_SIZE(parray[p].interface));

	return 0;
}

PUBLIC int
player_free(int p)
{
	int i;

	strfree(parray[p].login);
	strfree(parray[p].name);
	strfree(parray[p].passwd);
	strfree(parray[p].fullName);
	strfree(parray[p].emailAddress);

	if (parray[p].prompt != def_prompt)
		strfree(parray[p].prompt);

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

	return 0;
}

PUBLIC int
player_clear(int p)
{
	player_free(p);
	player_zero(p);
	return 0;
}

PUBLIC int
player_remove(int p)
{
	int i;

	if (!player_num_ok_chk(p)) {
		warnx("%s: invalid player number %d", __func__, p);
		return -1;
	}

	player_decline_offers(p, -1, -1);
	player_withdraw_offers(p, -1, -1);

	if (parray[p].simul_info.numBoards) {	// Player disconnected in
						// middle of simul
		for (i = 0; i < parray[p].simul_info.numBoards; i++) {
			if (parray[p].simul_info.boards[i] >= 0) {
				game_disconnect(parray[p].simul_info.boards[i],
				    p);
			}
		}
	}

	if (parray[p].game >= 0) {	// Player disconnected in the middle of
					// a game!
		pprintf(parray[p].opponent, "Your opponent has lost contact "
		    "or quit.\n");
		game_disconnect(parray[p].game, p);
	}

	for (i = 0; i < p_num; i++) {
		if (parray[i].status == PLAYER_EMPTY)
			continue;

		if (parray[i].last_tell == p)
			parray[i].last_tell = -1;

		if (parray[i].last_opponent == p)
			parray[i].last_opponent = -1;

		if (parray[i].partner == p) {
			pprintf_prompt(i, "Your partner has disconnected.\n");
			player_withdraw_offers(i, -1, PEND_BUGHOUSE);
			player_decline_offers(i, -1, PEND_BUGHOUSE);
			parray[i].partner = -1;
		}
	}

	player_clear(p);
	parray[p].status = PLAYER_EMPTY;
	return 0;
}

PRIVATE int
add_to_list(FILE *fp, enum ListWhich lw, int *size, int p)
{
	char buf[MAX_STRING_LENGTH] = { '\0' };

	_Static_assert(1023 < ARRAY_SIZE(buf), "Buffer too small");

#define SCAN_STR "%1023s"

	if (*size <= 0 || *size > MAX_GLOBAL_LIST_SIZE)
		return -2;

	while ((*size)-- > 0 && fscanf(fp, SCAN_STR, buf) == 1)
		list_add(p, lw, buf);

	return (*size <= 0 ? 0 : -1);
}

PRIVATE void
ReadV1PlayerFmt(int p, player *pp, FILE *fp, char *file, int version)
{
	char		*tmp;
	char		 tmp2[MAX_STRING_LENGTH] = { '\0' };
	int		 bs, ss, ws, ls, bugs;
	int		 i, size_cens, size_noplay, size_not, size_gnot,
			 size_chan, len;
	intmax_t	 array[2] = { 0 };
	intmax_t	 ltime_tmp[5] = { 0 };
	intmax_t	 wb_tmp[5] = { 0 };
	size_t		 n;

	/* XXX: not referenced */
	(void) version;

	/*
	 * Name
	 */
	if (fgets(tmp2, sizeof tmp2, fp) != NULL && // NOLINT
	    strcmp(tmp2, "NONE\n") != 0) {
		tmp2[strcspn(tmp2, "\n")] = '\0';
		pp->name = xstrdup(tmp2);
	} else {
		pp->name = NULL;
	}

	/*
	 * Full name
	 */
	if (fgets(tmp2, sizeof tmp2, fp) != NULL && // NOLINT
	     strcmp(tmp2, "NONE\n") != 0) {
		tmp2[strcspn(tmp2, "\n")] = '\0';
		pp->fullName = xstrdup(tmp2);
	} else {
		pp->fullName = NULL;
	}

	/*
	 * Password
	 */
	if (fgets(tmp2, sizeof tmp2, fp) != NULL && // NOLINT
	    strcmp(tmp2, "NONE\n") != 0) {
		tmp2[strcspn(tmp2, "\n")] = '\0';
		pp->passwd = xstrdup(tmp2);
	} else {
		pp->passwd = NULL;
	}

	/*
	 * Email
	 */
	if (fgets(tmp2, sizeof tmp2, fp) != NULL && // NOLINT
	    strcmp(tmp2, "NONE\n") != 0) {
		tmp2[strcspn(tmp2, "\n")] = '\0';
		pp->emailAddress = xstrdup(tmp2);
	} else {
		pp->emailAddress = NULL;
	}

	if (feof(fp) ||
	    ferror(fp) ||
	    fscanf(fp, "%d %d %d %d %d %d %jd %d %jd %d %d %d %d %d %d %jd %d %jd "
	    "%d %d %d %d %d %d %jd %d %jd %d %d %d %d %d %d %jd %d %jd %d %d %d %d "
	    "%d %d %jd %d %jd %u\n",
	    &pp->s_stats.num, &pp->s_stats.win, &pp->s_stats.los,
	    &pp->s_stats.dra, &pp->s_stats.rating, &ss,
	    &ltime_tmp[0], &pp->s_stats.best, &wb_tmp[0],

	    &pp->b_stats.num, &pp->b_stats.win, &pp->b_stats.los,
	    &pp->b_stats.dra, &pp->b_stats.rating, &bs,
	    &ltime_tmp[1], &pp->b_stats.best, &wb_tmp[1],

	    &pp->w_stats.num, &pp->w_stats.win, &pp->w_stats.los,
	    &pp->w_stats.dra, &pp->w_stats.rating, &ws,
	    &ltime_tmp[2], &pp->w_stats.best, &wb_tmp[2],

	    &pp->l_stats.num, &pp->l_stats.win, &pp->l_stats.los,
	    &pp->l_stats.dra, &pp->l_stats.rating, &ls,
	    &ltime_tmp[3], &pp->l_stats.best, &wb_tmp[3],

	    &pp->bug_stats.num, &pp->bug_stats.win, &pp->bug_stats.los,
	    &pp->bug_stats.dra, &pp->bug_stats.rating, &bugs,
	    &ltime_tmp[4], &pp->bug_stats.best, &wb_tmp[4],

	    &pp->lastHost) != 46) {
		fprintf(stderr, "Player %s is corrupt\n", parray[p].name);
		return;
	}

	for (n = 0; n < ARRAY_SIZE(ltime_tmp); n++) {
		if (ltime_tmp[n] < g_time_min ||
		    ltime_tmp[n] > g_time_max) {
			warnx("%s: player %s is corrupt "
			    "('ltime' out of bounds!)",
			    __func__, parray[p].name);
			return;
		}
	}

	pp->s_stats.ltime = ltime_tmp[0];
	pp->b_stats.ltime = ltime_tmp[1];
	pp->w_stats.ltime = ltime_tmp[2];
	pp->l_stats.ltime = ltime_tmp[3];
	pp->bug_stats.ltime = ltime_tmp[4];

	for (n = 0; n < ARRAY_SIZE(wb_tmp); n++) {
		if (wb_tmp[n] < g_time_min ||
		    wb_tmp[n] > g_time_max) {
			warnx("%s: player %s is corrupt "
			    "('whenbest' out of bounds!)",
			    __func__, parray[p].name);
			return;
		}
	}

	pp->s_stats.whenbest = wb_tmp[0];
	pp->b_stats.whenbest = wb_tmp[1];
	pp->w_stats.whenbest = wb_tmp[2];
	pp->l_stats.whenbest = wb_tmp[3];
	pp->bug_stats.whenbest = wb_tmp[4];

	pp->b_stats.sterr	= (bs / 10.0);
	pp->s_stats.sterr	= (ss / 10.0);
	pp->w_stats.sterr	= (ws / 10.0);
	pp->l_stats.sterr	= (ls / 10.0);
	pp->bug_stats.sterr	= (bugs / 10.0);

	if (fgets(tmp2, sizeof tmp2, fp) == NULL) {
		fprintf(stderr, "Player %s is corrupt\n", parray[p].name);
		return;
	} else {
		tmp2[strcspn(tmp2, "\n")] = '\0';
		pp->prompt = xstrdup(tmp2);
	}

	if (fscanf(fp, "%d %d %d %jd %jd %d %d %d %d %d %d %d %d %d %d %d %d %d "
	    "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
	    &pp->open, &pp->rated, &pp->ropen, &array[0], &array[1],
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

	for (n = 0; n < ARRAY_SIZE(array); n++) {
		if (array[n] < g_time_min ||
		    array[n] > g_time_max) {
			warnx("%s: player %s is corrupt "
			    "(time not within range)",
			    __func__, parray[p].name);
			return;
		}
	}

	pp->timeOfReg = array[0];
	pp->totalTime = array[1];

	if (pp->num_plan >= MAX_PLAN) {
		warnx("Player %s is corrupt\nToo many plans (%d)",
		   parray[p].name,
		   pp->num_plan);
		return;
	} else if (pp->num_formula >= MAX_FORMULA) {
		warnx("Player %s is corrupt\nToo many formulas (%d)",
		   parray[p].name,
		   pp->num_formula);
		return;
	} else if (pp->numAlias >= MAX_ALIASES) {
		warnx("Player %s is corrupt\nToo many aliases (%d)",
		    parray[p].name,
		    pp->numAlias);
		return;
	}

	if (pp->num_plan > 0) {
		for (i = 0; i < pp->num_plan; i++) {
			if (fgets(tmp2, sizeof tmp2, fp) == NULL) {
				warnx("%s: bad plan: feof %s", __func__, file);
				return;
			}

			if (!(len = strlen(tmp2))) {
				fprintf(stderr, "FICS: Error bad plan in "
				    "file %s\n", file);
				i--;
				pp->num_plan--;
			} else {
				// Get rid of '\n'.
				tmp2[strcspn(tmp2, "\n")] = '\0';

				pp->planLines[i] = (len > 1 ? xstrdup(tmp2) :
				    NULL);
			}
		}
	}

	if (pp->num_formula > 0) {
		for (i = 0; i < pp->num_formula; i++) {
			if (fgets(tmp2, sizeof tmp2, fp) == NULL) {
				warnx("%s: bad formula: feof %s", __func__,
				    file);
				return;
			}

			if (!(len = strlen(tmp2))) {
				fprintf(stderr, "FICS: Error bad formula in "
				    "file %s\n", file);
				i--;
				pp->num_formula--;
			} else {
				// Get rid of '\n'.
				tmp2[strcspn(tmp2, "\n")] = '\0';

				pp->formulaLines[i] = (len > 1 ? xstrdup(tmp2) :
				    NULL);
			}
		}
	}

	if (fgets(tmp2, sizeof tmp2, fp) == NULL) {
		warnx("%s: fgets() error", __func__);
		return;
	}

	tmp2[strcspn(tmp2, "\n")] = '\0';

	if (!strcmp(tmp2, "NONE"))
		pp->formula = NULL;
	else
		pp->formula = xstrdup(tmp2);

	if (pp->numAlias > 0) {
		for (i = 0; i < pp->numAlias; i++) {
			if (fgets(tmp2, sizeof tmp2, fp) == NULL) {
				warnx("%s: bad alias: feof %s", __func__, file);
				return;
			}

			if (!strlen(tmp2)) { // XXX
				fprintf(stderr, "FICS: Error bad alias in "
				    "file %s\n", file);
				i--;
				pp->numAlias--;
			} else {
				tmp2[strcspn(tmp2, "\n")] = '\0';
				tmp = eatword(tmp2);
				*tmp = '\0';
				tmp++;
				tmp = eatwhite(tmp);

				pp->alias_list[i].comm_name = xstrdup(tmp2);
				pp->alias_list[i].alias = xstrdup(tmp);
			}
		}
	}

	if (add_to_list(fp, L_CENSOR, &size_cens, p) == -1) {
		warnx("%s: add to list error (L_CENSOR): player: %s",
		    __func__, parray[p].name);
	} else if (add_to_list(fp, L_NOTIFY, &size_not, p) == -1) {
		warnx("%s: add to list error (L_NOTIFY): player: %s",
		    __func__, parray[p].name);
	} else if (add_to_list(fp, L_NOPLAY, &size_noplay, p) == -1) {
		warnx("%s: add to list error (L_NOPLAY): player: %s",
		    __func__, parray[p].name);
	} else if (add_to_list(fp, L_GNOTIFY, &size_gnot, p) == -1) {
		warnx("%s: add to list error (L_GNOTIFY): player: %s",
		    __func__, parray[p].name);
	} else if (add_to_list(fp, L_CHANNEL, &size_chan, p) == -1) {
		warnx("%s: add to list error (L_CHANNEL): player: %s",
		    __func__, parray[p].name);
	}
}

PRIVATE int
got_attr_value_player(int p, char *attr, char *value, FILE *fp, char *file)
{
	char	*tmp1;
	char	 tmp[MAX_LINE_SIZE] = { '\0' };
	int	 i, len;

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
		/* parray[p].i_admin = atoi(value) */;
	} else if (!strcmp(attr, "computer:")) {
		/* parray[p].computer = atoi(value) */;
	} else if (!strcmp(attr, "black_games:")) {
		parray[p].num_black = atoi(value);
	} else if (!strcmp(attr, "white_games:")) {
		parray[p].num_white = atoi(value);
	} else if (!strcmp(attr, "uscf:")) {
		/* parray[p].uscfRating = atoi(value) */;
	} else if (!strcmp(attr, "muzzled:")) {
		/* obsolete */;
	} else if (!strcmp(attr, "cmuzzled:")) {
		/* obsolete */;
	} else if (!strcmp(attr, "highlight:")) {
		parray[p].highlight = atoi(value);
	} else if (!strcmp(attr, "network:")) {
		/* parray[p].network_player = atoi(value) */;
	} else if (!strcmp(attr, "lasthost:")) {
		parray[p].lastHost = atoi(value);
	} else if (!strcmp(attr, "channel:")) {
		list_addsub(p, "channel", value, 1);
	} else if (!strcmp(attr, "num_comments:")) {
		parray[p].num_comments = atoi(value);
	} else if (!strcmp(attr, "num_plan:")) {
		/*
		 * num_plan
		 */

		if ((parray[p].num_plan = atoi(value)) >= MAX_PLAN) {
			warnx("%s: %s: too many plans (%d)", __func__, file,
			    parray[p].num_plan);
			return -1;
		} else if (parray[p].num_plan > 0) {
			for (i = 0; i < parray[p].num_plan; i++) {

				if (fgets(tmp, sizeof tmp, fp) == NULL) {
					warnx("%s: bad plan: feof %s",
					    __func__, file);
					return -1;
				}

				if (!(len = strlen(tmp))) {
					fprintf(stderr, "FICS: Error bad plan "
					    "in file %s\n", file);
					i--;
					parray[p].num_plan--;
				} else {
					// Get rid of '\n'.
					tmp[strcspn(tmp, "\n")] = '\0';

					parray[p].planLines[i] = (len > 1 ?
					    xstrdup(tmp) : NULL);
				}
			}
		} else {
			/* null */;
		}
	} else if (!strcmp(attr, "num_formula:")) {
		/*
		 * num_formula
		 */

		if ((parray[p].num_formula = atoi(value)) >= MAX_FORMULA) {
			warnx("%s: %s: too many formulas (%d)", __func__, file,
			    parray[p].num_formula);
			return -1;
		} else if (parray[p].num_formula > 0) {
			for (i = 0; i < parray[p].num_formula; i++) {
				if (fgets(tmp, sizeof tmp, fp) == NULL) {
					warnx("%s: bad formula: feof %s",
					    __func__, file);
					return -1;
				}

				if (!(len = strlen(tmp))) {
					fprintf(stderr, "FICS: Error bad "
					    "formula in file %s\n", file);
					i--;
					parray[p].num_formula--;
				} else {
					// Get rid of '\n'.
					tmp[strcspn(tmp, "\n")] = '\0';

					parray[p].formulaLines[i] = (len > 1 ?
					    xstrdup(tmp) : NULL);
				}
			}
		} else {
			/* null */;
		}
	} else if (!strcmp(attr, "formula:")) {
		/*
		 * formula
		 */

		parray[p].formula = xstrdup(value);
	} else if (!strcmp(attr, "num_alias:")) {
		/*
		 * num_alias
		 */

		if ((parray[p].numAlias = atoi(value)) >= MAX_ALIASES) {
			warnx("%s: %s: too many aliases (%d)", __func__, file,
			    parray[p].numAlias);
			return -1;
		} else if (parray[p].numAlias > 0) {
			for (i = 0; i < parray[p].numAlias; i++) {
				if (fgets(tmp, sizeof tmp, fp) == NULL) {
					warnx("%s: bad alias: feof %s",
					    __func__, file);
					return -1;
				}

				if (!strlen(tmp)) { // XXX
					fprintf(stderr, "FICS: Error bad alias "
					    "in file %s\n", file);
					i--;
					parray[p].numAlias--;
				} else {
					tmp[strcspn(tmp, "\n")] = '\0';
					tmp1 = tmp;
					tmp1 = eatword(tmp1);
					*tmp1 = '\0';
					tmp1++;
					tmp1 = eatwhite(tmp1);

					parray[p].alias_list[i].comm_name =
					    xstrdup(tmp);
					parray[p].alias_list[i].alias =
					    xstrdup(tmp1);
				}
			}
		} else {
			/* null */;
		}
	} else if (!strcmp(attr, "num_censor:")) {
		/*
		 * num_censor
		 */

		if ((i = atoi(value)) < 0) {
			warnx("%s: num censor negative", __func__);
			return -1;
		} else if (i > MAX_CENSOR) {
			warnx("%s: num censor too large", __func__);
			return -1;
		}

		while (i--) {
			if (fgets(tmp, sizeof tmp, fp) == NULL) {
				warnx("%s: bad censor: feof %s",
				    __func__, file);
				return -1;
			}

			if (!(len = strlen(tmp)) || len == 1) {	// blank lines
								// do occur!
				fprintf(stderr, "FICS: Error bad censor in "
				    "file %s\n", file);
			} else {
				tmp[strcspn(tmp, "\n")] = '\0';
				list_add(p, L_CENSOR, tmp);
			}
		}
	} else if (!strcmp(attr, "num_notify:")) {
		if ((i = atoi(value)) < 0) {
			warnx("%s: num notify negative", __func__);
			return -1;
		} else if (i > MAX_NOTIFY) {
			warnx("%s: num notify too large", __func__);
			return -1;
		}

		while (i--) {
			if (fgets(tmp, sizeof tmp, fp) == NULL) {
				warnx("%s: bad notify: feof %s",
				    __func__, file);
				return -1;
			}

			if (!(len = strlen(tmp)) || len == 1) {	// blank lines
								// do occur!
				fprintf(stderr, "FICS: Error bad notify in "
				    "file %s\n", file);
			} else {
				tmp[strcspn(tmp, "\n")] = '\0';
				list_add(p, L_NOTIFY, tmp);
			}
		}
	} else if (!strcmp(attr, "num_noplay:")) {
		if ((i = atoi(value)) < 0) {
			warnx("%s: num noplay negative", __func__);
			return -1;
		}

		while (i--) {
			if (fgets(tmp, sizeof tmp, fp) == NULL) {
				warnx("%s: bad noplay: feof %s",
				    __func__, file);
				return -1;
			}

			if (!(len = strlen(tmp)) || len == 1) {	// blank lines
								// do occur!
				fprintf(stderr, "FICS: Error bad noplay in "
				    "file %s\n", file);
			} else {
				tmp[strcspn(tmp, "\n")] = '\0';
				list_add(p, L_NOPLAY, tmp);
			}
		}
	} else if (!strcmp(attr, "num_gnotify:")) {
		if ((i = atoi(value)) < 0) {
			warnx("%s: num gnotify negative", __func__);
			return -1;
		}

		while (i--) {
			if (fgets(tmp, sizeof tmp, fp) == NULL) {
				warnx("%s: bad gnotify: feof %s",
				    __func__, file);
				return -1;
			}

			if (!(len = strlen(tmp)) || len == 1) {	// blank lines
								// do occur!
				fprintf(stderr, "FICS: Error bad gnotify in "
				    "file %s\n", file);
			} else {
				tmp[strcspn(tmp, "\n")] = '\0';
				list_add(p, L_GNOTIFY, tmp);
			}
		}
	} else {
		fprintf(stderr, "FICS: Error bad attribute >%s< from file %s\n",
		    attr, file);
	}

	return 0;
}

PUBLIC int
player_read(int p, char *name)
{
	FILE	*fp = NULL;
	char	 fname[MAX_FILENAME_SIZE] = { '\0' };
	char	 line[MAX_LINE_SIZE] = { '\0' };
	char	*attr, *value;
	char	*resolvedPath = NULL;
	int	 version = 0;
	size_t	 len = 0;

	parray[p].login = stolower(xstrdup(name)); // free on error?

	if (!is_valid_login_name(parray[p].login)) {
		warnx("%s: invalid login name: %s", __func__, parray[p].login);
		return -1;
	}

	snprintf(fname, sizeof fname, "%s/%c/%s", player_dir,
	    parray[p].login[0], parray[p].login);

	if ((resolvedPath = realpath(fname, NULL)) != NULL) {
		if (strncmp(resolvedPath, player_dir,
		    strlen(player_dir)) != 0) {
			warnx("%s: path traversal detected", __func__);
			free(resolvedPath);
			return -1;
		}
		mstrlcpy(fname, resolvedPath, sizeof fname);
		free(resolvedPath);
		resolvedPath = NULL;
	}

	if ((fp = fopen(fname, "r")) == NULL) { // Unregistered player
		parray[p].name = xstrdup(name);
		parray[p].registered = 0;
		return -1;
	}

	parray[p].registered = 1; // Lets load the file

	if (fgets(line, sizeof line, fp) == NULL) {	// Ok, so which version
		warnx("%s: fgets() error", __func__);	// file?
		fclose(fp);
		return -1;
	}

	if (line[0] == 'v')
		(void)sscanf(line, "%*c %d", &version);
	if (version > 0)	// Quick method:
		ReadV1PlayerFmt(p, &parray[p], fp, fname, version);
	else {			// Do it the old SLOW way
		do {
			if (feof(fp))
				break;
			if ((len = strlen(line)) <= 1)
				continue;

			line[len - 1] = '\0';
			attr = eatwhite(line);

			if (attr[0] == '#')
				continue; // Comment

			value = eatword(attr);

			if (!*value) {
				fprintf(stderr, "FICS: Error reading file %s\n",
				    fname);
				continue;
			}

			*value = '\0';
			value++;
			value = eatwhite(value);
			stolower(attr);
			got_attr_value_player(p, attr, value, fp, fname);

			if (fgets(line, sizeof line, fp) == NULL)
				break;
		} while (!feof(fp));
	}

	fclose(fp);

	if (version == 0) {
		player_save(p);	// Ensure old files are quickly converted e.g.
				// when someone fingers.
	}

	if (!parray[p].name) {
		parray[p].name = xstrdup(name);
		pprintf(p, "\n*** WARNING: Your Data file is corrupt. "
		    "Please tell an admin ***\n");
	}

	return 0;
}

PUBLIC int
player_delete(int p)
{
	char fname[MAX_FILENAME_SIZE];

	if (!parray[p].registered)	// Player must not be registered
		return -1;

	snprintf(fname, sizeof fname, "%s/%c/%s", player_dir,
	    parray[p].login[0], parray[p].login);
	unlink(fname);

	return 0;
}

PUBLIC int
player_markdeleted(int p)
{
	FILE	*fp;
	char	 fname[MAX_FILENAME_SIZE];
	char	 fname2[MAX_FILENAME_SIZE];
	int	 fd;

	if (!parray[p].registered)	// Player must not be registered
		return -1;

	snprintf(fname, sizeof fname, "%s/%c/%s", player_dir,
	    parray[p].login[0], parray[p].login);
	snprintf(fname2, sizeof fname2, "%s/%c/%s.delete", player_dir,
	    parray[p].login[0], parray[p].login);
	xrename(__func__, fname, fname2);

	if ((fd = open(fname2, g_open_flags[OPFL_APPEND], g_open_modes)) < 0) {
		warn("%s: open", __func__);
		return -1;
	} else if ((fp = fdopen(fd, "a")) != NULL) { // Touch the file
		fprintf(fp, "\n");
		fclose(fp);
	} else {
		close(fd);
	}

	return 0;
}

PRIVATE void
WritePlayerFile(FILE *fp, int p)
{
	int	 i;
	player	*pp = &parray[p];

	fprintf(fp, "v %d\n", PLAYER_VERSION);

	fprintf(fp, "%s\n", (pp->name ? pp->name : "NONE"));
	fprintf(fp, "%s\n", (pp->fullName ? pp->fullName : "NONE"));
	fprintf(fp, "%s\n", (pp->passwd ? pp->passwd : "NONE"));
	fprintf(fp, "%s\n", (pp->emailAddress ? pp->emailAddress : "NONE"));

	fprintf(fp, "%d %d %d %d %d %d %jd %d %jd %d %d %d %d %d %d %jd %d %jd %d "
	    "%d %d %d %d %d %jd %d %jd %d %d %d %d %d %d %jd %d %jd %d %d %d %d %d "
	    "%d %jd %d %jd %u\n",
	    pp->s_stats.num, pp->s_stats.win, pp->s_stats.los,
	    pp->s_stats.dra, pp->s_stats.rating,
	    (int)(pp->s_stats.sterr * 10.0),
	    (intmax_t)pp->s_stats.ltime, pp->s_stats.best,
	    (intmax_t)pp->s_stats.whenbest,

	    pp->b_stats.num, pp->b_stats.win, pp->b_stats.los,
	    pp->b_stats.dra, pp->b_stats.rating,
	    (int)(pp->b_stats.sterr * 10.0),
	    (intmax_t)pp->b_stats.ltime, pp->b_stats.best,
	    (intmax_t)pp->b_stats.whenbest,

	    pp->w_stats.num, pp->w_stats.win, pp->w_stats.los,
	    pp->w_stats.dra, pp->w_stats.rating,
	    (int)(pp->w_stats.sterr * 10.0),
	    (intmax_t)pp->w_stats.ltime, pp->w_stats.best,
	    (intmax_t)pp->w_stats.whenbest,

	    pp->l_stats.num, pp->l_stats.win, pp->l_stats.los,
	    pp->l_stats.dra, pp->l_stats.rating,
	    (int)(pp->l_stats.sterr * 10.0),
	    (intmax_t)pp->l_stats.ltime, pp->l_stats.best,
	    (intmax_t)pp->l_stats.whenbest,

	    pp->bug_stats.num, pp->bug_stats.win, pp->bug_stats.los,
	    pp->bug_stats.dra, pp->bug_stats.rating,
	    (int)(pp->bug_stats.sterr * 10.0),
	    (intmax_t)pp->bug_stats.ltime, pp->bug_stats.best,
	    (intmax_t)pp->bug_stats.whenbest,

	    pp->lastHost); /* fprintf() */

	fprintf(fp, "%s\n", pp->prompt);

	fprintf(fp, "%d %d %d %jd %jd %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
	    "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
	    pp->open, pp->rated, pp->ropen,
	    (intmax_t)pp->timeOfReg,
	    (intmax_t)pp->totalTime,
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

PUBLIC int
player_save(int p)
{
	FILE	*fp;
	char	 fname[MAX_FILENAME_SIZE];
	int	 fd;

	if (!player_num_ok_chk(p)) {
		warnx("%s: invalid player number %d", __func__, p);
		return -1;
	}

	if (!parray[p].registered)	// Player must not be registered
		return -1;

	if (parray[p].name == NULL) {	// Fixes a bug if name is null
		pprintf(p, "WARNING: Your player file could not be updated, "
		    "due to corrupt data.\n");
		return -1;
	}

	if (strcasecmp(parray[p].login, parray[p].name)) {
		pprintf(p, "WARNING: Your player file could not be updated, "
		    "due to corrupt data.\n");
		return -1;
	}

	snprintf(fname, sizeof fname, "%s/%c/%s", player_dir,
	    parray[p].login[0], parray[p].login);

	if ((fd = open(fname, g_open_flags[OPFL_WRITE], g_open_modes)) < 0) {
		warn("%s: Problem opening file %s for write", __func__, fname);
		return -1;
	} else if ((fp = fdopen(fd, "w")) == NULL) {
		warn("%s: Problem opening file %s for write", __func__, fname);
		close(fd);
		return -1;
	}

	WritePlayerFile(fp, p);
	fclose(fp);
	return 0;
}

PUBLIC int
player_find(int fd)
{
	for (int i = 0; i < p_num; i++) {
		if (parray[i].status == PLAYER_EMPTY)
			continue;
		if (parray[i].socket == fd)
			return i;
	}

	return -1;
}

PUBLIC int
player_find_bylogin(char *name)
{
	for (int i = 0; i < p_num; i++) {
		if (parray[i].status == PLAYER_EMPTY ||
		    parray[i].status == PLAYER_LOGIN ||
		    parray[i].status == PLAYER_PASSWORD)
			continue;

		if (!parray[i].login)
			continue;

		if (!strcmp(parray[i].login, name))
			return i;
	}

	return -1;
}

PUBLIC int
player_find_part_login(char *name)
{
	int	found = -1;
	int	i;
	size_t	namelen;

	if ((i = player_find_bylogin(name)) >= 0)
		return i;

	namelen = strlen(name);

	for (i = 0; i < p_num; i++) {
		if (parray[i].status == PLAYER_EMPTY ||
		    parray[i].status == PLAYER_LOGIN ||
		    parray[i].status == PLAYER_PASSWORD)
			continue;

		if (!parray[i].login)
			continue;

		if (!strncmp(parray[i].login, name, namelen)) {
			if (found >= 0) /* Ambiguous */
				return -2;
			found = i;
		}
	}

	return found;
}

PUBLIC int
player_censored(int p, int p1)
{
	if (in_list(p, L_CENSOR, parray[p1].login))
		return 1;
	return 0;
}

/*
 * Is p1 on p's notify list?
 */
PUBLIC int
player_notified(int p, int p1)
{
	if (!parray[p1].registered)
		return 0;

	/* Possible bug: 'p' has just arrived! */
	if (!parray[p].name)
		return 0;

	return in_list(p, L_NOTIFY, parray[p1].login);
}

PUBLIC void
player_notify_departure(int p)
{
	if (!parray[p].registered)
		return;

	for (int p1 = 0; p1 < p_num; p1++) {
		if (parray[p1].notifiedby &&
		    !player_notified(p1, p) &&
		    player_notified(p, p1) &&
		    parray[p1].status == PLAYER_PROMPT) {
			if (parray[p1].bell)
				pprintf_noformat(p1, "\007");
			pprintf(p1, "\nNotification: ");
			pprintf_highlight(p1, "%s", parray[p].name);
			pprintf_prompt(p1, " has departed and isn't on your "
			    "notify list.\n");
		}
	}
}

PUBLIC int
player_notify_present(int p)
{
	int	count = 0;
	int	p1;

	if (!parray[p].registered)
		return count;

	for (p1 = 0; p1 < p_num; p1++) {
		if (player_notified(p, p1) && parray[p1].status ==
		    PLAYER_PROMPT) {
			if (!count)
				pprintf(p, "Present company includes:");
			count++;

			pprintf(p, " %s", parray[p1].name);

			if (parray[p1].notifiedby &&
			    !player_notified(p1, p) &&
			    parray[p1].status == PLAYER_PROMPT) {
				if (parray[p1].bell)
					pprintf_noformat(p1, "\007");
				pprintf(p1, "\nNotification: ");
				pprintf_highlight(p1, "%s", parray[p].name);
				pprintf_prompt(p1, " has arrived and isn't on "
				    "your notify list.\n");
			}
		}
	}

	if (count)
		pprintf(p, ".\n");
	return count;
}

PUBLIC int
player_notify(int p, char *note1, char *note2)
{ // Notify those interested that 'p' has arrived/departed.
	int	count = 0;
	int	p1;

	if (!parray[p].registered)
		return count;

	for (p1 = 0; p1 < p_num; p1++) {
		if (player_notified(p1, p) && parray[p1].status ==
		    PLAYER_PROMPT) {
			if (parray[p1].bell)
				pprintf_noformat(p1, "\007");

			pprintf(p1, "\nNotification: ");
			pprintf_highlight(p1, "%s", parray[p].name);
			pprintf_prompt(p1, " has %s.\n", note1);

			if (!count)
				pprintf(p, "Your %s was noted by:", note2);
			count++;

			pprintf(p, " %s", parray[p1].name);
		}
	}

	if (count)
		pprintf(p, ".\n");
	return count;
}

/*
 * Show adjourned games upon logon
 */
PUBLIC int
showstored(int p)
{
	DIR		*dirp;
	char		 dname[MAX_FILENAME_SIZE];
	int		 c = 0, p1;
	multicol	*m = multicol_start(50);	// Limit to 50
							// (should be enough)
#ifdef USE_DIRENT
	struct dirent	*dp;
#else
	struct direct	*dp;
#endif

	snprintf(dname, sizeof dname, "%s/%c", adj_dir, parray[p].login[0]);

	if ((dirp = opendir(dname)) == NULL) {
		multicol_end(m);
		return COM_OK;
	}

	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if (file_has_pname(dp->d_name, parray[p].login)) {
			if (strcmp(file_wplayer(dp->d_name), parray[p].login)) {
				p1 = player_find_bylogin
				    (file_wplayer(dp->d_name));
			} else {
				p1 = player_find_bylogin
				    (file_bplayer(dp->d_name));
			}

			if (p1 >= 0) {
				if (c < 50)
					multicol_store(m, parray[p1].name);
				pprintf(p1, "\nNotification: ");
				pprintf_highlight(p1, "%s", parray[p].name);
				pprintf_prompt(p1, ", who has an adjourned "
				    "game with you, has arrived.\n");
				c++;
			}
		}
	}

	closedir(dirp);

	if (c == 1) {
		pprintf(p, "1 player, who has an adjourned game with you, is "
		    "online:\007");
	} else if (c > 1) {
		pprintf(p, "\n%d players, who have an adjourned game with you, "
		    "are online:\007", c);
	}

	if (c != 0)
		multicol_pprint(m, p, parray[p].d_width, 2);
	multicol_end(m);
	return COM_OK;
}

PUBLIC int
player_count(int CountAdmins)
{
	int	count;
	int	i;

	count = 0;
	i = 0;

	while (i < p_num) {
		if (parray[i].status == PLAYER_PROMPT &&
		    (CountAdmins || !in_list(i, L_ADMIN, parray[i].name)))
			count++;
		i++;
	}

	if (count > player_high)
		player_high = count;
	return count;
}

PUBLIC int
player_idle(int p)
{ // XXX
	if (parray[p].status != PLAYER_PROMPT)
		return (time(NULL) - parray[p].logon_time);

	return (time(NULL) - parray[p].last_command_time);
}

PUBLIC int
player_ontime(int p)
{ // XXX
	return (time(NULL) - parray[p].logon_time);
}

PRIVATE void
write_p_inout(int inout, int p, char *file, int maxlines)
{
	FILE	*fp;
	int	 fd;

	if ((fd = open(file, g_open_flags[OPFL_APPEND], g_open_modes)) < 0) {
		warn("%s: open", __func__);
		return;
	} else if ((fp = fdopen(fd, "a")) == NULL) {
		warn("%s: fdopen", __func__);
		close(fd);
		return;
	}

	fprintf(fp, "%d %s %jd %d %s\n", inout, parray[p].name,
	    (intmax_t)time(NULL), parray[p].registered,
	    dotQuad(parray[p].thisHost));

	fclose(fp);

	if (maxlines)
		truncate_file(file, maxlines);
}

PUBLIC void
player_write_login(int p)
{
	char fname[MAX_FILENAME_SIZE];

	if (parray[p].registered) {
		snprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s",
		    stats_dir, parray[p].login[0], parray[p].login,
		    STATS_LOGONS);
		write_p_inout(P_LOGIN, p, fname, 8);
	}

	snprintf(fname, sizeof fname, "%s/%s", stats_dir, STATS_LOGONS);
	write_p_inout(P_LOGIN, p, fname, 30);

	snprintf(fname, sizeof fname, "%s/%s", stats_dir, "logons.log");
	write_p_inout(P_LOGIN, p, fname, 0);
}

PUBLIC void
player_write_logout(int p)
{
	char fname[MAX_FILENAME_SIZE];

	if (parray[p].registered) {
		snprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s",
		    stats_dir, parray[p].login[0], parray[p].login,
		    STATS_LOGONS);
		write_p_inout(P_LOGOUT, p, fname, 8);
	}

	snprintf(fname, sizeof fname, "%s/%s", stats_dir, STATS_LOGONS);
	write_p_inout(P_LOGOUT, p, fname, 30);

	snprintf(fname, sizeof fname, "%s/%s", stats_dir, "logons.log");
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
	int		 ret, too_long;
	int64_t		 lval = 0;
	time_t		 last = 0;

	ret = snprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s",
	    stats_dir, parray[p].login[0], parray[p].login, STATS_LOGONS);
	too_long = (ret < 0 || (size_t)ret >= sizeof fname);

	if (too_long) {
		fprintf(stderr, "FICS: %s: warning: snprintf truncated\n",
		    __func__);
	}

	if ((fp = fopen(fname, "r")) == NULL)
		return 0;

	inout = 1;

	while (!feof(fp)) {
		if (inout == P_LOGIN)
			last = lval;

		_Static_assert(19 < ARRAY_SIZE(loginName),
		    "'loginName' too small");
		_Static_assert(19 < ARRAY_SIZE(ipstr),
		    "'ipstr' too small");

		if (fscanf(fp, ("%d %19s " "%" SCNd64 " %d %19s\n"), &inout,
		    loginName, &lval, &registered, ipstr) != 5) {
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
	int		 ret, too_long;
	int64_t		 lval;
	time_t		 last = 0;

	ret = snprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s",
	    stats_dir, parray[p].login[0], parray[p].login, STATS_LOGONS);
	too_long = (ret < 0 || (size_t)ret >= sizeof fname);

	if (too_long) {
		fprintf(stderr, "FICS: %s: warning: snprintf truncated\n",
		    __func__);
	}

	if ((fp = fopen(fname, "r")) == NULL)
		return 0;

	while (!feof(fp)) {
		_Static_assert(19 < ARRAY_SIZE(loginName),
		    "'loginName' too small");
		_Static_assert(19 < ARRAY_SIZE(ipstr),
		    "'ipstr' too small");

		if (fscanf(fp, ("%d %19s " "%" SCNd64 " %d %19s\n"), &inout,
		    loginName, &lval, &registered, ipstr) != 5) {
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

PUBLIC void
player_pend_print(int p, pending *pend)
{
	char	outstr[200] = { '\0' };
	char	tmp[200] = { '\0' };

	if (p == pend->whofrom) {
		(void)strlcpy(outstr, "You are offering ", sizeof outstr);
	} else {
		snprintf(outstr, sizeof outstr, "%s is offering ",
		    parray[pend->whofrom].name);
	}

	if (p == pend->whoto) {
		/* null */;
	} else {
		snprintf(tmp, sizeof tmp, "%s ", parray[pend->whoto].name);
	}

	if (strlcat(outstr, tmp, sizeof outstr) >= sizeof outstr) {
		fprintf(stderr, "FICS: %s: warning: strlcat() truncated\n",
		    __func__);
	}

	switch (pend->type) {
	case PEND_MATCH:
		snprintf(tmp, sizeof tmp, "%s.", game_str(pend->param5,
		    (pend->param1 * 60),
		    pend->param2,
		    (pend->param3 * 60),
		    pend->param4,
		    pend->char1,
		    pend->char2));
		break;
	case PEND_DRAW:
		strlcpy(tmp, "a draw.\n", sizeof tmp);
		break;
	case PEND_PAUSE:
		strlcpy(tmp, "to pause the clock.\n", sizeof tmp);
		break;
	case PEND_ABORT:
		strlcpy(tmp, "to abort the game.\n", sizeof tmp);
		break;
	case PEND_TAKEBACK:
		snprintf(tmp, sizeof tmp, "to takeback the last %d "
		    "half moves.\n", pend->param1);
		break;
	case PEND_SIMUL:
		strlcpy(tmp, "to play a simul match.\n", sizeof tmp);
		break;
	case PEND_SWITCH:
		strlcpy(tmp, "to switch sides.\n", sizeof tmp);
		break;
	case PEND_ADJOURN:
		strlcpy(tmp, "an adjournment.\n", sizeof tmp);
		break;
	case PEND_PARTNER:
		strlcpy(tmp, "to be bughouse partners.\n", sizeof tmp);
		break;
	}

	if (strlcat(outstr, tmp, sizeof outstr) >= sizeof outstr) {
		fprintf(stderr, "FICS: %s: warning: strlcat() truncated\n",
		    __func__);
	}

	pprintf(p, "%s\n", outstr);
}

PUBLIC int
player_find_pendto(int p, int p1, int type)
{
	for (int i = 0; i < parray[p].num_to; i++) {
		if (parray[p].p_to_list[i].whoto != p1 && p1 != -1)
			continue;
		if (type < 0 || parray[p].p_to_list[i].type == type)
			return i;
		if (type == PEND_BUGHOUSE &&
		    parray[p].p_to_list[i].type == PEND_MATCH &&
		    !strcmp(parray[p].p_to_list[i].char2, "bughouse"))
			return i;
	}

	return -1;
}

PUBLIC int
player_new_pendto(int p)
{
	if (parray[p].num_to >= MAX_PENDING)
		return -1;

	parray[p].num_to++;

	return (parray[p].num_to - 1);
}

PUBLIC int
player_remove_pendto(int p, int p1, int type)
{
	bool	removed = false;
	int	w;

	if ((w = player_find_pendto(p, p1, type)) < 0)
		return -1;

	for (; w < (parray[p].num_to - 1); w++) {
		if (w + 1 >= (int)ARRAY_SIZE(parray[0].p_to_list)) {
			warnx("%s: overflowed array index write", __func__);
			break;
		}

		parray[p].p_to_list[w] = parray[p].p_to_list[w + 1];
		removed = true;
	}

	UNUSED_VAR(removed);
	parray[p].num_to -= 1;

	return (0);
}

PUBLIC int
player_find_pendfrom(int p, int p1, int type)
{
	for (int i = 0; i < parray[p].num_from; i++) {
		if (parray[p].p_from_list[i].whofrom != p1 && p1 != -1)
			continue;
		if (type == PEND_ALL || parray[p].p_from_list[i].type == type)
			return i;
		if (type < 0 && parray[p].p_from_list[i].type != -type)
			return i;
		/*
		 * The above "if" allows a type of -PEND_SIMUL to
		 * match every request EXCEPT simuls, for example. I'm
		 * doing this because Heringer does not want to
		 * decline simul requests when he makes a move in a
		 * sumul. -- hersco.
		 */

		if (type == PEND_BUGHOUSE &&
		    parray[p].p_from_list[i].type == PEND_MATCH &&
		    !strcmp(parray[p].p_from_list[i].char2, "bughouse"))
			return i;
	}

	return -1;
}

PUBLIC int
player_new_pendfrom(int p)
{
	if (parray[p].num_from >= MAX_PENDING)
		return -1;

	parray[p].num_from++;

	return (parray[p].num_from - 1);
}

PUBLIC int
player_remove_pendfrom(int p, int p1, int type)
{
	bool	removed = false;
	int	w;

	if ((w = player_find_pendfrom(p, p1, type)) < 0)
		return -1;

	for (; w < (parray[p].num_from - 1); w++) {
		if (w + 1 >= (int)ARRAY_SIZE(parray[0].p_from_list)) {
			warnx("%s: overflowed array index write", __func__);
			break;
		}

		parray[p].p_from_list[w] = parray[p].p_from_list[w + 1];
		removed = true;
	}

	UNUSED_VAR(removed);
	parray[p].num_from -= 1;

	return (0);
}

PUBLIC int
player_add_request(int p, int p1, int type, int param)
{
	int	pendf;
	int	pendt;

	if (player_find_pendto(p, p1, type) >= 0)
		return -1; // Already exists

	if ((pendt = player_new_pendto(p)) == -1)
		return -1;
	if ((pendf = player_new_pendfrom(p1)) == -1) {
		parray[p].num_to--;
		return -1;
	}

	parray[p].p_to_list[pendt].type		= type;
	parray[p].p_to_list[pendt].whoto	= p1;
	parray[p].p_to_list[pendt].whofrom	= p;
	parray[p].p_to_list[pendt].param1	= param;

	parray[p1].p_from_list[pendf].type	= type;
	parray[p1].p_from_list[pendf].whoto	= p1;
	parray[p1].p_from_list[pendf].whofrom	= p;
	parray[p1].p_from_list[pendf].param1	= param;

	return 0;
}

PUBLIC int
player_remove_request(int p, int p1, int type)
{
	bool	removed;
	int	to = 0, from = 0;

	while (to != -1 && (to = player_find_pendto(p, p1, type)) != -1) {
		removed = false;

		for (; to < parray[p].num_to - 1; to++) {
			if (to + 1 >= (int)ARRAY_SIZE(parray[0].p_to_list)) {
				warnx("%s: overflowed array index read/write",
				    __func__);
				break;
			}

			parray[p].p_to_list[to] = parray[p].p_to_list[to + 1];
			removed = true;
		}

		UNUSED_VAR(removed);
		parray[p].num_to -= 1;
	}

	while (from != -1 && (from = player_find_pendfrom(p1, p, type)) != -1) {
		removed = false;

		for (; from < parray[p1].num_from - 1; from++) {
			if (from + 1 >= (int)ARRAY_SIZE(parray[0].p_from_list)) {
				warnx("%s: overflowed array index read/write",
				    __func__);
				break;
			}

			parray[p1].p_from_list[from] =
			    parray[p1].p_from_list[from + 1];
			removed = true;
		}

		UNUSED_VAR(removed);
		parray[p1].num_from -= 1;
	}

	if ((type == PEND_ALL || type == PEND_MATCH) && parray[p].partner >= 0)
		player_remove_request(parray[p].partner, p1, PEND_BUGHOUSE);

	return 0;
}

PUBLIC int
player_decline_offers(int p, int p1, int offerType)
{
	char	*pName = parray[p].name;
	char	*p2Name;
	int	 count = 0;
	int	 offer;
	int	 part, p2part;
	int	 type, p2;

	// First get rid of bughouse offers from partner.

	if ((offerType == PEND_MATCH || offerType == PEND_ALL) &&
	    parray[p].partner >= 0 &&
	    parray[parray[p].partner].partner == p) {
		count += player_decline_offers(parray[p].partner, p1,
		    PEND_BUGHOUSE);
	}

	while ((offer = player_find_pendfrom(p, p1, offerType)) >= 0) {
		if (offer >= (int)ARRAY_SIZE(parray[0].p_from_list)) {
			warnx("%s: 'offer' too large", __func__);
			break;
		}

		type	= parray[p].p_from_list[offer].type;
		p2	= parray[p].p_from_list[offer].whofrom;
		p2Name	= parray[p2].name;

		if ((part = parray[p].partner) >= (int)ARRAY_SIZE(parray)) {
			errx(1, "%s: 'part' (%d) too large", __func__,
			    part);
		} else if (part >= 0 && parray[part].partner != p)
			part = -1;

		if ((p2part = parray[p2].partner) >= (int)ARRAY_SIZE(parray)) {
			errx(1, "%s: 'p2part' (%d) too large", __func__,
			    p2part);
		} else if (p2part >= 0 && parray[p2part].partner != p2)
			p2part = -1;

		switch (type) {
		case PEND_MATCH:
			pprintf_prompt(p2, "\n%s declines the match offer.\n",
			    pName);
			pprintf(p, "You decline the match offer from %s.\n",
			    p2Name);

			if (!strcmp(parray[p].p_from_list[offer].char2,
			    "bughouse")) {
				if (part >= 0) {
					pprintf_prompt(part, "Your partner "
					    "declines the bughouse offer from "
					    "%s.\n", parray[p2].name);
				}

				if (p2part >= 0) {
					pprintf_prompt(p2part, "%s declines "
					    "the bughouse offer from your "
					    "partner.\n", parray[p].name);
				}
			}

			break;
		case PEND_DRAW:
			pprintf_prompt(p2, "\n%s declines draw request.\n",
			    pName);
			pprintf(p, "You decline the draw request from %s.\n",
			    p2Name);
			break;
		case PEND_PAUSE:
			pprintf_prompt(p2, "\n%s declines pause request.\n",
			    pName);
			pprintf(p, "You decline the pause request from %s.\n",
			    p2Name);
			break;
		case PEND_ABORT:
			pprintf_prompt(p2, "\n%s declines abort request.\n",
			    pName);
			pprintf(p, "You decline the abort request from %s.\n",
			    p2Name);
			break;
		case PEND_TAKEBACK:
			pprintf_prompt(p2, "\n%s declines the takeback request."
			    "\n", pName);
			pprintf(p, "You decline the takeback request from %s."
			    "\n", p2Name);
			break;
		case PEND_ADJOURN:
			pprintf_prompt(p2, "\n%s declines the adjourn request."
			    "\n", pName);
			pprintf(p, "You decline the adjourn request from %s.\n",
			    p2Name);
			break;
		case PEND_SWITCH:
			pprintf_prompt(p2, "\n%s declines the switch sides "
			    "request.\n", pName);
			pprintf(p, "You decline the switch sides request from "
			    "%s.\n", p2Name);
			break;
		case PEND_SIMUL:
			pprintf_prompt(p2, "\n%s declines the simul offer.\n",
			    pName);
			pprintf(p, "You decline the simul offer from %s.\n",
			    p2Name);
			break;
		case PEND_PARTNER:
			pprintf_prompt(p2, "\n%s declines your partnership "
			    "request.\n", pName);
			pprintf(p, "You decline the partnership request from "
			    "%s.\n", p2Name);
			break;
		} /* switch */

		player_remove_request(p2, p, type);
		count++;
	} /* while */

	return count;
}

PUBLIC int
player_withdraw_offers(int p, int p1, int offerType)
{
	char	*pName = parray[p].name;
	char	*p2Name;
	int	 count = 0;
	int	 offer;
	int	 part, p2part;
	int	 type, p2;

	// First get rid of bughouse offers from partner.

	if ((offerType == PEND_MATCH || offerType == PEND_ALL) &&
	    parray[p].partner >= 0 &&
	    parray[parray[p].partner].partner == p) {
		count += player_withdraw_offers(parray[p].partner, p1,
		    PEND_BUGHOUSE);
	}

	while ((offer = player_find_pendto(p, p1, offerType)) >= 0) {
		if (offer >= (int)ARRAY_SIZE(parray[0].p_to_list)) {
			warnx("%s: 'offer' too large", __func__);
			break;
		}

		type	= parray[p].p_to_list[offer].type;
		p2	= parray[p].p_to_list[offer].whoto;
		p2Name	= parray[p2].name;

		if ((part = parray[p].partner) >= (int)ARRAY_SIZE(parray)) {
			errx(1, "%s: 'part' (%d) too large", __func__,
			    part);
		} else if (part >= 0 && parray[part].partner != p)
			part = -1;

		if ((p2part = parray[p2].partner) >= (int)ARRAY_SIZE(parray)) {
			errx(1, "%s: 'p2part' (%d) too large", __func__,
			    p2part);
		} else if (p2part >= 0 && parray[p2part].partner != p2)
			p2part = -1;

		switch (type) {
		case PEND_MATCH:
			pprintf_prompt(p2, "\n%s withdraws the match offer.\n",
			    pName);
			pprintf(p, "You withdraw the match offer to %s.\n",
			    p2Name);

			if (!strcmp(parray[p].p_to_list[offer].char2,
			    "bughouse")) {
				if (part >= 0) {
					pprintf_prompt(part, "Your partner "
					    "withdraws the bughouse offer to "
					    "%s.\n", parray[p2].name);
				}
				if (p2part >= 0) {
					pprintf_prompt(p2part, "%s withdraws "
					    "the bughouse offer to your "
					    "partner.\n", parray[p].name);
				}
			}

			break;
		case PEND_DRAW:
			pprintf_prompt(p2, "\n%s withdraws draw request.\n",
			    pName);
			pprintf(p, "You withdraw the draw request to %s.\n",
			    p2Name);
			break;
		case PEND_PAUSE:
			pprintf_prompt(p2, "\n%s withdraws pause request.\n",
			    pName);
			pprintf(p, "You withdraw the pause request to %s.\n",
			    p2Name);
			break;
		case PEND_ABORT:
			pprintf_prompt(p2, "\n%s withdraws abort request.\n",
			    pName);
			pprintf(p, "You withdraw the abort request to %s.\n",
			    p2Name);
			break;
		case PEND_TAKEBACK:
			pprintf_prompt(p2, "\n%s withdraws the takeback "
			    "request.\n", pName);
			pprintf(p, "You withdraw the takeback request to %s.\n",
			    p2Name);
			break;
		case PEND_ADJOURN:
			pprintf_prompt(p2, "\n%s withdraws the adjourn request."
			    "\n", pName);
			pprintf(p, "You withdraw the adjourn request to %s.\n",
			    p2Name);
			break;
		case PEND_SWITCH:
			pprintf_prompt(p2, "\n%s withdraws the switch sides "
			    "request.\n", pName);
			pprintf(p, "You withdraw the switch sides request to "
			    "%s.\n", p2Name);
			break;
		case PEND_SIMUL:
			pprintf_prompt(p2, "\n%s withdraws the simul offer.\n",
			    pName);
			pprintf(p, "You withdraw the simul offer to %s.\n",
			    p2Name);
			break;
		case PEND_PARTNER:
			pprintf_prompt(p2, "\n%s withdraws partnership request."
			    "\n", pName);
			pprintf(p, "You withdraw the partnership request to %s."
			    "\n", p2Name);
			break;
		} /* switch */

		player_remove_request(p, p2, type);
		count++;
	} /* while */

	return count;
}

PUBLIC int
player_is_observe(int p, int g)
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

PUBLIC int
player_add_observe(int p, int g)
{
	if (parray[p].num_observe == MAX_OBSERVE)
		return -1;
	parray[p].observe_list[parray[p].num_observe] = g;
	parray[p].num_observe++;
	return 0;
}

PUBLIC int
player_remove_observe(int p, int g)
{
	int i;

	for (i = 0; i < parray[p].num_observe; i++) {
		if (parray[p].observe_list[i] == g)
			break;
	}

	if (i == parray[p].num_observe)
		return -1; // Not found!

	for (; i < (parray[p].num_observe - 1); i++)
		parray[p].observe_list[i] = parray[p].observe_list[i + 1];

	parray[p].num_observe--;

	return 0;
}

PUBLIC int
player_game_ended(int g)
{
	for (int p = 0; p < p_num; p++) {
		if (parray[p].status == PLAYER_EMPTY)
			continue;
		player_remove_observe(p, g);
	}

	player_remove_request(garray[g].white, garray[g].black, -1);
	player_remove_request(garray[g].black, garray[g].white, -1);

	player_save(garray[g].white);
	player_save(garray[g].black);

	return 0;
}

PUBLIC int
player_goto_board(int p, int board_num)
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

	start	= parray[p].game;
	on	= parray[p].simul_info.onBoard;

	do {
		if ((g = parray[p].simul_info.boards[on]) >= 0) {
			if (count == 0) {
				if (parray[garray[g].black].bell)
					pprintf(garray[g].black, "\007");

				pprintf(garray[g].black, "\n");
				pprintf_highlight(garray[g].black, "%s",
				    parray[p].name);
				pprintf_prompt(garray[g].black, " is at your "
				    "board!\n");
			} else if (count == 1) {
				if (parray[garray[g].black].bell)
					pprintf(garray[g].black, "\007");

				pprintf(garray[g].black, "\n");
				pprintf_highlight(garray[g].black, "%s",
				    parray[p].name);
				pprintf_prompt(garray[g].black, " will be at "
				    "your board NEXT!\n");
			} else {
				pprintf(garray[g].black, "\n");
				pprintf_highlight(garray[g].black, "%s",
				    parray[p].name);
				pprintf_prompt(garray[g].black, " is %d boards "
				    "away.\n", count);
			}

			count++;
		}

		if (++on >= parray[p].simul_info.numBoards)
			on = 0;
	} while (start != parray[p].simul_info.boards[on]);

	return 0;
}

PUBLIC int
player_goto_next_board(int p)
{
	int	g;
	int	on;
	int	start;

	on	= parray[p].simul_info.onBoard;
	start	= on;

	do {
		on++;

		if (on >= parray[p].simul_info.numBoards)
			on = 0;

		if ((g = parray[p].simul_info.boards[on]) >= 0)
			break;
	} while (start != on);

	if (g == -1) {
		pprintf(p, "\nMajor Problem! Can't find your next board.\n");
		return -1;
	}

	return player_goto_board(p, on);
}

PUBLIC int
player_goto_prev_board(int p)
{
	int	g;
	int	on;
	int	start;

	on	= parray[p].simul_info.onBoard;
	start	= on;

	do {
		--on;

		if (on < 0)
			on = (parray[p].simul_info.numBoards) - 1;

		if ((g = parray[p].simul_info.boards[on]) >= 0)
			break;
	} while (start != on);

	if (g == -1) {
		pprintf(p, "\nMajor Problem! Can't find your previous board."
		    "\n");
		return -1;
	}

	return player_goto_board(p, on);
}

PUBLIC int
player_goto_simulgame_bynum(int p, int num)
{
	int	g;
	int	on;
	int	start;

	on = parray[p].simul_info.onBoard;
	start = on;

	do {
		on++;

		if (on >= parray[p].simul_info.numBoards)
			on = 0;

		if ((g = parray[p].simul_info.boards[on]) == num)
			break;
	} while (start != on);

	if (g != num) {
		pprintf(p, "\nYou aren't playing that game!!\n");
		return -1;
	}

	return player_goto_board(p, on);
}

PUBLIC int
player_num_active_boards(int p)
{
	int count = 0;

	if (!parray[p].simul_info.numBoards)
		return 0;

	for (int i = 0; i < parray[p].simul_info.numBoards; i++) {
		if (parray[p].simul_info.boards[i] >= 0)
			count++;
	}

	return count;
}

PUBLIC int
player_num_results(int p, int result)
{
	int count = 0;

	if (!parray[p].simul_info.numBoards)
		return 0;

	for (int i = 0; i < parray[p].simul_info.numBoards; i++) {
		if (parray[p].simul_info.results[i] == result)
			count++;
	}

	return count;
}

PUBLIC int
player_simul_over(int p, int g, int result)
{
	char	tmp[1024];
	int	on, ong, p1, which = -1, won;

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

	pprintf(p, "\nBoard %d has completed.\n", (won + 1));

	on	= parray[p].simul_info.onBoard;
	ong	= parray[p].simul_info.boards[on];

	parray[p].simul_info.boards[won]	= -1;
	parray[p].simul_info.results[won]	= result;

	if (player_num_active_boards(p) == 0) {
		snprintf(tmp, sizeof tmp, "\n{Simul (%s vs. %d) is over.}\n"
		    "Results: %d Wins, %d Losses, %d Draws, %d Aborts\n",
		    parray[p].name,
		    parray[p].simul_info.numBoards,
		    player_num_results(p, RESULT_WIN),
		    player_num_results(p, RESULT_LOSS),
		    player_num_results(p, RESULT_DRAW),
		    player_num_results(p, RESULT_ABORT));

		for (p1 = 0; p1 < p_num; p1++) {
			if (parray[p].status != PLAYER_PROMPT)
				continue;
			if (!parray[p1].i_game && !player_is_observe(p1, g) &&
			    p1 != p)
				continue;
			pprintf_prompt(p1, "%s", tmp);
		}

		parray[p].simul_info.numBoards = 0;

		pprintf_prompt(p, "\nThat was the last board, thanks for "
		    "playing.\n");

		return 0;
	}

	if (ong == g) /* This game is over */
		player_goto_next_board(p);
	else
		player_goto_board(p, parray[p].simul_info.onBoard);

	pprintf_prompt(p, "\nThere are %d boards left.\n",
	    player_num_active_boards(p));
	return 0;
}

PRIVATE void
GetMsgFile(int p, char *fName, const size_t size, const char *func)
{
	int ret, too_long;

	ret = snprintf(fName, size, "%s/player_data/%c/%s.%s", stats_dir,
	    parray[p].login[0], parray[p].login, STATS_MESSAGES);
	too_long = (ret < 0 || (size_t)ret >= size);

	if (too_long) {
		fprintf(stderr, "FICS: %s: warning: snprintf truncated\n",
		    func);
	}
}

PUBLIC int
player_num_messages(int p)
{
	char fname[MAX_FILENAME_SIZE];

	if (!parray[p].registered)
		return 0;

	GetMsgFile(p, fname, sizeof fname, __func__);

	return lines_file(fname);
}

PUBLIC int
player_add_message(int top, int fromp, char *message)
{
	FILE	*fp;
	char	 fname[MAX_FILENAME_SIZE] = { '\0' };
	char	 messbody[1024] = { '\0' };
	char	 subj[256] = { '\0' };
	int	 fd;
	time_t	 t = time(NULL);

	if (!parray[top].registered)
		return -1;
	if (!parray[fromp].registered)
		return -1;

	GetMsgFile(top, fname, sizeof fname, __func__);

	if (lines_file(fname) >= MAX_MESSAGES && parray[top].adminLevel == 0)
		return -1;

	if ((fd = open(fname, g_open_flags[OPFL_APPEND], g_open_modes)) < 0)
		return -1;
	else if ((fp = fdopen(fd, "a")) == NULL) {
		close(fd);
		return -1;
	}

	fprintf(fp, "%s at %s: %s\n", parray[fromp].name, strltime(&t),
	    message);
	fclose(fp);

	pprintf(fromp, "\nThe following message was sent ");

	if (parray[top].i_mailmess) {
		snprintf(subj, sizeof subj, "FICS message from %s at FICS %s "
		    "(Do not reply by mail)", parray[fromp].name,
		    fics_hostname);
		snprintf(messbody, sizeof messbody, "%s at %s: %s\n",
		    parray[fromp].name, strltime(&t), message);
		mail_string_to_user(top, subj, messbody);
		pprintf(fromp, "(and emailed) ");
	}

	pprintf(fromp, "to %s:\n   %s\n", parray[top].name, message);
	return 0;
}

PUBLIC void
SaveTextListEntry(textlist **Entry, char *string, int n)
{
	*Entry = rmalloc(sizeof(textlist));

	(*Entry)->text	= xstrdup(string);
	(*Entry)->index	= n;
	(*Entry)->next	= NULL;
}

PUBLIC textlist *
ClearTextListEntry(textlist *entry)
{
	textlist *ret = entry->next;

	strfree(entry->text);
	rfree(entry);
	return ret;
}

PUBLIC void
ClearTextList(textlist *head)
{
	for (textlist *cur = head; cur != NULL; cur = ClearTextListEntry(cur)) {
		/* null */;
	}
}

/*
 * if which = 0 load all messages;
 * if it's (p1 + 1) load messages only from p1;
 * if it's -(p1 + 1) load all messages EXCEPT those from p1.
 */
PRIVATE int
SaveThisMsg(int which, char *line)
{
	char	Sender[MAX_LOGIN_NAME] = { '\0' };
	int	p1;

	_Static_assert(19 < ARRAY_SIZE(Sender), "Array too small");

	if (which == 0)
		return 1;

	if (sscanf(line, "%19s", Sender) != 1) {
		warnx("%s: failed to read sender", __func__);
		return 0;
	}

	if (which < 0) {
		p1 = (-which) - 1;
		return strcmp(Sender, parray[p1].name);
	}

	p1 = (which - 1);
	return !strcmp(Sender, parray[p1].name);
}

/*
 * which = 0 to load all messages;
 * it's (p1 + 1) to load messages only from p1;
 * and it's -(p1 + 1) to load all messages EXCEPT those from p1.
 */
PRIVATE int
LoadMsgs(int p, int which, textlist **Head)
{
	FILE		*fp;
	char		 fName[MAX_FILENAME_SIZE];
	char		 line[MAX_LINE_SIZE];
	int		 n = 0, nSave = 0;
	textlist**	 Cur = Head;

	*Head = NULL;
	GetMsgFile(p, fName, sizeof fName, __func__);

	if ((fp = fopen(fName, "r")) == NULL)
		return -1;

	while (fgets(line, sizeof line, fp) != NULL) {
		if (SaveThisMsg(which, line)) {
			SaveTextListEntry(Cur, line, ++n);
			Cur = &(*Cur)->next;
			nSave++;
		} else
			n++;
	}

	fclose(fp);
	return nSave;
}

/*
 * start > 0 and end > start (or end = 0) to save messages in range;
 * start < 0 and end < start (or end = 0) to clear messages in range;
 * if end = 0, range goes to end of file (not tested yet).
 */
PRIVATE int
LoadMsgRange(int p, int start, int end, textlist **Head)
{
	FILE		*fp;
	char		 fName[MAX_FILENAME_SIZE];
	char		 line[MAX_LINE_SIZE];
	int		 n = 1, nSave = 0, nKill = 0;
	textlist**	 Cur = Head;

	*Head = NULL;
	GetMsgFile(p, fName, sizeof fName, __func__);

	if ((fp = fopen(fName, "r")) == NULL) {
		pprintf(p, "You have no messages.\n");
		return -1;
	}

	for (n = 1; n <= end || end <= 0; n++) {
		if (fgets(line, sizeof line, fp) == NULL)
			break;
		if ((start < 0 && (n < -start || n > -end)) ||
		    (start >= 0 && n >= start)) {
			SaveTextListEntry(Cur, line, n);
			Cur = &(*Cur)->next;
			nSave++;
		} else
			nKill++;
	}

	fclose(fp);

	if (start < 0) {
		if (n <= -start)
			pprintf(p, "You do not have a message %d.\n", -start);
		return nKill;
	} else {
		if (n <= start)
			pprintf(p, "You do not have a message %d.\n", start);
		return nSave;
	}
}

PRIVATE int
WriteMsgFile(int p, textlist *Head)
{
	FILE		*fp;
	char		 fName[MAX_FILENAME_SIZE] = { '\0' };
	int		 fd;
	textlist	*Cur;

	GetMsgFile(p, fName, sizeof fName, __func__);

	if ((fd = open(fName, g_open_flags[OPFL_WRITE], g_open_modes)) < 0)
		return 0;
	else if ((fp = fdopen(fd, "w")) == NULL) {
		close(fd);
		return 0;
	}
	for (Cur = Head; Cur != NULL; Cur = Cur->next)
		fprintf(fp, "%s", Cur->text);
	fclose(fp);
	return 1;
}

PUBLIC int
ClearMsgsBySender(int p, param_list param)
{
	int		 nFound;
	int		 p1, connected;
	textlist	*Head;

	if (!FindPlayer(p, param[0].val.word, &p1, &connected))
		return -1;

	nFound = LoadMsgs(p, -(p1 + 1), &Head);

	if (nFound < 0) {
		pprintf(p, "You have no messages.\n");
	} else if (nFound == 0) {
		pprintf(p, "You have no messages from %s.\n", parray[p1].name);
	} else {
		if (WriteMsgFile(p, Head)) {
			pprintf(p, "Messages from %s cleared.\n",
			    parray[p1].name);
		} else {
			pprintf(p, "Problem writing message file; "
			    "please contact an admin.\n");
			fprintf(stderr, "Problem writing message file for "
			    "%s.\n", parray[p].name);
		}

		ClearTextList(Head);
	}

	if (!connected)
		player_remove(p1);
	return nFound;
}

PRIVATE void
ShowTextList(int p, textlist *Head, int ShowIndex)
{
	textlist *CurMsg;

	if (ShowIndex) {
		for (CurMsg = Head; CurMsg != NULL; CurMsg = CurMsg->next)
			pprintf(p, "%2d. %s", CurMsg->index, CurMsg->text);
	} else {
		for (CurMsg = Head; CurMsg != NULL; CurMsg = CurMsg->next)
			pprintf(p, "%s", CurMsg->text);
	}
}

PUBLIC int
player_show_messages(int p)
{
	int		 n;
	textlist	*Head;

	n = LoadMsgs(p, 0, &Head);

	if (n <= 0) {
		pprintf(p, "You have no messages.\n");
		return -1;
	} else {
		pprintf(p, "Messages:\n");
		ShowTextList(p, Head, 1);
		ClearTextList(Head);
		return 0;
	}
}

PUBLIC int
ShowMsgsBySender(int p, param_list param)
{
	int		 nFrom = -1, nTo = -1;
	int		 p1, connected;
	textlist	*Head;

	if (!FindPlayer(p, param[0].val.word, &p1, &connected))
		return -1;

	if (!parray[p1].registered) {
		pprintf(p, "Player \"%s\" is unregistered and cannot send or "
		    "receive messages.\n", parray[p1].name);
		return -1; /* no need to disconnect */
	}

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

PUBLIC int
ShowMsgRange(int p, int start, int end)
{
	int		 n;
	textlist	*Head;

	if ((n = LoadMsgRange(p, start, end, &Head)) > 0) {
		ShowTextList(p, Head, 1);
		ClearTextList(Head);
	}

	return n;
}

PUBLIC int
ClrMsgRange(int p, int start, int end)
{
	int		 n;
	textlist	*Head;

	n = LoadMsgRange(p, -start, -end, &Head);

	if (n > 0) {
		if (WriteMsgFile(p, Head))
			pprintf(p, "Message %d cleared.\n", start);
	}

	if (n >= 0)
		ClearTextList(Head);
	return n;
}

PUBLIC int
player_clear_messages(int p)
{
	char fname[MAX_FILENAME_SIZE];

	if (!parray[p].registered)
		return -1;
	GetMsgFile(p, fname, sizeof fname, __func__);
	unlink(fname);
	return 0;
}

/*
 * Find player matching the given string.
 *
 * First looks for exact match with a logged in player, then an exact
 * match with a registered player, then a partial unique match with a
 * logged in player, then a partial match with a registered player.
 *
 * Returns player number if the player is connected. Negative (player
 * number) if the player had to be connected and 0 if no player was
 * found.
 */
PUBLIC int
player_search(int p, char *name)
{
	char	*buffer[1000];
	char	 pdir[MAX_FILENAME_SIZE];
	int	 p1, count;

	// Exact match with connected player?
	if ((p1 = player_find_bylogin(name)) >= 0) {
		if (p1 + 1 >= (int)ARRAY_SIZE(parray))
			return 0;
		return (p1 + 1);
	}

	// Exact match with registered player?
	snprintf(pdir, sizeof pdir, "%s/%c", player_dir, name[0]);
	count = search_directory(pdir, name, buffer, ARRAY_SIZE(buffer));

	if (count > 0 && !strcmp(name, *buffer))
		goto ReadPlayerFromFile;	// Found an unconnected
						// registered player

	// Partial match with connected player?
	if ((p1 = player_find_part_login(name)) >= 0) {
		return (p1 + 1);
	} else if (p1 == -2) {
		// Ambiguous. Matches too many connected players.
		pprintf(p, "Ambiguous name '%s'; matches more than one player."
		    "\n", name);
		return 0;
	}

	// Partial match with registered player?
	if (count < 1) {
		pprintf(p, "There is no player matching that name.\n");
		return 0;
	} else if (count > 1) {
		pprintf(p, "-- Matches: %d names --", count);
		display_directory(p, buffer, count);
		return 0;
	}

  ReadPlayerFromFile:

	p1 = player_new();

	if (player_read(p1, *buffer)) {
		player_remove(p1);
		pprintf(p, "ERROR: a player named %s was expected but not "
		    "found!\n", *buffer);
		pprintf(p, "Please tell an admin about this incident. "
		    "Thank you.\n");
		return 0;
	}

	return (-p1) - 1;	// Negative to indicate player was not connected
}

PUBLIC int
player_kill(char *name)
{
	char	fname[MAX_FILENAME_SIZE];
	char	fname2[MAX_FILENAME_SIZE];

	snprintf(fname, sizeof fname, "%s/%c/%s", player_dir, name[0],
	    name);
	snprintf(fname2, sizeof fname2, "%s/%c/.rem.%s", player_dir, name[0],
	    name);
	xrename(__func__, fname, fname2);

	RemHist(name);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.games",
	    stats_dir, name[0], name);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.games",
	    stats_dir, name[0], name);
	xrename(__func__, fname, fname2);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.comments",
	    stats_dir, name[0], name);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.comments",
	    stats_dir, name[0], name);
	xrename(__func__, fname, fname2);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.logons",
	    stats_dir, name[0], name);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.logons",
	    stats_dir, name[0], name);
	xrename(__func__, fname, fname2);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.messages",
	    stats_dir, name[0], name);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.messages",
	    stats_dir, name[0], name);
	xrename(__func__, fname, fname2);

	return 0;
}

PUBLIC int
player_rename(char *name, char *newname)
{
	char	fname[MAX_FILENAME_SIZE];
	char	fname2[MAX_FILENAME_SIZE];

	snprintf(fname, sizeof fname, "%s/%c/%s", player_dir, name[0],
	    name);
	snprintf(fname2, sizeof fname2, "%s/%c/%s", player_dir, newname[0],
	    newname);
	xrename(__func__, fname, fname2);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.games",
	    stats_dir, name[0], name);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/%s.games",
	    stats_dir, newname[0], newname);
	xrename(__func__, fname, fname2);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.comments",
	    stats_dir, name[0], name);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/%s.comments",
	    stats_dir, newname[0], newname);
	xrename(__func__, fname, fname2);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.logons",
	    stats_dir, name[0], name);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/%s.logons",
	    stats_dir, newname[0], newname);
	xrename(__func__, fname, fname2);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.messages",
	    stats_dir, name[0], name);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/%s.messages",
	    stats_dir, newname[0], newname);
	xrename(__func__, fname, fname2);

	return 0;
}

PUBLIC int
player_raise(char *name)
{
	char	fname[MAX_FILENAME_SIZE];
	char	fname2[MAX_FILENAME_SIZE];

	snprintf(fname, sizeof fname, "%s/%c/%s", player_dir,
	    name[0], name);
	snprintf(fname2, sizeof fname2, "%s/%c/.rem.%s", player_dir,
	    name[0], name);
	xrename(__func__, fname2, fname);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.games",
	    stats_dir, name[0], name);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.games",
	    stats_dir, name[0], name);
	xrename(__func__, fname2, fname);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.comments",
	    stats_dir, name[0], name);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.comments",
	    stats_dir, name[0], name);
	xrename(__func__, fname2, fname);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.logons",
	    stats_dir, name[0], name);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.logons",
	    stats_dir, name[0], name);
	xrename(__func__, fname2, fname);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.messages",
	    stats_dir, name[0], name);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.messages",
	    stats_dir, name[0], name);
	xrename(__func__, fname2, fname);

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
	xrename(__func__, fname2, fname);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.games",
	    stats_dir, newname[0], newname);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.games",
	    stats_dir, name[0], name);
	xrename(__func__, fname2, fname);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.comments",
	    stats_dir, newname[0], newname);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.comments",
	    stats_dir, name[0], name);
	xrename(__func__, fname2, fname);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.logons",
	    stats_dir, newname[0], newname);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.logons",
	    stats_dir, name[0], name);
	xrename(__func__, fname2, fname);

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.messages",
	    stats_dir, newname[0], newname);
	snprintf(fname2, sizeof fname2, "%s/player_data/%c/.rem.%s.messages",
	    stats_dir, name[0], name);
	xrename(__func__, fname2, fname);

	return 0;
}

PUBLIC int
player_num_comments(int p)
{
	char fname[MAX_FILENAME_SIZE];

	if (!parray[p].registered)
		return 0;
	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s", stats_dir,
	    parray[p].login[0], parray[p].login, "comments");
	return lines_file(fname);
}

PUBLIC int
player_add_comment(int p_by, int p_to, char *comment)
{
	FILE	*fp;
	char	 fname[MAX_FILENAME_SIZE] = { '\0' };
	int	 fd;
	time_t	 t = time(NULL);

	if (!parray[p_to].registered)
		return -1;

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s", stats_dir,
	    parray[p_to].login[0], parray[p_to].login, "comments");

	if ((fd = open(fname, g_open_flags[OPFL_APPEND], g_open_modes)) < 0) {
		warn("%s: open", __func__);
		return -1;
	} else if ((fp = fdopen(fd, "a")) == NULL) {
		warn("%s: fdopen", __func__);
		close(fd);
		return -1;
	}

	fprintf(fp, "%s at %s: %s\n", parray[p_by].name, strltime(&t), comment);
	fclose(fp);
	parray[p_to].num_comments = player_num_comments(p_to);
	return 0;
}

PUBLIC int
player_show_comments(int p, int p1)
{
	char fname[MAX_FILENAME_SIZE] = { '\0' };

	snprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s", stats_dir,
	    parray[p1].login[0], parray[p1].login, "comments");

	if (psend_file(p, NULL, fname) == -1)
		warnx("%s: psend_file() error", __func__);
	return 0;
}

/*
 * Returns 1 if player is head admin and 0 otherwise.
 */
PUBLIC int
player_ishead(int p)
{
	return (strcasecmp(parray[p].name, settings_get("HADMINHANDLE")) == 0);
}

/* comproc.c
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
   foxbat			95/03/11	Added filters in cmatch
   Markus Uhlin			23/12/09	Fixed implicit ints
   Markus Uhlin			23/12/17	Fixed compiler warnings
   Markus Uhlin			23/12/17	Usage of 'time_t'
   Markus Uhlin			23/12/17	Reformatted com_stats()
   Markus Uhlin			23/12/19	Usage of 'time_t'
   Markus Uhlin			23/12/23	Fixed crypt()
   Markus Uhlin			24/01/03	Added usage of ARRAY_SIZE() and
						reformatted functions.
   Markus Uhlin			24/01/06	Fixed potentially insecure
						format strings.
   Markus Uhlin			24/03/23	Size-bounded string handling
						plus truncation checks.
   Markus Uhlin			24/11/19	Improved com_news().
						plogins: fscanf:
						added width specification
   Markus Uhlin			25/03/08	Calc string length once
   Markus Uhlin			25/03/11	Fixed possibly uninitialized
						value 'rat' in who_terse().
   Markus Uhlin			25/03/16	Fixed use of 32-bit 'time_t'.
   Markus Uhlin			25/03/16	Fixed untrusted array index.
   Markus Uhlin			25/03/25	com_unalias: fixed overflowed
						array index read/write.
*/

#include "stdinclude.h"
#include "common.h"

#include <sys/resource.h>

#include <err.h>

#include "board.h"
#include "command.h"
#include "comproc.h"
#include "config.h"
#include "eco.h"
#include "fics_getsalt.h"
#include "ficsmain.h"
#include "formula.h"
#include "gamedb.h"
#include "gameproc.h"
#include "lists.h"
#include "multicol.h"
#include "network.h"
#include "obsproc.h"
#include "playerdb.h"
#include "ratings.h"
#include "rmalloc.h"
#include "talkproc.h"
#include "utils.h"
#include "variable.h"

#if __linux__
#include <bsd/string.h>
#endif

#define WHO_OPEN         0x01
#define WHO_CLOSED       0x02
#define WHO_RATED        0x04
#define WHO_UNRATED      0x08
#define WHO_FREE         0x10
#define WHO_PLAYING      0x20
#define WHO_REGISTERED   0x40
#define WHO_UNREGISTERED 0x80
#define WHO_BUGTEAM      0x100

#define WHO_ALL 0xff

typedef int ((*who_cmp_t)(const void *, const void *));

PUBLIC const int	none = 0;
PUBLIC const int	blitz_rat = 1;
PUBLIC const int	std_rat = 2;
PUBLIC const int	wild_rat = 3;
PUBLIC const int	light_rat = 4;

PUBLIC int num_news = -1;

PRIVATE char *inout_string[] = { "login", "logout" };

PUBLIC int
com_rating_recalc(int p, param_list param)
{
	(void) p;	// maybe not referenced
	(void) param;	// not referenced

	ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
	rating_recalc();
	return COM_OK;
}

PUBLIC int
com_more(int p, param_list param)
{
	// not referenced
	(void) param;

	pmore_file(p);
	return COM_OK;
}

PUBLIC void
rscan_news2(FILE *fp, int p, int num)
{
	char		*junkp;
	char		 count[10] = { '\0' };
	char		 junk[MAX_LINE_SIZE] = { '\0' };
	long int	 lval;
	time_t		 crtime;

	if (num == 0)
		return;

	if (fgets(junk, sizeof junk, fp) == NULL || feof(fp) ||
	    sscanf(junk, "%ld %9s", &lval, count) != 2)
		return;

	rscan_news2(fp, p, num - 1);

	junkp = junk;
	junkp = nextword(junkp);
	junkp = nextword(junkp);

	crtime = lval;
	pprintf(p, "%3s (%s) %s", count, fix_time(strltime(&crtime)), junkp);
}

PUBLIC int
com_news(int p, param_list param)
{
	FILE		*fp = NULL;
	char		*junkp = NULL;
	char		 count[10] = { '\0' };
	char		 filename[MAX_FILENAME_SIZE] = { '\0' };
	char		 junk[MAX_LINE_SIZE] = { '\0' };
	int		 found = 0;
	long int	 lval = 0;
	time_t		 crtime = 0;

	snprintf(filename, sizeof filename, "%s/newnews.index", news_dir);

	if ((fp = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Can\'t find news index.\n");
		return COM_OK;
	}

#define SCAN_JUNK "%ld %9s"
	_Static_assert(9 < ARRAY_SIZE(count), "'count' too small");

	if (param[0].type == 0) {
		/*
		 * No params - then just display index of last ten
		 * news items.
		 */

		pprintf(p, "Index of recent news items:\n");

		if (fgets(junk, sizeof junk, fp) == NULL ||
		    sscanf(junk, SCAN_JUNK, &lval, count) != 2) {
			warnx("%s: error: fgets() or sscanf()", __func__);
			fclose(fp);
			return COM_FAILED;
		}

		rscan_news2(fp, p, 9);

		junkp = junk;
		junkp = nextword(junkp);
		junkp = nextword(junkp);

		crtime = lval;
		pprintf(p, "%3s (%s) %s", count, fix_time(strltime(&crtime)),
		    junkp);
		fclose(fp);
	} else if (param[0].type == TYPE_WORD &&
	    !strcmp(param[0].val.word, "all")) {
		/*
		 * Param all - displays index all news items.
		 */

		pprintf(p, "Index of all news items:\n");

		if (fgets(junk, sizeof junk, fp) == NULL ||
		    sscanf(junk, SCAN_JUNK, &lval, count) != 2) {
			warnx("%s: error: fgets() or sscanf()", __func__);
			fclose(fp);
			return COM_FAILED;
		}

		rscan_news(fp, p, 0);

		junkp = junk;
		junkp = nextword(junkp);
		junkp = nextword(junkp);

		crtime = lval;
		pprintf(p, "%3s (%s) %s", count, fix_time(strltime(&crtime)),
		    junkp);
		fclose(fp);
	} else {
		/*
		 * Check if the specific news file exist in index.
		 */

		while (!feof(fp) && !found) {
			junkp = junk;

			if (fgets(junk, sizeof junk, fp) == NULL || feof(fp))
				break;
			if (sscanf(junkp, SCAN_JUNK, &lval, count) != 2)
				warnx("%s: sscanf() error...", __func__);
			crtime = lval;

			if (!strcmp(count, param[0].val.word)) {
				found = 1;

				junkp = nextword(junkp);
				junkp = nextword(junkp);

				pprintf(p, "NEWS %3s (%s) %s\n", count,
				    fix_time(strltime(&crtime)), junkp);
			}
		}

		fclose(fp);

		if (!found) {
			pprintf(p, "Bad index number!\n");
			return COM_OK;
		}

		/*
		 * File exists - show it
		 */

		snprintf(filename, sizeof filename, "%s/news.%s", news_dir,
		    param[0].val.word);

		if ((fp = fopen(filename, "r")) == NULL) {
			pprintf(p, "No more info.\n");
			return COM_OK;
		}

		fclose(fp);
		snprintf(filename, sizeof filename, "news.%s",
		    param[0].val.word);

		if (psend_file(p, news_dir, filename) < 0) {
			pprintf(p, "Internal error - couldn't send news file!"
			    "\n");
		}
	}

	return COM_OK;
}

PUBLIC int
com_quit(int p, param_list param)
{
	// not referenced
	(void) param;

	if (parray[p].game >= 0 && garray[parray[p].game].status ==
	    GAME_EXAMINE)
		pcommand(p, "unexamine");
	if (parray[p].game >= 0) {
		pprintf(p, "You can't quit while you are playing a game.\n"
		    "Type 'resign' to resign the game, or you can request an "
		    "abort with 'abort'.\n");
		return COM_OK;
	}

	psend_logoutfile(p, mess_dir, MESS_LOGOUT);
	return COM_LOGOUT;
}

PUBLIC int
com_set(int p, param_list param)
{
	char	*val;
	int	 result;
	int	 which;

	if (param[1].type == TYPE_NULL)
		val = NULL;
	else
		val = param[1].val.string;

	result = var_set(p, param[0].val.word, val, &which);

	switch (result) {
	case VAR_OK:
		break;
	case VAR_BADVAL:
		pprintf(p, "Bad value given for variable %s.\n",
		    param[0].val.word);
		break;
	case VAR_NOSUCH:
		pprintf(p, "No such variable name %s.\n", param[0].val.word);
		break;
	case VAR_AMBIGUOUS:
		pprintf(p, "Ambiguous variable name %s.\n", param[0].val.word);
		break;
	}

	return COM_OK;
}

PUBLIC int
FindPlayer(int p, char *name, int *p1, int *connected)
{
	*p1 = player_search(p, name);

	if (*p1 == 0)
		return 0;
	if (*p1 < 0) {	// The player had to be connected and will be removed
			// later.
		*connected = 0;
		*p1 = (-*p1) - 1;
	} else {
		*connected = 1;
		*p1 = *p1 - 1;
	}

	return 1;
}

PRIVATE void
com_stats_andify(int *numbers, int howmany, char *dest, size_t dsize)
{
	char tmp[10] = { '\0' };

	*dest = '\0';

	while (howmany--) {
		snprintf(tmp, sizeof tmp, "%d", numbers[howmany]);
		strlcat(dest, tmp, dsize);

		if (howmany > 1)
			strlcpy(tmp, ", ", sizeof tmp);
		else if (howmany == 1)
			strlcpy(tmp, " and ", sizeof tmp);
		else
			strlcpy(tmp, ".\n", sizeof tmp);

		if (strlcat(dest, tmp, dsize) >= dsize) {
			(void) fprintf(stderr, "FICS: %s: warning: strlcat() "
			    "truncated\n", __func__);
		}
	}
}

PRIVATE void
com_stats_rating(char *hdr, statistics *stats, char *dest, const size_t dsize)
{
	char tmp[100] = { '\0' };

	snprintf(dest, dsize, "%-10s%4s    %5.1f   %4d   %4d   %4d   %4d", hdr,
	    ratstr(stats->rating),
	    stats->sterr,
	    stats->win,
	    stats->los,
	    stats->dra,
	    stats->num);

	if (stats->whenbest) {
		snprintf(tmp, sizeof tmp, "   %d", stats->best);
		strlcat(dest, tmp, dsize);
		strftime(tmp, sizeof tmp, " (%d-%b-%y)",
		    localtime((time_t *) &stats->whenbest));
		strlcat(dest, tmp, dsize);
	}

	if (strlcat(dest, "\n", dsize) >= dsize) {
		(void) fprintf(stderr, "FICS: %s (line %d): warning: strlcat() "
		    "truncated\n", __func__, __LINE__);
	}
}

PUBLIC int
com_stats(int p, param_list param)
{
#define NUMBERS_SIZE \
	(MAX_OBSERVE > MAX_SIMUL ? MAX_OBSERVE : MAX_SIMUL)
	char		 line[255] = { '\0' };
	char		 tmp[255] = { '\0' };
	int		 g, i, t;
	int		 numbers[NUMBERS_SIZE];
	int		 onTime;
	int		 p1, connected;

	if (param[0].type == TYPE_WORD) {
		if (!FindPlayer(p, param[0].val.word, &p1, &connected))
			return COM_OK;
	} else {
		p1 = p;
		connected = 1;
	}

	snprintf(line, sizeof line, "\nStatistics for %-11s ", parray[p1].name);

	if (connected && parray[p1].status == PLAYER_PROMPT) {
		snprintf(tmp, sizeof tmp, "On for: %s",
		    hms_desc(player_ontime(p1)));
		strlcat(line, tmp, sizeof line);
		snprintf(tmp, sizeof tmp, "   Idle: %s\n",
		    hms_desc(player_idle(p1)));
	} else {
		time_t last;

		if ((last = player_lastdisconnect(p1)) != 0) {
			snprintf(tmp, sizeof tmp, "(Last disconnected %s):\n",
			    strltime(&last));
		} else
			strlcpy(tmp, "(Never connected.)\n", sizeof tmp);
	}

	strlcat(line, tmp, sizeof line);
	pprintf(p, "%s", line);

	if (parray[p1].simul_info.numBoards) {
		t = 0;
		i = 0;

		while (i < parray[p1].simul_info.numBoards) {
			if ((numbers[t] = parray[p1].simul_info.boards[i] + 1) != 0)
				t++;
			i++;
		}
		pprintf(p, "%s is giving a simul: game%s ", parray[p1].name,
		    (t > 1 ? "s" : ""));
		com_stats_andify(numbers, t, tmp, sizeof tmp);
		pprintf(p, "%s", tmp);
	} else if (parray[p1].game >= 0) {
		g = parray[p1].game;

		if (garray[g].status == GAME_EXAMINE) {
			pprintf(p, "(Examining game %d: %s vs. %s)\n", (g + 1),
			    garray[g].white_name,
			    garray[g].black_name);
		} else {
			pprintf(p, "(playing game %d: %s vs. %s)\n", (g + 1),
			    parray[garray[g].white].name,
			    parray[garray[g].black].name);

			if (garray[g].link >= 0) {
				pprintf(p, "(partner is playing game %d: "
				    "%s vs. %s)\n",
				    (garray[g].link + 1),
				    parray[garray[garray[g].link].white].name,
				    parray[garray[garray[g].link].black].name);
			}
		}
	}

	if (parray[p1].num_observe) {
		t = 0;
		i = 0;

		while (i < parray[p1].num_observe) {
			if ((g = parray[p1].observe_list[i]) != -1 &&
			    (parray[p].adminLevel >= ADMIN_ADMIN ||
			    garray[g].private == 0))
				numbers[t++] = (g + 1);
			i++;
		}

		if (t) {
			pprintf(p, "%s is observing game%s ", parray[p1].name,
			    (t > 1 ? "s" : ""));
			com_stats_andify(numbers, t, tmp, sizeof tmp);
			pprintf(p, "%s", tmp);
		}
	}

	if (parray[p1].busy[0])
		pprintf(p, "(%s %s)\n", parray[p1].name, parray[p1].busy);

	if (!parray[p1].registered) {
		pprintf(p, "%s is NOT a registered player.\n\n",
		    parray[p1].name);
	} else {
		pprintf(p, "\n         rating     RD     win   loss   draw  "
		    "total   best\n");

		com_stats_rating("Blitz", &parray[p1].b_stats, tmp, sizeof tmp);
		pprintf(p, "%s", tmp);
		com_stats_rating("Standard", &parray[p1].s_stats, tmp,
		    sizeof tmp);
		pprintf(p, "%s", tmp);
		com_stats_rating("Lightning", &parray[p1].l_stats, tmp,
		    sizeof tmp);
		pprintf(p, "%s", tmp);
		com_stats_rating("Wild", &parray[p1].w_stats, tmp, sizeof tmp);
		pprintf(p, "%s", tmp);
	}

	pprintf(p, "\n");

	if (parray[p1].adminLevel > 0) {
		pprintf(p, "Admin Level: ");

		switch (parray[p1].adminLevel) {
		case 5:
			pprintf(p, "Authorized Helper Person\n");
			break;
		case 10:
			pprintf(p, "Administrator\n");
			break;
		case 15:
			pprintf(p, "Help File Librarian/Administrator\n");
			break;
		case 20:
			pprintf(p, "Master Administrator\n");
			break;
		case 50:
			pprintf(p, "Master Help File Librarian/Administrator"
			    "\n");
			break;
		case 60:
			pprintf(p, "Assistant Super User\n");
			break;
		case 100:
			pprintf(p, "Super User\n");
			break;
		default:
			pprintf(p, "%d\n", parray[p1].adminLevel);
			break;
		}
	}

	// Full Name
	if (parray[p].adminLevel > 0) {
		pprintf(p, "Full Name  : %s\n", (parray[p1].fullName ?
		    parray[p1].fullName : "(none)"));
	}

	// Address
	if ((p1 == p && parray[p1].registered) || parray[p].adminLevel > 0) {
		pprintf(p, "Address    : %s\n", (parray[p1].emailAddress ?
		    parray[p1].emailAddress : "(none)"));
	}

	// Host
	if (parray[p].adminLevel > 0) {
		pprintf(p, "Host       : %s\n", dotQuad(connected ?
		    parray[p1].thisHost : parray[p1].lastHost));
	}

	// Comments
	if (parray[p].adminLevel > 0 && parray[p1].registered) {
		if (parray[p1].num_comments) {
			pprintf(p, "Comments   : %d\n",
			    parray[p1].num_comments);
		}
	}

	if (connected &&
	    parray[p1].registered &&
	    (p == p1 || parray[p].adminLevel > 0)) {
		char *timeToStr = ctime((time_t *) &parray[p1].timeOfReg);

		timeToStr[strlen(timeToStr) - 1] = '\0';
		pprintf(p, "\n");

		onTime = ((time(NULL) - parray[p1].logon_time) +
		    parray[p1].totalTime);

		pprintf(p, "Total time on-line: %s\n", hms_desc(onTime));
		pprintf(p, "%% of life on-line:  %3.1f  (since %s)\n", // XXX
		    (double) ((onTime * 100) / (double) (time(NULL) -
		    parray[p1].timeOfReg)),
		    timeToStr);
	}

#ifdef TIMESEAL
	if (connected) {
		pprintf(p, "\nTimeseal   : %s\n",
		    (con[parray[p1].socket].timeseal ? "On" : "Off"));
	}
	if (parray[p].adminLevel > 0 && connected) {
		if (findConnection(parray[p1].socket) &&
		    con[parray[p1].socket].timeseal) {
			pprintf(p, "Unix acc   : %s\nSystem/OS  : %s\n",
			    con[parray[p1].socket].user,
			    con[parray[p1].socket].sys);
		}
	}
#endif

	if (parray[p1].num_plan) {
		pprintf(p, "\n");

		for (i = 0; i < parray[p1].num_plan; i++) {
			pprintf(p, "%2d: %s\n", (i + 1),
			    (parray[p1].planLines[i] != NULL
			    ? parray[p1].planLines[i]
			    : ""));
		}
	}

	if (!connected)
		player_remove(p1);
	return COM_OK;
}

PUBLIC int
com_password(int p, param_list param)
{
	char	*oldpassword = param[0].val.word;
	char	*newpassword = param[1].val.word;
	char	 salt[FICS_SALT_SIZE];

	if (!parray[p].registered) {
		pprintf(p, "Setting a password is only for registered players."
		    "\n");
		return COM_OK;
	}

	if (parray[p].passwd) {
		strncpy(salt, &(parray[p].passwd[0]), sizeof salt - 1);
		salt[sizeof salt - 1] = '\0';

		if (strcmp(crypt(oldpassword, salt), parray[p].passwd)) {
			pprintf(p, "Incorrect password, password not changed!"
			    "\n");
			return COM_OK;
		}

		rfree(parray[p].passwd);
		parray[p].passwd = NULL;
	}

	strlcpy(salt, fics_getsalt(), sizeof salt);
	parray[p].passwd = xstrdup(crypt(newpassword, salt));

	pprintf(p, "Password changed to \"%s\".\n", newpassword);
	return COM_OK;
}

PUBLIC int
com_uptime(int p, param_list param)
{
	int days, hours, mins, secs;
	struct rusage ru;
	unsigned long int uptime = (time(NULL) - startuptime);

	(void) param; // XXX: unused

	days	= (uptime / (60*60*24));
	hours	= ((uptime % (60*60*24)) / (60*60));
	mins	= (((uptime % (60*60*24)) % (60*60)) / 60);
	secs	= (((uptime % (60*60*24)) % (60*60)) % 60);

	pprintf(p, "Server location: %s   Server version : %s\n", fics_hostname,
	    VERS_NUM);
	pprintf(p, "The server has been up since %s.\n",
	    strltime(&startuptime));

	if (days == 0 && hours == 0 && mins == 0) {
		pprintf(p, "(Up for %d second%s)\n",
		    secs, (secs == 1 ? "" : "s"));
	} else if (days == 0 && hours == 0) {
		pprintf(p, "(Up for %d minute%s and %d second%s)\n",
		    mins, (mins == 1 ? "" : "s"),
		    secs, (secs == 1 ? "" : "s"));
	} else if (days == 0) {
		pprintf(p, "(Up for %d hour%s, %d minute%s and %d second%s)\n",
		    hours, (hours == 1 ? "" : "s"),
		    mins, (mins == 1 ? "" : "s"),
		    secs, (secs == 1 ? "" : "s"));
	} else {
		pprintf(p, "(Up for %d day%s, %d hour%s, %d minute%s and %d "
		    "second%s)\n",
		    days, (days == 1 ? "" : "s"),
		    hours, (hours == 1 ? "" : "s"),
		    mins, (mins == 1 ? "" : "s"),
		    secs, (secs == 1 ? "" : "s"));
	}

	pprintf(p, "\nAllocs: %u  Frees: %u  Allocs In Use: %u\n",
	    malloc_count, free_count, (malloc_count - free_count));
	if (parray[p].adminLevel >= ADMIN_ADMIN) {
		pprintf(p, "\nplayer size:%zu, game size:%zu, con size:%d, "
		    "g_num:%d\n", sizeof(player), sizeof(game), net_consize(),
		    g_num);

		getrusage(RUSAGE_SELF, &ru);

		pprintf(p, "pagesize = %d, maxrss = %ld, total = %ld\n",
		    getpagesize(),
		    ru.ru_maxrss,
		    (getpagesize() * ru.ru_maxrss));
	}

	pprintf(p, "\nPlayer limit: %d\n", (max_connections - 10));
	pprintf(p, "\nThere are currently %d players, with a high of %d "
	    "since last restart.\n", player_count(1), player_high);
	pprintf(p, "There are currently %d games, with a high of %d "
	    "since last restart.\n", game_count(), game_high);
	pprintf(p, "\nCompiled on %s\n", COMP_DATE);

	return COM_OK;
}

PUBLIC int
com_date(int p, param_list param)
{
	time_t t = time(NULL);

	(void) param; // XXX: unused

	pprintf(p, "Local time     - %s\n", strltime(&t));
	pprintf(p, "Greenwich time - %s\n", strgtime(&t));

	return COM_OK;
}

PRIVATE int
plogins(int p, char *fname)
{
	FILE		*fp = NULL;
	char		 ipstr[20] = { '\0' };
	char		 loginName[MAX_LOGIN_NAME + 1] = { '\0' };
	int		 registered = 0;
	long int	 lval = 0;
	time_t		 tval = 0;
	uint16_t	 inout = 0;

	if ((fp = fopen(fname, "r")) == NULL) {
		pprintf(p, "Sorry, no login information available.\n");
		return COM_OK;
	}

	_Static_assert(19 < ARRAY_SIZE(ipstr), "'ipstr' too small");
	_Static_assert(19 < ARRAY_SIZE(loginName), "'loginName' too small");

	while (!feof(fp)) {
		if (fscanf(fp, "%hu %19s %ld %d %19s\n", &inout, loginName,
		    &lval, &registered, ipstr) != 5) {
			fprintf(stderr, "FICS: Error in login info format. "
			    "%s\n", fname);
			fclose(fp);
			return COM_OK;
		}

		tval = lval;

		if (inout >= ARRAY_SIZE(inout_string)) {
			warnx("%s: %s: 'inout' too large (%u)", __func__, fname,
			    inout);
		} else {
			pprintf(p, "%s: %-17s %-6s", strltime(&tval), loginName,
			    inout_string[inout]);
		}

		if (parray[p].adminLevel > 0)
			pprintf(p, " from %s\n", ipstr);
		else
			pprintf(p, "\n");
	}

	fclose(fp);
	return COM_OK;
}

PUBLIC int
com_llogons(int p, param_list param)
{
	char fname[MAX_FILENAME_SIZE] = { '\0' };

	(void) param; // XXX: unused

	snprintf(fname, sizeof fname, "%s/%s", stats_dir, STATS_LOGONS);
	return plogins(p, fname);
}

PUBLIC int
com_logons(int p, param_list param)
{
	char fname[MAX_FILENAME_SIZE] = { '\0' };

	if (param[0].type == TYPE_WORD) {
		snprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s",
		    stats_dir, param[0].val.word[0], param[0].val.word,
		    STATS_LOGONS);
	} else {
		snprintf(fname, sizeof fname, "%s/player_data/%c/%s.%s",
		    stats_dir, parray[p].login[0], parray[p].login,
		    STATS_LOGONS);
	}
	return plogins(p, fname);
}

PRIVATE void
AddPlayerLists(int p1, char *ptmp, const size_t size)
{
	if (parray[p1].adminLevel >= 10 && parray[p1].i_admin)
		strlcat(ptmp, "(*)", size);
	if (in_list(p1, L_COMPUTER, parray[p1].name))
		strlcat(ptmp, "(C)", size);
	if (in_list(p1, L_FM, parray[p1].name))
		strlcat(ptmp, "(FM)", size);
	if (in_list(p1, L_IM, parray[p1].name))
		strlcat(ptmp, "(IM)", size);
	if (in_list(p1, L_GM, parray[p1].name))
		strlcat(ptmp, "(GM)", size);
	if (in_list(p1, L_TD, parray[p1].name))
		strlcat(ptmp, "(TD)", size);
	if (in_list(p1, L_TEAMS, parray[p1].name))
		strlcat(ptmp, "(T)", size);
	if (in_list(p1, L_BLIND, parray[p1].name))
		strlcat(ptmp, "(B)", size);
}

PRIVATE void
who_terse(int p, int num, int *plist, int type)
{
	char		 ptmp[80 + 20] = { '\0' }; // for highlight
	int		 i;
	int		 p1;
	int		 rat;
	multicol	*m = multicol_start(PARRAY_SIZE);

	for (i = 0; i < num; i++) {
		p1 = plist[i];

		if (type == blitz_rat)
			rat = parray[p1].b_stats.rating;
		else if (type == wild_rat)
			rat = parray[p1].w_stats.rating;
		else if (type == std_rat)
			rat = parray[p1].s_stats.rating;
		else if (type == light_rat)
			rat = parray[p1].l_stats.rating;
		else	// Fallback to std...
			rat = parray[p1].s_stats.rating;

		if (type == none) {
			strlcpy(ptmp, "     ", sizeof ptmp);
		} else {
			snprintf(ptmp, sizeof ptmp, "%-4s", ratstrii(rat,
			    parray[p1].registered));

			if (parray[p1].simul_info.numBoards) {
				strlcat(ptmp, "~", sizeof ptmp);
			} else if (parray[p1].game >= 0 &&
			    garray[parray[p1].game].status == GAME_EXAMINE) {
				strlcat(ptmp, "#", sizeof ptmp);
			} else if (parray[p1].game >= 0) {
				strlcat(ptmp, "^", sizeof ptmp);
			} else if (!parray[p1].open) {
				strlcat(ptmp, ":", sizeof ptmp);
			} else if (player_idle(p1) > 300) {
				strlcat(ptmp, ".", sizeof ptmp);
			} else {
				strlcat(ptmp, " ", sizeof ptmp);
			}
		}

		if (p == p1) {
			const size_t len = strlen(ptmp);

			psprintf_highlight(p, ptmp + len,
			    sizeof ptmp - len, "%s", parray[p1].name);
		} else {
			strlcat(ptmp, parray[p1].name, sizeof ptmp);
		}

		AddPlayerLists(p1, ptmp, sizeof ptmp);
		multicol_store(m, ptmp);
	}

	multicol_pprint(m, p, 80, 2);
	multicol_end(m);

	pprintf(p, "\n %d players displayed (of %d). (*) indicates system "
	    "administrator.\n", num, player_count(1));
}

PRIVATE void
who_verbose(int p, int num, int plist[])
{
	char	 p1WithAttrs[255] = { '\0' };
	char	 playerLine[255] = { '\0' };
	char	 tmp[255] = { '\0' };
	int	 p1;
	int	 ret, too_long;

	pprintf(p, " +---------------------------------------------------------------+\n");
	pprintf(p, " |      User              Standard    Blitz        On for   Idle |\n");
	pprintf(p, " +---------------------------------------------------------------+\n");

	for (int i = 0; i < num; i++) {
		p1 = plist[i];
		strlcpy(playerLine, " |", sizeof playerLine);

		if (parray[p1].game >= 0)
			snprintf(tmp, sizeof tmp, "%3d", parray[p1].game + 1);
		else
			strlcpy(tmp, "   ", sizeof tmp);

		strlcat(playerLine, tmp, sizeof playerLine);

		if (!parray[p1].open)
			strlcpy(tmp, "X", sizeof tmp);
		else
			strlcpy(tmp, " ", sizeof tmp);

		strlcat(playerLine, tmp, sizeof playerLine);

		if (parray[p1].registered) {
			if (parray[p1].rated)
				strlcpy(tmp, " ", sizeof tmp);
			else
				strlcpy(tmp, "u", sizeof tmp);
		} else {
			strlcpy(tmp, "U", sizeof tmp);
		}

		strlcat(playerLine, tmp, sizeof playerLine);
		strlcpy(p1WithAttrs, parray[p1].name, sizeof p1WithAttrs);
		AddPlayerLists(p1, p1WithAttrs, sizeof p1WithAttrs);
		p1WithAttrs[17] = '\0';

		if (p == p1) {
			size_t len;

			strlcpy(tmp, " ", sizeof tmp);
			len = strlen(tmp);
			psprintf_highlight(p, tmp + len, sizeof tmp - len,
			    "%-17s", p1WithAttrs);
		} else {
			ret = snprintf(tmp, sizeof tmp, " %-17s", p1WithAttrs);

			too_long = (ret < 0 || (size_t)ret >= sizeof tmp);

			if (too_long) {
				fprintf(stderr, "FICS: %s: warning: "
				    "snprintf truncated\n", __func__);
			}
		}

		strlcat(playerLine, tmp, sizeof playerLine);
		snprintf(tmp, sizeof tmp, " %4s        %-4s        %5s  ",
		    ratstrii(parray[p1].s_stats.rating,
		    parray[p1].registered),
		    ratstrii(parray[p1].b_stats.rating,
		    parray[p1].registered),
		    hms(player_ontime(p1), 0, 0, 0));
		strlcat(playerLine, tmp, sizeof playerLine);

		if (player_idle(p1) >= 60) {
			snprintf(tmp, sizeof tmp, "%5s   |\n",
			    hms(player_idle(p1), 0, 0, 0));
		} else {
			strlcpy(tmp, "        |\n", sizeof tmp);
		}

		strlcat(playerLine, tmp, sizeof playerLine);
		pprintf(p, "%s", playerLine);
	}

	pprintf(p, " |                                                               |\n");
	pprintf(p, " |    %3d Players Displayed                                      |\n", num);
	pprintf(p, " +---------------------------------------------------------------+\n");
}

PRIVATE void
who_winloss(int p, int num, int plist[])
{
	char	p1WithAttrs[255] = { '\0' };
	char	playerLine[255] = { '\0' };
	char	tmp[255] = { '\0' };
	int	p1;

	pprintf(p, "Name               Stand     win loss draw   Blitz    win loss draw    idle\n");
	pprintf(p, "----------------   -----     -------------   -----    -------------    ----\n");

	for (int i = 0; i < num; i++) {
		playerLine[0] = '\0';
		p1 = plist[i];

		/* Modified by hersco to include lists in 'who n.' */
		strlcpy(p1WithAttrs, parray[p1].name, sizeof p1WithAttrs);
		AddPlayerLists(p1, p1WithAttrs, sizeof p1WithAttrs);
		p1WithAttrs[17] = '\0';

		if (p1 == p) {
			psprintf_highlight(p, playerLine, sizeof playerLine,
			    "%-17s", p1WithAttrs);
		} else {
			snprintf(playerLine, sizeof playerLine, "%-17s",
			    p1WithAttrs);
		}

		snprintf(tmp, sizeof tmp, "  %4s     %4d %4d %4d   ",
		    ratstrii(parray[p1].s_stats.rating, parray[p1].registered),
		    parray[p1].s_stats.win,
		    parray[p1].s_stats.los,
		    parray[p1].s_stats.dra);
		strlcat(playerLine, tmp, sizeof playerLine);

		snprintf(tmp, sizeof tmp, "%4s    %4d %4d %4d   ",
		    ratstrii(parray[p1].b_stats.rating, parray[p1].registered),
		    parray[p1].b_stats.win,
		    parray[p1].b_stats.los,
		    parray[p1].b_stats.dra);
		strlcat(playerLine, tmp, sizeof playerLine);

		if (player_idle(p1) >= 60) {
			snprintf(tmp, sizeof tmp, "%5s\n", hms(player_idle(p1),
			    0, 0, 0));
		} else {
			strlcpy(tmp, "\n", sizeof tmp);
		}

		strlcat(playerLine, tmp, sizeof playerLine);
		pprintf(p, "%s", playerLine);
	}

	pprintf(p, "    %3d Players Displayed.\n", num);
}

PRIVATE int
who_ok(int p, unsigned int sel_bits)
{
	int p2;

	if (parray[p].status != PLAYER_PROMPT)
		return 0;
	if (sel_bits == WHO_ALL)
		return 1;

	if (sel_bits & WHO_OPEN) {
		if (!parray[p].open)
			return 0;
	}
	if (sel_bits & WHO_CLOSED) {
		if (parray[p].open)
			return 0;
	}
	if (sel_bits & WHO_RATED) {
		if (!parray[p].rated)
			return 0;
	}
	if (sel_bits & WHO_UNRATED) {
		if (parray[p].rated)
			return 0;
	}
	if (sel_bits & WHO_FREE) {
		if (parray[p].game >= 0)
			return 0;
	}
	if (sel_bits & WHO_PLAYING) {
		if (parray[p].game < 0)
			return 0;
	}
	if (sel_bits & WHO_REGISTERED) {
		if (!parray[p].registered)
			return 0;
	}
	if (sel_bits & WHO_UNREGISTERED) {
		if (parray[p].registered)
			return 0;
	}
	if (sel_bits & WHO_BUGTEAM) {
		if ((p2 = parray[p].partner) < 0 || parray[p2].partner != p)
			return 0;
	}

	return 1;
}

PRIVATE int
blitz_cmp(const void *pp1, const void *pp2)
{
	register int	p1 = *(int *) pp1;
	register int	p2 = *(int *) pp2;

	if (parray[p1].status != PLAYER_PROMPT) {
		if (parray[p2].status != PLAYER_PROMPT)
			return 0;
		else
			return -1;
	}
	if (parray[p2].status != PLAYER_PROMPT)
		return 1;
	if (parray[p1].b_stats.rating > parray[p2].b_stats.rating)
		return -1;
	if (parray[p1].b_stats.rating < parray[p2].b_stats.rating)
		return 1;
	if (parray[p1].registered > parray[p2].registered)
		return -1;
	if (parray[p1].registered < parray[p2].registered)
		return 1;
	return strcmp(parray[p1].login, parray[p2].login);
}

PRIVATE int
light_cmp(const void *pp1, const void *pp2)
{
	register int	p1 = *(int *) pp1;
	register int	p2 = *(int *) pp2;

	if (parray[p1].status != PLAYER_PROMPT) {
		if (parray[p2].status != PLAYER_PROMPT)
			return 0;
		else
			return -1;
	}
	if (parray[p2].status != PLAYER_PROMPT)
		return 1;
	if (parray[p1].l_stats.rating > parray[p2].l_stats.rating)
		return -1;
	if (parray[p1].l_stats.rating < parray[p2].l_stats.rating)
		return 1;
	if (parray[p1].registered > parray[p2].registered)
		return -1;
	if (parray[p1].registered < parray[p2].registered)
		return 1;
	return strcmp(parray[p1].login, parray[p2].login);
}

PRIVATE int
stand_cmp(const void *pp1, const void *pp2)
{
	register int	p1 = *(int *) pp1;
	register int	p2 = *(int *) pp2;

	if (parray[p1].status != PLAYER_PROMPT) {
		if (parray[p2].status != PLAYER_PROMPT)
			return 0;
		else
			return -1;
	}
	if (parray[p2].status != PLAYER_PROMPT)
		return 1;
	if (parray[p1].s_stats.rating > parray[p2].s_stats.rating)
		return -1;
	if (parray[p1].s_stats.rating < parray[p2].s_stats.rating)
		return 1;
	if (parray[p1].registered > parray[p2].registered)
		return -1;
	if (parray[p1].registered < parray[p2].registered)
		return 1;
	return strcmp(parray[p1].login, parray[p2].login);
}

PRIVATE int
wild_cmp(const void *pp1, const void *pp2)
{
	register int	p1 = *(int *) pp1;
	register int	p2 = *(int *) pp2;

	if (parray[p1].status != PLAYER_PROMPT) {
		if (parray[p2].status != PLAYER_PROMPT)
			return 0;
		else
			return -1;
	}
	if (parray[p2].status != PLAYER_PROMPT)
		return 1;
	if (parray[p1].w_stats.rating > parray[p2].w_stats.rating)
		return -1;
	if (parray[p1].w_stats.rating < parray[p2].w_stats.rating)
		return 1;
	if (parray[p1].registered > parray[p2].registered)
		return -1;
	if (parray[p1].registered < parray[p2].registered)
		return 1;
	return strcmp(parray[p1].login, parray[p2].login);
}

PRIVATE int
alpha_cmp(const void *pp1, const void *pp2)
{
	register int	p1 = *(int *) pp1;
	register int	p2 = *(int *) pp2;

	if (parray[p1].status != PLAYER_PROMPT) {
		if (parray[p2].status != PLAYER_PROMPT)
			return 0;
		else
			return -1;
	}
	if (parray[p2].status != PLAYER_PROMPT)
		return 1;
	return strcmp(parray[p1].login, parray[p2].login);
}

PRIVATE void
sort_players(int players[PARRAY_SIZE], who_cmp_t cmp_func)
{
	if (p_num <= 0) {
		warnx("%s: p_num <= 0", __func__);
		return;
	}
	for (int i = 0; i < p_num; i++)
		players[i] = i;
	qsort(players, p_num, sizeof(int), cmp_func);
}

/*
 * This is the of the most compliclicated commands in terms of
 * parameters.
 */
PUBLIC int
com_who(int p, param_list param)
{
	char		 c;
	float		 start_perc = 0;
	float		 stop_perc = 1.0;
	int		 i, len;
	int		 p1, count, num_who;
	int		 plist[PARRAY_SIZE];
	int		 sort_type = blitz_rat;
	int		 sortlist[PARRAY_SIZE];
	int		 startpoint;
	int		 stoppoint;
	int		 style = 0;
	int		 tmpI, tmpJ;
	unsigned int	 sel_bits = WHO_ALL;
	who_cmp_t	 cmp_func = blitz_cmp;

	if (param[0].type != TYPE_NULL) {
		len = strlen(param[0].val.string);

		for (i = 0; i < len; i++) {
			c = param[0].val.string[i];

			if (isdigit(c)) {
				if (i == 0 || !isdigit(param[0].val.string[i - 1])) {
					tmpI = (c - '0');

					if (tmpI == 1) {
						start_perc = 0.0;
						stop_perc = 0.333333;
					} else if (tmpI == 2) {
						start_perc = 0.333333;
						stop_perc = 0.6666667;
					} else if (tmpI == 3) {
						start_perc = 0.6666667;
						stop_perc = 1.0;
					} else if (i == (len - 1) ||
					    !isdigit(param[0].val.string[i + 1]))
						return COM_BADPARAMETERS;
				} else {
					tmpI = c - '0';
					tmpJ = (param[0].val.string[i - 1] -
					    '0');

					if (tmpI == 0)
						return COM_BADPARAMETERS;
					if (tmpJ > tmpI)
						return COM_BADPARAMETERS;

					start_perc = ((float) tmpJ - 1.0) /
					    (float) tmpI;
					stop_perc = ((float) tmpJ) /
					    (float) tmpI;
				}
			} else {
				switch (c) {
				case ' ':
				case '\n':
				case '\t':
					break;
				case 'o':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_OPEN;
					else
						sel_bits |= WHO_OPEN;
					break;
				case 'r':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_RATED;
					else
						sel_bits |= WHO_RATED;
					break;
				case 'f':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_FREE;
					else
						sel_bits |= WHO_FREE;
					break;
				case 'a':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_FREE | WHO_OPEN;
					else
						sel_bits |= (WHO_FREE | WHO_OPEN);
					break;
				case 'R':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_REGISTERED;
					else
						sel_bits |= WHO_REGISTERED;
					break;
				case 'l':	// Sort order
					cmp_func	= alpha_cmp;
					sort_type	= none;
					break;
				case 'A':	// Sort order
					cmp_func	= alpha_cmp;
					sort_type	= none;
					break;
				case 'w':	// Sort order
					cmp_func	= wild_cmp;
					sort_type	= wild_rat;
					break;
				case 's':	// Sort order
					cmp_func	= stand_cmp;
					sort_type	= std_rat;
					break;
				case 'b':	// Sort order
					cmp_func	= blitz_cmp;
					sort_type	= blitz_rat;
					break;
				case 'L':	// Sort order
					cmp_func	= light_cmp;
					sort_type	= light_rat;
					break;
				case 't':	// format
					style = 0;
					break;
				case 'v':	// format
					style = 1;
					break;
				case 'n':	// format
					style = 2;
					break;
				case 'U':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_UNREGISTERED;
					else
						sel_bits |= WHO_UNREGISTERED;
					break;
				case 'B':
					if (sel_bits == WHO_ALL)
						sel_bits = WHO_BUGTEAM;
					else
						sel_bits |= WHO_BUGTEAM;
					break;
				default:
					return COM_BADPARAMETERS;
				}
			}
		}
	}

	sort_players(sortlist, cmp_func);
	count = 0;

	for (p1 = 0; p1 < p_num; p1++) {
		if (!who_ok(sortlist[p1], sel_bits))
			continue;
		count++;
	}

	startpoint	= floor((float) count * start_perc);
	stoppoint	= ceil((float) count * stop_perc) - 1;
	num_who		= 0;
	count		= 0;

	for (p1 = 0; p1 < p_num; p1++) {
		if (!who_ok(sortlist[p1], sel_bits))
			continue;
		if (count >= startpoint && count <= stoppoint)
			plist[num_who++] = sortlist[p1];

		count++;
	}

	if (num_who == 0) {
		pprintf(p, "No logged in players match the flags in your who "
		    "request.\n");
		return COM_OK;
	}

	switch (style) {
	case 0:		// terse
		who_terse(p, num_who, plist, sort_type);
		break;
	case 1:		// verbose
		who_verbose(p, num_who, plist);
		break;
	case 2:		// win-loss
		who_winloss(p, num_who, plist);
		break;
	default:
		return COM_BADPARAMETERS;
		break;
	}

	return COM_OK;
}

PUBLIC int
com_refresh(int p, param_list param)
{
	int g, p1;

	if (param[0].type == TYPE_NULL) {
		if (parray[p].game >= 0) {
			send_board_to(parray[p].game, p);
		} else { /* Do observing in here */
			if (parray[p].num_observe) {
				for (g = 0; g < parray[p].num_observe; g++) {
					send_board_to(parray[p].observe_list[g],
					    p);
				}
			} else {
				pprintf(p, "You are neither playing, observing "
				    "nor examining a game.\n");
				return COM_OK;
			}
		}
	} else {
		if ((g = GameNumFromParam(p, &p1, &param[0])) < 0)
			return COM_OK;
		if (g >= g_num ||
		    (garray[g].status != GAME_ACTIVE &&
		    garray[g].status != GAME_EXAMINE)) {
			pprintf(p, "No such game.\n");
		} else if (garray[g].private && parray[p].adminLevel ==
		    ADMIN_USER) {
			pprintf(p, "Sorry, game %d is a private game.\n",
			    (g + 1));
		} else {
			if (garray[g].private) {
				pprintf(p, "Refreshing PRIVATE game %d\n",
				    (g + 1));
			}
			send_board_to(g, p);
		}
	}

	return COM_OK;
}

PUBLIC int
com_prefresh(int p, param_list param)
{
	int retval, part;

	(void) param;

	if ((part = parray[p].partner) < 0) {
		pprintf(p, "You do not have a partner.\n");
		return COM_OK;
	} else if ((retval = pcommand(p, "refresh %s", parray[part].name)) ==
	    COM_OK)
		return COM_OK_NOPROMPT;
	return retval;
}

PUBLIC int
com_open(int p, param_list param)
{
	int retval;

	(void) param;
	ASSERT(param[0].type == TYPE_NULL);

	if ((retval = pcommand(p, "set open")) != COM_OK)
		return retval;
	return COM_OK_NOPROMPT;
}

PUBLIC int
com_simopen(int p, param_list param)
{
	int retval;

	(void) param;
	ASSERT(param[0].type == TYPE_NULL);

	if ((retval = pcommand(p, "set simopen")) != COM_OK)
		return retval;
	return COM_OK_NOPROMPT;
}

PUBLIC int
com_bell(int p, param_list param)
{
	int retval;

	(void) param;
	ASSERT(param[0].type == TYPE_NULL);

	if ((retval = pcommand(p, "set bell")) != COM_OK)
		return retval;
	return COM_OK_NOPROMPT;
}

PUBLIC int
com_flip(int p, param_list param)
{
	int retval;

	(void) param;
	ASSERT(param[0].type == TYPE_NULL);

	if ((retval = pcommand(p, "set flip")) != COM_OK)
		return retval;
	return COM_OK_NOPROMPT;
}

PUBLIC int
com_highlight(int p, param_list param)
{
	// XXX: unused
	(void) param;

	pprintf(p, "Obsolete command. Please do set highlight <0-15>.\n");
	return COM_OK;
}

PUBLIC int
com_style(int p, param_list param)
{
	int retval;

	ASSERT(param[0].type == TYPE_INT);

	if ((retval = pcommand(p, "set style %d", param[0].val.integer)) !=
	    COM_OK)
		return retval;
	return COM_OK_NOPROMPT;
}

PUBLIC int
com_promote(int p, param_list param)
{
	int retval;

	ASSERT(param[0].type == TYPE_WORD);

	if ((retval = pcommand(p, "set promote %s", param[0].val.word)) !=
	    COM_OK)
		return retval;
	return COM_OK_NOPROMPT;
}

PUBLIC int
com_alias(int p, param_list param)
{
	int al;

	if (param[0].type == TYPE_NULL) {
		for (al = 0; al < parray[p].numAlias; al++) {
			pprintf(p, "%s -> %s\n",
			    parray[p].alias_list[al].comm_name,
			    parray[p].alias_list[al].alias);
		}

		return COM_OK;
	}

	al = alias_lookup(param[0].val.word, parray[p].alias_list,
	    parray[p].numAlias);

	if (param[1].type == TYPE_NULL) {
		if (al < 0) {
			pprintf(p, "You have no alias named '%s'.\n",
			    param[0].val.word);
		} else {
			pprintf(p, "%s -> %s\n",
			    parray[p].alias_list[al].comm_name,
			    parray[p].alias_list[al].alias);
		}
	} else {
		if (al < 0) {
			if (parray[p].numAlias >= MAX_ALIASES - 1) {
				pprintf(p, "You have your maximum of %d "
				    "aliases.\n", (MAX_ALIASES - 1));
			} else {
				if (!strcmp(param[0].val.string, "quit")) {
					pprintf(p, "You can't alias this "
					    "command.\n");
				} else if (!strcmp(param[0].val.string,
				    "unalias")) {
					pprintf(p, "You can't alias this "
					    "command.\n");
				} else {
					parray[p].alias_list[parray[p].numAlias].comm_name =
					    xstrdup(param[0].val.word);
					parray[p].alias_list[parray[p].numAlias].alias =
					    xstrdup(param[1].val.string);
					parray[p].numAlias++;
					pprintf(p, "Alias set.\n");
				}
			}
		} else {
			rfree(parray[p].alias_list[al].alias);
			parray[p].alias_list[al].alias =
			    xstrdup(param[1].val.string);
			pprintf(p, "Alias replaced.\n");
		}

		parray[p].alias_list[parray[p].numAlias].comm_name = NULL;
	}

	return COM_OK;
}

PUBLIC int
com_unalias(int p, param_list param)
{
	int al;

	ASSERT(param[0].type == TYPE_WORD);

	al = alias_lookup(param[0].val.word, parray[p].alias_list,
	    parray[p].numAlias);

	if (al < 0) {
		pprintf(p, "You have no alias named '%s'.\n",
		    param[0].val.word);
	} else {
		bool		removed = false;
		const int	sz = (int) ARRAY_SIZE(parray[0].alias_list);

		rfree(parray[p].alias_list[al].comm_name);
		rfree(parray[p].alias_list[al].alias);

		parray[p].alias_list[al].comm_name = NULL;
		parray[p].alias_list[al].alias = NULL;

		for (int i = al; i < parray[p].numAlias; i++) {
			if (i >= sz || i + 1 >= sz) {
				warnx("%s: overflowed array index read/write",
				    __func__);
				break;
			}

			parray[p].alias_list[i].comm_name =
			    parray[p].alias_list[i + 1].comm_name;
			parray[p].alias_list[i].alias =
			    parray[p].alias_list[i + 1].alias;
			removed = true;
		}

		if (!removed) {
			pprintf(p, "Remove error.\n");
			return COM_FAILED;
		}

		parray[p].numAlias--;
		parray[p].alias_list[parray[p].numAlias].comm_name = NULL;
		pprintf(p, "Alias removed.\n");
	}

	return COM_OK;
}

PUBLIC int
com_servers(int p, param_list param)
{
	pprintf(p, "There are no other servers known to this server.\n");
	(void) param;
	return COM_OK;
}

PUBLIC int
com_index(int p, param_list param)
{
	char	*iwant, *filenames[1000];
	char	 index_default[] = "_index";
	int	 i;

	if (param[0].type == TYPE_NULL) {
		iwant = index_default;
	} else {
		iwant = param[0].val.word;

		if (!safestring(iwant)) {
			pprintf(p, "Illegal character in category %s.\n",
			    iwant);
			return COM_OK;
		}
	}

	i = search_directory(index_dir, iwant, filenames,
	    ARRAY_SIZE(filenames));

	if (i == 0) {
		pprintf(p, "No index entry for \"%s\".\n", iwant);
	} else if (i == 1 || !strcmp(*filenames, iwant)) {
		if (psend_file(p, index_dir, *filenames)) {
			/*
			 * We should never reach this unless the file
			 * was just deleted.
			 */
			pprintf(p, "Index file %s could not be found! ",
			    *filenames);
			pprintf(p, "Please inform an admin of this. "
			    "Thank you.\n");
		}
	} else {
		pprintf(p, "Matches:");
		display_directory(p, filenames, i);
	}

	return COM_OK;
}

PUBLIC int
com_help(int p, param_list param)
{
	char	*iwant, *filenames[1000];
	char	 help_default[] = "_help";
	int	 i, UseLang = parray[p].language;

	if (param[0].type == TYPE_NULL) {
		iwant = help_default;
	} else {
		iwant = param[0].val.word;

		if (!safestring(iwant)) {
			pprintf(p, "Illegal character in command %s.\n", iwant);
			return COM_OK;
		}
	}

	i = search_directory(help_dir[UseLang], iwant, filenames,
	    ARRAY_SIZE(filenames));

	if (i == 0) {
		if (UseLang != LANG_DEFAULT) {
			i += search_directory(help_dir[LANG_DEFAULT], iwant,
			    filenames, ARRAY_SIZE(filenames));

			if (i > 0) {
				pprintf(p, "No help available in %s; using %s "
				    "instead.\n",
				    Language(UseLang),
				    Language(LANG_DEFAULT));
				UseLang = LANG_DEFAULT;
			}
		}

		if (i == 0) {
			pprintf(p, "No help available on \"%s\".\n", iwant);
			return COM_OK;
		}
	}

	if (i == 1 || !strcmp(*filenames, iwant)) {
		if (psend_file(p, help_dir[UseLang], *filenames)) {
			/*
			 * We should never reach this unless the file
			 * was just deleted.
			 */
			pprintf(p, "Helpfile %s could not be found! ",
			    *filenames);
			pprintf(p, "Please inform an admin of this. "
			    "Thank you.\n");
		}
	} else {
		pprintf(p, "Matches:");
		display_directory(p, filenames, i);
		pprintf(p, "[Type \"info\" for a list of FICS general "
		    "information files.]\n");
	}

	return COM_OK;
}

PUBLIC int
com_info(int p, param_list param)
{
	char	*filenames[1000];
	int	 n;

	// XXX: unused
	(void) param;

	if ((n = search_directory(info_dir, NULL, filenames,
	    ARRAY_SIZE(filenames))) > 0)
		display_directory(p, filenames, n);
	return COM_OK;
}

PRIVATE int
FindAndShowFile(int p, param_list param, char *dir)
{
	char		*iwant, *filenames[1000];
	int		 i;

	if (param[0].type == TYPE_NULL) {
		iwant = NULL;
	} else {
		iwant = param[0].val.word;

		if (!safestring(iwant)) {
			pprintf(p, "Illegal character in filename %s.\n",
			    iwant);
			return COM_OK;
		}
	}

	i = search_directory(dir, iwant, filenames, ARRAY_SIZE(filenames));

	if (i == 0) {
		pprintf(p, "No information available on \"%s\".\n",
		    (iwant ? iwant : ""));
	} else if (i == 1 || !strcmp(*filenames, iwant ? iwant : "")) {
		if (psend_file(p, dir, *filenames)) {
			/*
			 * We should never reach this unless the file
			 * was just deleted.
			 */
			pprintf(p, "File %s could not be found! ", *filenames);
			pprintf(p, "Please inform an admin of this. "
			    "Thank you.\n");
		}
	} else {
		if (iwant && *iwant)
			pprintf(p, "Matches:\n");
		display_directory(p, filenames, i);
	}

	return COM_OK;
}

PUBLIC int
com_uscf(int p, param_list param)
{
	return FindAndShowFile(p, param, uscf_dir);
}

PUBLIC int
com_adhelp(int p, param_list param)
{
	return FindAndShowFile(p, param, adhelp_dir);
}

PUBLIC int
com_mailsource(int p, param_list param)
{
	char		*buffer[1000];
	char		*iwant;
	char		 fname[MAX_FILENAME_SIZE];
	char		 subj[120];
	int		 count;

	if (!parray[p].registered) {
		pprintf(p, "Only registered people can use the mailsource "
		    "command.\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_NULL)
		iwant = NULL;
	else
		iwant = param[0].val.word;

	if ((count = search_directory(source_dir, iwant, buffer,
	    ARRAY_SIZE(buffer))) == 0) {
		pprintf(p, "Found no source file matching \"%s\".\n",
		    (iwant ? iwant : ""));
	} else if ((count == 1) || !strcmp(iwant ? iwant : "", *buffer)) {
		snprintf(subj, sizeof subj, "FICS source file from server "
		    "%s: %s",
		    fics_hostname,
		    *buffer);
		snprintf(fname, sizeof fname, "%s/%s",
		    source_dir,
		    *buffer);

		mail_file_to_user(p, subj, fname);

		pprintf(p, "Source file %s sent to %s\n", *buffer,
		    parray[p].emailAddress);
	} else {
		pprintf(p, "Found %d source files matching that:\n", count);

		if (iwant && *iwant) {
			display_directory(p, buffer, count);
		} else { // this junk is to get *.c *.h
			char		*s;
			multicol	*m = multicol_start(count);

			for (int i = 0; i < count; i++) {
				s = (buffer[i] + strlen(buffer[i]) - 2);

				if (s >= buffer[i] &&
				    (!strcmp(s, ".c") || !strcmp(s, ".h")))
					multicol_store(m, buffer[i]);
			}

			multicol_pprint(m, p, 78, 1);
			multicol_end(m);
		}
	}

	return COM_OK;
}

PUBLIC int
com_mailhelp(int p, param_list param)
{
	char		*buffer[1000];
	char		*iwant;
	char		 fname[MAX_FILENAME_SIZE];
	char		 subj[120];
	int		 count;
	int		 lang = parray[p].language;

	if (!parray[p].registered) {
		pprintf(p, "Only registered people can use the mailhelp "
		    "command.\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_NULL)
		iwant = NULL;
	else
		iwant = param[0].val.word;

	count = search_directory(help_dir[lang], iwant, buffer,
	    ARRAY_SIZE(buffer));

	if (count == 0 && lang != LANG_DEFAULT) {
		count += search_directory(help_dir[LANG_DEFAULT], iwant, buffer,
		    ARRAY_SIZE(buffer));

		if (count > 0) {
			pprintf(p, "No help available in %s; "
			    "using %s instead.\n",
			    Language(lang),
			    Language(LANG_DEFAULT));
			lang = LANG_DEFAULT;
		}
	}

	if (count == 0) {
		pprintf(p, "Found no help file matching \"%s\".\n",
		    (iwant ? iwant : ""));
	} else if (count == 1 || !strcmp(*buffer, iwant ? iwant : "")) {
		snprintf(subj, sizeof subj, "FICS help file from server %s: %s",
		    fics_hostname,
		    *buffer);
		snprintf(fname, sizeof fname, "%s/%s",
		    help_dir[lang],
		    *buffer);

		mail_file_to_user(p, subj, fname);

		pprintf(p, "Help file %s sent to %s\n", *buffer,
		    parray[p].emailAddress);
	} else {
		pprintf(p, "Found %d helpfiles matching that:\n", count);
		display_directory(p, buffer, count);
	}

	return COM_OK;
}

PUBLIC int
com_handles(int p, param_list param)
{
	char	*buffer[1000];
	char	 pdir[MAX_FILENAME_SIZE];
	int	 count;

	snprintf(pdir, sizeof pdir, "%s/%c", player_dir, param[0].val.word[0]);

	count = search_directory(pdir, param[0].val.word, buffer,
	    ARRAY_SIZE(buffer));
	pprintf(p, "Found %d names.\n", count);

	if (count > 0)
		display_directory(p, buffer, count);

	return COM_OK;
}

PUBLIC int
com_getpi(int p, param_list param)
{
	int p1;

	if (!in_list(p, L_TD, parray[p].name)) {
		pprintf(p, "Only TD programs are allowed to use this command."
		    "\n");
		return COM_OK;
	}

	if ((p1 = player_find_bylogin(param[0].val.word)) < 0 ||
	    parray[p1].registered == 0)
		return COM_OK;

	if (!parray[p1].registered) {
		pprintf(p, "*getpi %s -1 -1 -1*\n", parray[p1].name);
	} else {
		pprintf(p, "*getpi %s %d %d %d*\n", parray[p1].name,
		    parray[p1].w_stats.rating,
		    parray[p1].b_stats.rating,
		    parray[p1].s_stats.rating);
	}

	return COM_OK;
}

PUBLIC int
com_getps(int p, param_list param)
{
	int p1;

	if (((p1 = player_find_bylogin(param[0].val.word)) < 0 ||
	    parray[p1].registered == 0) ||
	    parray[p1].game < 0) {
		pprintf(p, "*status %s 1*\n", param[0].val.word);
		return COM_OK;
	}

	pprintf(p, "*status %s 0 %s*\n", parray[p1].name,
	    parray[(parray[p1].opponent)].name);
	return COM_OK;
}

PUBLIC int
com_limits(int p, param_list param)
{
	// XXX: unused
	(void) param;

	pprintf(p, "\nCurrent hardcoded limits:\n");
	pprintf(p, "  Max number of players: %d\n", MAX_PLAYER);
	pprintf(p, "  Max number of channels and max capacity: %d\n",
	    MAX_CHANNELS);
	pprintf(p, "  Max number of channels one can be in: %d\n",
	    MAX_INCHANNELS);
	pprintf(p, "  Max number of people on the notify list: %d\n",
	    MAX_NOTIFY);
	pprintf(p, "  Max number of aliases: %d\n", (MAX_ALIASES - 1));
	pprintf(p, "  Max number of games you can observe at a time: %d\n",
	    MAX_OBSERVE);
	pprintf(p, "  Max number of requests pending: %d\n", MAX_PENDING);
	pprintf(p, "  Max number of people on the censor list: %d\n",
	    MAX_CENSOR);
	pprintf(p, "  Max number of people in a simul game: %d\n", MAX_SIMUL);
	pprintf(p, "  Max number of messages one can receive: %d\n",
	    MAX_MESSAGES);
	pprintf(p, "  Min number of games to be active: %d\n", PROVISIONAL);

	if (parray[p].adminLevel < ADMIN_ADMIN &&
	    !titled_player(p, parray[p].login)) {
		pprintf(p, "  Size of journal (entries): %d\n", MAX_JOURNAL);
	} else {
		pprintf(p, "  Size of journal (entries): 26\n");
	}

	pprintf(p, "\nAdmin settable limits:\n");
	pprintf(p, "  Shout quota gives two shouts per %d seconds.\n",
	    quota_time);

	return COM_OK;
}

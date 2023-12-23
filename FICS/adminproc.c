/*
      fics - An internet chess server
      Copyright (c) 1993  Richard V. Nash

      This program is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 2 of the License, or
      (at your option) any later version.

      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      Continued development of this software is done by the GNU ICS
      development team. Contact <chess@caissa.onenet.net> with questions.


      adminproc.c - All administrative commands and related functions    */

#include "stdinclude.h"
#include "common.h"

#include <sys/param.h>

#include "adminproc.h"
#include "command.h"
#include "comproc.h"
#include "gamedb.h"
#include "gameproc.h"
#include "multicol.h"
#include "network.h"
#include "obsproc.h"
#include "playerdb.h"
#include "ratings.h"
#include "rmalloc.h"
#include "talkproc.h"
#include "utils.h"

#define PASSLEN 4

PUBLIC int num_anews = -1;

/*
 * adjudicate
 *
 * Usage: adjudicate white_player black_player result
 *
 *   Adjudicates a saved (stored) game between white_player and black_player.
 *   The result is one of: abort, draw, white, black.  "Abort" cancels the game
 *   (no win, loss or draw), "white" gives white_player the win, "black" gives
 *   black_player the win, and "draw" gives a draw.
 */
PUBLIC int com_adjudicate(int p, param_list param)
{
  int wp, wconnected, bp, bconnected, g, inprogress, confused = 0;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if (!FindPlayer(p, param[0].val.word, &wp, &wconnected))
    return COM_OK;
  if (!FindPlayer(p, param[1].val.word, &bp, &bconnected)) {
    if (!wconnected)
     player_remove(wp);
    return COM_OK;
  }

  inprogress = ((parray[wp].game >=0) &&(parray[wp].opponent == bp));

  if (inprogress) {
    g = parray[wp].game;
  } else {
    g = game_new();
    if (game_read(g, wp, bp) < 0) {
      confused = 1;
      pprintf(p, "There is no stored game %s vs. %s\n", parray[wp].name, parray[bp].name);
    } else {
      garray[g].white = wp;
      garray[g].black = bp;
    }
  }
  if (!confused) {
    if (strstr("abort", param[2].val.word) != NULL) {
      game_ended(g, WHITE, END_ADJABORT);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been aborted.",
	       parray[wp].name, parray[wp].name, parray[bp].name);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been aborted.",
	       parray[bp].name, parray[wp].name, parray[bp].name);
    } else if (strstr("draw", param[2].val.word) != NULL) {
      game_ended(g, WHITE, END_ADJDRAW);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been adjudicated "
	       "as a draw", parray[wp].name, parray[wp].name, parray[bp].name);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been adjudicated "
	       "as a draw", parray[bp].name, parray[wp].name, parray[bp].name);
    } else if (strstr("white", param[2].val.word) != NULL) {
      game_ended(g, WHITE, END_ADJWIN);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been adjudicated "
	       "as a win", parray[wp].name, parray[wp].name, parray[bp].name);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been adjudicated "
	       "as a loss", parray[bp].name, parray[wp].name, parray[bp].name);
    } else if (strstr("black", param[2].val.word) != NULL) {
      game_ended(g, BLACK, END_ADJWIN);
      pcommand(p, "message %s Your game \"%s vs. %s\" has been adjudicated "
	       "as a loss", parray[wp].name, parray[wp].name, parray[bp].name);

      pcommand(p, "message %s Your game \"%s vs. %s\" has been adjudicated "
	       "as a win", parray[bp].name, parray[wp].name, parray[bp].name);
    } else {
      confused = 1;
      pprintf(p, "Result must be one of: abort draw white black\n");
    }
  }
  if (!confused) {
    pprintf(p, "Game adjudicated.\n");
    if (!inprogress) {
      game_delete(wp, bp);
    } else {
      return (COM_OK);
    }
  }
  game_remove(g);
  if (!wconnected)
    player_remove(wp);
  if (!bconnected)
    player_remove(bp);
  return COM_OK;
}

/*
 * create_news_file:  Creates either a general or and admin news
 *                    file, depending upon the admin switch.
 */
PRIVATE int create_news_file(int p, param_list param, int admin)
{
  FILE *fp;
  char filename[MAX_FILENAME_SIZE];

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);

  if (admin) {
    if (param[0].val.integer > num_anews)
      pprintf(p, "There must be an admin news index #%d before you can create the file.", param[0].val.integer);
    else {
      sprintf(filename, "%s/adminnews.%d", news_dir, param[0].val.integer);
      fp = fopen(filename, "w");
      fprintf(fp, "%s\n", param[1].val.string);
      fclose(fp);
    }
  } else {
    if (param[0].val.integer > num_news)
      pprintf(p, "There must be a news index #%d before you can create the file.", param[0].val.integer);
    else {
      sprintf(filename, "%s/news.%d", news_dir, param[0].val.integer);
      fp = fopen(filename, "w");
      fprintf(fp, "%s\n", param[1].val.string);
      fclose(fp);
    }
  }

  return COM_OK;
}

PRIVATE int add_item(char *new_item, char *filename)
{
  FILE *new_fp, *old_fp;
  char tmp_file[MAX_FILENAME_SIZE];
  char junk[MAX_LINE_SIZE];

  sprintf(tmp_file, "%s/.tmp.idx", news_dir);
  new_fp = fopen(tmp_file, "w");
  old_fp = fopen(filename, "r");

  if (!new_fp || !old_fp)
    return 0;

  fprintf(new_fp, "%s", new_item);
  while (1) {
    fgets(junk, MAX_LINE_SIZE, old_fp);
    if (feof(old_fp))
      break;
    fprintf(new_fp, "%s", junk);
  }
  fclose(new_fp);
  fclose(old_fp);
  remove(filename);
  rename(tmp_file, filename);

  return 1;
}

/*
 * create_news_index:  Adds a new item to either the general or admin news
 *                     index file, depending upon the admin switch.
 */
PRIVATE int create_news_index(int p, param_list param, int admin)
{
  char filename[MAX_FILENAME_SIZE];
  char new_item[MAX_LINE_SIZE];

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);

  if (admin) {
    if (strlen(param[0].val.string) > 50)
      pprintf(p, "Sorry, you must limit an index to 50 charaters!  Admin news index not created.\n");
    else {
      num_anews++;
      sprintf(new_item, "%d %d %s\n", (int) time(0), num_anews, param[0].val.string);
      sprintf(filename, "%s/newadminnews.index", news_dir);
      if (add_item(new_item, filename)) {
	pprintf(p, "Index for admin news item #%d created.\n", num_anews);
	pprintf(p, "Please use 'canewsf' to include more info.\n");
      } else
	pprintf(p, "Something went wrong creating item.\nNotify Marsalis.\n");
    }
  } else {
    if (strlen(param[0].val.string) > 50)
      pprintf(p, "Sorry, you must limit an index to 50 charaters!  News index not created.\n");
    else {
      num_news++;
      sprintf(filename, "%s/newnews.index", news_dir);
      sprintf(new_item, "%d %d %s\n", (int) time(0), num_news, param[0].val.string);
      if (add_item(new_item, filename)) {
        pprintf(p, "Index for news item #%d created.\n", num_news);
        pprintf(p, "Please use 'cnewsf' to include more info.\n");
      } else
        pprintf(p, "Something went wrong creating item.\nNotify Marsalis.\n");
    }
  }

  return COM_OK;
}

/* cnewsi
 *
 * Usage: cnewsi message
 *
 *
 *   This command adds a new item to the news index.  The message is limited to
 *   45 characters for formating purposes.  In essence, the news index works
 *   like a newspaper headline, giving the user enough information to know
 *   whether they should read the entire news file for that item.  After
 *   creating the news item, the command reports the news item number along
 *   with a reminder to create a news file if necessary.
 */
PUBLIC int com_cnewsi(int p, param_list param)
{
  return create_news_index(p, param, 0);
}

/* cnewsf
 *
 * Usage: cnewsf # message
 *
 *   This command allows you to add additional information about a news item
 *   that had previously been created using 'cnewsi'.  The '#' is the number
 *   of the news index and 'message' is the additional text.  You can also
 *   modify a previous news item description and thus update the news item
 *   easily.
 */
PUBLIC int com_cnewsf(int p, param_list param)
{
  return create_news_file(p, param, 0);
}

PUBLIC int com_canewsi(int p, param_list param)
{
  return create_news_index(p, param, 1);
}

PUBLIC int com_canewsf(int p, param_list param)
{
  return create_news_file(p, param, 1);
}

/*
 * anews
 *
 *
 * Usage: anews [#, all]
 *
 *   This command is used to display anews (admin news) entries.  The
 *   entries are numbered.  "Anews #" displays that anews item.  "Anews
 *   all" will display all items.
 *
 */
PUBLIC int
com_anews(int p, param_list param)
{
	FILE		*fp;
	char		*junkp;
	char		 count[10];
	char		 filename[MAX_FILENAME_SIZE];
	char		 junk[MAX_LINE_SIZE];
	int		 found = 0;
	long int	 lval;
	time_t		 crtime;

	sprintf(filename, "%s/newadminnews.index", news_dir);

	if ((fp = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Cant find news index.\n");
		return COM_OK;
	}

	if (param[0].type == 0) {
		/*
		 * No params - then just display index over news.
		 */

		sprintf(filename, "%s/newadminnews.index", news_dir);

		pprintf(p, "Index of recent admin news items:\n");
		fgets(junk, MAX_LINE_SIZE, fp);
		sscanf(junk, "%ld %s", &lval, count);
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
		 * Param all - displays all news items.
		 */

		pprintf(p, "Index of all admin news items:\n");
		fgets(junk, MAX_LINE_SIZE, fp);
		sscanf(junk, "%ld %s", &lval, count);
		rscan_news(fp, p, 0);

		junkp = junk;
		junkp = nextword(junkp);
		junkp = nextword(junkp);

		crtime = lval;
		pprintf(p, "%3s (%s) %s", count, fix_time(strltime(&crtime)),
		    junkp);
		fclose(fp);
	} else {
		while (!feof(fp) && !found) {
			junkp = junk;
			fgets(junk, MAX_LINE_SIZE, fp);

			if (feof(fp))
				break;

			if (strlen(junk) > 1) {
				sscanf(junkp, "%ld %s", &lval, count);
				crtime = lval;

				if (!strcmp(count, param[0].val.word)) {
					found = 1;

					junkp = nextword(junkp);
					junkp = nextword(junkp);

					pprintf(p, "ANEWS %3s (%s) %s\n", count,
					    fix_time(strltime(&crtime)), junkp);
				}
			}
		}

		fclose(fp);

		if (!found) {
			pprintf(p, "Bad index number!\n");
			return COM_OK;
		}

		sprintf(filename, "%s/adminnews.%s", news_dir,
		    param[0].val.word);

		if ((fp = fopen(filename, "r")) == NULL) {
			pprintf(p, "No more info.\n");
			return COM_OK;
		}

		fclose(fp);
		sprintf(filename, "adminnews.%s", param[0].val.word);

		if (psend_file(p, news_dir, filename) < 0) {
			pprintf(p, "Internal error - couldn't send news file!"
			    "\n");
		}
	}

	return COM_OK;
}

PUBLIC int strcmpwild(char *mainstr, char *searchstr)
{
  int i;

  if (strlen(mainstr) < strlen(searchstr))
    return 1;
  for (i = 0; i < strlen(mainstr); i++) {
    if (searchstr[i] == '*')
      return 0;
    if (mainstr[i] != searchstr[i])
      return 1;
  }
  return 0;
}

/*
 * chkip
 *
 * Usage: chkip ip_address
 *
 *   This command returns the names of all users currently logged on
 *   from a given IP address.
 */
PUBLIC int com_checkIP(int p, param_list param)
{
  char *ipstr = param[0].val.word;
  int p1;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  pprintf(p, "Matches the following player(s): \n\n");
  for (p1 = 0; p1 < p_num; p1++)
    if (!strcmpwild(dotQuad(parray[p1].thisHost), ipstr) && (parray[p1].status != PLAYER_EMPTY))
      pprintf(p, "%16.16s %s\n", parray[p1].name, dotQuad(parray[p1].thisHost));
  return COM_OK;
}

PUBLIC int com_checkSOCKET(int p, param_list param)
{
  int fd = param[0].val.integer;
  int p1, flag;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  flag = 0;
  for (p1 = 0; p1 < p_num; p1++) {
    if (parray[p1].socket == fd) {
      flag = 1;
      pprintf(p, "Socket %d is used by %s\n", fd, parray[p1].name);
    }
  }
  if (!flag)
    pprintf(p, "Socket %d is unused!\n", fd);
  return COM_OK;
}

/*
 * chkpl
 *
 * Usage: chkpl handle
 *
 *   This command displays server information about a given user.  Items
 * displayed are:
 *
 *    number X in parray of size Y
 *    name
 *    login
 *    fullName
 *    emailAddress
 *    socket
 *    registered
 *    last_tell
 *    last_channel
 *    logon_time
 *    adminLevel
 *    thisHost
 *    lastHost
 *    num_comments
 */
PUBLIC int com_checkPLAYER(int p, param_list param)
{
  char *player = param[0].val.word;
  int p1;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  p1 = player_search(p, param[0].val.word);
  if (!p1)
    return COM_OK;
  if (p1 < 0) {
    p1 = (-p1) - 1;
    pprintf(p, "%s is not logged in.\n", player);
    stolower(player);
    pprintf(p, "name = %s\n", parray[p1].name);
    pprintf(p, "login = %s\n", parray[p1].login);
    pprintf(p, "fullName = %s\n", (parray[p1].fullName ? parray[p1].fullName : "(none)"));
    pprintf(p, "emailAddress = %s\n", (parray[p1].emailAddress ? parray[p1].emailAddress : "(none)"));
    pprintf(p, "adminLevel = %d\n", parray[p1].adminLevel);
/*    pprintf(p, "network_player = %d\n", parray[p1].network_player); */
    pprintf(p, "lastHost = %s\n", dotQuad(parray[p1].lastHost));
    pprintf(p, "num_comments = %d\n", parray[p1].num_comments);

    player_remove(p1);
    return COM_OK;
  } else {
    p1 = p1 - 1;
    pprintf(p, "%s is number %d in parray of size %d\n", player, p1, p_num + 1);
    pprintf(p, "name = %s\n", parray[p1].name);
    pprintf(p, "login = %s\n", parray[p1].login);
    pprintf(p, "fullName = %s\n", parray[p1].fullName ? parray[p1].fullName : "(none)");
    pprintf(p, "emailAddress = %s\n", parray[p1].emailAddress ? parray[p1].emailAddress : "(none)");
    pprintf(p, "socket = %d\n", parray[p1].socket);
    pprintf(p, "registered = %d\n", parray[p1].registered);
    pprintf(p, "last_tell = %d\n", parray[p1].last_tell);
    pprintf(p, "last_channel = %d\n", parray[p1].last_channel);
    pprintf(p, "logon_time = %s", ctime((time_t *) &parray[p1].logon_time));
    pprintf(p, "adminLevel = %d\n", parray[p1].adminLevel);
/*    pprintf(p, "network_player = %d\n", parray[p1].network_player); */
    pprintf(p, "thisHost = %s\n", dotQuad(parray[p1].thisHost));
    pprintf(p, "lastHost = %s\n", dotQuad(parray[p1].lastHost));
    pprintf(p, "num_comments = %d\n", parray[p1].num_comments);

  }
  return COM_OK;
}

/*
 * chkts
 *
 * Usage: chkts
 *
 *   This command displays all current users who are using timeseal.
 */
PUBLIC int com_checkTIMESEAL(int p, param_list param)
{
  int p1, count = 0;

  /* XXX: maybe unused */
  (void) p1;
  (void) count;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  pprintf(p, "The following player(s) are using timeseal:\n\n");

#ifdef TIMESEAL
  for (p1 = 0; p1 < p_num; p1++) {
    if (parray[p1].status != PLAYER_EMPTY
        && con[parray[p1].socket].timeseal) {
      pprintf(p, "%s\n", parray[p1].name);
      count++;
    }
  }
  pprintf(p, "\nNumber of people using timeseal:  %d\n", count);
#endif

  return COM_OK;
}

PUBLIC int
com_checkGAME(int p,param_list param)
{
	char		 tmp[10 + 1 + 7];	// enough to store number
						// 'black: ' and '\0'
	int		 found = 0;
	int		 p1, g, link;
	multicol	*m;
	time_t		 startTime;

	ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);

	if (g_num == 0) {
		pprintf(p, "No games are currently linked into the 'garray' "
		    "structure.\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_WORD) {    // a player name
		if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
			pprintf(p, "%s doesn't appear to be logged in.\n",
			    param[0].val.word);
			pprintf(p, "Searching through garray to find matching "
			    "game numbers.\n");
			pprintf(p, "Use chkgame <number> to view the results."
			    "\n");

			m = multicol_start(g_num * 2);	// Obviously no more
							// than that

			for (g = 0; g < g_num; g++) {
				multicol_store(m, tmp);

				if (!strcasecmp(garray[g].white_name,param[0].val.word))  {
					sprintf(tmp, "White: %d", g);
					multicol_store(m, tmp);
					found = 1;
				}
				if (!strcasecmp(garray[g].black_name,param[0].val.word))  {
					sprintf(tmp, "Black: %d", g);
					multicol_store(m, tmp);
					found = 1;
				}
			}

			if (found)
				multicol_pprint(m, p, parray[p].d_width, 2);
			else
				pprintf(p,"No matching games were found.\n");
			multicol_end(m);

			return COM_OK;
		}

		if ((g = parray[p1].game) < 0) {
			pprintf(p, "%s doesn't appear to be playing a game.\n",
			    parray[p1].name);
			pprintf(p, "Searching through garray to find matching "
			    "game numbers.\n");
			pprintf(p, "Use chkgame <number> to view the results."
			    "\n");

			m = multicol_start(g_num * 2);	// Obviously no more
							// than that

			for (g = 0; g < g_num; g++) {
				if (garray[g].white == p1) {
					sprintf(tmp, "White: %d", g);
					multicol_store(m, tmp);
					found = 1;
				}

				if (garray[g].black == p1) {
					sprintf(tmp, "Black: %d", g);
					multicol_store(m,tmp);
					found = 1;
				}
			}

			if (found)
				multicol_pprint(m, p, parray[p].d_width, 2);
			else
				pprintf(p, "No matching games were found.\n");
			multicol_end(m);

			return COM_OK;
		}
	} else {
		if ((g = param[0].val.integer - 1) < 0 || g >= g_num) {
			pprintf(p, "The current range of game numbers is 1 to "
			    "%d.\n", g_num);
			return COM_OK;
		}
	}

	startTime = untenths(garray[g].timeOfStart);

	pprintf(p, "Current stored info for game %d (garray[%d]):\n", (g + 1),
	    g);
	pprintf(p, "Initial white time: %d    Initial white increment %d\n",
	    (garray[g].wInitTime / 600),
	    (garray[g].wIncrement / 10));
	pprintf(p, "Initial black time: %d    Initial black increment %d\n",
	    (garray[g].bInitTime / 600),
	    (garray[g].bIncrement / 10));
	pprintf(p, "Time of starting: %s\n", strltime(&startTime));
	pprintf(p, "Game is: %s (%d) vs. %s (%d)\n",
	    garray[g].white_name,
	    garray[g].white_rating,
	    garray[g].black_name,
	    garray[g].black_rating);
	pprintf(p, "White parray entry: %d    Black parray entry %d\n",
	    garray[g].white,
	    garray[g].black);

	if ((link = garray[g].link) >= 0) {
		pprintf(p, "Bughouse linked to game: %d\n",
		    (garray[g].link + 1));
		pprintf(p, "Partner is playing game: %s (%d) vs. %s (%d)\n",
		    garray[link].white_name,
		    garray[link].white_rating,
		    garray[link].black_name,
		    garray[link].black_rating);
	} else {
		pprintf(p, "Game is not bughouse or link to partner's game not "
		    "found.\n");
	}

	pprintf(p, "Game is %s\n", (garray[g].rated ? "rated" : "unrated"));
	pprintf(p, "Game is %s\n", (garray[g].private ? "private" :
	    "not private"));

	if (garray[g].type == TYPE_UNTIMED)
		pprintf(p, "Games is of type: untimed\n");
	else if (garray[g].type == TYPE_BLITZ)
		pprintf(p, "Games is of type: blitz\n");
	else if (garray[g].type == TYPE_STAND)
		pprintf(p, "Games is of type: standard\n");
	else if (garray[g].type == TYPE_NONSTANDARD)
		pprintf(p, "Games is of type: non-standard\n");
	else if (garray[g].type == TYPE_WILD)
		pprintf(p, "Games is of type: wild\n");
	else if (garray[g].type == TYPE_LIGHT)
		pprintf(p, "Games is of type: lightning\n");
	else if (garray[g].type == TYPE_BUGHOUSE)
		pprintf(p, "Games is of type: bughouse\n");
	else
		pprintf(p, "Games is of type: Unknown - Error!\n");

	pprintf(p, "%d halfmove(s) have been made\n", garray[g].numHalfMoves);

	if (garray[g].status == GAME_ACTIVE)
		game_update_time(g);

	pprintf(p, "White's time %s    Black's time ",
	    tenth_str((garray[g].wTime > 0 ? garray[g].wTime : 0), 0));
	pprintf(p, "%s\n",
	    tenth_str((garray[g].bTime > 0 ? garray[g].bTime : 0), 0));
	pprintf(p, "The clock is%sticking\n", ((garray[g].clockStopped ||
	    garray[g].status != GAME_ACTIVE) ? " not " : " "));

	if (garray[g].status == GAME_EMPTY)
		pprintf(p, "Game status: GAME_EMPTY\n");
	else if (garray[g].status == GAME_NEW)
		pprintf(p, "Game status: GAME_NEW\n");
	else if (garray[g].status == GAME_ACTIVE)
		pprintf(p, "Game status: GAME_ACTIVE\n");
	else if (garray[g].status == GAME_EXAMINE)
		pprintf(p, "Game status: GAME_EXAMINE\n");
	else
		pprintf(p, "Game status: Unknown - Error!\n");
	return COM_OK;
}

/*
 * remplayer
 *
 * Usage:  remplayer name
 *
 *   Removes an account.  A copy of its files are saved under .rem.* which can
 *   be found in the appropriate directory (useful in case of an accident).
 *
 *   The account's details, messages, games and logons are all saved as
 *   'zombie' files.  These zombie accounts are not listed in handles or
 *   totals.
 */
PUBLIC int com_remplayer(int p, param_list param)
{
  char *player = param[0].val.word;
  char playerlower[MAX_LOGIN_NAME];
  int p1, lookup;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  strcpy(playerlower, player);
  stolower(playerlower);
  p1 = player_new();
  lookup = player_read(p1, playerlower);
  if (!lookup) {
    if ((parray[p].adminLevel <= parray[p1].adminLevel) && !player_ishead(p)) {
      pprintf(p, "You can't remove an admin with a level higher than or equal to yourself.\n");
      player_remove(p1);
      return COM_OK;
    }
  }
  player_remove(p1);
  if (lookup) {
    pprintf(p, "No player by the name %s is registered.\n", player);
    return COM_OK;
  }
  if (player_find_bylogin(playerlower) >= 0) {
    pprintf(p, "A player by that name is logged in.\n");
    return COM_OK;
  }
  if (!player_kill(playerlower)) {
    pprintf(p, "Player %s removed.\n", player);
    UpdateRank(TYPE_BLITZ, NULL, NULL, player);
    UpdateRank(TYPE_STAND, NULL, NULL, player);
    UpdateRank(TYPE_WILD, NULL, NULL, player);
  } else {
    pprintf(p, "Remplayer failed.\n");
  }
  return COM_OK;
}

/*
 * raisedead
 *
 * Usage:  raisedead oldname [newname]
 *
 *   Restores an account that has been previously removed using "remplayer".
 *   The zombie files from which it came are removed.  Under most
 *   circumstances, you restore the account to the same handle it had
 *   before (oldname).  However, in some circumstances you may need to
 *   restore the account to a different handle, in which case you include
 *   "newname" as the new handle.  After "raisedead", you may need to use the
 *   "asetpasswd" command to get the player started again as a registered
 *   user, especially if the account had been locked
 *   by setting the password to *.
 */
PUBLIC int com_raisedead(int p, param_list param)
{
  char *player = param[0].val.word;
  char playerlower[MAX_LOGIN_NAME], newplayerlower[MAX_LOGIN_NAME];

  int p1, p2, lookup;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  strcpy(playerlower, player);
  stolower(playerlower);
  if (player_find_bylogin(playerlower) >= 0) {
    pprintf(p, "A player by that name is logged in.\n");
    pprintf(p, "Can't raise until they leave.\n");
    return COM_OK;
  }
  p1 = player_new();
  lookup = player_read(p1, playerlower);
  player_remove(p1);
  if (!lookup) {
    pprintf(p, "A player by the name %s is already registered.\n", player);
    pprintf(p, "Obtain a new handle for the dead person.\n");
    pprintf(p, "Then use raisedead [oldname] [newname].\n");
    return COM_OK;
  }
  if (param[1].type == TYPE_NULL) {
    if (!player_raise(playerlower)) {
      pprintf(p, "Player %s raised from dead.\n", player);

      p1 = player_new();
      if (!(lookup = player_read(p1, playerlower))) {
	if (parray[p1].s_stats.rating > 0)
	  UpdateRank(TYPE_STAND, player, &parray[p1].s_stats, player);
	if (parray[p1].b_stats.rating > 0)
	  UpdateRank(TYPE_BLITZ, player, &parray[p1].b_stats, player);
	if (parray[p1].w_stats.rating > 0)
	  UpdateRank(TYPE_WILD, player, &parray[p1].w_stats, player);
      }
      player_remove(p1);
    } else {
      pprintf(p, "Raisedead failed.\n");
    }
    return COM_OK;
  } else {
    char *newplayer = param[1].val.word;
    strcpy(newplayerlower, newplayer);
    stolower(newplayerlower);
    if (player_find_bylogin(newplayerlower) >= 0) {
      pprintf(p, "A player by the requested name is logged in.\n");
      pprintf(p, "Can't reincarnate until they leave.\n");
      return COM_OK;
    }
    p2 = player_new();
    lookup = player_read(p2, newplayerlower);
    player_remove(p2);
    if (!lookup) {
      pprintf(p, "A player by the name %s is already registered.\n", player);
      pprintf(p, "Obtain another new handle for the dead person.\n");
      return COM_OK;
    }
    if (!player_reincarn(playerlower, newplayerlower)) {
      pprintf(p, "Player %s reincarnated to %s.\n", player, newplayer);
      p2 = player_new();
      if (!(lookup = player_read(p2, newplayerlower))) {
	strfree(parray[p2].name);
	parray[p2].name = xstrdup(newplayer);
	player_save(p2);
	if (parray[p2].s_stats.rating > 0)
	  UpdateRank(TYPE_STAND, newplayer, &parray[p2].s_stats, newplayer);
	if (parray[p2].b_stats.rating > 0)
	  UpdateRank(TYPE_BLITZ, newplayer, &parray[p2].b_stats, newplayer);
	if (parray[p2].w_stats.rating > 0)
	  UpdateRank(TYPE_WILD, newplayer, &parray[p2].w_stats, newplayer);
      }
      player_remove(p2);
    } else {
      pprintf(p, "Raisedead failed.\n");
    }
  }
  return COM_OK;
}

/*
 * addplayer
 *
 * Usage: addplayer playername emailaddress realname
 *
 *   Adds a local player to the server with the handle of "playername".  For
 *   example:
 *
 *      addplayer Hawk u940456@daimi.aau.dk Henrik Gram
 */
PUBLIC int
com_addplayer(int p, param_list param)
{
	char	*newemail = param[1].val.word;
	char	*newname = param[2].val.string;
	char	*newplayer = param[0].val.word;
	char	 newplayerlower[MAX_LOGIN_NAME];
	char	 password[PASSLEN + 1];
	char	 salt[3];
	char	 text[2048];
	int	 i;
	int	 p1;

	ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);

	if (strlen(newplayer) >= MAX_LOGIN_NAME) {
		pprintf(p, "Player name is too long\n");
		return COM_OK;
	}

	if (strlen(newplayer) < 3) {
		pprintf(p, "Player name is too short\n");
		return COM_OK;
	}

	if (!alphastring(newplayer)) {
		pprintf(p, "Illegal characters in player name. "
		    "Only A-Za-z allowed.\n");
		return COM_OK;
	}

	strcpy(newplayerlower, newplayer);
	stolower(newplayerlower);

	p1 = player_new();

	if (!player_read(p1, newplayerlower)) {
		pprintf(p, "A player by the name %s is already registered.\n",
		    newplayerlower);
		player_remove(p1);
		return COM_OK;
	}

	parray[p1].name            = xstrdup(newplayer);
	parray[p1].login           = xstrdup(newplayerlower);
	parray[p1].fullName        = xstrdup(newname);
	parray[p1].emailAddress    = xstrdup(newemail);

	if (strcmp(newemail, "none")) {
		for (i = 0; i < PASSLEN; i++)
			password[i] = 'a' + rand() % 26;
		password[i] = '\0';

		salt[0] = ('a' + rand() % 26);
		salt[1] = ('a' + rand() % 26);
		salt[2] = '\0';

		parray[p1].passwd = xstrdup(crypt(password, salt));
	} else {
		password[0] = '\0';
		parray[p1].passwd = xstrdup(password);
	}

	parray[p1].registered	= 1;
	parray[p1].rated	= 1;

	player_add_comment(p, p1, "Player added by addplayer.");
	player_save(p1);
	player_remove(p1);

	pprintf(p, "Added: >%s< >%s< >%s< >%s<\n", newplayer, newname, newemail,
	    password);

	if (strcmp(newemail, "none")) {
		sprintf(text, "\nYour player account has been created.\n\n"
		    "Login Name: %s\n"
		    "Full Name: %s\n"
		    "Email Address: %s\n"
		    "Initial Password: %s\n\n"

		    "If any of this information is incorrect, please\n"
		    "contact the administrator to get it corrected.\n\n"

		    "You may change your password with the password\n"
		    "command on the server.\n\n"

		    "Please be advised that if this is an unauthorized\n"
		    "duplicate account for you, by using it you take\n"
		    "the risk of being banned from accessing this chess\n"
		    "server.\n\n"

		    "To connect to the server and use this account:\n\n"
		    "\ttelnet %s 5000\n\n"
		    "and enter your handle name and password.\n\n"

		    "Regards,\n\nThe FICS admins\n", newplayer, newname,
		    newemail, password, fics_hostname);

		mail_string_to_address(newemail, "FICS Account Created", text);

		if ((p1 = player_find_part_login(newplayer)) >= 0) {
			pprintf_prompt(p1, "\n\nYou are now registered! "
			    "Confirmation together with\npassword is sent to "
			    "your email address.\n\n");
			return COM_OK;
		}

		return COM_OK;
	} else {
		if ((p1 = player_find_part_login(newplayer)) >= 0) {
			pprintf_prompt(p1, "\n\nYou are now registered! "
			    "You have NO password!\n\n");
			return COM_OK;
		}
	}

	return COM_OK;
}

PUBLIC int com_pose(int p, param_list param)
{
  int p1;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
    pprintf(p, "%s is not logged in.\n", param[0].val.word);
    return COM_OK;
  }
  if ((parray[p].adminLevel <= parray[p1].adminLevel) && !player_ishead(p)) {
    pprintf(p, "You can only pose as players below your adminlevel.\n");
    return COM_OK;
  }
  pprintf(p, "Command issued as %s\n", parray[p1].name);
  pcommand(p1, "%s\n", param[1].val.string);
  return COM_OK;
}

/*
 * asetv
 *
 * Usage: asetv user instructions
 *
 *   This command executes "set" instructions as if they had been made by the
 *   user indicated.  For example, "asetv DAV shout 0" would set DAV's shout
 *   variable to 0.
 */
PUBLIC int com_asetv(int p, param_list param)
{
  int p1;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
    pprintf(p, "%s is not logged in.\n", param[0].val.word);
    return COM_OK;
  }
  if ((parray[p].adminLevel <= parray[p1].adminLevel) && !player_ishead(p)) {
    pprintf(p, "You can only aset players below your adminlevel.\n");
    return COM_OK;
  }
  pprintf(p, "Command issued as %s\n", parray[p1].name);
  pcommand(p1, "set %s\n", param[1].val.string);
  return COM_OK;
}

/*
 * announce
 *
 * Usage: announce message
 *
 *   Broadcasts your message to all logged on users.  Announcements reach all
 *   users and cannot be censored in any way (such as by "set shout 0").
 */
PUBLIC int com_announce(int p, param_list param)
{
  int p1;
  int count = 0;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if (!printablestring(param[0].val.string)) {
    pprintf(p, "Your message contains some unprintable character(s).\n");
    return COM_OK;
  }
  for (p1 = 0; p1 < p_num; p1++) {
    if (p1 == p)
      continue;
    if (parray[p1].status != PLAYER_PROMPT)
      continue;
    count++;
    pprintf_prompt(p1, "\n\n    **ANNOUNCEMENT** from %s: %s\n\n", parray[p].name, param[0].val.string);
  }
  pprintf(p, "\n(%d) **ANNOUNCEMENT** from %s: %s\n\n", count, parray[p].name, param[0].val.string);
  return COM_OK;
}

/*
 * annunreg
 *
 * Usage:  annunreg message
 *
 *   Broadcasts your message to all logged on unregistered users, and admins,
 *   too.  Announcements reach all unregistered users and admins and cannot be
 *   censored in any way (such as by "set shout 0").
 */
PUBLIC int com_annunreg(int p, param_list param)
{
  int p1;
  int count = 0;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if (!printablestring(param[0].val.string)) {
    pprintf(p, "Your message contains some unprintable character(s).\n");
    return COM_OK;
  }
  for (p1 = 0; p1 < p_num; p1++) {
    if (p1 == p)
      continue;
    if (parray[p1].status != PLAYER_PROMPT)
      continue;
    if ((parray[p1].registered) && (parray[p1].adminLevel < ADMIN_ADMIN))
      continue;
    count++;
    pprintf_prompt(p1, "\n\n    **UNREG ANNOUNCEMENT** from %s: %s\n\n", parray[p].name, param[0].val.string);
  }
  pprintf(p, "\n(%d) **UNREG ANNOUNCEMENT** from %s: %s\n\n", count, parray[p].name, param[0].val.string);
  return COM_OK;
}

PUBLIC int com_muzzle(int p, param_list param)
{
  pprintf(p, "Obsolete command: Please use +muzzle and -muzzle.\n");
  return COM_OK;
}

PUBLIC int com_cmuzzle(int p, param_list param)
{
  pprintf(p, "Obsolete command: Please use +cmuzzle and -cmuzzle.\n");
  return COM_OK;
}

/*
 * asetpasswd
 *
 * Usage: asetpasswd player {password,*}
 *
 *   This command sets the password of the player to the password given.
 *   If '*' is specified then the player's account is locked, and no password
 *   will work until a new one is set by asetpasswd.
 *
 *   If the player is connected, he is told of the new password and the name
 *   of the admin who changed it, or likewise of his account status.  An
 *   email message is mailed to the player's email address as well.
 */
PUBLIC int com_asetpasswd(int p, param_list param)
{
  int p1, connected;
  char subject[400], text[10100];
  char salt[3];

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if ((parray[p].adminLevel <= parray[p1].adminLevel) && !player_ishead(p)) {
    pprintf(p, "You can only set password for players below your adminlevel.\n");
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  if (!parray[p1].registered) {
    pprintf(p, "You cannot set the password of an unregistered player!\n");
    return COM_OK;
  }
  if (parray[p1].passwd)
    rfree(parray[p1].passwd);
  if (param[1].val.word[0] == '*') {
    parray[p1].passwd = xstrdup(param[1].val.word);
    pprintf(p, "Account %s locked!\n", parray[p1].name);
    sprintf(text, "Password of %s is now useless.  Your account at our FICS has been locked.\n", parray[p1].name);
  } else {
    salt[0] = 'a' + rand() % 26;
    salt[1] = 'a' + rand() % 26;
    salt[2] = '\0';
    parray[p1].passwd = xstrdup(crypt(param[1].val.word, salt));
    sprintf(text, "Password of %s changed to \"%s\".\n", parray[p1].name, param[1].val.word);
    pprintf(p, "%s", text);
  }
  if (param[1].val.word[0] == '*') {
    sprintf(subject, "FICS: %s has locked your account.", parray[p].name);
    if (connected)
      pprintf_prompt(p1, "\n%s\n", subject);
  } else {
    sprintf(subject, "FICS: %s has changed your password.", parray[p].name);
    if (connected)
      pprintf_prompt(p1, "\n%s\n", subject);
  }
  mail_string_to_address(parray[p1].emailAddress, subject, text);
  player_save(p1);
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * asetemail
 *
 * Usage: asetemail player [address]
 *
 *   Sets the email address of the player to the address given.  If the
 *   address is omited, then the player's email address is cleared.  The
 *   person's email address is revealed to them when they use the "finger"
 *   command, but no other users -- except admins -- will have another
 *   player's email address displayed.
 */
PUBLIC int com_asetemail(int p, param_list param)
{
  int p1, connected;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if ((parray[p].adminLevel <= parray[p1].adminLevel) && !player_ishead(p)) {
    pprintf(p, "You can only set email addr for players below your adminlevel.\n");
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  if (parray[p1].emailAddress)
    rfree(parray[p1].emailAddress);
  if (param[1].type == TYPE_NULL) {
    parray[p1].emailAddress = NULL;
    pprintf(p, "Email address for %s removed\n", parray[p1].name);
  } else {
    parray[p1].emailAddress = xstrdup(param[1].val.word);
    pprintf(p, "Email address of %s changed to \"%s\".\n", parray[p1].name, param[1].val.word);
  }
  player_save(p1);
  if (connected) {
    if (param[1].type == TYPE_NULL) {
      pprintf_prompt(p1, "\n\n%s has removed your email address.\n\n", parray[p].name);
    } else {
      pprintf_prompt(p1, "\n\n%s has changed your email address.\n\n", parray[p].name);
    }
  } else {
    player_remove(p1);
  }
  return COM_OK;
}

/*
 * asetrealname
 *
 * Usage:  asetrealname user newname
 *
 *   This command sets the user's real name (as displayed to admins on finger
 *   notes) to "newname".
 */
PUBLIC int com_asetrealname(int p, param_list param)
{
  int p1, connected;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if ((parray[p].adminLevel <= parray[p1].adminLevel) && !player_ishead(p)) {
    pprintf(p, "You can only set real names for players below your adminlevel.\n");
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  if (parray[p1].fullName)
    rfree(parray[p1].fullName);
  if (param[1].type == TYPE_NULL) {
    parray[p1].fullName = NULL;
    pprintf(p, "Real name for %s removed\n", parray[p1].name);
  } else {
    parray[p1].fullName = xstrdup(param[1].val.word);
    pprintf(p, "Real name of %s changed to \"%s\".\n", parray[p1].name, param[1].val.word);
  }
  player_save(p1);
  if (connected) {
    if (param[1].type == TYPE_NULL) {
      pprintf_prompt(p1, "\n\n%s has removed your real name.\n\n", parray[p].name);
    } else {
      pprintf_prompt(p1, "\n\n%s has changed your real name.\n\n", parray[p].name);
    }
  } else {
    player_remove(p1);
  }
  return COM_OK;
}

/*
 * asethandle
 *
 * Usage: asethandle oldname newname
 *
 *   This command changes the handle of the player from oldname to
 *   newname.  The various player information, messages, logins, comments
 *   and games should be automatically transferred to the new account.
 */
PUBLIC int com_asethandle(int p, param_list param)
{
  char *player = param[0].val.word;
  char *newplayer = param[1].val.word;
  char playerlower[MAX_LOGIN_NAME], newplayerlower[MAX_LOGIN_NAME];
  int p1;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  strcpy(playerlower, player);
  stolower(playerlower);
  strcpy(newplayerlower, newplayer);
  stolower(newplayerlower);
  if (player_find_bylogin(playerlower) >= 0) {
    pprintf(p, "A player by that name is logged in.\n");
    return COM_OK;
  }
  if (player_find_bylogin(newplayerlower) >= 0) {
    pprintf(p, "A player by that new name is logged in.\n");
    return COM_OK;
  }
  p1 = player_new();
  if (player_read(p1, playerlower)) {
    pprintf(p, "No player by the name %s is registered.\n", player);
    player_remove(p1);
    return COM_OK;
  } else {
    if ((parray[p].adminLevel <= parray[p1].adminLevel) && !player_ishead(p)) {
      pprintf(p, "You can't set handles for an admin with a level higher than or equal to yourself.\n");
      player_remove(p1);
      return COM_OK;
    }
  }
  player_remove(p1);

  p1 = player_new();
  if ((!player_read(p1, newplayerlower)) && (strcmp(playerlower, newplayerlower))) {
    pprintf(p, "Sorry that handle is already taken.\n");
    player_remove(p1);
    return COM_OK;
  }
  player_remove(p1);

  if ((!player_rename(playerlower, newplayerlower)) && (!player_read(p1, newplayerlower))) {
    pprintf(p, "Player %s renamed to %s.\n", player, newplayer);
    strfree(parray[p1].name);
    parray[p1].name = xstrdup(newplayer);
    player_save(p1);
    if (parray[p1].s_stats.rating > 0)
      UpdateRank(TYPE_STAND, newplayer, &parray[p1].s_stats, player);
    if (parray[p1].b_stats.rating > 0)
      UpdateRank(TYPE_BLITZ, newplayer, &parray[p1].b_stats, player);
    if (parray[p1].w_stats.rating > 0)
      UpdateRank(TYPE_WILD, newplayer, &parray[p1].w_stats, player);
  } else {
    pprintf(p, "Asethandle failed.\n");
  }
  player_remove(p1);
  return COM_OK;
}

/*
 * asetadmin
 *
 * Usage: asetadmin player AdminLevel
 *
 *   Sets the admin level of the player with the following restrictions.
 *   1. You can only set the admin level of players lower than yourself.
 *   2. You can only set the admin level to a level that is lower than
 *      yourself.
 */
PUBLIC int com_asetadmin(int p, param_list param)
{
  int p1, connected;

  ASSERT(parray[p].adminLevel >= ADMIN_GOD);
  if (!FindPlayer(p, param[0].val.word,&p1, &connected))
    return COM_OK;

  if ((parray[p].adminLevel <= parray[p1].adminLevel) && !player_ishead(p)) {
    pprintf(p, "You can only set adminlevel for players below your adminlevel.\n");
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  if ((parray[p1].login) == (parray[p].login)) {
    pprintf(p, "You can't change your own adminlevel.\n");
    return COM_OK;
  }
  if ((param[1].val.integer >= parray[p].adminLevel) && !player_ishead(p)) {
    pprintf(p, "You can't promote someone to or above your adminlevel.\n");
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  //oldlevel = parray[p1].adminLevel; XXX: set but not used
  parray[p1].adminLevel = param[1].val.integer;
  pprintf(p, "Admin level of %s set to %d.\n", parray[p1].name, parray[p1].adminLevel);
  player_save(p1);
  if (connected) {
    pprintf_prompt(p1, "\n\n%s has set your admin level to %d.\n\n", parray[p].name, parray[p1].adminLevel);
  } else {
    player_remove(p1);
  }
  return COM_OK;
}

PRIVATE void SetRating(int p1, param_list param, statistics *s)
{
  s->rating = param[1].val.integer;
  if (s->ltime == 0L)
    s->sterr = 70.0;

  if (param[2].type == TYPE_INT) {
    s->win = param[2].val.integer;
    if (param[3].type == TYPE_INT) {
      s->los = param[3].val.integer;
      if (param[4].type == TYPE_INT) {
	s->dra = param[4].val.integer;
	if (param[5].type == TYPE_INT) {
	  s->sterr = (double) param[5].val.integer;
	}
      }
    }
  }
  s->num = s->win + s->los + s->dra;
  if (s->num == 0) {
    s->ltime = 0L;
  } else {
    s->ltime = time(0);
  }
}

/*
 * asetblitz
 *
 * Usage: asetblitz handle rating won lost drew RD
 *
 *   This command allows admins to set a user's statistics for Blitz games.
 *   The parameters are self-explanatory: rating, # of wins, # of losses,
 *   # of draws, and ratings deviation.
 */
PUBLIC int com_asetblitz(int p, param_list param)
{
  int p1, connected;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  SetRating(p1, param, &parray[p1].b_stats);
  player_save(p1);
  UpdateRank(TYPE_BLITZ, parray[p1].name, &parray[p1].b_stats,
	     parray[p1].name);
  pprintf(p, "Blitz rating for %s modified.\n", parray[p1].name);
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * asetwild
 *
 * Usage: asetwild handle rating won lost drew RD
 *
 *   This command allows admins to set a user's statistics for Wild games.
 *   The parameters are self-explanatory: rating, # of wins, # of losses,
 *   # of draws, and ratings deviation.
 */
PUBLIC int com_asetwild(int p, param_list param)
{
  int p1, connected;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  SetRating(p1, param, &parray[p1].w_stats);
  player_save(p1);
  UpdateRank(TYPE_WILD, parray[p1].name, &parray[p1].w_stats,
	     parray[p1].name);
  pprintf(p, "Wild rating for %s modified.\n", parray[p1].name);
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * asetstd
 *
 * Usage: asetstd handle rating won lost drew RD
 *
 *   This command allows admins to set a user's statistics for Standard games.
 *   The parameters are self-explanatory: rating, # of wins, # of losses, # of
 *   draws, and ratings deviation.
 */
PUBLIC int com_asetstd(int p, param_list param)
{
  int p1, connected;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  SetRating(p1, param, &parray[p1].s_stats);
  player_save(p1);
  UpdateRank(TYPE_STAND, parray[p1].name, &parray[p1].s_stats,
	     parray[p1].name);
  pprintf(p, "Standard rating for %s modified.\n", parray[p1].name);
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * asetlight
 *
 * Usage: asetlight handle rating won lost drew RD
 *
 *   This command allows admins to set a user's statistics for Lightning games.
 *   The parameters are self-explanatory: rating, # of wins, # of losses, # of
 *   draws, and ratings deviation.
 */
PUBLIC int com_asetlight(int p, param_list param)
{
  int p1, connected;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  SetRating(p1, param, &parray[p1].l_stats);
  player_save(p1);
  pprintf(p, "Lightning rating for %s modified.\n", parray[p1].name);
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * nuke
 *
 * Usage: nuke user
 *
 *   This command disconnects the user from the server.  The user is informed
 *   that she/he has been nuked by the admin named and a comment is
 *   automatically placed in the user's files (if she/he is a registered
 *   user, of course).
 */
PUBLIC int com_nuke(int p, param_list param)
{
  int p1, fd;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
    pprintf(p, "%s isn't logged in.\n", param[0].val.word);
  } else {
    if ((parray[p].adminLevel > parray[p1].adminLevel) || player_ishead(p)) {
      pprintf(p, "Nuking: %s\n", param[0].val.word);
      pprintf(p, "Please leave a comment explaining why %s was nuked.\n", parray[p1].name);
      pprintf(p1, "\n\n**** You have been kicked out by %s! ****\n\n", parray[p].name);
      pcommand(p, "addcomment %s Nuked\n", parray[p1].name);
      fd = parray[p1].socket;
      process_disconnection(fd);
      net_close_connection(fd);
      return COM_OK;
    } else {
      pprintf(p, "You need a higher adminlevel to nuke %s!\n", param[0].val.word);
    }
  }
  return COM_OK;
}

/*
 * summon
 *
 * Usage: summon player
 *
 *   This command gives a beep and a message to the player indicating that you
 *   want to talk with him/her.  The command is useful for waking someone up,
 *   for example a sleepy admin or an ignorant player.
 */
PUBLIC int com_summon(int p, param_list param)
{
  int p1;
  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if ((p1 = player_find_part_login(param[0].val.word)) < 0) {
    pprintf(p, "%s isn't logged in.\n", param[0].val.word);
    return COM_OK;
  } else {
    pprintf(p1, "\a\n");
    pprintf_highlight(p1, "%s", parray[p].name);
    pprintf_prompt(p1, " needs to talk with you.  Use tell %s <message>  to reply.\a\n", parray[p].name);
    pprintf(p, "Summoning sent to %s.\n", parray[p1].name);
    return COM_OK;
  }
}

/*
 * addcomment
 *
 * Usage: addcomment user comment
 *
 *   Places "comment" in the user's comments.  If a user has comments, the
 *   number of comments is indicated to admins using the "finger" command.
 *   The comments themselves are displayed by the "showcomments" command.
 */
PUBLIC int com_addcomment(int p, param_list param)
{
  int p1, connected;

  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if (player_add_comment(p, p1, param[1].val.string)) {
    pprintf(p, "Error adding comment!\n");
  } else {
    pprintf(p, "Comment added for %s.\n", parray[p1].name);
    player_save(p1);
  }
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * showcomment
 *
 * Usage: showcomment user
 *
 *   This command will display all of the comments added to the user's account.
 */
PUBLIC int com_showcomment(int p, param_list param)
{
  int p1, connected;
  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  ASSERT(param[0].type == TYPE_WORD);

  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;
  player_show_comments(p, p1);
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

/*
 * admin
 *
 * Usage: admin
 *
 *   This command toggles your admin symbol (*) on/off.  This symbol appears
 *   in who listings.
 */
PUBLIC int com_admin(int p, param_list param)
{
  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  parray[p].i_admin = !(parray[p].i_admin);
  if (parray[p].i_admin) {
    pprintf(p, "Admin mode (*) is now shown\n");
  } else {
    pprintf(p, "Admin mode (*) is now not shown\n");
  }
  return COM_OK;
}

/*
 * quota
 *
 * Usage: quota [n]
 *
 *   The command sets the number of seconds (n) for the shout quota, which
 *   affects only those persons on the shout quota list.  If no parameter
 *   (n) is given, the current setting is displayed.
 */
PUBLIC int com_quota(int p, param_list param)
{
  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if (param[0].type == TYPE_NULL) {
    pprintf(p, "The current shout quota is 2 shouts per %d seconds.\n", quota_time);
    return COM_OK;
  }
  quota_time = param[0].val.integer;
  pprintf(p, "The shout quota is now 2 shouts per %d seconds.\n", quota_time);
  return COM_OK;
}


/*
 * asetmaxplayer
 *
 * Usage: asetmaxplayer [n]
 *
 *   The command sets the maximum number of players (n) who can connect.
 */
PUBLIC int com_asetmaxplayer(int p, param_list param)
{
  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  if (param[0].type != TYPE_NULL) {
    pprintf(p, "Previously %d total conenctions allowed...\n", max_connections);
    max_connections = param[0].val.integer;
    if ((max_connections > MAX_PLAYER) || (max_connections > getdtablesize()-4)) {
      max_connections = MIN(MAX_PLAYER, getdtablesize()-4);
      pprintf (p, "Value too high. System OS limits us to %d.\n",
              max_connections);
      pprintf (p, "For saftey's sake, it should not be higher than %d.\n",
              max_connections-2);
    }
  }
  pprintf(p,
    "There are currently %d regular and %d admin connections available,\n",
    max_connections-10, 10 );
  pprintf(p,
    "with %d maximum logins before unregistered login restrictions begin.\n",
    MAX(max_connections-50, 200) );
    pprintf(p, "Total allowed connections: %d.\n", max_connections );
  return COM_OK;
}

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
   Richard Nash	              	93/10/22	Created
   foxbat                       95/03/11        added filters in cmatch.
*/

#include "stdinclude.h"

#include "common.h"
#include "comproc.h"
#include "command.h"
#include "utils.h"
#include "ficsmain.h"
#include "config.h"
#include "playerdb.h"
#include "network.h"
#include "rmalloc.h"
#include "channel.h"
#include "variable.h"
#include "gamedb.h"
#include "gameproc.h"
#include "board.h"
/* #include "hostinfo.h" */
#include "multicol.h"
#include "ratings.h"
#include "formula.h"
#include "lists.h"
#include "eco.h"
#include <string.h>

#include <sys/resource.h>

/* grimm */
#if defined(SGI)
#else
/* int system(char *arg); */
#endif

const none = 0;
const blitz_rat = 1;
const std_rat = 2;
const wild_rat = 3;

int quota_time;

PUBLIC int com_rating_recalc(int p, param_list param)
{
  ASSERT(parray[p].adminLevel >= ADMIN_ADMIN);
  rating_recalc();
  return COM_OK;
}

PUBLIC int com_more(int p, param_list param)
{
/* in_push(IN_HELP); */
  pmore_file(p);
  return COM_OK;
}

PUBLIC int com_news(int p, param_list param)
{
  FILE *fp;
  char filename[MAX_FILENAME_SIZE];
  char junk[MAX_LINE_SIZE];
  char *junkp;
  int crtime;
  char count[10];
  int flag, len;

  if (((param[0].type == 0) || (!strcmp(param[0].val.word, "all")))) {

/* no params - then just display index over news */

    pprintf(p, "\n    **** BULLETIN BOARD ****\n\n");
    sprintf(filename, "%s/news.index", news_dir);
    fp = fopen(filename, "r");
    if (!fp) {
      fprintf(stderr, "Can't find news index.\n");
      return COM_OK;
    }
    flag = 0;
    while (!feof(fp)) {
      junkp = junk;
      fgets(junk, MAX_LINE_SIZE, fp);
      if (feof(fp))
	break;
      if ((len = strlen(junk)) > 1) {
	junk[len - 1] = '\0';
	sscanf(junkp, "%d %s", &crtime, count);
	junkp = nextword(junkp);
	junkp = nextword(junkp);
	if (((param[0].type == TYPE_WORD) && (!strcmp(param[0].val.word, "all")))) {
	  pprintf(p, "%3s (%s) %s\n", count, strltime(&crtime), junkp);
	  flag = 1;
	} else {
	  if ((crtime - player_lastconnect(p)) > 0) {
	    pprintf(p, "%3s (%s) %s\n", count, strltime(&crtime), junkp);
	    flag = 1;
	  }
	}
      }
    }
    fclose(fp);
    crtime = player_lastconnect(p);
    if (!flag) {
      pprintf(p, "There are no news since your last login (%s).\n", strltime(&crtime));
    } else {
      pprintf(p, "\n");
    }
  } else {

/* check if the specific news file exist in index */

    sprintf(filename, "%s/news.index", news_dir);
    fp = fopen(filename, "r");
    if (!fp) {
      fprintf(stderr, "Can't find news index.\n");
      return COM_OK;
    }
    flag = 0;
    while ((!feof(fp)) && (!flag)) {
      junkp = junk;
      fgets(junk, MAX_LINE_SIZE, fp);
      if (feof(fp))
	break;
      if ((len = strlen(junk)) > 1) {
	junk[len - 1] = '\0';
	sscanf(junkp, "%d %s", &crtime, count);
	if (!strcmp(count, param[0].val.word)) {
	  flag = 1;
	  junkp = nextword(junkp);
	  junkp = nextword(junkp);
	  pprintf(p, "\nNEWS %3s (%s)\n\n         %s\n\n", count, strltime(&crtime), junkp);
	}
      }
    }
    fclose(fp);
    if (!flag) {
      pprintf(p, "Bad index number!\n");
      return COM_OK;
    }
/* file exists - show it */

    sprintf(filename, "%s/news.%s", news_dir, param[0].val.word);
    fp = fopen(filename, "r");
    if (!fp) {
      pprintf(p, "No more info.\n");
      return COM_OK;
    }
    fclose(fp);
    sprintf(filename, "news.%s", param[0].val.word);
    if (psend_file(p, news_dir, filename) < 0) {
      pprintf(p, "Internal error - couldn't send news file!\n");
    }
  }
  return COM_OK;
}

PUBLIC int com_quit(int p, param_list param)
{
  if ((parray[p].game >= 0) && (garray[parray[p].game].status == GAME_EXAMINE)) {
    pcommand(p, "unexamine");
  }

  if (parray[p].game >= 0) {
    pprintf(p, "You can't quit while you are playing a game.\nType 'resign' to resign the game, or you can request an abort with 'abort'.\n");
    return COM_OK;
  }
  psend_file(p, mess_dir, MESS_LOGOUT);
  return COM_LOGOUT;
}

/*

PUBLIC int com_query(int p, param_list param)
{
  int p1;
  int count = 0;

  if (!parray[p].registered) {
    pprintf(p, "Only registered players can use the query command.\n");
    return COM_OK;
  }
  if (parray[p].muzzled) {
    pprintf(p, "You are muzzled.\n");
    return COM_OK;
  }
  if (!printablestring(param[0].val.string)) {
    pprintf(p, "Your message contains some unprintable character(s).\n");
    return COM_OK;
  }
  if (!parray[p].query_log) {
    parray[p].query_log = tl_new(5);
  } else {
    if (tl_numinlast(parray[p].query_log, 60 * 60) >= 2) {
      pprintf(p, "Your can only query twice per hour.\n");
      return COM_OK;
    }
  }
  in_push(IN_SHOUT);
  for (p1 = 0; p1 < p_num; p1++) {
    if (p1 == p)
      continue;
    if (parray[p1].status != PLAYER_PROMPT)
      continue;
    if (player_censored(p1, p))
      continue;
    count++;
    if (parray[p1].highlight) {
      pprintf_prompt(p1, "\n\033[7m%s queries:\033[0m %s\n", parray[p].name,
		     param[0].val.string);
    } else {
      pprintf_prompt(p1, "\n%s queries: %s\n", parray[p].name,
		     param[0].val.string);
    }
  }
  pprintf(p, "Query heard by %d player(s).\n", count);
  tl_logevent(parray[p].query_log, 1);
  in_pop();
  return COM_OK;
}

*/

int CheckShoutQuota(int p)
{
  int timenow = time(0);
  int timeleft = 0;
  if (in_list("quota", parray[p].name)) {
    if ((timeleft = timenow - parray[p].lastshout_a) < quota_time) {
      return (quota_time - timeleft);
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

PUBLIC int com_shout(int p, param_list param)
{
  int p1;
  int count = 0;
  int timeleft;			/* time left for quota if applicable */

  if (!parray[p].registered) {
    pprintf(p, "Only registered players can use the shout command.\n");
    return COM_OK;
  }
  if (parray[p].muzzled) {
    pprintf(p, "You are muzzled.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    if (in_list("quota", parray[p].name)) {
      pprintf(p, "[You are on the quota list.]\n");
      if ((timeleft = CheckShoutQuota(p))) {
	pprintf(p, "Next shout available in %d seconds.\n", timeleft);
      } else {
	pprintf(p, "Your next shout is ready for use.\n");
      }
    } else {
      pprintf(p, "[You are not on the quota list.]\n");
      pprintf(p, "Please specify what it is you want to shout.\n");
    }
    return COM_OK;
  }
  if ((timeleft = CheckShoutQuota(p))) {
    pprintf(p, "[You are on the quota list.]\n");
    pprintf(p, "Shout not sent. Next shout in %d seconds.\n", timeleft);
    return COM_OK;
  }
  parray[p].lastshout_a = parray[p].lastshout_b;
  parray[p].lastshout_b = time(0);
  if (!printablestring(param[0].val.string)) {
    pprintf(p, "Your message contains some unprintable character(s).\n");
    return COM_OK;
  }
/*  in_push(IN_SHOUT); */
  for (p1 = 0; p1 < p_num; p1++) {
    if (p1 == p)
      continue;
    if (parray[p1].status != PLAYER_PROMPT)
      continue;
    if (!parray[p1].i_shout)
      continue;
    if (player_censored(p1, p))
      continue;
    count++;
    pprintf_prompt(p1, "\n%s shouts: %s\n", parray[p].name,
		   param[0].val.string);
  }
  pprintf(p, "(%d) %s shouts: %s\n", count, parray[p].name,
	  param[0].val.string);
/*  in_pop(); */
  if ((in_list("quota", parray[p].name)) && (timeleft = CheckShoutQuota(p))) {
    pprintf(p, "[You are on the quota list.]\n");
    pprintf(p, "Next shout in %d seconds.\n", timeleft);
    return COM_OK;
  }
  return COM_OK;
}

PUBLIC int com_cshout(int p, param_list param)
{
  int p1;
  int count = 0;

  if (!parray[p].registered) {
    pprintf(p, "Only registered players can use the cshout command.\n");
    return COM_OK;
  }
  if (parray[p].cmuzzled) {
    pprintf(p, "You are c-muzzled.\n");
    return COM_OK;
  }
  if (!printablestring(param[0].val.string)) {
    pprintf(p, "Your message contains some unprintable character(s).\n");
    return COM_OK;
  }
/*  in_push(IN_SHOUT); */
  for (p1 = 0; p1 < p_num; p1++) {
    if (p1 == p)
      continue;
    if (parray[p1].status != PLAYER_PROMPT)
      continue;
    if (!parray[p1].i_cshout)
      continue;
    if (player_censored(p1, p))
      continue;
    count++;
    pprintf_prompt(p1, "\n%s c-shouts: %s\n", parray[p].name,
		   param[0].val.string);
  }
  pprintf(p, "(%d) %s c-shouts: %s\n", count, parray[p].name,
	  param[0].val.string);
/*  in_pop(); */
  return COM_OK;
}

PUBLIC int com_it(int p, param_list param)
{
  int p1;
  int count = 0;
  int timeleft;

  if (!parray[p].registered) {
    pprintf(p, "Only registered players can use the it command.\n");
    return COM_OK;
  }
  if (parray[p].muzzled) {
    pprintf(p, "You are muzzled.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    if (in_list("quota", parray[p].name)) {
      pprintf(p, "[You are on the quota list.]\n");
      if ((timeleft = CheckShoutQuota(p))) {
	pprintf(p, "Next shout available in %d seconds.\n", timeleft);
      } else {
	pprintf(p, "Your next shout is ready for use.\n");
      }
    } else {
      pprintf(p, "[You are not on the quota list.]\n");
      pprintf(p, "Please specify what it is you want to shout.\n");
    }
    return COM_OK;
  }
  if ((timeleft = CheckShoutQuota(p))) {
    pprintf(p, "[You are on the quota list.]\n");
    pprintf(p, "Shout not sent. Next shout in %d seconds.\n", timeleft);
    return COM_OK;
  }
  parray[p].lastshout_a = parray[p].lastshout_b;
  parray[p].lastshout_b = time(0);

  if (!printablestring(param[0].val.string)) {
    pprintf(p, "Your message contains some unprintable character(s).\n");
    return COM_OK;
  }
/*  in_push(IN_SHOUT); */
  for (p1 = 0; p1 < p_num; p1++) {
    if (p1 == p)
      continue;
    if (parray[p1].status != PLAYER_PROMPT)
      continue;
    if (!parray[p1].i_shout)
      continue;
    if (player_censored(p1, p))
      continue;
    count++;
    if ((!strncmp(param[0].val.string, "\'", 1)) ||
	(!strncmp(param[0].val.string, ",", 1)) ||
	(!strncmp(param[0].val.string, ".", 1))) {
      pprintf_prompt(p1, "\n--> %s%s\n", parray[p].name,
		     param[0].val.string);
    } else {
      pprintf_prompt(p1, "\n--> %s %s\n", parray[p].name,
		     param[0].val.string);
    }
  }
  if ((!strncmp(param[0].val.string, "\'", 1)) ||
      (!strncmp(param[0].val.string, ",", 1)) ||
      (!strncmp(param[0].val.string, ".", 1))) {
    pprintf(p, "(%d) --> %s%s\n", count, parray[p].name, param[0].val.string);
  } else {
    pprintf(p, "(%d) --> %s %s\n", count, parray[p].name, param[0].val.string);
  }
/*  in_pop(); */
  if ((in_list("quota", parray[p].name)) && (timeleft = CheckShoutQuota(p))) {
    pprintf(p, "[You are on the quota list.]\n");
    pprintf(p, "Next shout in %d seconds.\n", timeleft);
    return COM_OK;
  }
  return COM_OK;
}

#define TELL_TELL 0
#define TELL_SAY 1
#define TELL_WHISPER 2
#define TELL_KIBITZ 3
#define TELL_CHANNEL 4
PRIVATE int tell(int p, int p1, char *msg, int why, int ch)
{
  char tmp[MAX_LINE_SIZE];

  if (!printablestring(msg)) {
    pprintf(p, "Your message contains some unprintable character(s).\n");
    return COM_OK;
  }
/*  if (p1 == p) {
 *   pprintf(p, "Quit talking to yourself! It's embarrassing.\n");
 *   return COM_OK;
 * }
 */
  if ((!parray[p1].i_tell) && (!parray[p].registered)) {
    pprintf(p, "Player \"%s\" isn't listening to unregistered tells.\n",
	    parray[p1].name);
    return COM_OK;
  }
  if ((player_censored(p1, p)) && (parray[p].adminLevel == 0)) {
    pprintf(p, "Player \"%s\" is censoring you.\n", parray[p1].name);
    return COM_OK;
  }
/*  in_push(IN_TELL); */
  switch (why) {
  case TELL_SAY:
    pprintf_highlight(p1, "\n%s", parray[p].name);
    pprintf_prompt(p1, " says: %s\n", msg);
    break;
  case TELL_WHISPER:
    pprintf(p1, "\n%s", parray[p].name);
    pprintf_prompt(p1, " whispers: %s\n", msg);
    break;
  case TELL_KIBITZ:
    pprintf(p1, "\n%s", parray[p].name);
    pprintf_prompt(p1, " kibitzes: %s\n", msg);
    break;
  case TELL_CHANNEL:
    pprintf(p1, "\n%s", parray[p].name);
    pprintf_prompt(p1, "(%d): %s\n", ch, msg);
    break;
  case TELL_TELL:
  default:
    if (parray[p1].highlight) {
      pprintf_highlight(p1, "\n%s", parray[p].name);
    } else {
      pprintf(p1, "\n%s", parray[p].name);
    }
    pprintf_prompt(p1, " tells you: %s\n", msg);
    break;
  }
  tmp[0] = '\0';
  if (!(parray[p1].busy[0] == '\0')) {
    sprintf(tmp, ", who %s (idle: %d minutes)", parray[p1].busy,
	    ((player_idle(p1) % 3600) / 60));
  } else {
    if (((player_idle(p1) % 3600) / 60) > 2) {
      sprintf(tmp, ", who has been idle %d minutes", ((player_idle(p1) % 3600) / 60));
    }
    /* else sprintf(tmp," "); */
  }
  if ((why == TELL_SAY) || (why == TELL_TELL)) {
    pprintf(p, "(told %s%s)\n", parray[p1].name,
            (((parray[p1].game>=0) && (garray[parray[p1].game].status == GAME_EXAMINE)) 
            ? ", who is examining a game" : 
	    (parray[p1].game >= 0 && (parray[p1].game != parray[p].game))
	    ? ", who is playing" : tmp));
    parray[p].last_tell = p1;
  }
/*  in_pop(); */
  return COM_OK;
}

PRIVATE int chtell(int p, int ch, char *msg)
{
  int p1;
  int i, count = 0, listening = 0;

  if ((ch == 0) && (parray[p].adminLevel == 0)) {
    pprintf(p, "Only admins may send tells to channel 0.\n");
    return COM_OK;
  }
  if (ch < 0) {
    pprintf(p, "The lowest channel number is 0.\n");
    return COM_OK;
  }
  if (ch >= MAX_CHANNELS) {
    pprintf(p, "The maximum channel number is %d.\n", MAX_CHANNELS - 1);
    return COM_OK;
  }
/*  in_push(IN_TELL); */
  for (i = 0; i < numOn[ch]; i++) {
    p1 = channels[ch][i];
    if (p1 == p) {
      listening = 1;
      continue;
    }
    if (player_censored(p1, p))
      continue;
    if ((parray[p1].status == PLAYER_PASSWORD)
	|| (parray[p1].status == PLAYER_LOGIN))
      continue;
    tell(p, p1, msg, TELL_CHANNEL, ch);
    count++;
  }
  if (count) {
    /* parray[p].last_tell = -1; */
    parray[p].last_channel = ch;
  }
  pprintf(p, "(%d->(%d))", ch, count);
  if (!listening)
    pprintf(p, " (You're not listening to channel %d.)", ch);
  pprintf(p, "\n");
/*  in_pop(); */
  return COM_OK;
}

PUBLIC int com_whisper(int p, param_list param)
{
  int g;
  int p1;
  int count = 0;

  if (!parray[p].num_observe && parray[p].game < 0) {
    pprintf(p, "You are not playing or observing a game.\n");
    return COM_OK;
  }
  if (!parray[p].registered && (parray[p].game == -1)) {
    pprintf(p, "You must be registered to whisper other people's games.\n");
    return COM_OK;
  }
  if (parray[p].game >= 0)
    g = parray[p].game;
  else
    g = parray[p].observe_list[0];
  for (p1 = 0; p1 < p_num; p1++) {
    if (p1 == p)
      continue;
    if (parray[p1].status != PLAYER_PROMPT)
      continue;
    if (player_is_observe(p1, g)) {
      tell(p, p1, param[0].val.string, TELL_WHISPER, 0);
      if ((parray[p].adminLevel >= ADMIN_ADMIN) || !garray[g].private)
	count++;
    }
  }
  pprintf(p, "whispered to %d.\n", count);
  return COM_OK;
}

PUBLIC int com_kibitz(int p, param_list param)
{
  int g;
  int p1;
  int count = 0;

  if (!parray[p].num_observe && parray[p].game < 0) {
    pprintf(p, "You are not playing or observing a game.\n");
    return COM_OK;
  }
  if (!parray[p].registered && (parray[p].game == -1)) {
    pprintf(p, "You must be registered to kibitz other people's games.\n");
    return COM_OK;
  }
  if (parray[p].game >= 0)
    g = parray[p].game;
  else
    g = parray[p].observe_list[0];
  for (p1 = 0; p1 < p_num; p1++) {
    if (p1 == p)
      continue;
    if (parray[p1].status != PLAYER_PROMPT)
      continue;
    if (player_is_observe(p1, g) || parray[p1].game == g) {
      tell(p, p1, param[0].val.string, TELL_KIBITZ, 0);
      if ((parray[p].adminLevel >= ADMIN_ADMIN) || !garray[g].private || (parray[p1].game == g))
	count++;
    }
  }
  pprintf(p, "kibitzed to %d.\n", count);
  return COM_OK;
}

PUBLIC int com_tell(int p, param_list param)
{
  int p1;

  if (param[0].type == TYPE_NULL)
    return COM_BADPARAMETERS;
  if (param[0].type == TYPE_WORD) {
    stolower(param[0].val.word);
    if (!strcmp(param[0].val.word, ".")) {
      if (parray[p].last_tell < 0) {
	pprintf(p, "No one to tell anything to.\n");
	return COM_OK;
      } else {
	return tell(p, parray[p].last_tell, param[1].val.string, TELL_TELL, 0);
      }
    }
    if (!strcmp(param[0].val.word, ",")) {
      if (parray[p].last_channel < 0) {
	pprintf(p, "No previous channel.\n");
	return COM_OK;
      } else {
	return chtell(p, parray[p].last_channel, param[1].val.string);
      }
    }
    p1 = player_find_part_login(param[0].val.word);
    if ((p1 < 0) || (parray[p1].status == PLAYER_PASSWORD)
	|| (parray[p1].status == PLAYER_LOGIN)) {
      pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
    return tell(p, p1, param[1].val.string, TELL_TELL, 0);
  } else {			/* Channel */
    return chtell(p, param[0].val.integer, param[1].val.string);
  }
}

PUBLIC int com_xtell(int p, param_list param)
{
  int p1;
  char *msg;
  char tmp[2048];

  msg = param[1].val.string;
  p1 = player_find_part_login(param[0].val.word);
  if ((p1 < 0) || (parray[p1].status == PLAYER_PASSWORD)
      || (parray[p1].status == PLAYER_LOGIN)) {
    pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
    return COM_OK;
  }
  if (!printablestring(msg)) {
    pprintf(p, "Your message contains some unprintable character(s).\n");
    return COM_OK;
  }
  if ((!parray[p1].i_tell) && (!parray[p].registered)) {
    pprintf(p, "Player \"%s\" isn't listening to unregistered tells.\n",
	    parray[p1].name);
    return COM_OK;
  }
  if ((player_censored(p1, p)) && (parray[p].adminLevel == 0)) {
    pprintf(p, "Player \"%s\" is censoring you.\n", parray[p1].name);
    return COM_OK;
  }
  if (parray[p1].highlight) {
    pprintf_highlight(p1, "\n%s", parray[p].name);
  } else {
    pprintf(p1, "\n%s", parray[p].name);
  }
  pprintf_prompt(p1, " tells you: %s\n", msg);

  tmp[0] = '\0';
  if (!(parray[p1].busy[0] == '\0')) {
    sprintf(tmp, ", who %s (idle: %d minutes)", parray[p1].busy,
	    ((player_idle(p1) % 3600) / 60));
  } else {
    if (((player_idle(p1) % 3600) / 60) > 2) {
      sprintf(tmp, ", who has been idle %d minutes", ((player_idle(p1) % 3600) / 60));
    }
  }
  pprintf(p, "(told %s%s)\n", parray[p1].name,
          (((parray[p1].game>=0) && (garray[parray[p1].game].status == GAME_EXAMINE)) 
          ? ", who is examining a game" : 
	  (parray[p1].game >= 0 && (parray[p1].game != parray[p].game))
          ? ", who is playing" : tmp));
  return COM_OK;
}

PUBLIC int com_say(int p, param_list param)
{
  if (parray[p].opponent < 0) {
    if (parray[p].last_opponent < 0) {
      pprintf(p, "No one to say anything to, try tell.\n");
      return COM_OK;
    } else {
      return tell(p, parray[p].last_opponent, param[0].val.string, TELL_SAY, 0);
    }
  }
  return tell(p, parray[p].opponent, param[0].val.string, TELL_SAY, 0);
}

PUBLIC int com_set(int p, param_list param)
{
  int result;
  int which;
  char *val;

  if (param[1].type == TYPE_NULL)
    val = NULL;
  else
    val = param[1].val.string;
  result = var_set(p, param[0].val.word, val, &which);
  switch (result) {
  case VAR_OK:
    break;
  case VAR_BADVAL:
    pprintf(p, "Bad value given for variable %s.\n", param[0].val.word);
    break;
  case VAR_NOSUCH:
    pprintf(p, "No such variable name %s.\n", param[0].val.word);
    break;
  case VAR_AMBIGUOUS:
    pprintf(p, "Ambiguous variable name %s.\n", param[0].val.word);
    break;
  }
  player_save(p);
  return COM_OK;
}

PUBLIC int FindPlayer(int p, parameter * param, int *p1, int *connected)
{
  if (param->type == TYPE_WORD) {
    *p1 = player_search(p, param->val.word);
    if (*p1 == 0)
      return 0;
    if (*p1 < 0) {		/* player had to be connected and will be
				   removed later */
      *connected = 0;
      *p1 = (-*p1) - 1;
    } else {
      *connected = 1;
      *p1 = *p1 - 1;
    }
  } else {
    *p1 = p;
    *connected = 1;
  }
  return 1;
}

PRIVATE void com_stats_andify(int *numbers, int howmany, char *dest)
{
  char tmp[10];

  *dest = '\0';
  while (howmany--) {
    sprintf(tmp, "%d", numbers[howmany]);
    strcat(dest, tmp);
    if (howmany > 1)
      sprintf(tmp, ", ");
    else if (howmany == 1)
      sprintf(tmp, " and ");
    else
      sprintf(tmp, ".\n");
    strcat(dest, tmp);
  }
  return;
}

PRIVATE void com_stats_rating(char *hdr, statistics * stats, char *dest)
{
  char tmp[100];

  sprintf(dest, "%-10s%4s    %5.1f   %4d   %4d   %4d   %4d",
	  hdr, ratstr(stats->rating), stats->sterr, stats->win, stats->los, stats->dra, stats->num);
  if (stats->whenbest) {
    sprintf(tmp, "   %d", stats->best);
    strcat(dest, tmp);
    strftime(tmp, sizeof(tmp), " (%d-%b-%y)", localtime((time_t *) & stats->whenbest));
    strcat(dest, tmp);
  }
  strcat(dest, "\n");
  return;
}

PUBLIC int com_stats(int p, param_list param)
{
  int g, i, t;
  int p1, connected;
  char line[255], tmp[255];
  int numbers[MAX_OBSERVE > MAX_SIMUL ? MAX_OBSERVE : MAX_SIMUL];

  if (!FindPlayer(p, &param[0], &p1, &connected))
    return COM_OK;

  sprintf(line, "\nStatistics for %-11s ", parray[p1].name);
  if ((connected) && (parray[p1].status == PLAYER_PROMPT)) {
    sprintf(tmp, "On for: %s", hms(player_ontime(p1), 0, 0, 0));
    strcat(line, tmp);
    sprintf(tmp, "   Idle: %s\n", hms(player_idle(p1), 0, 0, 0));
  } else {
    if ((t = player_lastdisconnect(p1)))
      sprintf(tmp, "(Last disconnected %s):\n", strltime(&t));
    else
      sprintf(tmp, "(Never connected.)\n");
  }
  strcat(line, tmp);
  pprintf(p, "%s", line);
  if (parray[p1].simul_info.numBoards) {
    for (i = 0, t = 0; i < parray[p1].simul_info.numBoards; i++) {
      if ((numbers[t] = parray[p1].simul_info.boards[i] + 1) != 0)
	t++;
    }
    pprintf(p, "%s is giving a simul: game%s ", parray[p1].name, ((t > 1) ? "s" : ""));
    com_stats_andify(numbers, t, tmp);
    pprintf(p, tmp);
  } else if (parray[p1].game >= 0) {
    g = parray[p1].game;
    if (garray[g].status == GAME_EXAMINE) {
      pprintf(p, "(Examining game %d: %s vs. %s)\n", g + 1, 
            parray[garray[g].white].name, parray[garray[g].black].name);
    } else {
      pprintf(p, "(playing game %d: %s vs. %s)\n", g + 1,
	    parray[garray[g].white].name, parray[garray[g].black].name);
    }
  }
  if (parray[p1].num_observe) {
    for (i = 0, t = 0; i < parray[p1].num_observe; i++) {
      g = parray[p1].observe_list[i];
      if ((g != -1) && ((parray[p].adminLevel >= ADMIN_ADMIN) || (garray[g].private == 0)))
	numbers[t++] = g + 1;
    }
    if (t) {
      pprintf(p, "%s is observing game%s ", parray[p1].name, ((t > 1) ? "s" : ""));
      com_stats_andify(numbers, t, tmp);
      pprintf(p, tmp);
    }
  }
  if (parray[p1].busy[0]) {
    pprintf(p, "(%s %s)\n", parray[p1].name, parray[p1].busy);
  }
  if (!parray[p1].registered) {
    pprintf(p, "%s is NOT a registered player.\n\n", parray[p1].name);
  } else {
    pprintf(p, "\n         rating     RD     win   loss   draw  total   best\n");
    com_stats_rating("Blitz", &parray[p1].b_stats, tmp);
    pprintf(p, tmp);
    com_stats_rating("Standard", &parray[p1].s_stats, tmp);
    pprintf(p, tmp);
    com_stats_rating("Wild", &parray[p1].w_stats, tmp);
    pprintf(p, tmp);
  }
  pprintf(p, "\n");
  if (parray[p1].adminLevel > 0) {
    pprintf(p, "Admin Level: ");
    switch (parray[p1].adminLevel) {
/*
    case 0:
      pprintf(p, "Normal User\n");
      break;
    case 5:
      pprintf(p, "Extra Cool User\n");    vek wants to be 5
      break;

 Forget it - you can do some admin stuff if your level is > than 0     - DAV

*/
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
      pprintf(p, "Master Help File Librarian/Administrator\n");
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
  if (parray[p].adminLevel > 0)
    pprintf(p, "Full Name  : %s\n", (parray[p1].fullName ? parray[p1].fullName : "(none)"));
  if (((p1 == p) && (parray[p1].registered)) || (parray[p].adminLevel > 0))
    pprintf(p, "Address    : %s\n", (parray[p1].emailAddress ? parray[p1].emailAddress : "(none)"));
  if (parray[p].adminLevel > 0) {
    pprintf(p, "Host       : %s\n",
/*
	    ((hp = gethostbyaddr((const char*) (connected ? &parray[p1].thisHost : &parray[p1].lastHost), sizeof(parray[p1].thisHost), AF_INET)) == 0) ? "" : hp->h_name,
*/
	    dotQuad(connected ? parray[p1].thisHost : parray[p1].lastHost));
  }
  if ((parray[p].adminLevel > 0) && (parray[p1].registered))
    if (parray[p1].num_comments)
      pprintf(p, "Comments   : %d\n", parray[p1].num_comments);

  if (parray[p1].num_plan) {
    pprintf(p, "\n");
    for (i = 0; i < parray[p1].num_plan; i++)
      pprintf(p, "%2d: %s\n", i + 1, (parray[p1].planLines[i] != NULL) ? parray[p1].planLines[i] : "");
  }
  if (!connected)
    player_remove(p1);
  return COM_OK;
}



PUBLIC int com_variables(int p, param_list param)
{
  int p1, connected;
  int i;

  if (!FindPlayer(p, &param[0], &p1, &connected))
    return COM_OK;

  pprintf(p, "Variable settings of %s:\n", parray[p1].name);
/*  if (parray[p1].fullName)
    pprintf(p, "   Realname: %s\n", parray[p1].fullName);
*/
  if (parray[p1].uscfRating)
    pprintf(p, "   USCF: %d\n", parray[p1].uscfRating);
  pprintf(p, "   time=%-3d    inc=%-3d    private=%d\n",
	  parray[p1].d_time, parray[p1].d_inc, parray[p1].private);
  pprintf(p, "   rated=%d     ropen=%d    open=%d     simopen=%d\n",
     parray[p1].rated, parray[p1].ropen, parray[p1].open, parray[p1].sopen);
  pprintf(p, "   shout=%d     cshout=%d   kib=%d      tell=%d     notifiedby=%d\n",
	  parray[p1].i_shout, parray[p1].i_cshout, parray[p1].i_kibitz, parray[p1].i_tell, parray[p1].notifiedby);
  pprintf(p, "   pin=%d       gin=%d      style=%-3d  flip=%d\n",
	  parray[p1].i_login, parray[p1].i_game, parray[p1].style + 1, parray[p1].flip);
  pprintf(p, "   highlight=%d bell=%d     auto=%d     mailmess=%d  pgn=%d\n",
	  parray[p1].highlight, parray[p1].bell, parray[p1].automail, parray[p1].i_mailmess, parray[p1].pgn);
  pprintf(p, "   width=%-3d   height=%-3d\n",
	  parray[p1].d_width, parray[p1].d_height);
  if (parray[p1].prompt && parray[p1].prompt != def_prompt)
    pprintf(p, "   Prompt: %s\n", parray[p1].prompt);

  {				/* added code to print channels */
    int count = 0;
    for (i = 0; i < MAX_CHANNELS; i++) {
      if (on_channel(i, p1)) {
	if (!count)
	  pprintf(p, "\n  Channels:");
	pprintf(p, " %d", i);
	count++;
      }
    }
    if (count)
      pprintf(p, "\n");
  }
/*  if (parray[p1].numAlias && (p == p1)) {
    pprintf(p, "\n   Aliases:\n");
    for (i = 0; i < parray[p1].numAlias; i++) {
      pprintf(p, "      %s %s\n", parray[p1].alias_list[i].comm_name,
	      parray[p1].alias_list[i].alias);
    }
  }
*/
  if (parray[p1].num_formula) {
    pprintf(p, "\n");
    for (i = 0; i < parray[p1].num_formula; i++) {
      if (parray[p1].formulaLines[i] != NULL)
	pprintf(p, " f%d: %s\n", i + 1, parray[p1].formulaLines[i]);
      else
	pprintf(p, " f%d:\n", i + 1);
    }
  }
  if (parray[p1].formula != NULL)
    pprintf(p, "\nFormula: %s\n", parray[p1].formula);

  if (!connected)
    player_remove(p1);
  return COM_OK;
}



PUBLIC int com_password(int p, param_list param)
{
  char *oldpassword = param[0].val.word;
  char *newpassword = param[1].val.word;
  char salt[3];

  if (!parray[p].registered) {
    pprintf(p, "Setting a password is only for registered players.\n");
    return COM_OK;
  }
  if (parray[p].passwd) {
    salt[0] = parray[p].passwd[0];
    salt[1] = parray[p].passwd[1];
    salt[2] = '\0';
    if (strcmp(crypt(oldpassword, salt), parray[p].passwd)) {
      pprintf(p, "Incorrect password, password not changed!\n");
      return COM_OK;
    }
    rfree(parray[p].passwd);
    parray[p].passwd = NULL;
  }
  salt[0] = 'a' + rand() % 26;
  salt[1] = 'a' + rand() % 26;
  salt[2] = '\0';
  parray[p].passwd = strdup(crypt(newpassword, salt));
  pprintf(p, "Password changed to \"%s\".\n", newpassword);
  return COM_OK;
}

PUBLIC int com_uptime(int p, param_list param)
{
  unsigned long uptime = time(0) - startuptime;
  struct rusage ru;
  int days  = (uptime / (60*60*24));
  int hours = ((uptime % (60*60*24)) / (60*60));
  int mins  = (((uptime % (60*60*24)) % (60*60)) / 60); 
  int secs  = (((uptime % (60*60*24)) % (60*60)) % 60);

  pprintf(p, "Server location: %s   Server version : %s\n", fics_hostname,VERS_NUM);
  pprintf(p, "The server has been up since %s.\n", strltime(&startuptime));
  if ((days==0) && (hours==0) && (mins==0)) {
    pprintf(p, "(Up for %d second%s)\n", 
               secs, (secs==1) ? "" : "s"); 
  } else if ((days==0) && (hours==0)) {
    pprintf(p, "(Up for %d minute%s and %d second%s)\n", 
               mins, (mins==1) ? "" : "s", 
               secs, (secs==1) ? "" : "s");
  } else if (days==0) {
    pprintf(p, "(Up for %d hour%s, %d minute%s and %d second%s)\n",
               hours, (hours==1) ? "" : "s",
               mins, (mins==1) ? "" : "s", 
               secs, (secs==1) ? "" : "s");
  } else {
    pprintf(p, "(Up for %d day%s, %d hour%s, %d minute%s and %d second%s)\n",
               days, (days==1) ? "" : "s",
               hours, (hours==1) ? "" : "s",
               mins, (mins==1) ? "" : "s", 
               secs, (secs==1) ? "" : "s");
  }
  pprintf(p, "\nAllocs: %u  Frees: %u  Allocs In Use: %u\n",
	  malloc_count, free_count, malloc_count - free_count);
  if (parray[p].adminLevel >= ADMIN_ADMIN) {
    pprintf(p, "\nplayer size:%d, game size:%d, con size:%d, g_num:%d\n",
	    sizeof(player), sizeof(game), net_consize(), g_num);
    getrusage(RUSAGE_SELF, &ru);
    pprintf(p, "pagesize = %d, maxrss = %d, total = %d\n", getpagesize(), ru.ru_maxrss, getpagesize() * ru.ru_maxrss);
  }
  pprintf(p, "\nPlayer limit: %d\n", max_connections);
  pprintf(p, "\nThere are currently %d players, with a high of %d since last restart.\n", player_count(), player_high);
  pprintf(p, "There are currently %d games, with a high of %d since last restart.\n", game_count(), game_high);
  pprintf(p, "\nCompiled on %s\n", COMP_DATE);
  return COM_OK;
}

PUBLIC int com_date(int p, param_list param)
{
  int t = time(0);
  pprintf(p, "Local time     - %s\n", strltime(&t));
  pprintf(p, "Greenwich time - %s\n", strgtime(&t));
  return COM_OK;
}

char *inout_string[] = {
  "login", "logout"
};

PRIVATE int plogins(p, fname)
int p;
char *fname;
{
  FILE *fp;
  int inout, thetime, registered;
  char loginName[MAX_LOGIN_NAME + 1];
  char ipstr[20];

  fp = fopen(fname, "r");
  if (!fp) {
    pprintf(p, "Sorry, no login information available.\n");
    return COM_OK;
  }
  while (!feof(fp)) {
    if (fscanf(fp, "%d %s %d %d %s\n", &inout, loginName, &thetime,
	       &registered, ipstr) != 5) {
      fprintf(stderr, "FICS: Error in login info format. %s\n", fname);
      fclose(fp);
      return COM_OK;
    }
    pprintf(p, "%s: %-17s %-6s", strltime(&thetime), loginName,
	    inout_string[inout]);
    if (parray[p].adminLevel > 0) {
      pprintf(p, " from %s\n", ipstr);
    } else
      pprintf(p, "\n");
  }
  fclose(fp);
  return COM_OK;
}

PUBLIC int com_llogons(int p, param_list param)
{
  char fname[MAX_FILENAME_SIZE];

  sprintf(fname, "%s/%s", stats_dir, STATS_LOGONS);
  return plogins(p, fname);
}

PUBLIC int com_logons(int p, param_list param)
{
  char fname[MAX_FILENAME_SIZE];

  if (param[0].type == TYPE_WORD) {
    sprintf(fname, "%s/player_data/%c/%s.%s", stats_dir, param[0].val.word[0], param[0].val.word, STATS_LOGONS);
  } else {
    sprintf(fname, "%s/player_data/%c/%s.%s", stats_dir, parray[p].login[0], parray[p].login, STATS_LOGONS);
  }
  return plogins(p, fname);
}

#define WHO_OPEN 0x01
#define WHO_CLOSED 0x02
#define WHO_RATED 0x04
#define WHO_UNRATED 0x08
#define WHO_FREE 0x10
#define WHO_PLAYING 0x20
#define WHO_REGISTERED 0x40
#define WHO_UNREGISTERED 0x80

PRIVATE void who_terse(int p, int num, int *plist, int type)
{
  char ptmp[80 + 20];		/* for highlight */
  multicol *m = multicol_start(PARRAY_SIZE);
  int i;
  int p1;
  int rat;

  /* altered DAV 3/15/95 */

  for (i = 0; i < num; i++) {
    p1 = plist[i];
    if (type == blitz_rat)
      rat = parray[p1].b_stats.rating;
    if (type == wild_rat)
      rat = parray[p1].w_stats.rating;
    if (type == std_rat)
      rat = parray[p1].s_stats.rating;

    if (type == none) {
      sprintf(ptmp, "     ");
    } else {
      sprintf(ptmp, "%-4s", ratstrii(rat, parray[p1].registered));
      if (parray[p1].simul_info.numBoards) {
	strcat(ptmp, "~");
      } else if ((parray[p1].game >= 0) && (garray[parray[p1].game].status == GAME_EXAMINE)) {
        strcat(ptmp, "#");
      } else if (parray[p1].game >= 0) {
	strcat(ptmp, "^");
      } else if (!parray[p1].open) {
	strcat(ptmp, ":");
      } else if (player_idle(p1) > 300) {
	strcat(ptmp, ".");
      } else {
	strcat(ptmp, " ");
      }
    }
    if (p == p1) {
      psprintf_highlight(p, ptmp + strlen(ptmp), "%s", parray[p1].name);
    } else {
      strcat(ptmp, parray[p1].name);
    }
    if ((parray[p1].adminLevel >= 10) && (parray[p1].i_admin))
      strcat(ptmp, "(*)");
    if (in_list("computer", parray[p1].name))
      strcat(ptmp, "(C)");
/* grimm's fishlist
    if (in_list("fish", parray[p1].name)) strcat(ptmp, "(Fish)");
*/
    if (in_list("fm", parray[p1].name))
      strcat(ptmp, "(FM)");
    if (in_list("im", parray[p1].name))
      strcat(ptmp, "(IM)");
    if (in_list("gm", parray[p1].name))
      strcat(ptmp, "(GM)");
    if (in_list("td", parray[p1].name))
      strcat(ptmp, "(TD)");
    multicol_store(m, ptmp);
  }
  multicol_pprint(m, p, 80, 2);
  multicol_end(m);
  pprintf(p, "\n %d Players displayed (of %d). (*) indicates system administrator.\n", num, player_count());
}

PRIVATE void who_verbose(p, num, plist)
int p;
int num;
int plist[];
{
  int i, p1;
  char playerLine[255], tmp[255];	/* +8 for highlight */

  pprintf(p,
      " +---------------------------------------------------------------+\n"
    );
  pprintf(p,
      " |      User              Standard    Blitz        On for   Idle |\n"
    );
  pprintf(p,
      " +---------------------------------------------------------------+\n"
    );

  for (i = 0; i < num; i++) {
    p1 = plist[i];

    strcpy(playerLine, " |");

    if (parray[p1].game >= 0)
      sprintf(tmp, "%3d", parray[p1].game + 1);
    else
      sprintf(tmp, "   ");
    strcat(playerLine, tmp);

    if (!parray[p1].open)
      sprintf(tmp, "X");
    else
      sprintf(tmp, " ");
    strcat(playerLine, tmp);

    if (parray[p1].registered)
      if (parray[p1].rated) {
	sprintf(tmp, " ");
      } else {
	sprintf(tmp, "u");
      }
    else
      sprintf(tmp, "U");
    strcat(playerLine, tmp);

    /* Modified by DAV 3/15/95 */
    if (p == p1) {
      strcpy(tmp, " ");
      psprintf_highlight(p, tmp + strlen(tmp), "%-17s", parray[p1].name);
    } else {
      sprintf(tmp, " %-17s", parray[p1].name);
    }
    strcat(playerLine, tmp);

    sprintf(tmp, " %4s        %-4s        %5s  ",
	    ratstrii(parray[p1].s_stats.rating, parray[p1].registered),
	    ratstrii(parray[p1].b_stats.rating, parray[p1].registered),
	    hms(player_ontime(p1), 0, 0, 0));
    strcat(playerLine, tmp);

    if (player_idle(p1) >= 60) {
      sprintf(tmp, "%5s   |\n", hms(player_idle(p1), 0, 0, 0));
    } else {
      sprintf(tmp, "        |\n");
    }
    strcat(playerLine, tmp);
    pprintf(p, "%s", playerLine);
  }

  pprintf(p,
      " |                                                               |\n"
    );
  pprintf(p,
     " |    %3d Players Displayed                                      |\n",
	  num
    );
  pprintf(p,
      " +---------------------------------------------------------------+\n"
    );
}

PRIVATE void who_winloss(p, num, plist)
int p;
int num;
int plist[];
{
  int i, p1;
  char playerLine[255], tmp[255];	/* for highlight */

  pprintf(p,
	  "Name               Stand     win loss draw   Blitz    win loss draw    idle\n"
    );
  pprintf(p,
	  "----------------   -----     -------------   -----    -------------    ----\n"
    );

  for (i = 0; i < num; i++) {
    p1 = plist[i];
    if (p1 == p) {
      psprintf_highlight(p, playerLine, "%-17s", parray[p1].name);
    } else {
      sprintf(playerLine, "%-17s", parray[p1].name);
    }
    sprintf(tmp, "  %4s     %4d %4d %4d   ",
	    ratstrii(parray[p1].s_stats.rating, parray[p1].registered),
	    (int) parray[p1].s_stats.win,
	    (int) parray[p1].s_stats.los,
	    (int) parray[p1].s_stats.dra);
    strcat(playerLine, tmp);

    sprintf(tmp, "%4s    %4d %4d %4d   ",
	    ratstrii(parray[p1].b_stats.rating, parray[p1].registered),
	    (int) parray[p1].b_stats.win,
	    (int) parray[p1].b_stats.los,
	    (int) parray[p1].b_stats.dra);
    strcat(playerLine, tmp);

    if (player_idle(p1) >= 60) {
      sprintf(tmp, "%5s\n", hms(player_idle(p1), 0, 0, 0));
    } else {
      sprintf(tmp, "     \n");
    }
    strcat(playerLine, tmp);

    pprintf(p, "%s", playerLine);
  }
  pprintf(p, "    %3d Players Displayed.\n", num);
}

PRIVATE int who_ok(p, sel_bits)
int p;
unsigned int sel_bits;
{
  if (parray[p].status != PLAYER_PROMPT)
    return 0;
  if (sel_bits == 0xff)
    return 1;
  if (sel_bits & WHO_OPEN)
    if (!parray[p].open)
      return 0;
  if (sel_bits & WHO_CLOSED)
    if (parray[p].open)
      return 0;
  if (sel_bits & WHO_RATED)
    if (!parray[p].rated)
      return 0;
  if (sel_bits & WHO_UNRATED)
    if (parray[p].rated)
      return 0;
  if (sel_bits & WHO_FREE)
    if (parray[p].game >= 0)
      return 0;
  if (sel_bits & WHO_PLAYING)
    if (parray[p].game < 0)
      return 0;
  if (sel_bits & WHO_REGISTERED)
    if (!parray[p].registered)
      return 0;
  if (sel_bits & WHO_UNREGISTERED)
    if (parray[p].registered)
      return 0;
  return 1;
}


PRIVATE int blitz_cmp(const void *pp1, const void *pp2)
{
  register int p1 = *(int *) pp1;
  register int p2 = *(int *) pp2;
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

PRIVATE int stand_cmp(const void *pp1, const void *pp2)
{
  register int p1 = *(int *) pp1;
  register int p2 = *(int *) pp2;
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

PRIVATE int wild_cmp(const void *pp1, const void *pp2)
{
  register int p1 = *(int *) pp1;
  register int p2 = *(int *) pp2;
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

PRIVATE int alpha_cmp(const void *pp1, const void *pp2)
{
  register int p1 = *(int *) pp1;
  register int p2 = *(int *) pp2;
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

PUBLIC void sort_players(int players[PARRAY_SIZE],
			  int ((*cmp_func) (const void *, const void *)))
{
  int i;

  for (i = 0; i < p_num; i++) {
    players[i] = i;
  }
  qsort(players, p_num, sizeof(int), cmp_func);
}

/* This is the of the most compliclicated commands in terms of parameters */
PUBLIC int com_who(int p, param_list param)
{
  int style = 0;
  float stop_perc = 1.0;
  float start_perc = 0;
  unsigned int sel_bits = 0xff;
  int sortlist[PARRAY_SIZE], plist[PARRAY_SIZE];
  int ((*cmp_func) (const void *, const void *)) = blitz_cmp;
  int startpoint;
  int stoppoint;
  int i, len;
  int tmpI, tmpJ;
  char c;
  int p1, count, num_who;
  int sort_type = blitz_rat;

  if (param[0].type == TYPE_WORD) {
    len = strlen(param[0].val.word);
    for (i = 0; i < len; i++) {
      c = param[0].val.word[i];
      if (isdigit(c)) {
	if (i == 0 || !isdigit(param[0].val.word[i - 1])) {
	  tmpI = c - '0';
	  if (tmpI == 1) {
	    start_perc = 0.0;
	    stop_perc = 0.333333;
	  } else if (tmpI == 2) {
	    start_perc = 0.333333;
	    stop_perc = 0.6666667;
	  } else if (tmpI == 3) {
	    start_perc = 0.6666667;
	    stop_perc = 1.0;
	  } else if ((i == len - 1) || (!isdigit(param[0].val.word[i + 1])))
	    return COM_BADPARAMETERS;
	} else {
	  tmpI = c - '0';
	  tmpJ = param[0].val.word[i - 1] - '0';
	  if (tmpI == 0)
	    return COM_BADPARAMETERS;
	  if (tmpJ > tmpI)
	    return COM_BADPARAMETERS;
	  start_perc = ((float) tmpJ - 1.0) / (float) tmpI;
	  stop_perc = ((float) tmpJ) / (float) tmpI;
	}
      } else {
	switch (c) {
	case 'o':
	  if (sel_bits == 0xff)
	    sel_bits = WHO_OPEN;
	  else
	    sel_bits |= WHO_OPEN;
	  break;
	case 'r':
	  if (sel_bits == 0xff)
	    sel_bits = WHO_RATED;
	  else
	    sel_bits |= WHO_RATED;
	  break;
	case 'f':
	  if (sel_bits == 0xff)
	    sel_bits = WHO_FREE;
	  else
	    sel_bits |= WHO_FREE;
	  break;
	case 'a':
	  if (sel_bits == 0xff)
	    sel_bits = WHO_FREE | WHO_OPEN;
	  else
	    sel_bits |= (WHO_FREE | WHO_OPEN);
	  break;
	case 'R':
	  if (sel_bits == 0xff)
	    sel_bits = WHO_REGISTERED;
	  else
	    sel_bits |= WHO_REGISTERED;
	  break;
	case 'l':		/* Sort order */
	  cmp_func = alpha_cmp;
	  sort_type = none;
	  break;
	case 'A':		/* Sort order */
	  cmp_func = alpha_cmp;
	  break;
	case 'w':		/* Sort order */
	  cmp_func = wild_cmp;
	  sort_type = wild_rat;
	  break;
	case 's':		/* Sort order */
	  cmp_func = stand_cmp;
	  sort_type = std_rat;
	  break;
	case 'b':		/* Sort order */
	  cmp_func = blitz_cmp;
	  sort_type = blitz_rat;
	  break;
	case 't':		/* format */
	  style = 0;
	  break;
	case 'v':		/* format */
	  style = 1;
	  break;
	case 'n':		/* format */
	  style = 2;
	  break;
	case 'U':
	  if (sel_bits == 0xff)
	    sel_bits = WHO_UNREGISTERED;
	  else
	    sel_bits |= WHO_UNREGISTERED;
	  break;
	default:
	  return COM_BADPARAMETERS;
	  break;
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
  startpoint = floor((float) count * start_perc);
  stoppoint = ceil((float) count * stop_perc) - 1;
  num_who = 0;
  count = 0;
  for (p1 = 0; p1 < p_num; p1++) {
    if (!who_ok(sortlist[p1], sel_bits))
      continue;
    if ((count >= startpoint) && (count <= stoppoint)) {
      plist[num_who++] = sortlist[p1];
    }
    count++;
  }
  if (num_who == 0) {
    pprintf(p, "No logged in players match the flags in your who request.\n");
    return COM_OK;
  }
  switch (style) {
  case 0:			/* terse */
    who_terse(p, num_who, plist, sort_type);
    break;
  case 1:			/* verbose */
    who_verbose(p, num_who, plist);
    break;
  case 2:			/* win-loss */
    who_winloss(p, num_who, plist);
    break;
  default:
    return COM_BADPARAMETERS;
    break;
  }
  return COM_OK;
}




PRIVATE int notorcen(int p, param_list param, int *num, int max,
		      char **list, char *listname)
{
  int i, p1, connected;

  if (param[0].type != TYPE_WORD) {
    if (!*num) {
      pprintf(p, "Your %s list is empty.\n", listname);
      return COM_OK;
    } else
      pprintf(p, "-- Your %s list contains %d names: --", listname, *num);
    /* New code to print names in columns */
    {
      multicol *m = multicol_start(MAX_NOTIFY + 1);
      for (i = 0; i < *num; i++)
	multicol_store_sorted(m, list[i]);
      multicol_pprint(m, p, 78, 2);
      multicol_end(m);
    }
    return COM_OK;
  }
  if (*num >= max) {
    pprintf(p, "Sorry, your %s list is already full.\n", listname);
    return COM_OK;
  }
  if (!FindPlayer(p, &param[0], &p1, &connected))
    return COM_OK;

  for (i = 0; i < *num; i++) {
    if (!strcasecmp(list[i], param[0].val.word)) {
      pprintf(p, "Your %s list already includes %s.\n",
	      listname, parray[p1].name);
      if (!connected)
	player_remove(p1);
      return COM_OK;
    }
  }
  if (p1 == p) {
    pprintf(p, "You can't %s yourself.\n", listname);
    return COM_OK;
  }
  list[*num] = strdup(parray[p1].name);
  ++(*num);
  pprintf(p, "%s is now on your %s list.\n", parray[p1].name, listname);
  if (!connected)
    player_remove(p1);
  return COM_OK;
}


PRIVATE int unnotorcen(int p, param_list param, int *num, int max,
		        char **list, char *listname)
{
  char *pname = NULL;
  int i, j;
  int unc = 0;

  if (param[0].type == TYPE_WORD) {
    pname = param[0].val.word;
  }
  for (i = 0; i < *num; i++) {
    if (!pname || !strcasecmp(pname, list[i])) {
      pprintf(p, "%s is removed from your %s list.\n",
	      list[i], listname);
      rfree(list[i]);
      list[i] = NULL;
      unc++;
    }
  }
  if (unc) {
    i = 0;
    j = 0;
    while (j < *num) {
      if (list[j] != NULL) {
	list[i++] = list[j];
      }
      j++;
    }
    while (i < j) {
      list[i++] = NULL;
    }
    (*num) -= unc;
  } else {
    pprintf(p, "No one was removed from your %s list.\n", listname);
  }
  return COM_OK;
}


PUBLIC int com_notify(int p, param_list param)
{
  return notorcen(p, param, &parray[p].num_notify, MAX_NOTIFY,
		  parray[p].notifyList, "notify");
}

PUBLIC int com_censor(int p, param_list param)
{
  return notorcen(p, param, &parray[p].num_censor, MAX_CENSOR,
		  parray[p].censorList, "censor");
}

PUBLIC int com_unnotify(int p, param_list param)
{
  return unnotorcen(p, param, &parray[p].num_notify, MAX_NOTIFY,
		    parray[p].notifyList, "notify");
}

PUBLIC int com_uncensor(int p, param_list param)
{
  return unnotorcen(p, param, &parray[p].num_censor, MAX_CENSOR,
		    parray[p].censorList, "censor");
}


PUBLIC int com_channel(int p, param_list param)
{
  int i, err;

  if (param[0].type == TYPE_NULL) {	/* Turn off all channels */
    for (i = 0; i < MAX_CHANNELS; i++) {
      if (!channel_remove(i, p))
	pprintf(p, "Channel %d turned off.\n", i);
    }
  } else {
    i = param[0].val.integer;
    if ((i == 0) && (parray[p].adminLevel == 0)) {
      pprintf(p, "Only admins may join channel 0.\n");
      return COM_OK;
    }
    if (i < 0) {
      pprintf(p, "The lowest channel number is 0.\n");
      return COM_OK;
    }
    if (i >= MAX_CHANNELS) {
      pprintf(p, "The maximum channel number is %d.\n", MAX_CHANNELS - 1);
      return COM_OK;
    }
    if (on_channel(i, p)) {
      if (!channel_remove(i, p))
	pprintf(p, "Channel %d turned off.\n", i);
    } else {
      if (!(err = channel_add(i, p)))
	pprintf(p, "Channel %d turned on.\n", i);
      else {
	if (err == 1)
	  pprintf(p, "Channel %d is already full.\n", i);
	if (err == 2)
	  pprintf(p, "Maximum channel number exceeded.\n");
      }
    }
  }
  return COM_OK;
}

PUBLIC int com_inchannel(int p, param_list param)
{
  int c1, c2;
  int i, j, count = 0;

  if (param[0].type == TYPE_NULL) {	/* List everyone on every channel */
    c1 = -1;
    c2 = -1;
  } else if (param[1].type == TYPE_NULL) {	/* One parameter */
    c1 = param[0].val.integer;
    if (c1 < 0) {
      pprintf(p, "The lowest channel number is 0.\n");
      return COM_OK;
    }
    c2 = -1;
  } else {			/* Two parameters */
    c1 = param[0].val.integer;
    c2 = param[2].val.integer;
    if ((c1 < 0) || (c2 < 0)) {
      pprintf(p, "The lowest channel number is 0.\n");
      return COM_OK;
    }
    pprintf(p, "Two parameter inchannel is not implemented.\n");
    return COM_OK;
  }
  if ((c1 >= MAX_CHANNELS) || (c2 >= MAX_CHANNELS)) {
    pprintf(p, "The maximum channel number is %d.\n", MAX_CHANNELS - 1);
    return COM_OK;
  }
  for (i = 0; i < MAX_CHANNELS; i++) {
    if (numOn[i] && ((c1 < 0) || (i == c1))) {
      pprintf(p, "Channel %d:", i);
      for (j = 0; j < numOn[i]; j++) {
	pprintf(p, " %s", parray[channels[i][j]].name);
      }
      count++;
      pprintf(p, "\n");
    }
  }
  if (!count) {
    if (c1 < 0)
      pprintf(p, "No channels in use.\n");
    else
      pprintf(p, "Channel not in use.\n");
  }
  return COM_OK;
}


PUBLIC int create_new_match(int white_player, int black_player,
			     int wt, int winc, int bt, int binc,
			     int rated, char *category, char *board,
			     int white)
{
  int g = game_new(), p;
  char outStr[1024];
  int reverse = 0;

  if (g < 0)
    return COM_FAILED;
  if (white == 0) {
    reverse = 1;
  } else if (white == -1) {
    if (!bt) {
      if (parray[white_player].lastColor == parray[black_player].lastColor) {
	if ((parray[white_player].num_white - parray[white_player].num_black) >
	  (parray[black_player].num_white - parray[black_player].num_black))
	  reverse = 1;
      } else if (parray[white_player].lastColor == WHITE)
	reverse = 1;
    } else
      reverse = 1;		/* Challenger is always white in unbalanced
				   match */
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

  wt = wt * 60;			/* To Seconds */
  bt = bt * 60;
  garray[g].white = white_player;
  garray[g].black = black_player;
  strcpy(garray[g].white_name, parray[white_player].name);
  strcpy(garray[g].black_name, parray[black_player].name);
  garray[g].status = GAME_ACTIVE;
  garray[g].type = game_isblitz(wt / 60, winc, bt / 60, binc, category, board);
  if ((garray[g].type == TYPE_UNTIMED) || (garray[g].type == TYPE_NONSTANDARD))
    garray[g].rated = 0;
  else
    garray[g].rated = rated;
  garray[g].private = parray[white_player].private ||
    parray[black_player].private;
  garray[g].white = white_player;
  if (garray[g].type == TYPE_BLITZ) {
    garray[g].white_rating = parray[white_player].b_stats.rating;
    garray[g].black_rating = parray[black_player].b_stats.rating;
  } else if (garray[g].type == TYPE_WILD) {
    garray[g].white_rating = parray[white_player].w_stats.rating;
    garray[g].black_rating = parray[black_player].w_stats.rating;
  } else {    
    garray[g].white_rating = parray[white_player].s_stats.rating;
    garray[g].black_rating = parray[black_player].s_stats.rating;
  }
  if (board_init(&garray[g].game_state, category, board)) {
    pprintf(white_player, "PROBLEM LOADING BOARD. Game Aborted.\n");
    pprintf(black_player, "PROBLEM LOADING BOARD. Game Aborted.\n");
    fprintf(stderr, "FICS: PROBLEM LOADING BOARD %s %s. Game Aborted.\n",
	    category, board);
  }
  garray[g].game_state.gameNum = g;
  garray[g].wTime = wt * 10;
  garray[g].wInitTime = wt * 10;
  garray[g].wIncrement = winc * 10;
  if (bt == 0) {
    garray[g].bTime = wt * 10;
  } else {
    garray[g].bTime = bt * 10;
  }
  garray[g].bInitTime = bt * 10;
  garray[g].bIncrement = binc * 10;
  if (garray[g].game_state.onMove == BLACK) {	/* Start with black */
    garray[g].numHalfMoves = 1;
    garray[g].moveListSize = 1;
    garray[g].moveList = (move_t *) rmalloc(sizeof(move_t));
    garray[g].moveList[0].fromFile = -1;
    garray[g].moveList[0].fromRank = -1;
    garray[g].moveList[0].toFile = -1;
    garray[g].moveList[0].toRank = -1;
    garray[g].moveList[0].color = WHITE;
    strcpy(garray[g].moveList[0].moveString, "NONE");
    strcpy(garray[g].moveList[0].algString, "NONE");
  } else {
    garray[g].numHalfMoves = 0;
    garray[g].moveListSize = 0;
    garray[g].moveList = NULL;
  }
  garray[g].timeOfStart = tenth_secs();
  garray[g].startTime = tenth_secs();
  garray[g].lastMoveTime = garray[g].startTime;
  garray[g].lastDecTime = garray[g].startTime;
  garray[g].clockStopped = 0;
  sprintf(outStr, "\n{Game %d (%s vs. %s) Creating %s %s match.}\n",
	  g + 1, parray[white_player].name,
	  parray[black_player].name,
	  rstr[garray[g].rated],
	  bstr[garray[g].type]);
  pprintf(white_player, "%s", outStr);
  pprintf(black_player, "%s", outStr);

  for (p = 0; p < p_num; p++) {
    if ((p == white_player) || (p == black_player))
      continue;
    if (parray[p].status != PLAYER_PROMPT)
      continue;
    if (!parray[p].i_game)
      continue;
    pprintf_prompt(p, "%s", outStr);
  }
  parray[white_player].game = g;
  parray[white_player].opponent = black_player;
  parray[white_player].side = WHITE;
  parray[white_player].promote = QUEEN;
  parray[black_player].game = g;
  parray[black_player].opponent = white_player;
  parray[black_player].side = BLACK;
  parray[black_player].promote = QUEEN;
  send_boards(g);

  strcpy(garray[g].boardList[garray[g].numHalfMoves], boardToFEN(g));

  return COM_OK;
}

PRIVATE int accept_match(int p, int p1)
{
  int g, adjourned, foo;
  int wt, winc, bt, binc, rated, white;
  char *category, *board;
  pending *pend;
  char tmp[100];

  unobserveAll(p);		/* stop observing when match starts */
  unobserveAll(p1);

  pend = &parray[p].p_from_list[player_find_pendfrom(p, p1, PEND_MATCH)];
  wt = pend->param1;
  winc = pend->param2;
  bt = pend->param3;
  binc = pend->param4;
  rated = pend->param5;
  category = pend->char1;
  board = pend->char2;
  white = (pend->param6 == -1) ? -1 : 1 - pend->param6;

  pprintf(p, "You accept the challenge of %s.\n", parray[p1].name);
  pprintf(p1, "\n%s accepts your challenge.\n", parray[p].name);
  player_remove_request(p, p1, -1);
  player_remove_request(p1, p, -1);

  while ((foo = player_find_pendto(p, -1, -1)) != -1) {
    foo = parray[p].p_to_list[foo].whoto;
    pprintf_prompt(foo, "\n%s, who was challenging you, has joined a match with %s.\n", parray[p].name, parray[p1].name);
    pprintf(p, "Challenge to %s withdrawn.\n", parray[foo].name);
    player_remove_request(p, foo, -1);
  }

  while ((foo = player_find_pendto(p1, -1, -1)) != -1) {
    foo = parray[p1].p_to_list[foo].whoto;
    pprintf_prompt(foo, "\n%s, who was challenging you, has joined a match with %s.\n", parray[p1].name, parray[p].name);
    pprintf(p1, "Challenge to %s withdrawn.\n", parray[foo].name);
    player_remove_request(p1, foo, -1);
  }

  while ((foo = player_find_pendfrom(p, -1, -1)) != -1) {
    foo = parray[p].p_from_list[foo].whofrom;
    pprintf_prompt(foo, "\n%s, whom you were challenging, has joined a match with %s.\n", parray[p].name, parray[p1].name);
    pprintf(p, "Challenge from %s removed.\n", parray[foo].name);
    player_remove_request(foo, p, -1);
  }

  while ((foo = player_find_pendfrom(p1, -1, -1)) != -1) {
    foo = parray[p1].p_from_list[foo].whofrom;
    pprintf_prompt(foo, "\n%s, whom you were challenging, has joined a match with %s.\n", parray[p1].name, parray[p].name);
    pprintf(p1, "Challenge from %s removed.\n", parray[foo].name);
    player_remove_request(foo, p1, -1);
  }

  g = game_new();
  adjourned = 0;
  if (game_read(g, p, p1) >= 0)
    adjourned = 1;
  else if (game_read(g, p1, p) >= 0) {
    int swap;
    adjourned = 1;
    swap = p;
    p = p1;
    p1 = swap;
  }
  if (!adjourned) {		/* no adjourned game, so begin a new game */
    game_remove(g);

    if (create_new_match(p, p1, wt, winc, bt, binc, rated, category, board, white) != COM_OK) {
      sprintf(tmp, "There was a problem creating the new match.\n");
      pprintf(p, tmp);
      pprintf_prompt(p1, tmp);
    }
  } else {			/* resume adjourned game */
    game_delete(p, p1);

    sprintf(tmp, "{Game %d (%s vs. %s) Continuing %s %s match.}\n", g + 1, parray[p].name, parray[p1].name, rstr[garray[g].rated], bstr[garray[g].type]);
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
  return COM_OK;
}

PUBLIC int com_match(int p, param_list param)
{
  int adjourned;		/* adjourned game? */
  int g;			/* more adjourned game junk */
  int p1;
  int pendfrom, pendto;
  int ppend, p1pend;
  int wt = -1;			/* white start time */
  int winc = -1;		/* white increment */
  int bt = -1;			/* black start time */
  int binc = -1;		/* black increment */
  int rated = -1;		/* 1 = rated, 0 = unrated */
  int white = -1;		/* 1 = want white, 0 = want black */
  char category[100], board[100], parsebuf[100];
  char strFormula[MAX_STRING_LENGTH];
  char *val;
  int type;
  int confused = 0;
  char *colorstr[] = {"", "[black] ", "[white] "};
  char *adjustr[] = {"", " (adjourned)"};

  if ((parray[p].game >= 0) && (garray[parray[p].game].status == GAME_EXAMINE)) {
    pprintf(p, "You can't challenge while you are examining a game.\n");
    return COM_OK;
  }
  if (parray[p].game >= 0) {
    pprintf(p, "You can't challenge while you are playing a game.\n");
    return COM_OK;
  }
  stolower(param[0].val.word);
  p1 = player_find_part_login(param[0].val.word);
  if (p1 < 0) {
    pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
    return COM_OK;
  }

  if (p1 == p) {  /*  Allowing to match yourself to enter analysis mode */  
    pprintf(p, "Starting a game in examine mode.\n");
    {
      int g = game_new();

      unobserveAll(p);

      player_decline_offers(p, -1, PEND_MATCH);
      player_withdraw_offers(p, -1, PEND_MATCH);
      player_withdraw_offers(p, -1, PEND_SIMUL);

      garray[g].wInitTime = garray[g].wIncrement = 0;
      garray[g].bInitTime = garray[g].bIncrement = 0;
      garray[g].timeOfStart = tenth_secs();
      garray[g].wTime = garray[g].bTime = 0;
      garray[g].rated = 0;
      garray[g].clockStopped = 0;
      garray[g].type = TYPE_UNTIMED;
      garray[g].white = garray[g].black = p;
      garray[g].status = GAME_EXAMINE;
      garray[g].startTime = tenth_secs();
      garray[g].lastMoveTime = garray[g].startTime;
      garray[g].lastDecTime = garray[g].startTime;

      parray[p].side = WHITE; /* oh well... */
      parray[p].game = g;

      category[0]='\0'; board[0]='\0';
      if (board_init(&garray[g].game_state, category, board)) {
        pprintf(p, "PROBLEM LOADING BOARD. Game Aborted.\n");
        fprintf(stderr, "FICS: PROBLEM LOADING BOARD %s %s. Game Aborted.\n",
	    category, board);
        }
      garray[g].game_state.gameNum = g;
      strcpy(garray[g].white_name, parray[p].name);
      strcpy(garray[g].black_name, parray[p].name);
      garray[g].white_rating = garray[g].black_rating = parray[p].s_stats.rating;

      send_boards(g);

      strcpy(garray[g].boardList[garray[g].numHalfMoves], boardToFEN(g));

    }
    return COM_OK;
  }

  if (parray[p].open == 0) {
    parray[p].open = 1;
    pprintf(p, "Setting you open for matches.\n");
  }
  if (player_censored(p1, p)) {
    pprintf(p, "Player \"%s\" is censoring you.\n", parray[p1].name);
    return COM_OK;
  }
  if (player_censored(p, p1)) {
    pprintf(p, "You are censoring \"%s\".\n", parray[p1].name);
    return COM_OK;
  }
  if (!parray[p1].open) {
    pprintf(p, "Player \"%s\" is not open to match requests.\n", parray[p1].name);
    return COM_OK;
  }
  if (parray[p1].game >= 0) {
    pprintf(p, "Player \"%s\" is involved in another game.\n", parray[p1].name);
    return COM_OK;
  }
/* look for an adjourned game between p and p1 */
  g = game_new();
  adjourned = ((game_read(g, p, p1) < 0) && (game_read(g, p1, p) < 0)) ? 0 : 1;
  if (adjourned) {
    type = garray[g].type;
    wt = garray[g].wInitTime / 600;
    bt = garray[g].bInitTime / 600;
    winc = garray[g].wIncrement / 10;
    binc = garray[g].bIncrement / 10;
    rated = garray[g].rated;
  }
  game_remove(g);

  pendto = player_find_pendto(p, p1, PEND_MATCH);
  pendfrom = player_find_pendfrom(p, p1, PEND_MATCH);
  category[0] = '\0';
  board[0] = '\0';

  if (!adjourned) {
    if (param[1].type != TYPE_NULL) {
      int numba;		/* temp for atoi() */

      val = param[1].val.string;
      while (!confused && (sscanf(val, " %99s", parsebuf) == 1)) {
	val = eatword(eatwhite(val));
	if ((category[0] != '\0') && (board[0] == '\0'))
	  strcpy(board, parsebuf);
	else if (isdigit(*parsebuf)) {
	  if ((numba = atoi(parsebuf)) < 0) {
	    pprintf(p, "You can't specify negative time controls.\n");
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
	} else if (category[0] == '\0')
	  strcpy(category, parsebuf);
	else
	  confused = 1;
      }
      if (confused) {
	pprintf(p, "Can't interpret %s in match command.\n", parsebuf);
	return COM_OK;
      }
    }
    rated = ((rated == -1) ? parray[p].rated : rated) && parray[p1].registered && parray[p].registered;
    if (winc == -1)
      winc = (wt == -1) ? parray[p].d_inc : 0;	/* match 5 == match 5 0 */
    if (wt == -1)
      wt = parray[p].d_time;
    if (bt == -1)
      bt = 0;
    if (binc == -1)
      binc = winc;

    if (category[0] && !board[0]) {
      pprintf(p, "You must specify a board and a category.\n");
      return COM_OK;
    }
    if (category[0]) {
      char fname[MAX_FILENAME_SIZE];

      sprintf(fname, "%s/%s/%s", board_dir, category, board);
      if (!file_exists(fname)) {
	pprintf(p, "No such category/board: %s/%s\n", category, board);
	return COM_OK;
      }
    }
    if ((pendfrom < 0) && (parray[p1].ropen == 0) && (rated != parray[p1].rated)) {
      pprintf(p, "%s only wants to play %s games.\n", parray[p1].name,
	      rstr[parray[p1].rated]);
      pprintf(p1, "Ignoring %srated match request from %s.\n",
	      (parray[p1].rated ? "un" : ""), parray[p].name);
      return COM_OK;
    }
    type = game_isblitz(wt, winc, bt, binc, category, board);
    if (rated && (type == TYPE_STAND || type == TYPE_BLITZ || type == TYPE_WILD)) {
      if (parray[p].network_player == parray[p1].network_player) {
	rated = 1;
      } else {
	pprintf(p, "Network vs. local player forced to not rated\n");
	rated = 0;
      }
    }
    if (rated && (type == TYPE_NONSTANDARD)) {
      pprintf(p, "Game is non-standard - reverting to unrated\n");
      rated = 0;
    }
    if (rated && (type == TYPE_UNTIMED)) {
      pprintf(p, "Game is untimed - reverting to unrated\n");
      rated = 0;
    }
    /* Now check formula. */
    if ((pendfrom < 0 || param[1].type != TYPE_NULL) &&
	!GameMatchesFormula(p, p1, wt, winc, bt, binc, rated, type, strFormula)) {
      pprintf(p, "Match request does not fit formula for %s:\n",
	      parray[p1].name);
      pprintf(p, "%s's formula: %s\n", parray[p1].name, parray[p1].formula);
      pprintf(p, "Evaluated: %s\n", strFormula);
      pprintf_prompt(p1, "Ignoring (formula):  %s (%d) %s (%d) %s.\n",
		     parray[p].name,
		     GetRating(&parray[p], type),
		     parray[p1].name,
		     GetRating(&parray[p1], type),
	    game_str(rated, wt * 60, winc, bt * 60, binc, category, board));
      return COM_OK;
    }
    /* Ok match offer will be made */

  }				/* adjourned games shouldn't have to worry
				   about that junk? */
  if (pendto >= 0) {
    pprintf(p, "Updating offer already made to \"%s\".\n", parray[p1].name);
  }
  if (pendfrom >= 0) {
    if (pendto >= 0) {
      pprintf(p, "Internal error\n");
      fprintf(stderr, "FICS: This shouldn't happen. You can't have a match pending from and to the same person.\n");
      return COM_OK;
    }
    if (adjourned || ((wt == parray[p].p_from_list[pendfrom].param1) &&
		      (winc == parray[p].p_from_list[pendfrom].param2) &&
		      (bt == parray[p].p_from_list[pendfrom].param3) &&
		      (binc == parray[p].p_from_list[pendfrom].param4) &&
		      (rated == parray[p].p_from_list[pendfrom].param5) &&
		      ((white == -1) || (white + parray[p].p_from_list[pendfrom].param6 == 1)) &&
	       (!strcmp(category, parray[p].p_from_list[pendfrom].char1)) &&
		 (!strcmp(board, parray[p].p_from_list[pendfrom].char2)))) {
      /* Identical match, should accept! */
      accept_match(p, p1);
      return COM_OK;
    } else {
      player_remove_pendfrom(p, p1, PEND_MATCH);
      player_remove_pendto(p1, p, PEND_MATCH);
    }
  }
  if (pendto < 0) {
    ppend = player_new_pendto(p);
    if (ppend < 0) {
      pprintf(p, "Sorry you can't have any more pending matches.\n");
      return COM_OK;
    }
    p1pend = player_new_pendfrom(p1);
    if (p1pend < 0) {
      pprintf(p, "Sorry %s can't have any more pending matches.\n", parray[p1].name);
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
  strcpy(parray[p].p_to_list[ppend].char1, category);
  strcpy(parray[p].p_to_list[ppend].char2, board);
  parray[p].p_to_list[ppend].type = PEND_MATCH;
  parray[p].p_to_list[ppend].whoto = p1;
  parray[p].p_to_list[ppend].whofrom = p;

  parray[p1].p_from_list[p1pend].param1 = wt;
  parray[p1].p_from_list[p1pend].param2 = winc;
  parray[p1].p_from_list[p1pend].param3 = bt;
  parray[p1].p_from_list[p1pend].param4 = binc;
  parray[p1].p_from_list[p1pend].param5 = rated;
  parray[p1].p_from_list[p1pend].param6 = white;
  strcpy(parray[p1].p_from_list[p1pend].char1, category);
  strcpy(parray[p1].p_from_list[p1pend].char2, board);
  parray[p1].p_from_list[p1pend].type = PEND_MATCH;
  parray[p1].p_from_list[p1pend].whoto = p1;
  parray[p1].p_from_list[p1pend].whofrom = p;

  if (pendfrom >= 0) {
    pprintf(p, "Declining offer from %s and offering new match parameters.\n", parray[p1].name);
    pprintf(p1, "\n%s declines your match offer a match with these parameters:", parray[p].name);
  }
  if (pendto >= 0) {
    pprintf(p, "Updating match request to: ");
    pprintf(p1, "\n%s updates the match request.\n", parray[p].name);
  } else {
    pprintf(p, "Issuing: ");
    pprintf(p1, "\n", parray[p].name);
  }

  pprintf(p, "%s (%s) %s", parray[p].name,
	  ratstrii(GetRating(&parray[p], type), parray[p].registered),
	  colorstr[white + 1]);
  pprintf_highlight(p, "%s", parray[p1].name);
  pprintf(p, " (%s) %s%s.\n",
	  ratstrii(GetRating(&parray[p1], type), parray[p1].registered),
	  game_str(rated, wt * 60, winc, bt * 60, binc, category, board),
	  adjustr[adjourned]);
  pprintf(p1, "Challenge: ");
  pprintf_highlight(p1, "%s", parray[p].name);
  pprintf(p1, " (%s) %s", ratstrii(GetRating(&parray[p], type), parray[p].registered), colorstr[white + 1]);
  pprintf(p1, "%s (%s) %s%s.\n", parray[p1].name,
	  ratstrii(GetRating(&parray[p1], type), parray[p1].registered),
	  game_str(rated, wt * 60, winc, bt * 60, binc, category, board),
	  adjustr[adjourned]);
  if (parray[p1].bell == 1)
    pprintf_noformat(p1, "\007");
  if (in_list("computer", parray[p].name)) {
    pprintf(p1, "--** %s is a ", parray[p].name);
    pprintf_highlight(p1, "computer");
    pprintf(p1, " **--\n");
  }
  if (in_list("computer", parray[p1].name)) {
    pprintf(p, "--** %s is a ", parray[p1].name);
    pprintf_highlight(p, "computer");
    pprintf(p, " **--\n");
  }
  if (in_list("abuser", parray[p].name)) {
    pprintf(p1, "--** %s is in the ", parray[p].name);
    pprintf_highlight(p1, "abuser");
    pprintf(p1, " list **--\n");
  }
  if (in_list("abuser", parray[p1].name)) {
    pprintf(p, "--** %s is in the ", parray[p1].name);
    pprintf_highlight(p, "abuser");
    pprintf(p, " list **--\n");
  }
  if (rated) {
    int win, draw, loss;
    double newsterr;

    rating_sterr_delta(p1, p, type, time(0), RESULT_WIN, &win, &newsterr);
    rating_sterr_delta(p1, p, type, time(0), RESULT_DRAW, &draw, &newsterr);
    rating_sterr_delta(p1, p, type, time(0), RESULT_LOSS, &loss, &newsterr);
    pprintf(p1, "Your %s rating will change:  Win: %s%d,  Draw: %s%d,  Loss: %s%d\n",
	    bstr[type],
	    (win >= 0) ? "+" : "", win,
	    (draw >= 0) ? "+" : "", draw,
	    (loss >= 0) ? "+" : "", loss);
    pprintf(p1, "Your new RD will be %5.1f\n", newsterr);

    rating_sterr_delta(p, p1, type, time(0), RESULT_WIN, &win, &newsterr);
    rating_sterr_delta(p, p1, type, time(0), RESULT_DRAW, &draw, &newsterr);
    rating_sterr_delta(p, p1, type, time(0), RESULT_LOSS, &loss, &newsterr);
    pprintf(p, "Your %s rating will change:  Win: %s%d,  Draw: %s%d,  Loss: %s%d\n",
	    bstr[type],
	    (win >= 0) ? "+" : "", win,
	    (draw >= 0) ? "+" : "", draw,
	    (loss >= 0) ? "+" : "", loss);
    pprintf(p, "Your new RD will be %5.1f\n", newsterr);
  }
  pprintf_prompt(p1, "You can \"accept\" or \"decline\", or propose different parameters.\n");
  return COM_OK;
}

PUBLIC int com_accept(int p, param_list param)
{
  int acceptNum = -1;
  int type = -1;
  int p1;

  if (parray[p].num_from == 0) {
    pprintf(p, "You have no offers to accept.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    if (parray[p].num_from != 1) {
      pprintf(p, "You have more than one offer to accept.\nUse \"pending\" to see them and \"accept n\" to choose which one.\n");
      return COM_OK;
    }
    acceptNum = 0;
  } else if (param[0].type == TYPE_INT) {
    if ((param[0].val.integer < 1) || (param[0].val.integer > parray[p].num_from)) {
      pprintf(p, "Out of range. Use \"pending\" to see the list of offers.\n");
      return COM_OK;
    }
    acceptNum = param[0].val.integer - 1;
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
    }
    if (!strcmp(param[0].val.word, "simmatch")) {
      type = PEND_SIMUL;
    }
    if (!strcmp(param[0].val.word, "switch")) {
      type = PEND_SWITCH;
    }
    if (!strcmp(param[0].val.word, "all")) {
      while (parray[p].num_from != 0) {
	pcommand(p, "accept 1");
      }
      return COM_OK;
    }
    if (type > 0) {
      if ((acceptNum = player_find_pendfrom(p, -1, type)) < 0) {
	pprintf(p, "There are no pending %s offers.\n", param[0].val.word);
	return COM_OK;
      }
    } else {			/* Word must be a name */
      p1 = player_find_part_login(param[0].val.word);
      if (p1 < 0) {
	pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
	return COM_OK;
      }
      if ((acceptNum = player_find_pendfrom(p, p1, -1)) < 0) {
	pprintf(p, "There are no pending offers from %s.\n", parray[p1].name);
	return COM_OK;
      }
    }
  }
  switch (parray[p].p_from_list[acceptNum].type) {
  case PEND_MATCH:
    accept_match(p, parray[p].p_from_list[acceptNum].whofrom);
    return (COM_OK);
    break;
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
    pcommand(p, "takeback %d", parray[p].p_from_list[acceptNum].param1);
    break;
  case PEND_SIMUL:
    pcommand(p, "simmatch %s",
	     parray[parray[p].p_from_list[acceptNum].whofrom].name);
    break;
  case PEND_SWITCH:
    pcommand(p, "switch");
    break;
  case PEND_ADJOURN:
    pcommand(p, "adjourn");
    break;
  }
  return COM_OK_NOPROMPT;
}



PUBLIC int com_decline(int p, param_list param)
{
  int declineNum;
  int p1 = -1, type = -1;
  int count;

  if (parray[p].num_from == 0) {
    pprintf(p, "You have no pending offers from other players.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    if (parray[p].num_from == 1) {
      p1 = parray[p].p_from_list[0].whofrom;
      type = parray[p].p_from_list[0].type;
    } else {
      pprintf(p, "You have more than one pending offer. Please specify which one\nyou wish to decline.\n'Pending' will give you the list.\n");
      return COM_OK;
    }
  } else {
    if (param[0].type == TYPE_WORD) {
      /* Draw adjourn match takeback abort or <name> */
      if (!strcmp(param[0].val.word, "match")) {
	type = PEND_MATCH;
      } else if (!strcmp(param[0].val.word, "draw")) {
	type = PEND_DRAW;
      } else if (!strcmp(param[0].val.word, "pause")) {
	type = PEND_PAUSE;
      } else if (!strcmp(param[0].val.word, "abort")) {
	type = PEND_ABORT;
      } else if (!strcmp(param[0].val.word, "takeback")) {
	type = PEND_TAKEBACK;
      } else if (!strcmp(param[0].val.word, "adjourn")) {
	type = PEND_ADJOURN;
      } else if (!strcmp(param[0].val.word, "switch")) {
	type = PEND_SWITCH;
      } else if (!strcmp(param[0].val.word, "simul")) {
	type = PEND_SIMUL;
      } else if (!strcmp(param[0].val.word, "all")) {
      } else {
	p1 = player_find_part_login(param[0].val.word);
	if (p1 < 0) {
	  pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
	  return COM_OK;
	}
      }
    } else {			/* Must be an integer */
      declineNum = param[0].val.integer - 1;
      if (declineNum >= parray[p].num_from || declineNum < 0) {
	pprintf(p, "Invalid offer number. Must be between 1 and %d.\n", parray[p].num_from);
	return COM_OK;
      }
      p1 = parray[p].p_from_list[declineNum].whofrom;
      type = parray[p].p_from_list[declineNum].type;
    }
  }
  count = player_decline_offers(p, p1, type);
  if (count != 1)
    pprintf(p, "%d offers declined\n", count);
  return COM_OK;
}

PUBLIC int com_withdraw(int p, param_list param)
{
  int withdrawNum;
  int p1 = -1, type = -1;
  int count;

  if (parray[p].num_to == 0) {
    pprintf(p, "You have no pending offers to other players.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    if (parray[p].num_to == 1) {
      p1 = parray[p].p_to_list[0].whoto;
      type = parray[p].p_to_list[0].type;
    } else {
      pprintf(p, "You have more than one pending offer. Please specify which one\nyou wish to withdraw.\n'Pending' will give you the list.\n");
      return COM_OK;
    }
  } else {
    if (param[0].type == TYPE_WORD) {
      /* Draw adjourn match takeback abort or <name> */
      if (!strcmp(param[0].val.word, "match")) {
	type = PEND_MATCH;
      } else if (!strcmp(param[0].val.word, "draw")) {
	type = PEND_DRAW;
      } else if (!strcmp(param[0].val.word, "pause")) {
	type = PEND_PAUSE;
      } else if (!strcmp(param[0].val.word, "abort")) {
	type = PEND_ABORT;
      } else if (!strcmp(param[0].val.word, "takeback")) {
	type = PEND_TAKEBACK;
      } else if (!strcmp(param[0].val.word, "adjourn")) {
	type = PEND_ADJOURN;
      } else if (!strcmp(param[0].val.word, "switch")) {
	type = PEND_SWITCH;
      } else if (!strcmp(param[0].val.word, "simul")) {
	type = PEND_SIMUL;
      } else if (!strcmp(param[0].val.word, "all")) {
      } else {
	p1 = player_find_part_login(param[0].val.word);
	if (p1 < 0) {
	  pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
	  return COM_OK;
	}
      }
    } else {			/* Must be an integer */
      withdrawNum = param[0].val.integer - 1;
      if (withdrawNum >= parray[p].num_to || withdrawNum < 0) {
	pprintf(p, "Invalid offer number. Must be between 1 and %d.\n", parray[p].num_to);
	return COM_OK;
      }
      p1 = parray[p].p_to_list[withdrawNum].whoto;
      type = parray[p].p_to_list[withdrawNum].type;
    }
  }
  count = player_withdraw_offers(p, p1, type);
  if (count != 1)
    pprintf(p, "%d offers withdrawn\n", count);
  return COM_OK;
}

PUBLIC int com_pending(int p, param_list param)
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
    pprintf(p, "\nThere are no offers pending FROM other players.\n");
  } else {
    pprintf(p, "\nOffers FROM other players:\n");
    for (i = 0; i < parray[p].num_from; i++) {
      pprintf(p, " %d: ", i + 1);
      player_pend_print(p, &parray[p].p_from_list[i]);
    }
    pprintf(p, "\nIf you wish to accept any of these offers type 'accept n'\nor just 'accept' if there is only one offer.\n");
  }
  return COM_OK;
}

PUBLIC int com_refresh(int p, param_list param)
{
  int g, p1;

  if (param[0].type == TYPE_NULL) {
    if (parray[p].game >= 0) {
      send_board_to(parray[p].game, p);
    } else {			/* Do observing in here */
      if (parray[p].num_observe) {
	for (g = 0; g < parray[p].num_observe; g++) {
	  send_board_to(parray[p].observe_list[g], p);
	}
      } else {
        pprintf(p, "You are neither playing nor observing a game.\n");
        return COM_OK;
      }
    }
  } else {
    g = GameNumFromParam (p, &p1, &param[0]);
    if (g < 0)
      return COM_OK;
    if ((g >= g_num) || ((garray[g].status != GAME_ACTIVE)
                        && (garray[g].status != GAME_EXAMINE))) {
      pprintf(p, "No such game.\n");
    } else if (garray[g].private && parray[p].adminLevel==ADMIN_USER) {
      pprintf (p, "Sorry, game %d is a private game.\n", g+1);
    } else {
      if (garray[g].private)
        pprintf(p, "Refreshing PRIVATE game %d\n", g+1);
      send_board_to(g, p);
    }
  }
  return COM_OK;
}

PUBLIC int com_open(int p, param_list param)
{
  int retval;
  ASSERT(param[0].type == TYPE_NULL);
  if ((retval = pcommand(p, "set open")) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

PUBLIC int com_simopen(int p, param_list param)
{
  int retval;
  ASSERT(param[0].type == TYPE_NULL);
  if ((retval = pcommand(p, "set simopen")) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

PUBLIC int com_bell(int p, param_list param)
{
  int retval;
  ASSERT(param[0].type == TYPE_NULL);
  if ((retval = pcommand(p, "set bell")) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

PUBLIC int com_flip(int p, param_list param)
{
  int retval;
  ASSERT(param[0].type == TYPE_NULL);
  if ((retval = pcommand(p, "set flip")) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

PUBLIC int com_highlight(int p, param_list param)
{
  pprintf(p, "Obsolete command. Please do set highlight <0-15>.\n");
  return COM_OK;
}

PUBLIC int com_style(int p, param_list param)
{
  int retval;
  ASSERT(param[0].type == TYPE_INT);
  if ((retval = pcommand(p, "set style %d", param[0].val.integer)) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

PUBLIC int com_promote(int p, param_list param)
{
  int retval;
  ASSERT(param[0].type == TYPE_WORD);
  if ((retval = pcommand(p, "set promote %s", param[0].val.word)) != COM_OK)
    return retval;
  else
    return COM_OK_NOPROMPT;
}

PUBLIC int com_alias(int p, param_list param)
{
  int al;

  if (param[0].type == TYPE_NULL) {
    for (al = 0; al < parray[p].numAlias; al++) {
      pprintf(p, "%s -> %s\n", parray[p].alias_list[al].comm_name,
	      parray[p].alias_list[al].alias);
    }
    return COM_OK;
  }
  al = alias_lookup(param[0].val.word, parray[p].alias_list, parray[p].numAlias);
  if (param[1].type == TYPE_NULL) {
    if (al < 0) {
      pprintf(p, "You have no alias named '%s'.\n", param[0].val.word);
    } else {
      pprintf(p, "%s -> %s\n", parray[p].alias_list[al].comm_name,
	      parray[p].alias_list[al].alias);
    }
  } else {
    if (al < 0) {
      if (parray[p].numAlias >= MAX_ALIASES - 1) {
	pprintf(p, "You have your maximum of %d aliases.\n", MAX_ALIASES - 1);
      } else {

	if (!strcmp(param[0].val.string, "quit")) {	/* making sure they
							   can't alias quit */
	  pprintf(p, "You can't alias this command.\n");
	} else if (!strcmp(param[0].val.string, "unalias")) {	/* making sure they
								   can't alias unalias
								   :) */
	  pprintf(p, "You can't alias this command.\n");
	} else {
	  parray[p].alias_list[parray[p].numAlias].comm_name =
	    strdup(param[0].val.word);
	  parray[p].alias_list[parray[p].numAlias].alias =
	    strdup(param[1].val.string);
	  parray[p].numAlias++;
	  pprintf(p, "Alias set.\n");

	}
      }
    } else {
      rfree(parray[p].alias_list[al].alias);
      parray[p].alias_list[al].alias = strdup(param[1].val.string);
      pprintf(p, "Alias replaced.\n");
    }
    parray[p].alias_list[parray[p].numAlias].comm_name = NULL;
  }
  return COM_OK;
}

PUBLIC int com_unalias(int p, param_list param)
{
  int al;
  int i;

  ASSERT(param[0].type == TYPE_WORD);
  al = alias_lookup(param[0].val.word, parray[p].alias_list, parray[p].numAlias);
  if (al < 0) {
    pprintf(p, "You have no alias named '%s'.\n", param[0].val.word);
  } else {
    rfree(parray[p].alias_list[al].comm_name);
    rfree(parray[p].alias_list[al].alias);
    for (i = al; i < parray[p].numAlias; i++) {
      parray[p].alias_list[i].comm_name = parray[p].alias_list[i + 1].comm_name;
      parray[p].alias_list[i].alias = parray[p].alias_list[i + 1].alias;
    }
    parray[p].numAlias--;
    parray[p].alias_list[parray[p].numAlias].comm_name = NULL;
    pprintf(p, "Alias removed.\n");
  }
  return COM_OK;
}

PUBLIC int com_servers(int p, param_list param)
{
/*
  int i;

  ASSERT(param[0].type == TYPE_NULL);
  if (numServers == 0) {
         */ pprintf(p, "There are no other servers known to this server.\n");
  return COM_OK;
}
 /* pprintf(p, "There are %d known servers.\n", numServers); pprintf(p, "(Not
    all of these may be active)\n"); pprintf(p, "%-30s%-7s\n", "HOST",
    "PORT"); for (i = 0; i < numServers; i++) pprintf(p, "%-30s%-7d\n",
    serverNames[i], serverPorts[i]); return COM_OK; } */
PUBLIC int com_sendmessage(int p, param_list param)
{
  int p1, connected = 1;

  if (!parray[p].registered) {
    pprintf(p, "You are not registered and cannot send messages.\n");
    return COM_OK;
  }
  if ((param[0].type == TYPE_NULL) || (param[1].type == TYPE_NULL)) {
    pprintf(p, "No message sent.\n");
    return COM_OK;
  }
  if (!FindPlayer(p, &param[0], &p1, &connected))
    return COM_OK;

  if ((player_censored(p1, p)) && (parray[p].adminLevel == 0)) {
    pprintf(p, "Player \"%s\" is censoring you.\n", parray[p1].name);
    return COM_OK;
  }
  if (player_add_message(p1, p, param[1].val.string)) {
    pprintf(p, "Couldn't send message to %s. Message buffer full.\n",
	    parray[p1].name);
  } else {
    if (connected)
      pprintf_prompt(p1, "\n%s just sent you a message.\n", parray[p].name);
  }
  if (!connected)
    player_remove(p1);
  return COM_OK;
}


PUBLIC int com_messages(int p, param_list param)
{
  if (param[0].type != TYPE_NULL) {
    if (param[1].type != TYPE_NULL)
      return com_sendmessage(p, param);
    else {
      ShowMsgsBySender(p, param);
      return COM_OK;
    }
  }
  if (player_num_messages(p) <= 0) {
    pprintf(p, "You have no messages.\n");
    return COM_OK;
  }
  pprintf(p, "Messages:\n");
  player_show_messages(p);
  return COM_OK;
}

PUBLIC int com_clearmessages(int p, param_list param)
{
  if (player_num_messages(p) <= 0) {
    pprintf(p, "You have no messages.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    pprintf(p, "Messages cleared.\n");
    player_clear_messages(p);
    return COM_OK;
  }
  ClearMsgsBySender(p, param);
  return COM_OK;
}

PUBLIC int com_help(int p, param_list param)
{
  int i;
  static char nullify = '\0';
  char *iwant, *filenames[1000];	/* enough for all helpfile names */

  if (param[0].type == TYPE_NULL) {
    iwant = &nullify;
  } else {
    iwant = param[0].val.word;
    if (!safestring(iwant)) {
      pprintf(p, "Illegal character in command %s.\n", iwant);
      return COM_OK;
#if 0
    } else {
      char sublist[][8] = {"talk", "play", ""};

      for (i=0; (sublist[i][0] != '\0') && strcmp(iwant, sublist[i]); i++);
      if (sublist[i][0] != '\0') {
        char searchdir[MAX_FILENAME_SIZE];
        pprintf(p, "The following \"%s\" commands are available.\n\n",
               sublist[i]);
        sprintf(searchdir, "%s/../%s_help", comhelp_dir, sublist[i]);
        i = search_directory(searchdir, NULL, filenames, 1000);
        display_directory(p, filenames, i);
        pprintf(p, "\nType \"help [command]\" for more specific help.\n");
        return COM_OK;
      }
#endif
    }
  }

  i = search_directory((*iwant) ? help_dir : comhelp_dir, iwant, filenames, 1000);
  if (i == 0) {
    pprintf(p, "No help available on \"%s\".\n", iwant);
  } else if ((i == 1) || !strcmp(*filenames, iwant)) {
    if (psend_file(p, help_dir, *filenames)) {
      /* we should never reach this unless the file was just deleted */
      pprintf(p, "Helpfile %s could not be found! ", *filenames);
      pprintf(p, "Please inform an admin of this. Thank you.\n");
    }
  } else {
    if (*iwant)
      pprintf(p, "Matches:");
    display_directory(p, filenames, i);
    pprintf(p, "[Type \"info\" for a list of FICS general information files.]\n");
  }
  return COM_OK;
}

PUBLIC int com_info(int p, param_list param)
{
  int n;
  char *filenames[1000];

  if ((n = search_directory(info_dir, NULL, filenames, 1000)) > 0)
    display_directory(p, filenames, n);
  return COM_OK;
}

PUBLIC int com_adhelp(int p, param_list param)
{
  int i;
  static char nullify = '\0';
  char *iwant, *filenames[1000];	/* enough for all helpfile names */

  if (param[0].type == TYPE_NULL) {
    iwant = &nullify;
  } else {
    iwant = param[0].val.word;
    if (!safestring(iwant)) {
      pprintf(p, "Illegal character in command %s.\n", iwant);
      return COM_OK;
    }
  }

  i = search_directory(adhelp_dir, iwant, filenames, 1000);
  if (i == 0) {
    pprintf(p, "No help available on \"%s\".\n", iwant);
  } else if ((i == 1) || !strcmp(*filenames, iwant)) {
    if (psend_file(p, adhelp_dir, *filenames)) {
      /* we should never reach this unless the file was just deleted */
      pprintf(p, "Helpfile %s could not be found! ", *filenames);
      pprintf(p, "Please inform an admin of this. Thank you.\n");
    }
  } else {
    if (*iwant)
      pprintf(p, "Matches:\n");
    display_directory(p, filenames, i);
  }
  return COM_OK;
}

PUBLIC int com_mailsource(int p, param_list param)
{
  static char nullify = '\0';
  char *iwant, *buffer[1000];
/* char command[MAX_FILENAME_SIZE]; */
  char subj[81], fname[MAX_FILENAME_SIZE];
  int count;

  if (!parray[p].registered) {
    pprintf(p, "Only registered people can use the mailsource command.\n");
    return COM_OK;
  }

  if (param[0].type == TYPE_NULL) {
    iwant = &nullify;
  } else {
    iwant = param[0].val.word;
  }

  count = search_directory(source_dir, iwant, buffer, 1000);
  if (count == 0) {
    pprintf(p, "Found no source file matching \"%s\".\n", iwant);
  } else if ((count == 1) || !strcmp(iwant, *buffer)) {
  /*  sprintf(command, "%s -s\"FICS sourcefile: %s\" %s < %s/%s&", MAILPROGRAM,
	*buffer, parray[p].emailAddress, source_dir, *buffer);
    system(command);  */

    sprintf(subj, "FICS source file from server %s: %s", fics_hostname, *buffer); 
    sprintf(fname, "%s/%s",source_dir, *buffer);
    mail_file_to_user (p, subj, fname);
    pprintf(p, "Source file %s sent to %s\n", *buffer, parray[p].emailAddress);
  } else {
    pprintf(p, "Found %d source files matching that:\n", count);
    if (*iwant)
      display_directory(p, buffer, count);
    else {		/* this junk is to get *.c *.h */
      multicol *m = multicol_start(count);
      char *s;
      int i;
      for (i=0; i < count; i++) {
        if (((s = buffer[i] + strlen(buffer[i]) - 2) >= buffer[i]) && (!strcmp(s, ".c") || !strcmp(s, ".h")))
	  multicol_store(m, buffer[i]);
      }
      multicol_pprint(m, p, 78, 1);
      multicol_end(m);
    }
  }
  return COM_OK;
}

PUBLIC int com_mailhelp(int p, param_list param)
{				/* Sparky  */
  /* FILE *fp; char tmp[MAX_LINE_SIZE]; char fname[MAX_FILENAME_SIZE]; */
  char command[MAX_FILENAME_SIZE];
  char subj[81], fname[MAX_FILENAME_SIZE];
  char *iwant, *buffer[1000];

  int count;
  static char nullify = '\0';

  if (!parray[p].registered) {
    pprintf(p, "Only registered people can use the mailhelp command.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL)
    iwant = &nullify;
  else
    iwant = param[0].val.word;

  count = search_directory(help_dir, iwant, buffer, 1000);
  if (count == 0) {
    pprintf(p, "Found no help file matching \"%s\".\n", iwant);
  } else if (count == 1) {
  /*  sprintf(command, "%s -s \"FICS helpfile: %s\" %s < %s/%s&", MAILPROGRAM,
	*buffer, parray[p].emailAddress, help_dir, *buffer);
    system(command);a */


    sprintf(subj, "FICS help file from server %s: %s", fics_hostname, *buffer);
    sprintf(fname, "%s/%s",help_dir, *buffer);
    mail_file_to_user (p, subj, fname);

    pprintf(p, "Help file %s sent to %s\n", *buffer, parray[p].emailAddress);
  } else {
    pprintf(p, "Found %d helpfiles matching that:\n", count);
    display_directory(p, buffer, count);
  }

  return COM_OK;
}

PUBLIC int com_mailmess(int p, param_list param)
{
  char command[MAX_FILENAME_SIZE];
  char *buffer[1000];
  char mdir[MAX_FILENAME_SIZE];
  char filename[MAX_FILENAME_SIZE];
  char subj[81], fname[MAX_FILENAME_SIZE];


  if (!parray[p].registered) {
    pprintf(p, "Only registered people can use the mailmess command.\n");
    return COM_OK;
  }
  sprintf(filename, "%s.messages", parray[p].login);
  sprintf(mdir, "%s/player_data/%c/", stats_dir, parray[p].login[0]);

  if (search_directory(mdir, filename, buffer, 1000)) {
/*    Sprintf(command, "%s -s \"Your FICS messages\" %s < %s%s&", MAILPROGRAM,
	    parray[p].emailAddress, mdir, filename);
    system(command);  */

    sprintf(subj, "Your FICS messages from server %s", fics_hostname);
    sprintf(fname, "%s/%s", mdir, filename);
    mail_file_to_user (p, subj, fname);
 


    pprintf(p, "Messages sent to %s\n", parray[p].emailAddress);
  } else {
    pprintf(p, "You have no messages.\n");
  }
  return COM_OK;

}

PUBLIC int com_handles(int p, param_list param)
{
  char *buffer[1000];
  char pdir[MAX_FILENAME_SIZE];
  int count;

  sprintf(pdir, "%s/%c", player_dir, param[0].val.word[0]);
  count = search_directory(pdir, param[0].val.word, buffer, 1000);
  pprintf(p, "Found %d names.\n", count);
  if (count > 0)
    display_directory(p, buffer, count);
  return COM_OK;
}

PUBLIC int com_znotify(int p, param_list param)
{
  int p1, count = 0;

  for (p1 = 0; p1 < p_num; p1++) {
    if (player_notified(p, p1)) {
      if (!count)
	pprintf(p, "Present company on your notify list:\n  ");
      pprintf(p, " %s", parray[p1].name);
      count++;
    }
  }
  if (count)
    pprintf(p, ".\n");
  else
    pprintf(p, "No one from your notify list is logged on.\n");

  count = 0;
  for (p1 = 0; p1 < p_num; p1++) {
    if (player_notified(p1, p)) {
      if (!count)
	pprintf(p,
		"The following players have you on their notify list:\n  ");
      pprintf(p, " %s", parray[p1].name);
      count++;
    }
  }
  if (count)
    pprintf(p, ".\n");
  else
    pprintf(p, "No one logged in has you on their notify list.\n");
  return COM_OK;
}

PUBLIC int com_qtell(int p, param_list param)
{
  int p1;
  char tmp[MAX_STRING_LENGTH];
  char dummy[2];
  char buffer1[MAX_STRING_LENGTH];	/* no highlight and no bell */
  char buffer2[MAX_STRING_LENGTH];	/* no highlight and bell */
  char buffer3[MAX_STRING_LENGTH];	/* highlight and no bell */
  char buffer4[MAX_STRING_LENGTH];	/* highlight and and bell */
  int i, count;
/*  FILE *fp; */

  if (!in_list("td", parray[p].name)) {
    pprintf(p, "Only TD programs are allowed to use this command.\n");
    return COM_OK;
  }
  strcpy(buffer1, ":\0");
  strcpy(buffer2, ":\0");
  strcpy(buffer3, ":\0");
  strcpy(buffer4, ":\0");

  sprintf(tmp, "%s", param[1].val.string);
  for (i = 0, count = 0; ((tmp[i] != '\0') && (count < 1029));) {
    if ((tmp[i] == '\\') && (tmp[i + 1] == 'n')) {
      strcat(buffer1, "\n:");
      strcat(buffer2, "\n:");
      strcat(buffer3, "\n:");
      strcat(buffer4, "\n:");
      count += 2;
      i += 2;
    } else if ((tmp[i] == '\\') && (tmp[i + 1] == 'b')) {
      strcat(buffer2, "\007");
      strcat(buffer4, "\007");
      count++;
      i += 2;
    } else if ((tmp[i] == '\\') && (tmp[i + 1] == 'H')) {
      strcat(buffer3, "\033[7m");
      strcat(buffer4, "\033[7m");
      count += 4;
      i += 2;
    } else if ((tmp[i] == '\\') && (tmp[i + 1] == 'h')) {
      strcat(buffer3, "\033[0m");
      strcat(buffer4, "\033[0m");
      count += 4;
      i += 2;
    } else {
      dummy[0] = tmp[i];
      dummy[1] = '\0';
      strcat(buffer1, dummy);
      strcat(buffer2, dummy);
      strcat(buffer3, dummy);
      strcat(buffer4, dummy);
      count++;
      i++;
    }
  }

  if (param[0].type == TYPE_WORD) {
/*
    fp = fopen("/tmp/fics-log", "a");
    fprintf(fp, "PLAYER \"%s\" - MESSAGE \"%s\"\n", param[0].val.word, param[1].val.string);
    fclose(fp);
*/
    if ((p1 = player_find_bylogin(param[0].val.word)) < 0) {
      pprintf(p, "*qtell %s 1*\n", param[0].val.word);
      return COM_OK;
    }
    pprintf_prompt(p1, "\n%s\n", (parray[p1].highlight && parray[p1].bell) ? buffer4 :
		   (parray[p1].highlight && !parray[p1].bell) ? buffer3 :
		   (!parray[p1].highlight && parray[p1].bell) ? buffer2 :
		   buffer1);
    pprintf(p, "*qtell %s 0*\n", parray[p1].name);

  } else {
    int p1;
    int i;
    int ch = param[0].val.integer;

/*
    fp = fopen("/tmp/fics-log", "a");
    fprintf(fp, "CHANNEL \"%d\" - MESSAGE \"%s\"\n", param[0].val.integer, param[1].val.string);
    fclose(fp);
*/

    if (ch == 0) {
      pprintf(p, "*qtell %d 1*\n", param[0].val.integer);
      return COM_OK;
    }
    if (ch < 0) {
      pprintf(p, "*qtell %d 1*\n", param[0].val.integer);
      return COM_OK;
    }
    if (ch >= MAX_CHANNELS) {
      pprintf(p, "*qtell %d 1*\n", param[0].val.integer);
      return COM_OK;
    }
    for (i = 0; i < numOn[ch]; i++) {
      p1 = channels[ch][i];
      if (p1 == p)
	continue;
      if (player_censored(p1, p))
	continue;
      if ((parray[p1].status == PLAYER_PASSWORD)
	  || (parray[p1].status == PLAYER_LOGIN))
	continue;
      pprintf_prompt(p1, "\n%s\n", (parray[p1].highlight && parray[p1].bell) ? buffer4 :
		     (parray[p1].highlight && !parray[p1].bell) ? buffer3 :
		     (!parray[p1].highlight && parray[p1].bell) ? buffer2 :
		     buffer1);
    }
    pprintf(p, "*qtell %d 0*\n", param[0].val.integer);
  }
  return COM_OK;
}

PUBLIC int com_getpi(int p, param_list param)
{
  int p1;

  if (!in_list("td", parray[p].name)) {
    pprintf(p, "Only TD programs are allowed to use this command.\n");
    return COM_OK;
  }
  if (((p1 = player_find_bylogin(param[0].val.word)) < 0) || (parray[p1].registered == 0)) {
    /* Darkside suggested not to return anything */
    return COM_OK;
  }
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

PUBLIC int com_getps(int p, param_list param)
{
  int p1;

  if ((((p1 = player_find_bylogin(param[0].val.word)) < 0) || (parray[p1].registered == 0)) || (parray[p1].game < 0)) {
    pprintf(p, "*status %s 1*\n", param[0].val.word);
    return COM_OK;
  }
  pprintf(p, "*status %s 0 %s*\n", parray[p1].name, parray[(parray[p1].opponent)].name);
  return COM_OK;
}
PUBLIC int com_limits(int p, param_list param)
{
  pprintf(p, "\nCurrent hardcoded limits:\n");
  pprintf(p, "  Max number of players: %d\n", MAX_PLAYER);
  pprintf(p, "  Max number of channels and max capacity: %d\n", MAX_CHANNELS);
  pprintf(p, "  Max number of channels one can be in: %d\n", MAX_INCHANNELS);
  pprintf(p, "  Max number of people on the notify list: %d\n", MAX_NOTIFY);
  pprintf(p, "  Max number of aliases: %d\n", MAX_ALIASES);
  pprintf(p, "  Max number of games you can observe at a time: %d\n", MAX_OBSERVE);
  pprintf(p, "  Max number of requests pending: %d\n", MAX_PENDING);
  pprintf(p, "  Max number of people on the censor list: %d\n", MAX_CENSOR);
  pprintf(p, "  Max number of people in a simul game: %d\n", MAX_SIMUL);
  pprintf(p, "  Max number of messages one can receive: %d\n", MAX_MESSAGES);
  pprintf(p, "  Min number of games to be active: %d\n", PROVISIONAL);
  pprintf(p, "\nAdmin settable limits:\n");
  pprintf(p, "  Quota list gives two shouts per %d seconds.\n", quota_time);
  return COM_OK;
}

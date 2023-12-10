/* talkproc.c
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
   name				yy/mm/dd	Change
   hersco and Marsalis         	95/07/24	Created
   Sparky                       95/10/05
                                Modifed tell function for the following
                                changes:  no longer informs you that someone
                                is censoring in tells to channels, whisper or
                                kibitz.  Titles now shown instead of ratings
                                in kibitz and whisper, admins on duty now are
                                shown as (*), and computers marked as (C) as
                                well as with rating.

*/

#include "stdinclude.h"

#include "common.h"
#include "talkproc.h"
#include "comproc.h"
#include "command.h"
#include "utils.h"
#include "ficsmain.h"
#include "config.h"
#include "playerdb.h"
#include "network.h"
#include "rmalloc.h"
#include "variable.h"
#include "gamedb.h"
#include "gameproc.h"
#include "obsproc.h"
#include "multicol.h"
#include "lists.h"
#include "formula.h"

#include <sys/resource.h>

/* grimm */
#if defined(SGI)
#else
/* int system(char *arg); */
#endif

int quota_time;

#if 0
PUBLIC int com_query(int p, param_list param)
{
  int p1;
  int count = 0;

  if (!parray[p].registered) {
    pprintf(p, "Only registered players can use the query command.\n");
    return COM_OK;
  }
  if (in_list(p, L_MUZZLE, parray[p].login)) {
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
#endif

/* hawk: hacked it to fit ALL persons - quota list is not needed anymore */
int CheckShoutQuota(int p)
{
  int timenow = time(0);
  int timeleft = 0;

  if (((timeleft = timenow - parray[p].lastshout_a) < quota_time) &&
      (parray[p].adminLevel == 0))  {
    return (quota_time - timeleft);
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
  if (in_list(p, L_MUZZLE, parray[p].login)) {
    pprintf(p, "You are muzzled.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    if ((timeleft = CheckShoutQuota(p))) {
      pprintf(p, "Next shout available in %d seconds.\n", timeleft);
    } else {
      pprintf(p, "Your next shout is ready for use.\n");
    }
    return COM_OK;
  }
  if ((timeleft = CheckShoutQuota(p))) {
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
  if ((timeleft = CheckShoutQuota(p))) {
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
  if (in_list(p, L_CMUZZLE, parray[p].login)) {
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
  if (in_list(p, L_MUZZLE, parray[p].login)) {
    pprintf(p, "You are muzzled.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    if ((timeleft = CheckShoutQuota(p))) {
      pprintf(p, "Next shout available in %d seconds.\n", timeleft);
    } else {
      pprintf(p, "Your next shout is ready for use.\n");
    }
    return COM_OK;
  }
  if ((timeleft = CheckShoutQuota(p))) {
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
  if ((timeleft = CheckShoutQuota(p))) {
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
  int rating;
  int rating1;

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
    if ((why != TELL_KIBITZ) || (why != TELL_WHISPER) || (why != TELL_CHANNEL))
    {
       pprintf(p, "Player \"%s\" is censoring you.\n", parray[p1].name);
    }
    return COM_OK;
  }
/*  in_push(IN_TELL); */
  switch (why) {
  case TELL_SAY:
    pprintf_highlight(p1, "\n%s", parray[p].name);
    pprintf_prompt(p1, " says: %s\n", msg);
    break;
  case TELL_WHISPER:
  case TELL_KIBITZ:
    rating = GetRating(&parray[p], TYPE_BLITZ);
    if (rating < ( rating1 = ( GetRating(&parray[p], TYPE_STAND ) ) ) )
      rating = rating1;
    if (in_list(p, L_FM, parray[p].name))
      pprintf(p1, "\n%s(FM)", parray[p].name);
    else if (in_list(p, L_IM, parray[p].name))
      pprintf(p1, "\n%s(IM)", parray[p].name);
    else if (in_list(p, L_GM, parray[p].name))
      pprintf(p1, "\n%s(GM)", parray[p].name);
    else if ((parray[p].adminLevel >= 10) && (parray[p].i_admin))
      pprintf(p1, "\n%s(*)", parray[p].name);
    else if ((rating >= parray[p1].kiblevel) ||
             ((parray[p].adminLevel >= 10) && (parray[p].i_admin)))
        if (!parray[p].registered)
            pprintf(p1, "\n%s(++++)", parray[p].name);

        else if (rating != 0)
           if (in_list(p, L_COMPUTER, parray[p].name))
               pprintf(p1, "\n%s(%d)(C)", parray[p].name, rating);
           else
               pprintf(p1, "\n%s(%d)", parray[p].name, rating);
        else
            pprintf(p1, "\n%s(----)", parray[p].name, rating);
    else break;

    if (why == TELL_WHISPER)
       pprintf_prompt(p1, " whispers: %s\n", msg);
    else
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
    sprintf(tmp, ", who %s (idle: %s)", parray[p1].busy,
	    hms_desc(player_idle(p1)));
  } else {
    if (((player_idle(p1) % 3600) / 60) > 2) {
      sprintf(tmp, ", who has been idle %s", hms_desc(player_idle(p1)));
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

PUBLIC int com_ptell(int p, param_list param) /*tells partner - doesn't change last tell */

{
  char tmp[MAX_LINE_SIZE];
  int p1;

   if (parray[p].partner < 0) {
     pprintf (p, "You do not have a partner at present.\n");
     return COM_OK;
   }

   p1 = parray[p].partner;
   if ((p1 < 0) || (parray[p1].status == PLAYER_PASSWORD)
       || (parray[p1].status == PLAYER_LOGIN)) {
     pprintf(p, "Your partner is not logged in.\n");
     return COM_OK;
   }
   if (parray[p1].highlight) {
      pprintf_highlight(p1, "\n%s", parray[p].name);
    } else {
      pprintf(p1, "\n%s", parray[p].name);
    }
    pprintf_prompt(p1, " (your partner) tells you: %s\n", param[0].val.string);
    tmp[0] = '\0';
    if (!(parray[p1].busy[0] == '\0')) {
      sprintf(tmp, ", who %s (idle: %s)", parray[p1].busy,
              hms_desc(player_idle(p1)));
    } else {
      if (((player_idle(p1) % 3600) / 60) > 2) {
        sprintf(tmp, ", who has been idle %s", hms_desc(player_idle(p1)));
      }
    }
    /* else sprintf(tmp," "); */
    pprintf(p, "(told %s%s)\n", parray[p1].name,
            (((parray[p1].game>=0) && (garray[parray[p1].game].status == GAME_EXAMINE))
            ? ", who is examining a game" :
            (parray[p1].game >= 0 && (parray[p1].game != parray[p].game))
            ? ", who is playing" : tmp));
 return COM_OK;
}

PRIVATE int chtell(int p, int ch, char *msg)
{
  int p1, count = 0;

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

  for (p1 = 0; p1 < p_num; p1++) {
    if ((p1 == p) || (parray[p1].status != PLAYER_PROMPT))
      continue;
    if ((on_channel(ch, p1)) && (!player_censored(p1, p))) {
      tell(p, p1, msg, TELL_CHANNEL, ch);
      count++;
    }
  }

  if (count)
    parray[p].last_channel = ch;

  pprintf(p, "(%d->(%d))\n", ch, count);
  if (!on_channel(ch, p))
    pprintf(p, " (You're not listening to channel %d.)\n", ch);

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
    if (player_is_observe(p1, g) ||
        (garray[g].link >= 0 && player_is_observe(p1, garray[g].link))) {
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
    if ((player_is_observe(p1, g) || parray[p1].game == g ||
         (garray[g].link >= 0 &&
          (player_is_observe(p1, garray[g].link) || parray[p1].game == garray[g].link)
         )
        ) && parray[p1].i_kibitz) {
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
    sprintf(tmp, ", who %s (idle: %s)", parray[p1].busy,
	    hms_desc(player_idle(p1)));
  } else {
    if (((player_idle(p1) % 3600) / 60) > 2) {
      sprintf(tmp, ", who has been idle %s", hms_desc(player_idle(p1)));
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

#if 0
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
  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
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
  list[*num] = xstrdup(parray[p1].name);
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

#endif

#if 0 /* now in lists.c */
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
    if ((i == 0) && (!on_channel(i, p)) && (parray[p].adminLevel == 0)) {
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
	if (err == 3)
	  pprintf(p, "Maximum channel number exceeded.\n");
      }
    }
  }
  return COM_OK;
}
#endif

PUBLIC int com_inchannel(int p, param_list param)
{
  int p1,count = 0;
  char tmp[18];


  if (param[0].type == NULL) {
    pprintf (p,"inchannel [no params] has been removed\n");
    pprintf (p,"Please use inchannel [name] or inchannel [number]\n");
    return COM_OK;
  }

  if (param[0].type == TYPE_WORD) {
    p1 = player_find_part_login(param[0].val.word);
    if ((p1 < 0) || (parray[p1].status != PLAYER_PROMPT)) {
      pprintf(p, "No user named \"%s\" is logged in.\n", param[0].val.word);
      return COM_OK;
    }
    pprintf (p,"%s is in the following channels:",parray[p1].name); /* no \n */
    if (list_channels (p,p1))
      pprintf (p," No channels found.\n",parray[p1].name);
    return COM_OK;
  } else {
    sprintf (tmp,"%d",param[0].val.integer);
    for (p1 = 0; p1 < p_num; p1++) {
      if (parray[p1].status != PLAYER_PROMPT)
	continue;
      if (in_list(p1,L_CHANNEL,tmp)) {
        if (!count)
          pprintf(p,"Channel %d:",param[0].val.integer);
        pprintf (p," %s%s",parray[p1].name,(((parray[p1].adminLevel >= 10) && (parray[p1].i_admin) && (param[0].val.integer < 2)) ? "(*)" : ""));
        count++;
        }
    }
    if (!count)
      pprintf(p,"Channel %d is empty.\n",param[0].val.integer);
    else
      pprintf (p,"\n%d %s in channel %d.\n",count,(count == 1 ? "person is" : "people are"),param[0].val.integer);
    return COM_OK;
  }
}

#if 0 /* if anyone can do inchannel NULL without n^3 computation
		please do so*/
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
    c2 = param[1].val.integer;
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
      /* First get rid of ghosts. */
      for (j=0; j < numOn[i]; j++) {
        if (parray[channels[i][j]].status == PLAYER_PROMPT)
          continue;
        channels[i][j] = channels[i][--numOn[i]];
        j--;
      }
      pprintf(p, "Channel %d:", i);
      for (j = 0; j < numOn[i]; j++) {
	pprintf(p, " %s", parray[channels[i][j]].name);
        if ((i==0 || i==1) && parray[channels[i][j]].adminLevel >= 10
                           && parray[channels[i][j]].i_admin)
          pprintf(p, "(*)");
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
#endif

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
  if (!FindPlayer(p, param[0].val.word, &p1, &connected))
    return COM_OK;

  if (!parray[p1].registered) {
    pprintf(p, "Player \"%s\" is unregistered and cannot receive messages.\n",
            parray[p1].name);
    return COM_OK; /* no need to removed */
  }

  if ((player_censored(p1, p)) && (parray[p].adminLevel == 0)) {
    pprintf(p, "Player \"%s\" is censoring you.\n", parray[p1].name);
    if (!connected)
      player_remove(p1);
    return COM_OK;
  }
  if (player_add_message(p1, p, param[1].val.string)) {
    pprintf(p, "Couldn't send message to %s. Message buffer full.\n",
	    parray[p1].name);
  } else {
    if (connected) {
      pprintf(p1, "\n%s just sent you a message:\n", parray[p].name);
      pprintf_prompt(p1, "    %s\n", param[1].val.string);
    }
  }
  if (!connected)
    player_remove(p1);
  return COM_OK;
}

PUBLIC int com_messages(int p, param_list param)
{
  int start;
  /* int end = -1; */

  if (!parray[p].registered) {
    pprintf (p, "Unregistered players may not send or receive messages.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    player_show_messages (p);
  } else if (param[0].type == TYPE_WORD) {
    if (param[1].type != TYPE_NULL)
      return com_sendmessage(p, param);
    else ShowMsgsBySender(p, param);
  } else {
    start = param[0].val.integer;
    ShowMsgRange (p, start, start);
  }
  return COM_OK;
}

PUBLIC int com_clearmessages(int p, param_list param)
{
  int start;

  if (player_num_messages(p) <= 0) {
    pprintf(p, "You have no messages.\n");
    return COM_OK;
  }
  if (param[0].type == TYPE_NULL) {
    pprintf(p, "Messages cleared.\n");
    player_clear_messages(p);
  } else if (param[0].type == TYPE_WORD) {
    ClearMsgsBySender(p, param);
  } else if (param[0].type == TYPE_INT) {
    start = param[0].val.integer;
    ClrMsgRange (p, start, start);
  }
  return COM_OK;
}

PUBLIC int com_mailmess(int p, param_list param)
{
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
    sprintf(subj, "Your FICS messages from server %s", fics_hostname);
    sprintf(fname, "%s/%s", mdir, filename);
    mail_file_to_user (p, subj, fname);
    pprintf(p, "Messages sent to %s\n", parray[p].emailAddress);
  } else {
    pprintf(p, "You have no messages.\n");
  }
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
    if (player_notified(p1, p) && parray[p1].status == PLAYER_PROMPT) {
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
  int i,count;
/*  FILE *fp; */

  if (!in_list(p, L_TD, parray[p].name)) {
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
    int ch = param[0].val.integer;

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
    sprintf (tmp,"%d",param[0].val.integer);
    for (p1 = 0; p1 < p_num ; p1++) {
      if ((p1 == p) || (player_censored(p1, p)) || (parray[p1].status != PLAYER_PROMPT))
	continue;
      if (in_list(p1,L_CHANNEL,tmp))
        pprintf_prompt(p1, "\n%s\n", (parray[p1].highlight && parray[p1].bell) ? buffer4 :
		     (parray[p1].highlight && !parray[p1].bell) ? buffer3 :
		     (!parray[p1].highlight && parray[p1].bell) ? buffer2 :
		     buffer1);
    }
    pprintf(p, "*qtell %d 0*\n", param[0].val.integer);
  }
  return COM_OK;
}

PUBLIC int on_channel(int ch, int p)
{
 char tmp[10];  /* 9 digits ought to be enough :) */

 sprintf (tmp,"%d",ch);
 return in_list(p, L_CHANNEL,tmp );  /* since needs ch converted to a string keep
                                        hidden from view */
}

#if 0

void WhichList(int p, char *partial, char **Full)
{
  int i, gotit = -1;
  int slen = strlen(partial);
  static char *MyList[] = {"notify", "censor", "channel", ""};

  for (i=0; MyList[i][0] != '\0'; i++) {
    if (strncmp(partial, MyList[i], slen))
      continue;
    if (gotit >= 0) {
      pprintf(p, "Ambiguous list name.\n");
      *Full = NULL;
      return;
    }
    gotit = i;
  }
  if (gotit >= 0)
    *Full = MyList[gotit];
  else **Full = '\0';
  return;
}

PUBLIC int com_plus(int p, param_list param)
{
  char *list;
  int chan;
  parameter *Param1 = &param[1];

  WhichList (p, param[0].val.word, &list);

  if (list == NULL) return COM_OK;

  if (!strcmp(list, "notify"))
    return notorcen (p, Param1, &parray[p].num_notify, MAX_NOTIFY,
		     parray[p].notifyList, "notify");
  if (!strcmp(list, "censor"))
    return notorcen (p, Param1, &parray[p].num_censor, MAX_NOTIFY,
		     parray[p].censorList, "censor");
  if (!strcmp(list, "channel")) {
    chan = atoi(param[1].val.word);
    if (chan < 0) {
    }

    switch (channel_add(chan, p)) {
      case 0:  pprintf(p, "Channel %d turned on.\n", chan);  break;
      case 1:  pprintf(p, "Channel %d is already full.\n", chan);  break;
      case 2:  pprintf(p, "You are already on channel %d.\n", chan);  break;
      case 3:  pprintf(p, "Maximum number of channels exceeded.\n");  break;
    }
    return COM_OK;
  }
  return com_addlist(p, param);
}

PUBLIC int com_minus(int p, param_list param)
{
  char *list;
  int chan;
  parameter *Param1 = &param[1];

  WhichList (p, param[0].val.word, &list);

  if (list == NULL) return COM_OK;

  if (!strcmp(list, "notify"))
    return unnotorcen (p, Param1, &parray[p].num_notify, MAX_NOTIFY,
		       parray[p].notifyList, "notify");
  if (!strcmp(list, "censor"))
    return unnotorcen (p, Param1, &parray[p].num_censor, MAX_CENSOR,
		       parray[p].censorList, "censor");
  if (!strcmp(list, "channel")) {
    chan = atoi(param[1].val.word);
    if (on_channel(chan, p))
      channel_remove(chan, p);
    else
      pprintf(p, "You are not on channel %d", chan);
    return COM_OK;
  }
  return com_sublist(p, param);
}
#endif

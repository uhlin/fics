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
   Markus Uhlin                 23/12/10	Cleaned up the file
   Markus Uhlin                 23/12/10	Fixed compiler warnings
   Markus Uhlin                 24/01/04	Fixed pprintf() calls
   Markus Uhlin                 24/04/14	Refactored and reformatted ALL
						functions.
   Markus Uhlin                 25/01/18	Fixed -Wshadow
   Markus Uhlin                 25/04/04	tell: fixed constant expression
						result
*/

#include "stdinclude.h"
#include "common.h"

#include "command.h"
#include "comproc.h"
#include "formula.h"
#include "gamedb.h"
#include "gameproc.h"
#include "lists.h"
#include "maxxes-utils.h"
#include "network.h"
#include "playerdb.h"
#include "talkproc.h"
#include "utils.h"

#include <sys/resource.h>

#define TELL_TELL        0
#define TELL_SAY         1
#define TELL_WHISPER     2
#define TELL_KIBITZ      3
#define TELL_CHANNEL     4

int quota_time;

// hawk: hacked it to fit ALL persons - quota list is not needed anymore
PRIVATE int
CheckShoutQuota(int p)
{
	time_t	timeleft = 0;
	time_t	timenow = time(NULL);

	timeleft = timenow - parray[p].lastshout_a;

	if (timeleft < quota_time && parray[p].adminLevel == 0)
		return (quota_time - timeleft);
	return 0;
}

PUBLIC int
com_shout(int p, param_list param)
{
	int	count = 0;
	int	p1;
	int	timeleft; // time left for quota if applicable

	if (!parray[p].registered) {
		pprintf(p, "Only registered players can use the shout "
		    "command.\n");
		return COM_OK;
	}

	if (in_list(p, L_MUZZLE, parray[p].login)) {
		pprintf(p, "You are muzzled.\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_NULL) {
		if ((timeleft = CheckShoutQuota(p))) {
			pprintf(p, "Next shout available in %d seconds.\n",
			    timeleft);
		} else {
			pprintf(p, "Your next shout is ready for use.\n");
		}

		return COM_OK;
	}

	if ((timeleft = CheckShoutQuota(p))) {
		pprintf(p, "Shout not sent. Next shout in %d seconds.\n",
		    timeleft);
		return COM_OK;
	}

	parray[p].lastshout_a = parray[p].lastshout_b;
	parray[p].lastshout_b = time(NULL);

	if (!printablestring(param[0].val.string)) {
		pprintf(p, "Your message contains some unprintable "
		    "character(s).\n");
		return COM_OK;
	}

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

		pprintf_prompt(p1, "\n%s shouts: %s\n",
		    parray[p].name,
		    param[0].val.string);
	}

	pprintf(p, "(%d) %s shouts: %s\n", count, parray[p].name,
	    param[0].val.string);

	if ((timeleft = CheckShoutQuota(p))) {
		pprintf(p, "Next shout in %d seconds.\n", timeleft);
		return COM_OK;
	}

	return COM_OK;
}

PUBLIC int
com_cshout(int p, param_list param)
{
	int	count = 0;
	int	p1;

	if (!parray[p].registered) {
		pprintf(p, "Only registered players can use the cshout "
		    "command.\n");
		return COM_OK;
	}

	if (in_list(p, L_CMUZZLE, parray[p].login)) {
		pprintf(p, "You are c-muzzled.\n");
		return COM_OK;
	}

	if (!printablestring(param[0].val.string)) {
		pprintf(p, "Your message contains some unprintable "
		    "character(s).\n");
		return COM_OK;
	}

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

		pprintf_prompt(p1, "\n%s c-shouts: %s\n",
		    parray[p].name,
		    param[0].val.string);
	}

	pprintf(p, "(%d) %s c-shouts: %s\n", count, parray[p].name,
	    param[0].val.string);
	return COM_OK;
}

PUBLIC int
com_it(int p, param_list param)
{
	int	count = 0;
	int	p1;
	int	timeleft;

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
			pprintf(p, "Next shout available in %d seconds.\n",
			    timeleft);
		} else {
			pprintf(p, "Your next shout is ready for use.\n");
		}

		return COM_OK;
	}

	if ((timeleft = CheckShoutQuota(p))) {
		pprintf(p, "Shout not sent. Next shout in %d seconds.\n",
		    timeleft);
		return COM_OK;
	}

	parray[p].lastshout_a = parray[p].lastshout_b;
	parray[p].lastshout_b = time(NULL);

	if (!printablestring(param[0].val.string)) {
		pprintf(p, "Your message contains some unprintable "
		    "character(s).\n");
		return COM_OK;
	}

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

		if (!strncmp(param[0].val.string, "\'", 1) ||
		    !strncmp(param[0].val.string, ",", 1) ||
		    !strncmp(param[0].val.string, ".", 1)) {
			pprintf_prompt(p1, "\n--> %s%s\n", parray[p].name,
			    param[0].val.string);
		} else {
			pprintf_prompt(p1, "\n--> %s %s\n", parray[p].name,
			    param[0].val.string);
		}
	}

	if (!strncmp(param[0].val.string, "\'", 1) ||
	    !strncmp(param[0].val.string, ",", 1) ||
	    !strncmp(param[0].val.string, ".", 1)) {
		pprintf(p, "(%d) --> %s%s\n", count, parray[p].name,
		    param[0].val.string);
	} else {
		pprintf(p, "(%d) --> %s %s\n", count, parray[p].name,
		    param[0].val.string);
	}

	if ((timeleft = CheckShoutQuota(p))) {
		pprintf(p, "Next shout in %d seconds.\n", timeleft);
		return COM_OK;
	}

	return COM_OK;
}

PRIVATE int
tell(int p, int p1, char *msg, int why, int ch)
{
	char	tmp[MAX_LINE_SIZE] = { '\0' };
	int	rating1;
	int	rating;

	if (!printablestring(msg)) {
		pprintf(p, "Your message contains some unprintable "
		    "character(s).\n");
		return COM_OK;
	}

	if ((!parray[p1].i_tell) && (!parray[p].registered)) {
		pprintf(p, "Player \"%s\" isn't listening to unregistered "
		    "tells.\n", parray[p1].name);
		return COM_OK;
	}

	if (player_censored(p1, p) && parray[p].adminLevel == 0) {
		if (why != TELL_KIBITZ &&
		    why != TELL_WHISPER &&
		    why != TELL_CHANNEL) {
			pprintf(p, "Player \"%s\" is censoring you.\n",
			    parray[p1].name);
		}

		return COM_OK;
	}

	switch (why) {
	case TELL_SAY:
		pprintf_highlight(p1, "\n%s", parray[p].name);
		pprintf_prompt(p1, " says: %s\n", msg);
		break;
	case TELL_WHISPER:
	case TELL_KIBITZ:
		rating = GetRating(&parray[p], TYPE_BLITZ);

		if (rating < (rating1 = (GetRating(&parray[p], TYPE_STAND))))
			rating = rating1;

		if (in_list(p, L_FM, parray[p].name)) {
			pprintf(p1, "\n%s(FM)", parray[p].name);
		} else if (in_list(p, L_IM, parray[p].name)) {
			pprintf(p1, "\n%s(IM)", parray[p].name);
		} else if (in_list(p, L_GM, parray[p].name)) {
			pprintf(p1, "\n%s(GM)", parray[p].name);
		} else if (parray[p].adminLevel >= 10 && parray[p].i_admin) {
			pprintf(p1, "\n%s(*)", parray[p].name);
		} else if ((rating >= parray[p1].kiblevel) ||
		    ((parray[p].adminLevel >= 10) && (parray[p].i_admin))) {
			if (!parray[p].registered) {
				pprintf(p1, "\n%s(++++)", parray[p].name);
			} else if (rating != 0) {
				if (in_list(p, L_COMPUTER, parray[p].name)) {
					pprintf(p1, "\n%s(%d)(C)",
					    parray[p].name,
					    rating);
				} else {
					pprintf(p1, "\n%s(%d)",
					    parray[p].name,
					    rating);
				}
			} else {
				pprintf(p1, "\n%s(----)", parray[p].name);
			}
		} else {
			break;
		}

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
		msnprintf(tmp, sizeof tmp, ", who %s (idle: %s)",
		    parray[p1].busy,
		    hms_desc(player_idle(p1)));
	} else {
		if (((player_idle(p1) % 3600) / 60) > 2) {
			msnprintf(tmp, sizeof tmp, ", who has been idle %s",
			    hms_desc(player_idle(p1)));
		}
	}

	if (why == TELL_SAY || why == TELL_TELL) {
		pprintf(p, "(told %s%s)\n", parray[p1].name,
		    (((parray[p1].game >= 0) &&
		    (garray[parray[p1].game].status == GAME_EXAMINE)) ?
		    ", who is examining a game" :
		    (parray[p1].game >= 0 &&
		    (parray[p1].game != parray[p].game)) ?
		    ", who is playing" : tmp));
		parray[p].last_tell = p1;
	}

	return COM_OK;
}

/*
 * Tells partner - doesn't change last tell.
 */
PUBLIC int
com_ptell(int p, param_list param)
{
	char	tmp[MAX_LINE_SIZE];
	int	p1;

	if (parray[p].partner < 0) {
		pprintf(p, "You do not have a partner at present.\n");
		return COM_OK;
	}

	p1 = parray[p].partner;

	if (p1 < 0 ||
	    parray[p1].status == PLAYER_PASSWORD ||
	    parray[p1].status == PLAYER_LOGIN) {
		pprintf(p, "Your partner is not logged in.\n");
		return COM_OK;
	}

	if (parray[p1].highlight) {
		pprintf_highlight(p1, "\n%s", parray[p].name);
	} else {
		pprintf(p1, "\n%s", parray[p].name);
	}

	pprintf_prompt(p1, " (your partner) tells you: %s\n",
	    param[0].val.string);
	tmp[0] = '\0';

	if (!(parray[p1].busy[0] == '\0')) {
		msnprintf(tmp, sizeof tmp, ", who %s (idle: %s)",
		    parray[p1].busy,
		    hms_desc(player_idle(p1)));
	} else {
		if (((player_idle(p1) % 3600) / 60) > 2) {
			msnprintf(tmp, sizeof tmp, ", who has been idle %s",
			    hms_desc(player_idle(p1)));
		}
	}

	pprintf(p, "(told %s%s)\n", parray[p1].name,
	    (((parray[p1].game >= 0) &&
	    (garray[parray[p1].game].status == GAME_EXAMINE)) ?
	    ", who is examining a game" :
	    (parray[p1].game >= 0 &&
	    (parray[p1].game != parray[p].game)) ?
	    ", who is playing" : tmp));
	return COM_OK;
}

PRIVATE int
chtell(int p, int ch, char *msg)
{
	int	p1, count = 0;

	if (ch == 0 && parray[p].adminLevel == 0) {
		pprintf(p, "Only admins may send tells to channel 0.\n");
		return COM_OK;
	}

	if (ch < 0) {
		pprintf(p, "The lowest channel number is 0.\n");
		return COM_OK;
	}

	if (ch >= MAX_CHANNELS) {
		pprintf(p, "The maximum channel number is %d.\n",
		    (MAX_CHANNELS - 1));
		return COM_OK;
	}

	for (p1 = 0; p1 < p_num; p1++) {
		if (p1 == p || parray[p1].status != PLAYER_PROMPT)
			continue;
		if (on_channel(ch, p1) && !player_censored(p1, p)) {
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

PUBLIC int
com_whisper(int p, param_list param)
{
	int	count = 0;
	int	g;
	int	p1;

	if (!parray[p].num_observe && parray[p].game < 0) {
		pprintf(p, "You are not playing or observing a game.\n");
		return COM_OK;
	}

	if (!parray[p].registered && (parray[p].game == -1)) {
		pprintf(p, "You must be registered to whisper other people's "
		    "games.\n");
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
		    (garray[g].link >= 0 &&
		    player_is_observe(p1, garray[g].link))) {
			tell(p, p1, param[0].val.string, TELL_WHISPER, 0);

			if (parray[p].adminLevel >= ADMIN_ADMIN ||
			    !garray[g].private)
				count++;
		}
	}

	pprintf(p, "whispered to %d.\n", count);
	return COM_OK;
}

PUBLIC int
com_kibitz(int p, param_list param)
{
	int	count = 0;
	int	g;
	int	p1;

	if (!parray[p].num_observe && parray[p].game < 0) {
		pprintf(p, "You are not playing or observing a game.\n");
		return COM_OK;
	}

	if (!parray[p].registered && (parray[p].game == -1)) {
		pprintf(p, "You must be registered to kibitz other people's "
		    "games.\n");
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
		if ((player_is_observe(p1, g) ||
		    parray[p1].game == g ||
		    (garray[g].link >= 0 &&
		    (player_is_observe(p1, garray[g].link) ||
		    parray[p1].game == garray[g].link))) &&
		    parray[p1].i_kibitz) {
			tell(p, p1, param[0].val.string, TELL_KIBITZ, 0);

			if (parray[p].adminLevel >= ADMIN_ADMIN ||
			    !garray[g].private ||
			    parray[p1].game == g)
				count++;
		}
	}

	pprintf(p, "kibitzed to %d.\n", count);
	return COM_OK;
}

PUBLIC int
com_tell(int p, param_list param)
{
	int	p1;

	if (param[0].type == TYPE_NULL)
		return COM_BADPARAMETERS;

	if (param[0].type == TYPE_WORD) {
		stolower(param[0].val.word);

		if (!strcmp(param[0].val.word, ".")) {
			if (parray[p].last_tell < 0) {
				pprintf(p, "No one to tell anything to.\n");
				return COM_OK;
			} else {
				return tell(p, parray[p].last_tell,
				    param[1].val.string, TELL_TELL, 0);
			}
		}

		if (!strcmp(param[0].val.word, ",")) {
			if (parray[p].last_channel < 0) {
				pprintf(p, "No previous channel.\n");
				return COM_OK;
			} else {
				return chtell(p, parray[p].last_channel,
				    param[1].val.string);
			}
		}

		p1 = player_find_part_login(param[0].val.word);

		if (p1 < 0 ||
		    parray[p1].status == PLAYER_PASSWORD ||
		    parray[p1].status == PLAYER_LOGIN) {
			pprintf(p, "No user named \"%s\" is logged in.\n",
			    param[0].val.word);
			return COM_OK;
		}

		return tell(p, p1, param[1].val.string, TELL_TELL, 0);
	} else {	// Channel
		return chtell(p, param[0].val.integer, param[1].val.string);
	}
}

PUBLIC int
com_xtell(int p, param_list param)
{
	char	*msg;
	char	 tmp[2048];
	int	 p1;

	msg	= param[1].val.string;
	p1	= player_find_part_login(param[0].val.word);

	if (p1 < 0 ||
	    parray[p1].status == PLAYER_PASSWORD ||
	    parray[p1].status == PLAYER_LOGIN) {
		pprintf(p, "No user named \"%s\" is logged in.\n",
		    param[0].val.word);
		return COM_OK;
	}

	if (!printablestring(msg)) {
		pprintf(p, "Your message contains some unprintable "
		    "character(s).\n");
		return COM_OK;
	}

	if ((!parray[p1].i_tell) && (!parray[p].registered)) {
		pprintf(p, "Player \"%s\" isn't listening to unregistered "
		    "tells.\n", parray[p1].name);
		return COM_OK;
	}

	if (player_censored(p1, p) && parray[p].adminLevel == 0) {
		pprintf(p, "Player \"%s\" is censoring you.\n",
		    parray[p1].name);
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
		msnprintf(tmp, sizeof tmp, ", who %s (idle: %s)",
		    parray[p1].busy,
		    hms_desc(player_idle(p1)));
	} else {
		if (((player_idle(p1) % 3600) / 60) > 2) {
			msnprintf(tmp, sizeof tmp, ", who has been idle %s",
			    hms_desc(player_idle(p1)));
		}
	}

	pprintf(p, "(told %s%s)\n", parray[p1].name,
	    ((parray[p1].game >= 0 &&
	    garray[parray[p1].game].status == GAME_EXAMINE) ?
	    ", who is examining a game" :
	    (parray[p1].game >= 0 && parray[p1].game != parray[p].game) ?
	    ", who is playing" : tmp));

	return COM_OK;
}

PUBLIC int
com_say(int p, param_list param)
{
	if (parray[p].opponent < 0) {
		if (parray[p].last_opponent < 0) {
			pprintf(p, "No one to say anything to, try tell.\n");
			return COM_OK;
		} else {
			return tell(p, parray[p].last_opponent,
			    param[0].val.string, TELL_SAY, 0);
		}
	}

	return tell(p, parray[p].opponent, param[0].val.string, TELL_SAY, 0);
}

PUBLIC int
com_inchannel(int p, param_list param)
{
	char	 tmp[18];
	int	 p1, count = 0;

	if (param[0].type == TYPE_NULL) {
		pprintf(p, "inchannel [no params] has been removed\n");
		pprintf(p, "Please use inchannel [name] or inchannel"
		    "[number]\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_WORD) {
		p1 = player_find_part_login(param[0].val.word);

		if ((p1 < 0) || (parray[p1].status != PLAYER_PROMPT)) {
			pprintf(p, "No user named \"%s\" is logged in.\n",
			    param[0].val.word);
			return COM_OK;
		}

		pprintf(p, "%s is in the following channels:", parray[p1].name);

		if (list_channels(p, p1))
			pprintf(p, " No channels found.\n");
		return COM_OK;
	} else {
		msnprintf(tmp, sizeof tmp, "%d", param[0].val.integer);

		for (p1 = 0; p1 < p_num; p1++) {
			if (parray[p1].status != PLAYER_PROMPT)
				continue;
			if (in_list(p1, L_CHANNEL, tmp)) {
				if (!count) {
					pprintf(p, "Channel %d:",
					    param[0].val.integer);
				}

				pprintf(p, " %s%s", parray[p1].name,
				    (((parray[p1].adminLevel >= 10) &&
				    (parray[p1].i_admin) &&
				    (param[0].val.integer < 2)) ? "(*)" : ""));

				count++;
			}
		}

		if (!count) {
			pprintf(p, "Channel %d is empty.\n",
			    param[0].val.integer);
		} else {
			pprintf(p, "\n%d %s in channel %d.\n", count,
			    (count == 1 ? "person is" : "people are"),
			    param[0].val.integer);
		}

		return COM_OK;
	}
}

PUBLIC int
com_sendmessage(int p, param_list param)
{
	int p1, connected = 1;

	if (!parray[p].registered) {
		pprintf(p, "You are not registered and cannot send messages."
		    "\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_NULL || param[1].type == TYPE_NULL) {
		pprintf(p, "No message sent.\n");
		return COM_OK;
	}

	if (!FindPlayer(p, param[0].val.word, &p1, &connected))
		return COM_OK;

	if (!parray[p1].registered) {
		pprintf(p, "Player \"%s\" is unregistered and cannot receive "
		    "messages.\n", parray[p1].name);
		return COM_OK;
	}

	if (player_censored(p1, p) && parray[p].adminLevel == 0) {
		pprintf(p, "Player \"%s\" is censoring you.\n",
		    parray[p1].name);

		if (!connected)
			player_remove(p1);
		return COM_OK;
	}

	if (player_add_message(p1, p, param[1].val.string)) {
		pprintf(p, "Couldn't send message to %s. "
		    "Message buffer full.\n", parray[p1].name);
	} else {
		if (connected) {
			pprintf(p1, "\n%s just sent you a message:\n",
			    parray[p].name);
			pprintf_prompt(p1, "    %s\n", param[1].val.string);
		}
	}

	if (!connected)
		player_remove(p1);
	return COM_OK;
}

PUBLIC int
com_messages(int p, param_list param)
{
	int start;

	if (!parray[p].registered) {
		pprintf(p, "Unregistered players may not send or receive "
		    "messages.\n");
		return COM_OK;
	}

	if (param[0].type == TYPE_NULL) {
		player_show_messages(p);
	} else if (param[0].type == TYPE_WORD) {
		if (param[1].type != TYPE_NULL)
			return com_sendmessage(p, param);
		else
			ShowMsgsBySender(p, param);
	} else {
		start = param[0].val.integer;
		ShowMsgRange(p, start, start);
	}

	return COM_OK;
}

PUBLIC int
com_clearmessages(int p, param_list param)
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
		ClrMsgRange(p, start, start);
	}

	return COM_OK;
}

PUBLIC int
com_mailmess(int p, param_list param)
{
	char	*buffer[1000];
	char	 filename[MAX_FILENAME_SIZE];
	char	 fname[MAX_FILENAME_SIZE];
	char	 mdir[MAX_FILENAME_SIZE];
	char	 subj[120];
	int	 ret, too_long;

	if (!parray[p].registered) {
		pprintf(p, "Only registered people can use the mailmess "
		    "command.\n");
		return COM_OK;
	}

	snprintf(filename, sizeof filename, "%s.messages", parray[p].login);
	snprintf(mdir, sizeof mdir, "%s/player_data/%c/", stats_dir,
	    parray[p].login[0]);

	if (search_directory(mdir, filename, buffer, ARRAY_SIZE(buffer))) {
		snprintf(subj, sizeof subj, "Your FICS messages from server %s",
		    fics_hostname);

		ret = snprintf(fname, sizeof fname, "%s/%s", mdir, filename);
		too_long = (ret < 0 || (size_t)ret >= sizeof fname);

		if (!too_long) {
			mail_file_to_user(p, subj, fname);
			pprintf(p, "Messages sent to %s\n",
			    parray[p].emailAddress);
		}
	} else {
		pprintf(p, "You have no messages.\n");
	}

	return COM_OK;
}

PUBLIC int
com_znotify(int p, param_list param)
{
	int	p1, count = 0;

	for (p1 = 0; p1 < p_num; p1++) {
		if (player_notified(p, p1)) {
			if (!count) {
				pprintf(p, "Present company on your notify "
				    "list:\n  ");
			}

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
		if (player_notified(p1, p) &&
		    parray[p1].status == PLAYER_PROMPT) {
			if (!count) {
				pprintf(p, "The following players have you "
				    "on their notify list:\n  ");
			}

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

PUBLIC int
com_qtell(int p, param_list param)
{
	char buffer1[MAX_STRING_LENGTH];	// no highlight and no bell
	char buffer2[MAX_STRING_LENGTH];	// no highlight and bell
	char buffer3[MAX_STRING_LENGTH];	// highlight and no bell
	char buffer4[MAX_STRING_LENGTH];	// highlight and and bell
	char dummy[2];
	char tmp[MAX_STRING_LENGTH];
	int i, count;
	int p1;

	if (!in_list(p, L_TD, parray[p].name)) {
		pprintf(p, "Only TD programs are allowed to use this command."
		    "\n");
		return COM_OK;
	}

	mstrlcpy(buffer1, ":", sizeof buffer1);
	mstrlcpy(buffer2, ":", sizeof buffer2);
	mstrlcpy(buffer3, ":", sizeof buffer3);
	mstrlcpy(buffer4, ":", sizeof buffer4);

	msnprintf(tmp, sizeof tmp, "%s", param[1].val.string);

	count = 0;
	i = 0;
	while (tmp[i] != '\0' && count < 1029) {
		if (tmp[i] == '\\' && tmp[i + 1] == 'n') {
			mstrlcat(buffer1, "\n:", sizeof buffer1);
			mstrlcat(buffer2, "\n:", sizeof buffer2);
			mstrlcat(buffer3, "\n:", sizeof buffer3);
			mstrlcat(buffer4, "\n:", sizeof buffer4);

			count += 2;
			i += 2;
		} else if (tmp[i] == '\\' && tmp[i + 1] == 'b') {
			mstrlcat(buffer2, "\007", sizeof buffer2);
			mstrlcat(buffer4, "\007", sizeof buffer4);

			count++;
			i += 2;
		} else if (tmp[i] == '\\' && tmp[i + 1] == 'H') {
			mstrlcat(buffer3, "\033[7m", sizeof buffer3);
			mstrlcat(buffer4, "\033[7m", sizeof buffer4);

			count += 4;
			i += 2;
		} else if (tmp[i] == '\\' && tmp[i + 1] == 'h') {
			mstrlcat(buffer3, "\033[0m", sizeof buffer3);
			mstrlcat(buffer4, "\033[0m", sizeof buffer4);

			count += 4;
			i += 2;
		} else {
			dummy[0] = tmp[i];
			dummy[1] = '\0';

			mstrlcat(buffer1, dummy, sizeof buffer1);
			mstrlcat(buffer2, dummy, sizeof buffer2);
			mstrlcat(buffer3, dummy, sizeof buffer3);
			mstrlcat(buffer4, dummy, sizeof buffer4);

			count++;
			i++;
		}
	} // while

	if (param[0].type == TYPE_WORD) {
		if ((p1 = player_find_bylogin(param[0].val.word)) < 0) {
			pprintf(p, "*qtell %s 1*\n", param[0].val.word);
			return COM_OK;
		}

		pprintf_prompt(p1, "\n%s\n",
		    (parray[p1].highlight && parray[p1].bell) ?
		    buffer4 :
		    (parray[p1].highlight && !parray[p1].bell) ?
		    buffer3 :
		    (!parray[p1].highlight && parray[p1].bell) ?
		    buffer2 : buffer1);
		pprintf(p, "*qtell %s 0*\n", parray[p1].name);
	} else {
		int	ch = param[0].val.integer;

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

		msnprintf(tmp, sizeof tmp, "%d", param[0].val.integer);

		for (p1 = 0; p1 < p_num ; p1++) {
			if (p1 == p || player_censored(p1, p) ||
			    parray[p1].status != PLAYER_PROMPT)
				continue;
			if (in_list(p1, L_CHANNEL, tmp)) {
				pprintf_prompt(p1, "\n%s\n",
				    (parray[p1].highlight && parray[p1].bell) ?
				    buffer4 :
				    (parray[p1].highlight && !parray[p1].bell) ?
				    buffer3 :
				    (!parray[p1].highlight && parray[p1].bell) ?
				    buffer2 : buffer1);
			}
		}

		pprintf(p, "*qtell %d 0*\n", param[0].val.integer);
	}

	return COM_OK;
}

PUBLIC int
on_channel(int ch, int p)
{
	char tmp[20];

	snprintf(tmp, sizeof tmp, "%d", ch);
	return in_list(p, L_CHANNEL, tmp);	// since needs 'ch' converted
						// to a string keep hidden from
						// view
}

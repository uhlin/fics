/* ficsmain.h
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
   Markus Uhlin                 23/12/13	Cleaned up the file
*/

#ifndef _FICSMAIN_H
#define _FICSMAIN_H

/*
 * Heartbead functions occur approx in this time, including checking
 * for new connections and decrementing timeleft counters.
 */
#define HEARTBEATTIME 2

/*
 * Number of seconds that an idle connection can stand at login or
 * password prompt.
 */
#define MAX_LOGIN_IDLE 120

/*
 * Players who have been idle for more than 1 hour is logged out.
 */
#define MAX_IDLE_TIME 3600

#define DEFAULT_PROMPT "fics% "

#define MESS_WELCOME		"welcome"
#define MESS_LOGIN		"login"
#define MESS_LOGOUT		"logout"
#define MESS_MOTD		"motd"
#define MESS_ADMOTD		"admotd"
#define MESS_UNREGISTERED	"unregistered"

#define STATS_MESSAGES	"messages"
#define STATS_LOGONS	"logons"
#define STATS_GAMES	"games"
#define STATS_JOURNAL	"journal"

/* Arguments */
extern int	port;
extern int	withConsole;

#endif /* _FICSMAIN_H */

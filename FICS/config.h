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
   Richard Nash                 94/03/08	Created
   Sparky                       95/12/30	Beautified
   Markus Uhlin                 23/12/17	Revised
   Markus Uhlin                 23/12/18	Include 'ficspaths.h'
*/

#ifndef _CONFIG_H
#define _CONFIG_H

/*
 * The port on which the server binds
 */
#define DEFAULT_PORT      5000

/*
 * Must be a dns recognisable host name
 */
#define SERVER_HOSTNAME   "rpblc.net"

/*
 * At AFICS we just use 'fics'. But for your server you might want to
 * change this e.g. to BICS, EICS, DICS etc.
 */
#define SERVER_NAME       "Xfics" /* for pgn output */

#define SERVER_LOCATION   "Las Vegas, USA" /* for pgn output */

/*
 * Which is the default language for help files? See 'variable.h' for
 * the currently available settings.
 */
#define LANG_DEFAULT      LANG_ENGLISH

/*
 * Locations of the data, players and games directories.
 */
#include "ficspaths.h"

/*
 * Where the standard ucb mail program is
 */
//#define MAILPROGRAM "/usr/bin/mail"

/*
 * 'SENDMAILPROG' is a faster and more reliable means of sending mail
 * if defined. Make sure your system mailer agent is defined here
 * properly for your system with respect to name, location and
 * options. These may differ significantly depending on the type of
 * system and what mailer is installed.
 */
#define SENDMAILPROG		"/usr/sbin/sendmail"
#define SENDMAILPROG_ARGS	"-t"

/*
 * Details of the head admin
 */
#define HADMINHANDLE      "maxxe"
#define HADMINEMAIL       "maxxe@rpblc.net"

/*
 * Registration mail address
 */
#define REGMAIL           "maxxe@rpblc.net"

#endif    /* _CONFIG_H */

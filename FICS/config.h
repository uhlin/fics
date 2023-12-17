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
   Richard Nash	              	94/03/08	Created
   Sparky                       95/12/30	Beautified.
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
#define SERVER_HOSTNAME   "jujube.rpblc.net"

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
#define DEFAULT_ADHELP    "/usr/home/fics/FICS.DIST/data/admin"
#define DEFAULT_ADJOURNED "/usr/home/fics/FICS.DIST/games/adjourned"
#define DEFAULT_BOARDS    "/usr/home/fics/FICS.DIST/data/boards"
#define DEFAULT_BOOK      "/usr/home/fics/FICS.DIST/data/book"
#define DEFAULT_COMHELP   "/usr/home/fics/FICS.DIST/data/com_help"
#define DEFAULT_CONFIG    "/usr/home/fics/FICS.DIST/config"
#define DEFAULT_HELP      "/usr/home/fics/FICS.DIST/data/help"
#define DEFAULT_HISTORY   "/usr/home/fics/FICS.DIST/games/history"
#define DEFAULT_INDEX     "/usr/home/fics/FICS.DIST/data/index"
#define DEFAULT_INFO      "/usr/home/fics/FICS.DIST/data/info"
#define DEFAULT_JOURNAL   "/usr/home/fics/FICS.DIST/games/journal"
#define DEFAULT_LISTS     "/usr/home/fics/FICS.DIST/data/lists"
#define DEFAULT_MESS      "/usr/home/fics/FICS.DIST/data/messages"
#define DEFAULT_NEWS      "/usr/home/fics/FICS.DIST/data/news"
#define DEFAULT_PLAYERS   "/usr/home/fics/FICS.DIST/players"
#define DEFAULT_SOURCE    "/usr/home/fics/FICS.DIST/FICS"
#define DEFAULT_STATS     "/usr/home/fics/FICS.DIST/data/stats"
#define DEFAULT_USAGE     "/usr/home/fics/FICS.DIST/data/usage"
#define DEFAULT_USCF      "/usr/home/fics/FICS.DIST/data/uscf"
#define HELP_DANISH       "/usr/home/fics/FICS.DIST/data/Danish"
#define HELP_FRENCH       "/usr/home/fics/FICS.DIST/data/French"
#define HELP_SPANISH      "/usr/home/fics/FICS.DIST/data/Spanish"
#define MESS_FULL         "/usr/home/fics/FICS.DIST/data/messages/full"
#define MESS_FULL_UNREG   "/usr/home/fics/FICS.DIST/data/messages/full_unreg"
#define USAGE_DANISH      "/usr/home/fics/FICS.DIST/data/usage_danish"
#define USAGE_FRENCH      "/usr/home/fics/FICS.DIST/data/usage_french"
#define USAGE_SPANISH     "/usr/home/fics/FICS.DIST/data/usage_spanish"

/*
 * Where the standard ucb mail program is
 */
#define MAILPROGRAM       "/usr/bin/mail"

/*
 * 'SENDMAILPROG' is a faster and more reliable means of sending mail
 * if defined. Make sure your system mailer agent is defined here
 * properly for your system with respect to name, location and
 * options. These may differ significantly depending on the type of
 * system and what mailer is installed.
 */
#define SENDMAILPROG "/usr/sbin/sendmail -t"

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

/* config.h
 *
 */

/* Configure file locations in this include file. */

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

/* CONFIGURE THIS: The port on which the server binds */

#define DEFAULT_PORT      5000

/* Must be a dns recognisable host name */

#define SERVER_HOSTNAME   "fics.somewhere.domain"

/* At AFICS we just use 'fics' but for your server you might want to change this
     eg to BICS, EICS, DICS   etc */

#define SERVER_NAME       "Xfics" /* for pgn output */

#define SERVER_LOCATION   "Unconfigured City, Country"   /* for pgn output */


/* Which is the default language for help files, see variable.h for the
    current available settings */

#define LANG_DEFAULT      LANG_ENGLISH

/* CONFIGURE THESE: Locations of the data, players, and games directories */
/* These must be absolute paths because some mail daemons may be called */
/* from outside the pwd */

#define DEFAULT_MESS      "/usr/home/fics/FICS.DIST/data/messages"
#define DEFAULT_INDEX     "/usr/home/fics/FICS.DIST/data/index"
#define DEFAULT_HELP      "/usr/home/fics/FICS.DIST/data/help"
#define DEFAULT_COMHELP   "/usr/home/fics/FICS.DIST/data/com_help"
#define HELP_SPANISH      "/usr/home/fics/FICS.DIST/data/Spanish"
#define HELP_FRENCH       "/usr/home/fics/FICS.DIST/data/French"
#define HELP_DANISH       "/usr/home/fics/FICS.DIST/data/Danish"
#define DEFAULT_INFO      "/usr/home/fics/FICS.DIST/data/info"
#define DEFAULT_ADHELP    "/usr/home/fics/FICS.DIST/data/admin"
#define DEFAULT_USCF      "/usr/home/fics/FICS.DIST/data/uscf"
#define DEFAULT_STATS     "/usr/home/fics/FICS.DIST/data/stats"
#define DEFAULT_CONFIG    "/usr/home/fics/FICS.DIST/config"
#define DEFAULT_PLAYERS   "/usr/home/fics/FICS.DIST/players"
#define DEFAULT_ADJOURNED "/usr/home/fics/FICS.DIST/games/adjourned"
#define DEFAULT_HISTORY   "/usr/home/fics/FICS.DIST/games/history"
#define DEFAULT_JOURNAL   "/usr/home/fics/FICS.DIST/games/journal"
#define DEFAULT_BOARDS    "/usr/home/fics/FICS.DIST/data/boards"
#define DEFAULT_SOURCE    "/usr/home/fics/FICS.DIST/FICS"
#define DEFAULT_LISTS     "/usr/home/fics/FICS.DIST/data/lists"
#define DEFAULT_NEWS      "/usr/home/fics/FICS.DIST/data/news"
#define DEFAULT_BOOK      "/usr/home/fics/FICS.DIST/data/book"
#define MESS_FULL         "/usr/home/fics/FICS.DIST/data/messages/full"
#define MESS_FULL_UNREG   "/usr/home/fics/FICS.DIST/data/messages/full_unreg"
#define DEFAULT_USAGE     "/usr/home/fics/FICS.DIST/data/usage"
#define USAGE_SPANISH     "/usr/home/fics/FICS.DIST/data/usage_spanish"
#define USAGE_FRENCH      "/usr/home/fics/FICS.DIST/data/usage_french"
#define USAGE_DANISH      "/usr/home/fics/FICS.DIST/data/usage_danish"


/* Where the standard ucb mail program is */

#define MAILPROGRAM       "/usr/bin/mail"

/* SENDMAILPROG is a faster and more reliable means of sending mail if
   defined.  Make sure your system mailer agent is defined here properly
   for your system with respect to name, location and options.  These may
   differ significatly depending on the type of system and what mailer is
   installed  */
/* The floowing works fine for SunOS4.1.X with berkeley sendmail  */

/* #define SENDMAILPROG   "/usr/lib/sendmail -t" */

/* Details of the head admin */

#define HADMINHANDLE      "AdminGuy"
#define HADMINEMAIL       "AdminGuy@this.place"

/* Registration mail address */

#define REGMAIL           "AdminGuy@this.place"

#endif    /* _CONFIG_H */

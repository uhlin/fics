/* utils.h
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
   Markus Uhlin			23/12/10	Renamed strdup() to xstrdup()
   Markus Uhlin			23/12/10	Deleted check_emailaddr()
   Markus Uhlin			23/12/17	Added argument lists
*/

#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>
#include "multicol.h"

#define MAX_WORD_SIZE 1024

/* Maximum length of an output line */
#define MAX_LINE_SIZE 1024

/* Maximum size of a filename */
#ifdef FILENAME_MAX
#  define MAX_FILENAME_SIZE FILENAME_MAX
#else
#  define MAX_FILENAME_SIZE 1024
#endif

#define SetFlag(VAR, FLAG) (VAR |= (FLAG))
#define ClearFlag(VAR, FLAG) (VAR &= ~(FLAG))
#define CheckFlag(VAR, FLAG) (VAR & (FLAG))

extern char *dotQuad();
extern char *eattailwhite(char *);
extern char *eatwhite(char *);
extern char *eatword(char *);
extern char *file_bplayer();
extern char *file_wplayer();
extern char *fix_time(char *);
extern char *getword( );
extern char *hms();
extern char *hms_desc();
extern char *nextword(char *);
extern char *ratstr();
extern char *ratstrii();
extern char *stolower( );
extern char *strgtime(time_t *);
extern char *strltime(time_t *);
extern char *tenth_str();
extern char *xstrdup(const char *);
extern int alphastring( );
extern int available_space();
extern int count_lines(FILE *);
extern int display_directory(int, char **, int);
extern int file_exists();
extern int file_has_pname();
extern int iswhitespace( );
extern int lines_file();
extern int mail_file_to_address(char *, char *, char *);
extern int mail_file_to_user();
extern int mail_string_to_address();
extern int mail_string_to_user();
extern int pcommand(int, char *, ...);
extern int pmail_file( );
extern int pmore_file( );
extern int pprintf(int, char *, ...);
extern int pprintf_highlight(int, char *, ...);
extern int pprintf_noformat(int, char *, ...);
extern int pprintf_prompt(int, char *, ...);
extern int printablestring( );
extern int psend_command( );
extern int psend_file( );
extern int psend_logoutfile( );
extern int psend_raw_file( );
extern int psprintf_highlight(int, char *, char *, ...);
extern int safechar( );
extern int safestring( );
extern int search_directory(char *, char *, char **, int);
extern int truncate_file();
extern int untenths();
extern unsigned tenth_secs();
extern void pprintf_dohightlight(int);
extern void sprintf_dohightlight(int,char *);

#endif /* _UTILS_H */

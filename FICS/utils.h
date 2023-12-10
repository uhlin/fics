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

extern int count_lines(FILE *);
extern int iswhitespace( );
extern char *getword( );
/* Returns a pointer to the first whitespace in the argument */
extern char *eatword( );
/* Returns a pointer to the first non-whitespace char in the argument */
extern char *eatwhite( );
/* Returns a pointer to the same string with trailing spaces removed */
extern char *eattailwhite( );
/* Returns the next word in a given string >eatwhite(eatword(foo))< */
extern char *nextword( );

extern int check_emailaddr(char *);
extern int mail_string_to_address();
extern int mail_string_to_user();
extern int mail_file_to_address();
extern int mail_file_to_user();
extern int pcommand(int, char *, ...);
extern int pprintf(int, char *, ...);
extern void pprintf_dohightlight(int);
extern void sprintf_dohightlight(int,char *);
extern int pprintf_highlight(int, char *, ...);
extern int psprintf_highlight(int, char *, char *, ...);
extern int pprintf_prompt(int, char *, ...);
extern int pprintf_noformat(int, char *, ...);
extern int psend_raw_file( );
extern int psend_file( );
extern int psend_logoutfile( );
extern int pmore_file( );
extern int pmail_file( );
extern int psend_command( );
extern char *fix_time(char *);
extern char *stolower( );

extern int safechar( );
extern int safestring( );
extern int alphastring( );
extern int printablestring( );
extern char *xstrdup(const char *);

extern char *hms_desc();
extern char *hms();
extern char *strltime();
extern char *strgtime();
extern unsigned tenth_secs();
extern char *tenth_str();
extern int untenths();

extern int truncate_file();
extern int lines_file();

extern int file_has_pname();
extern char *file_wplayer();
extern char *file_bplayer();

extern char *dotQuad();

extern int available_space();
extern int file_exists();
extern char *ratstr();
extern char *ratstrii();

extern int search_directory(char *, char *, char **, int);
extern int display_directory(int, char **, int);

#endif /* _UTILS_H */

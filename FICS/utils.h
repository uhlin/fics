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
   Markus Uhlin			23/12/28	Completed adding argument lists
   Markus Uhlin			24/01/04	Added usage of PRINTFLIKE()
*/

#ifndef _UTILS_H
#define _UTILS_H

#include <stdio.h>

#include "common.h" /* PRINTFLIKE() */
#include "multicol.h"

#define MAX_WORD_SIZE 1024

/*
 * Maximum length of an output line
 */
#define MAX_LINE_SIZE 1024

/*
 * Maximum size of a filename
 */
#ifdef FILENAME_MAX
#  define MAX_FILENAME_SIZE FILENAME_MAX
#else
#  define MAX_FILENAME_SIZE 1024
#endif

#define SetFlag(VAR, FLAG)	(VAR |= (FLAG))
#define ClearFlag(VAR, FLAG)	(VAR &= ~(FLAG))
#define CheckFlag(VAR, FLAG)	(VAR & (FLAG))

__FICS_BEGIN_DECLS
extern char		*dotQuad(unsigned int);
extern char		*eattailwhite(char *);
extern char		*eatwhite(char *);
extern char		*eatword(char *);
extern char		*file_bplayer(char *);
extern char		*file_wplayer(char *);
extern char		*fix_time(char *);
extern char		*getword(char *);
extern char		*hms(int, int, int, int);
extern char		*hms_desc(int);
extern char		*nextword(char *);
extern char		*ratstr(int);
extern char		*ratstrii(int, int);
extern char		*stolower(char *);
extern char		*strgtime(time_t *);
extern char		*strltime(time_t *);
extern char		*tenth_str(unsigned int, int);
extern char		*xstrdup(const char *);
extern int		 alphastring(char *);
extern int		 available_space(void);
extern int		 count_lines(FILE *);
extern int		 display_directory(int, char **, int);
extern int		 file_exists(char *);
extern int		 file_has_pname(char *, char *);
extern int		 iswhitespace(int);
extern int		 lines_file(char *);
extern int		 mail_file_to_address(char *, char *, char *);
extern int		 mail_file_to_user(int, char *, char *);
extern int		 mail_string_to_address(char *, char *, char *);
extern int		 mail_string_to_user(int, char *, char *);
extern int		 pcommand(int, char *, ...) PRINTFLIKE(2);
extern int		 pmore_file(int);
extern int		 pprintf(int, const char *, ...) PRINTFLIKE(2);
extern int		 pprintf_highlight(int, char *, ...) PRINTFLIKE(2);
extern int		 pprintf_noformat(int, char *, ...) PRINTFLIKE(2);
extern int		 pprintf_prompt(int, char *, ...) PRINTFLIKE(2);
extern int		 printablestring(char *);
extern int		 psend_command(int, char *, char *);
extern int		 psend_file(int, char *, char *);
extern int		 psend_logoutfile(int, char *, char *);
extern int		 psend_raw_file(int, char *, char *);
extern int		 psprintf_highlight(int, char *, size_t, char *, ...)
			     PRINTFLIKE(4);
extern int		 safechar(int);
extern int		 safestring(char *);
extern int		 search_directory(char *, char *, char **, int);
extern int		 truncate_file(char *, int);
extern int		 untenths(unsigned int);
extern unsigned int	 tenth_secs(void);
//extern void		 pprintf_dohightlight(int);
//extern void		 sprintf_dohightlight(int, char *);
__FICS_END_DECLS

#endif /* _UTILS_H */

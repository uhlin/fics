/* comproc.h
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
*/

#ifndef _COMPROC_H
#define _COMPROC_H

extern const int	none;
extern const int	blitz_rat;
extern const int	std_rat;
extern const int	wild_rat;
extern const int	light_rat;

extern int com_rating_recalc();
extern int com_more();
extern void rscan_news(FILE *, int, int);
extern void rscan_news2(FILE *, int, int);
extern int num_news;  /* The number of news items in the index file. */

extern int com_quit();
extern int com_index();
extern int com_help();
extern int com_info();
extern int com_adhelp();
extern int com_uscf();
extern int com_set();
extern int FindPlayer();
extern int com_stats();
extern int com_password();
extern int com_uptime();
extern int com_date();
extern int com_llogons();
extern int com_logons();
extern int com_who();
extern int com_refresh();
extern int com_prefresh();
extern int com_open();
extern int com_simopen();
extern int com_bell();
extern int com_flip();
extern int com_highlight();
extern int com_style();
extern int com_promote();
extern int com_alias();
extern int com_unalias();
extern int com_servers();
extern int com_mailsource();
extern int com_mailhelp();
extern int com_handles();
extern int com_news();
extern int com_getpi();
extern int com_getps();
extern int com_limits();

#endif /* _COMPROC_H */

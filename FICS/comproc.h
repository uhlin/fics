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
   Richard Nash			93/10/22	Created
   Markus Uhlin			23/12/09	Fixed implicit ints
   Markus Uhlin			23/12/19	Sorted declarations
   Markus Uhlin			24/01/03	Added argument lists
*/

#ifndef _COMPROC_H
#define _COMPROC_H

extern const int	none;
extern const int	blitz_rat;
extern const int	std_rat;
extern const int	wild_rat;
extern const int	light_rat;

/*
 * The number of news items in the index file.
 */
extern int num_news;

extern int FindPlayer(int, char *, int *, int *);

extern int	com_adhelp(int, param_list);
extern int	com_alias(int, param_list);
extern int	com_bell(int, param_list);
extern int	com_date(int, param_list);
extern int	com_flip(int, param_list);
extern int	com_getpi(int, param_list);
extern int	com_getps(int, param_list);
extern int	com_handles(int, param_list);
extern int	com_help(int, param_list);
extern int	com_highlight(int, param_list);
extern int	com_index(int, param_list);
extern int	com_info(int, param_list);
extern int	com_limits(int, param_list);
extern int	com_llogons(int, param_list);
extern int	com_logons(int, param_list);
extern int	com_mailhelp(int, param_list);
extern int	com_mailsource(int, param_list);
extern int	com_more(int, param_list);
extern int	com_news(int, param_list);
extern int	com_open(int, param_list);
extern int	com_password(int, param_list);
extern int	com_prefresh(int, param_list);
extern int	com_promote(int, param_list);
extern int	com_quit(int, param_list);
extern int	com_rating_recalc(int, param_list);
extern int	com_refresh(int, param_list);
extern int	com_servers(int, param_list);
extern int	com_set(int, param_list);
extern int	com_simopen(int, param_list);
extern int	com_stats(int, param_list);
extern int	com_style(int, param_list);
extern int	com_unalias(int, param_list);
extern int	com_uptime(int, param_list);
extern int	com_uscf(int, param_list);
extern int	com_who(int, param_list);

extern void	rscan_news(FILE *, int, int);
extern void	rscan_news2(FILE *, int, int);

#endif /* _COMPROC_H */

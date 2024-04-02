/* ratings.h
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

#ifndef _RATINGS_H
#define _RATINGS_H

#define STATS_VERSION 2

#define RESULT_WIN 0
#define RESULT_DRAW 1
#define RESULT_LOSS 2
#define RESULT_ABORT 3

#define PROVISIONAL 20

#define MAX_RANK_LINE 50
#define MAX_BEST 20

#define SHOW_BLITZ 0x1
#define SHOW_STANDARD 0x2
#define SHOW_WILD 0x4

#include "command.h"

typedef struct _rateStruct {
  char name[MAX_LOGIN_NAME];
  int rating;
} rateStruct;

extern int	Best(int, param_list, int);
extern int	DisplayRank(int, param_list, int);
extern int	DisplayRankedPlayers(int, int, int, int, int);
extern int	DisplayTargetRank(int, char *, int, int);
extern int	ShowFromString(char *);
extern int	com_assess(int, param_list);
extern int	com_best(int, param_list);
extern int	com_fixrank(int, param_list);
extern int	com_hbest(int, param_list);
extern int	com_hrank(int, param_list);
extern int	com_rank(int, param_list);
extern int	com_statistics(int, param_list);
extern int	is_active(int);
extern int	rating_delta(int, int, int, int, int);
extern int	rating_update(int);
extern void	UpdateRank(int, char *, statistics *, char *);
extern void	rating_add(int, int);
extern void	rating_init(void);
extern void	rating_recalc(void);
extern void	rating_remove(int, int);
extern void	rating_sterr_delta(int, int, int, int, int, int *, double *);
extern void	save_ratings(void);

#endif /* _RATINGS_H */

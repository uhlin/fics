/*
    ratings.c

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
   Richard Nash                 93/10/22	Created
   vek leeds@math.gatech.edu    95/04/05	Glicko system, with sterr
   Markus Uhlin                 23/12/17	Fixed the includes
   Markus Uhlin                 23/12/17	Usage of 'time_t'
   Markus Uhlin                 24/04/05	Reformatted ALL functions and
						reenabled disabled code.
   Markus Uhlin                 24/04/05	Replaced unbounded string
						handling functions.
   Markus Uhlin                 24/05/20	Fixed clang warnings
   Markus Uhlin                 24/07/07	Return value checking of the
						fscanf() calls.
*/

#include "stdinclude.h"
#include "common.h"

#include <err.h>
#include <errno.h>

#include "command.h"
#include "comproc.h"
#include "config.h"
#include "gamedb.h"
#include "lists.h"
#include "playerdb.h"
#include "ratings.h"
#include "utils.h"

#if __linux__
#include <bsd/string.h>
#endif

// Constants for Glicko system
#define Gd	3.25
#define Gr0	1720
#define Gs0	350
#define Gq	0.00575646273249
#define Gp	0.000010072398601964
// End of Glicko system variables

#define LOWESTHIST	800
#define MAXHIST		30

PRIVATE double	Ratings_B_Average;
PRIVATE double	Ratings_B_StdDev;
PRIVATE double	Ratings_S_Average;
PRIVATE double	Ratings_S_StdDev;
PRIVATE double	Ratings_L_Average;
PRIVATE double	Ratings_L_StdDev;
PRIVATE double	Ratings_W_Average;
PRIVATE double	Ratings_W_StdDev;

PRIVATE double	Rb_M = 0.0,
		Rb_S = 0.0,
		Rb_total = 0.0;
PRIVATE int	Rb_count = 0;

PRIVATE double	Rs_M = 0.0,
		Rs_S = 0.0,
		Rs_total = 0.0;
PRIVATE int	Rs_count = 0;

PRIVATE double	Rl_M = 0.0,
		Rl_S = 0.0,
		Rl_total = 0.0;
PRIVATE int	Rl_count = 0;

PRIVATE double	Rw_M = 0.0,
		Rw_S = 0.0,
		Rw_total = 0.0;
PRIVATE int	Rw_count = 0;

PRIVATE rateStruct	bestS[MAX_BEST];
PRIVATE int		numS = 0;
PRIVATE rateStruct	bestB[MAX_BEST];
PRIVATE int		numB = 0;
PRIVATE rateStruct	bestW[MAX_BEST];
PRIVATE int		numW = 0;

PRIVATE int	sHist[MAXHIST] = { 0 };
PRIVATE int	bHist[MAXHIST] = { 0 };
PRIVATE int	wHist[MAXHIST] = { 0 };
PRIVATE int	lHist[MAXHIST] = { 0 };

PRIVATE char sdir[] = DEFAULT_STATS;

PUBLIC int
is_active(int Games)
{
	return (Games >= PROVISIONAL);
}

PUBLIC void
rating_add(int rating, int type)
{
	int	which;

	if ((which = (rating - LOWESTHIST) / 100) < 0)
		which = 0;

	if (which >= MAXHIST)
		which = MAXHIST - 1;

	if (type == TYPE_BLITZ) {
		bHist[which] += 1;

		Rb_count++;
		Rb_total += rating;

		if (Rb_count == 1) {
			Rb_M = rating;
		} else {
			Rb_S = Rb_S + (rating - Rb_M) * (rating - Rb_M);
			Rb_M = Rb_M + (rating - Rb_M) / (Rb_count);
		}

		Ratings_B_StdDev	= sqrt(Rb_S / Rb_count);
		Ratings_B_Average	= (Rb_total / (double)Rb_count);
	} else if (type == TYPE_WILD) { // TYPE_WILD
		wHist[which] += 1;

		Rw_count++;
		Rw_total += rating;

		if (Rw_count == 1) {
			Rw_M = rating;
		} else {
			Rw_S = Rw_S + (rating - Rw_M) * (rating - Rw_M);
			Rw_M = Rw_M + (rating - Rw_M) / (Rw_count);
		}

		Ratings_W_StdDev	= sqrt(Rw_S / Rw_count);
		Ratings_W_Average	= (Rw_total / (double)Rw_count);
	} else if (type == TYPE_LIGHT) { // TYPE_LIGHT
		lHist[which] += 1;

		Rl_count++;
		Rl_total += rating;

		if (Rl_count == 1) {
			Rl_M = rating;
		} else {
			Rl_S = Rl_S + (rating - Rl_M) * (rating - Rl_M);
			Rl_M = Rl_M + (rating - Rl_M) / (Rl_count);
		}

		Ratings_L_StdDev	= sqrt(Rl_S / Rl_count);
		Ratings_L_Average	= (Rl_total / (double)Rl_count);

		// Insert bughouse stuff
	} else { // TYPE_STAND
		sHist[which] += 1;

		Rs_count++;
		Rs_total += rating;

		if (Rs_count == 1) {
			Rs_M = rating;
		} else {
			Rs_S = Rs_S + (rating - Rs_M) * (rating - Rs_M);
			Rs_M = Rs_M + (rating - Rs_M) / (Rs_count);
		}

		Ratings_S_StdDev	= sqrt(Rs_S / Rs_count);
		Ratings_S_Average	= (Rs_total / (double)Rs_count);
	}
}

PUBLIC void
rating_remove(int rating, int type)
{
	int	which;

	if ((which = (rating - LOWESTHIST) / 100) < 0)
		which = 0;

	if (which >= MAXHIST)
		which = MAXHIST - 1;

	if (type == TYPE_BLITZ) {
		bHist[which] = bHist[which] - 1;

		if (bHist[which] < 0)
			bHist[which] = 0;
		if (Rb_count == 0)
			return;

		Rb_count--;
		Rb_total -= rating;

		if (Rb_count == 0) {
			Rb_M = 0;
			Rb_S = 0;
		} else {
			Rb_M = Rb_M - (rating - Rb_M) / (Rb_count);
			Rb_S = Rb_S - (rating - Rb_M) * (rating - Rb_M);

			if (Rb_S < 0)
				Rb_S = 0;
		}

		if (Rb_count) {
			Ratings_B_StdDev	= sqrt(Rb_S / Rb_count);
			Ratings_B_Average	= (Rb_total / (double)Rb_count);
		} else {
			Ratings_B_StdDev	= 0;
			Ratings_B_Average	= 0;
		}
	} else if (type == TYPE_WILD) { // TYPE_WILD
		wHist[which] = wHist[which] - 1;

		if (wHist[which] < 0)
			wHist[which] = 0;
		if (Rw_count == 0)
			return;

		Rw_count--;
		Rw_total -= rating;

		if (Rw_count == 0) {
			Rw_M = 0;
			Rw_S = 0;
		} else {
			Rw_M = Rw_M - (rating - Rw_M) / (Rw_count);
			Rw_S = Rw_S - (rating - Rw_M) * (rating - Rw_M);

			if (Rw_S < 0)
				Rw_S = 0;
		}

		if (Rw_count) {
			Ratings_W_StdDev	= sqrt(Rw_S / Rw_count);
			Ratings_W_Average	= (Rw_total / (double)Rw_count);
		} else {
			Ratings_W_StdDev	= 0;
			Ratings_W_Average	= 0;
		}
	} else if (type == TYPE_LIGHT) { // TYPE_LIGHT
		lHist[which] = lHist[which] - 1;

		if (lHist[which] < 0)
			lHist[which] = 0;
		if (Rl_count == 0)
			return;

		Rl_count--;
		Rl_total -= rating;

		if (Rl_count == 0) {
			Rl_M = 0;
			Rl_S = 0;
		} else {
			Rl_M = Rl_M - (rating - Rl_M) / (Rl_count);
			Rl_S = Rl_S - (rating - Rl_M) * (rating - Rl_M);

			if (Rl_S < 0)
				Rl_S = 0;
		}

		if (Rl_count) {
			Ratings_L_StdDev	= sqrt(Rl_S / Rl_count);
			Ratings_L_Average	= (Rl_total / (double)Rl_count);
		} else {
			Ratings_L_StdDev	= 0;
			Ratings_L_Average	= 0;
		}

		// insert bughouse stuff here
	} else { // TYPE_STAND
		sHist[which] = sHist[which] - 1;

		if (sHist[which] < 0)
			sHist[which] = 0;
		if (Rs_count == 0)
			return;

		Rs_count--;
		Rs_total -= rating;

		if (Rs_count == 0) {
			Rs_M = 0;
			Rs_S = 0;
		} else {
			Rs_M = Rs_M - (rating - Rs_M) / (Rs_count);
			Rs_S = Rs_S - (rating - Rs_M) * (rating - Rs_M);

			if (Rs_S < 0)
				Rs_S = 0;
		}

		if (Rs_count) {
			Ratings_S_StdDev	= sqrt(Rs_S / Rs_count);
			Ratings_S_Average	= (Rs_total / (double)Rs_count);
		} else {
			Ratings_S_StdDev	= 0;
			Ratings_S_Average	= 0;
		}
	}
}

PRIVATE void
load_ratings(void)
{
	FILE	*fp;
	char	 fname[MAX_FILENAME_SIZE] = { '\0' };

	snprintf(fname, sizeof fname, "%s/newratingsV%d_data", stats_dir,
	    STATS_VERSION);

	if ((fp = fopen(fname, "r")) == NULL) {
		warn("%s: can't read ratings data", __func__);
		return;
	} else if (fscanf(fp, "%lf %lf %lf %d", &Rb_M, &Rb_S, &Rb_total,
	    &Rb_count) != 4 ||
	    fscanf(fp, "%lf %lf %lf %d", &Rs_M, &Rs_S, &Rs_total,
	    &Rs_count) != 4 ||
	    fscanf(fp, "%lf %lf %lf %d", &Rw_M, &Rw_S, &Rw_total,
	    &Rw_count) != 4 ||
	    fscanf(fp, "%lf %lf %lf %d", &Rl_M, &Rl_S, &Rl_total,
	    &Rl_count) != 4) {
		warn("%s: fscanf", __func__);
		fclose(fp);
		return;
	}

	for (int i = 0; i < MAXHIST; i++) {
		int ret, errno_save;

		errno = 0;
		ret = fscanf(fp, "%d %d %d %d", &sHist[i], &bHist[i], &wHist[i],
		    &lHist[i]);
		errno_save = errno;
		if (ret != 4) {
			if (feof(fp) || ferror(fp))
				break;
			errno = errno_save;
			warn("%s: too few items assigned (iteration: %d)",
			    __func__, i);
		}
	}

	fclose(fp);

	if (Rs_count) {
		Ratings_S_StdDev	= sqrt(Rs_S / Rs_count);
		Ratings_S_Average	= (Rs_total / (double)Rs_count);
	} else {
		Ratings_S_StdDev	= 0;
		Ratings_S_Average	= 0;
	}
	if (Rb_count) {
		Ratings_B_StdDev	= sqrt(Rb_S / Rb_count);
		Ratings_B_Average	= (Rb_total / (double)Rb_count);
	} else {
		Ratings_B_StdDev	= 0;
		Ratings_B_Average	= 0;
	}
	if (Rw_count) {
		Ratings_W_StdDev	= sqrt(Rw_S / Rw_count);
		Ratings_W_Average	= (Rw_total / (double)Rw_count);
	} else {
		Ratings_W_StdDev	= 0;
		Ratings_W_Average	= 0;
	}
	if (Rl_count) {
		Ratings_L_StdDev	= sqrt(Rl_S / Rl_count);
		Ratings_L_Average	= (Rl_total / (double)Rl_count);
	} else {
		Ratings_L_StdDev	= 0;
		Ratings_L_Average	= 0;
	}
}

PUBLIC void
save_ratings(void)
{
	FILE	*fp;
	char	 fname[MAX_FILENAME_SIZE] = { '\0' };

	snprintf(fname, sizeof fname, "%s/newratingsV%d_data", stats_dir,
	    STATS_VERSION);

	if ((fp = fopen(fname, "w")) == NULL) {
		warn("%s: can't write ratings data", __func__);
		return;
	}

	fprintf(fp, "%10f %10f %10f %d\n", Rb_M, Rb_S, Rb_total, Rb_count);
	fprintf(fp, "%10f %10f %10f %d\n", Rs_M, Rs_S, Rs_total, Rs_count);
	fprintf(fp, "%10f %10f %10f %d\n", Rw_M, Rw_S, Rw_total, Rw_count);
	fprintf(fp, "%10f %10f %10f %d\n", Rl_M, Rl_S, Rl_total, Rl_count);

	for (int i = 0; i < MAXHIST; i++) {
		fprintf(fp, "%d %d %d %d\n", sHist[i], bHist[i], wHist[i],
		    lHist[i]);
	}

	fclose(fp);
}

PRIVATE void
BestRemove(int p)
{
	int	i;

	for (i = 0; i < numB; i++) {
		if (!strcmp(bestB[i].name, parray[p].name))
			break;
	}
	if (i < numB) {
		for (; i < numB - 1; i++) {
			strlcpy(bestB[i].name, bestB[i + 1].name,
			    sizeof bestB[i].name);
			bestB[i].rating = bestB[i + 1].rating;
		}
		numB--;
	}

	for (i = 0; i < numS; i++) {
		if (!strcmp(bestS[i].name, parray[p].name))
			break;
	}
	if (i < numS) {
		for (; i < numS - 1; i++) {
			strlcpy(bestS[i].name, bestS[i + 1].name,
			    sizeof bestS[i].name);
			bestS[i].rating = bestS[i + 1].rating;
		}
		numS--;
	}

	for (i = 0; i < numW; i++) {
		if (!strcmp(bestW[i].name, parray[p].name))
			break;
	}
	if (i < numW) {
		for (; i < numW - 1; i++) {
			strlcpy(bestW[i].name, bestW[i + 1].name,
			    sizeof bestW[i].name);
			bestW[i].rating = bestW[i + 1].rating;
		}
		numW--;
	}
}

PRIVATE void
BestAdd(int p)
{
	int where, j;

	if (parray[p].b_stats.rating > 0 &&
	    parray[p].b_stats.num > 19) {
		for (where = 0; where < numB; where++) {
			if (parray[p].b_stats.rating > bestB[where].rating)
				break;
		}

		if (where < MAX_BEST) {
			for (j = numB; j > where; j--) {
				if (j == MAX_BEST)
					continue;
				strlcpy(bestB[j].name, bestB[j - 1].name,
				    sizeof(bestB[j].name));
				bestB[j].rating = bestB[j - 1].rating;
			}

			strlcpy(bestB[where].name, parray[p].name,
			    sizeof(bestB[where].name));
			bestB[where].rating = parray[p].b_stats.rating;

			if (numB < MAX_BEST)
				numB++;
		}
	}

	if (parray[p].s_stats.rating > 0 &&
	    parray[p].s_stats.num > 19) {
		for (where = 0; where < numS; where++) {
			if (parray[p].s_stats.rating > bestS[where].rating)
				break;
		}

		if (where < MAX_BEST) {
			for (j = numS; j > where; j--) {
				if (j == MAX_BEST)
					continue;
				strlcpy(bestS[j].name, bestS[j - 1].name,
				    sizeof(bestS[j].name));
				bestS[j].rating = bestS[j - 1].rating;
			}

			strlcpy(bestS[where].name, parray[p].name,
			    sizeof(bestS[where].name));
			bestS[where].rating = parray[p].s_stats.rating;

			if (numS < MAX_BEST)
				numS++;
		}
	}

	if (parray[p].w_stats.rating > 0 &&
	    parray[p].w_stats.num > 19) {
		for (where = 0; where < numW; where++) {
			if (parray[p].w_stats.rating > bestW[where].rating)
				break;
		}

		if (where < MAX_BEST) {
			for (j = numW; j > where; j--) {
				if (j == MAX_BEST)
					continue;
				strlcpy(bestW[j].name, bestW[j - 1].name,
				    sizeof(bestW[j].name));
				bestW[j].rating = bestW[j - 1].rating;
			}

			strlcpy(bestW[where].name, parray[p].name,
			    sizeof(bestW[where].name));
			bestW[where].rating = parray[p].w_stats.rating;

			if (numW < MAX_BEST)
				numW++;
		}
	}
}

PUBLIC void
BestUpdate(int p)
{
	BestRemove(p);
	BestAdd(p);
}

PRIVATE void
zero_stats(void)
{
	for (int i = 0; i < MAXHIST; i++) {
		sHist[i] = 0;
		bHist[i] = 0;
		wHist[i] = 0;
		lHist[i] = 0;
	}

	Rb_M = 0.0, Rb_S = 0.0, Rb_total = 0.0;
	Rb_count = 0;

	Rs_M = 0.0, Rs_S = 0.0, Rs_total = 0.0;
	Rs_count = 0;

	Rw_M = 0.0, Rw_S = 0.0, Rw_total = 0.0;
	Rw_count = 0;

	Rl_M = 0.0, Rl_S = 0.0, Rl_total = 0.0;
	Rl_count = 0;

	numS = 0;
	numB = 0;
	numW = 0;
}

#if 0
PUBLIC int
com_best(int p, param_list param)
{
	pprintf(p, "Standard                Blitz                   Wild\n");

	for (int i = 0; i < MAX_BEST; i++) {
		if (i >= numS && i >= numB)
			break;

		if (i < numS) {
			pprintf(p, "%4d %-17s  ",
			    bestS[i].rating,
			    bestS[i].name);
		} else {
			pprintf(p, "                        ");
		}

		if (i < numB) {
			pprintf(p, "%4d %-17s  ",
			    bestB[i].rating,
			    bestB[i].name);
		} else {
			pprintf(p, "                        ");
		}

		if (i < numW) {
			pprintf(p, "%4d %-17s\n",
			    bestW[i].rating,
			    bestW[i].name);
		} else {
			pprintf(p, "\n");
		}
	}

	return COM_OK;
}
#endif

PUBLIC void
rating_init(void)
{
	zero_stats();
	load_ratings();
}

/*
 * This recalculates the rating info from the player data. (Which can
 * take a long time!)
 */
PUBLIC void
rating_recalc(void)
{
	DIR		*dirp;
	char		 dname[MAX_FILENAME_SIZE];
	int		 c;
	int		 p1;
#if USE_DIRENT
	struct dirent	*dp;
#else
	struct direct	*dp;
#endif
	time_t		 t = time(NULL);

	fprintf(stderr, "FICS: Recalculating ratings at %s\n", strltime(&t));
	zero_stats();

	for (c = 'a'; c <= 'z'; c++) {
		snprintf(dname, sizeof dname, "%s/%c", player_dir, c);

		if ((dirp = opendir(dname)) == NULL)
			continue;

		for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
			if (dp->d_name[0] != '.') {
				p1 = player_new();

				if (player_read(p1, dp->d_name)) {
					player_remove(p1);

					fprintf(stderr, "FICS: Problem reading "
					    "player %s.\n", dp->d_name);

					continue;
				}

				if (parray[p1].b_stats.rating > 0) {
					rating_add(parray[p1].b_stats.rating,
					    TYPE_BLITZ);
				}
				if (parray[p1].s_stats.rating > 0) {
					rating_add(parray[p1].s_stats.rating,
					    TYPE_STAND);
				}
				if (parray[p1].l_stats.rating > 0) {
					rating_add(parray[p1].l_stats.rating,
					    TYPE_LIGHT);
				}

				// insert bughouse stuff here

				if (parray[p1].w_stats.rating > 0) {
					rating_add(parray[p1].w_stats.rating,
					    TYPE_WILD);
				}

				player_remove(p1);
			}
		} /* for */

		closedir(dirp);
	} /* for */

	if (Rs_count) {
		Ratings_S_StdDev	= sqrt(Rs_S / Rs_count);
		Ratings_S_Average	= (Rs_total / (double)Rs_count);
	} else {
		Ratings_S_StdDev	= 0;
		Ratings_S_Average	= 0;
	}
	if (Rb_count) {
		Ratings_B_StdDev	= sqrt(Rb_S / Rb_count);
		Ratings_B_Average	= (Rb_total / (double)Rb_count);
	} else {
		Ratings_B_StdDev	= 0;
		Ratings_B_Average	= 0;
	}
	if (Rl_count) {
		Ratings_L_StdDev	= sqrt(Rl_S / Rl_count);
		Ratings_L_Average	= (Rl_total / (double)Rl_count);
	} else {
		Ratings_L_StdDev	= 0;
		Ratings_L_Average	= 0;
	}
	if (Rw_count) {
		Ratings_W_StdDev	= sqrt(Rw_S / Rw_count);
		Ratings_W_Average	= (Rw_total / (double)Rw_count);
	} else {
		Ratings_W_StdDev	= 0;
		Ratings_W_Average	= 0;
	}

	save_ratings();

	t = time(NULL);
	fprintf(stderr, "FICS: Finished at %s\n", strltime(&t));
}

PRIVATE int
Round(double x)
{
	return (x < 0 ? (int)(x - 0.5) : (int)(x + 0.5));
}

PRIVATE double
Gf(double ss)
{
	return (1.0 / sqrt(1.0 + Gp * ss * ss));
}

/*
 * Confusing but economical: calculate error and attenuation function
 * together.
 */
PRIVATE double
GE(int r, int rr, double ss, double *fss)
{
	*fss = Gf(ss);
	return (1.0 / (1.0 + pow(10.0, (rr - r) * (*fss) / 400.0)));
}

PRIVATE double
current_sterr(double s, int t)
{
	if (t < 0)
		t = 0; // this shouldn't happen
	return sqrt(s * s + Gd * Gd * log(1.0 + t / 60.0));
}

/*
 * Calculates new rating and standard error. By vek. The person who
 * invented the ratings system is Mark E. Glickman, Ph.D. His e-mail
 * address is glickman@hustat.harvard.edu as of April '95. Postscript
 * copy of the note I coded this from should be available for ftp from
 * ics.onenet.net, if not elsewhere.
 */
PUBLIC void
rating_sterr_delta(int p1, int p2, int type, int gtime, int result,
    int *deltarating, double *newsterr)
{
	double		 E, fs2, denominator, GK, w; // Parts of fancy formulas
	double		 delta, sigma; // Results to return
	double		 s1, s2;
	int		 t1, r1, t2, r2; // Initial sterrs and ratings
	statistics	*p1_stats;
	statistics	*p2_stats;

	if (type == TYPE_BLITZ) {
		p1_stats = &parray[p1].b_stats;
		p2_stats = &parray[p2].b_stats;
	} else if (type == TYPE_WILD) {
		p1_stats = &parray[p1].w_stats;
		p2_stats = &parray[p2].w_stats;
	} else if (type == TYPE_LIGHT) {
		p1_stats = &parray[p1].l_stats;
		p2_stats = &parray[p2].l_stats;

		// insert bughouse stuff here
	} else {
		p1_stats = &parray[p1].s_stats;
		p2_stats = &parray[p2].s_stats;
	}

	/*
	 * Calculate effective pre-game sterr's. ltime == 0 implies
	 * never had sterr.
	 */
	if (p1_stats->ltime == 0)
		s1 = Gs0;
	else {
		t1 = gtime - p1_stats->ltime;
		s1 = current_sterr(p1_stats->sterr, t1);

		if (s1 > Gs0)
			s1 = Gs0;
	}

	if (p2_stats->ltime == 0)
		s2 = Gs0;
	else {
		t2 = gtime - p2_stats->ltime;
		s2 = current_sterr(p2_stats->sterr, t2);

		if (s2 > Gs0)
			s2 = Gs0;
	}

	if (p1_stats->rating == 0 && p1_stats->num == 0)
		r1 = Gr0;
	else
		r1 = p1_stats->rating;

	if (p2_stats->rating == 0 && p2_stats->num == 0)
		r2 = Gr0;
	else
		r2 = p2_stats->rating;

	if (result == RESULT_WIN) {
		w = 1.0;
	} else if (result == RESULT_DRAW) {
		w = 0.5;
	} else {
		w = 0.0;
	}

	E = GE(r1, r2, s2, &fs2);
	denominator = 1.0 / (s1 * s1) + Gq * Gq * fs2 * fs2 * E * (1.0 - E);
	GK = Gq * fs2 / denominator;
	delta = GK * (w - E);

	if (p1_stats->rating == 0 && p1_stats->num == 0)
		*deltarating = Round(Gr0 + delta);
	else
		*deltarating = Round(delta);	// Returned values: deltarating,
						// newsterr.

	sigma = 1.0 / sqrt(denominator);
	*newsterr = (double) sigma;
}

PUBLIC int
rating_delta(int p1, int p2, int type, int result, int gtime)
{
	int	delta;
	double	sigma;

	rating_sterr_delta(p1, p2, type, gtime, result, &delta, &sigma);
	return delta;
}

PUBLIC int
rating_update(int g)
{
	double		 wSigma, bSigma;
	int		 gtime;
	int		 inprogress = (g == parray[garray[g].black].game);
	int		 wDelta, bDelta;
	int		 wRes, bRes;
	statistics	*b_stats;
	statistics	*w_stats;

	/*
	 * If this is adjudication of stored game - be quiet about
	 * ratings change.
	 */
	if (garray[g].type == TYPE_BLITZ) {
		w_stats = &parray[garray[g].white].b_stats;
		b_stats = &parray[garray[g].black].b_stats;
	} else if (garray[g].type == TYPE_STAND) {
		w_stats = &parray[garray[g].white].s_stats;
		b_stats = &parray[garray[g].black].s_stats;
	} else if (garray[g].type == TYPE_WILD) {
		w_stats = &parray[garray[g].white].w_stats;
		b_stats = &parray[garray[g].black].w_stats;
	} else if (garray[g].type == TYPE_LIGHT) {
		w_stats = &parray[garray[g].white].l_stats;
		b_stats = &parray[garray[g].black].l_stats;
	} else {
		fprintf(stderr, "FICS: Can't update untimed ratings!\n");
		return -1;
	}

	switch (garray[g].result) {
	case END_CHECKMATE:
	case END_RESIGN:
	case END_FLAG:
	case END_ADJWIN:
		if (garray[g].winner == WHITE) {
			wRes = RESULT_WIN;
			bRes = RESULT_LOSS;
		} else {
			bRes = RESULT_WIN;
			wRes = RESULT_LOSS;
		}
		break;
	case END_AGREEDDRAW:
	case END_REPETITION:
	case END_50MOVERULE:
	case END_STALEMATE:
	case END_NOMATERIAL:
	case END_BOTHFLAG:
	case END_ADJDRAW:
	case END_FLAGNOMATERIAL:
		wRes = bRes = RESULT_DRAW;
		break;
	default:
		fprintf(stderr, "FICS: Update undecided game %d?\n",
		    garray[g].result);
		return -1;
	}

	gtime = untenths(garray[g].timeOfStart);

	rating_sterr_delta(garray[g].white, garray[g].black, garray[g].type,
	    gtime, wRes, &wDelta, &wSigma);
	rating_sterr_delta(garray[g].black, garray[g].white, garray[g].type,
	    gtime, bRes, &bDelta, &bSigma);

	w_stats->ltime = gtime;
	b_stats->ltime = gtime;

	if (wRes == RESULT_WIN) {
		w_stats->win++;
	} else if (wRes == RESULT_LOSS) {
		w_stats->los++;
	} else {
		w_stats->dra++;
	}

	w_stats->num++;

	if (bRes == RESULT_WIN) {
		b_stats->win++;
	} else if (bRes == RESULT_LOSS) {
		b_stats->los++;
	} else {
		b_stats->dra++;
	}

	b_stats->num++;

	rating_remove(w_stats->rating, garray[g].type);
	rating_remove(b_stats->rating, garray[g].type);

	if (inprogress) {
		pprintf(garray[g].white, "%s rating adjustment: %d ",
		    ((garray[g].type == TYPE_BLITZ) ? "Blitz" :
		    ((garray[g].type == TYPE_WILD) ? "Wild" :
		    ((garray[g].type == TYPE_LIGHT) ? "Lightning" :
		    "Standard"))),
		    w_stats->rating);
		pprintf(garray[g].black, "%s rating adjustment: %d ",
		    ((garray[g].type == TYPE_BLITZ) ? "Blitz" :
		    ((garray[g].type == TYPE_WILD) ? "Wild" :
		    ((garray[g].type == TYPE_LIGHT) ? "Lightning" :
		    "Standard"))),
		    b_stats->rating);
	}

	if (wDelta < -1000) {
		pprintf(garray[g].white, "not changed due to bug "
		    "(way too small)! sorry!\n");
		fprintf(stderr, "FICS: Got too small ratings bug for %s "
		    "(w) vs. %s\n",
		    parray[garray[g].white].login,
		    parray[garray[g].black].login);
	} else if (wDelta > 3000) {
		pprintf(garray[g].white, "not changed due to bug "
		    "(way too big)! sorry!\n");
		fprintf(stderr, "FICS: Got too big ratings bug for %s "
		    "(w) vs. %s\n",
		    parray[garray[g].white].login,
		    parray[garray[g].black].login);
	} else {
		w_stats->rating += wDelta;
		w_stats->sterr = wSigma;
	}

	if (bDelta < -1000) {
		pprintf(garray[g].black, "not changed due to bug "
		    "(way too small)! sorry!\n");
		fprintf(stderr, "FICS: Got too small ratings bug for %s "
		    "(b) vs. %s\n",
		    parray[garray[g].black].login,
		    parray[garray[g].white].login);
	} else if (bDelta > 3000) {
		pprintf(garray[g].black, "not changed due to bug "
		    "(way too big)! sorry!\n");
		fprintf(stderr, "FICS: Got too big ratings bug for %s "
		    "(b) vs. %s\n",
		    parray[garray[g].black].login,
		    parray[garray[g].white].login);
	} else {
		b_stats->rating += bDelta;
		b_stats->sterr = bSigma;
	}

	rating_add(w_stats->rating, garray[g].type);
	rating_add(b_stats->rating, garray[g].type);

	if (w_stats->rating > w_stats->best &&
	    is_active(w_stats->num)) {
		w_stats->best = w_stats->rating;
		w_stats->whenbest = time(NULL);
	}
	if (b_stats->rating > b_stats->best &&
	    is_active(b_stats->num)) {
		b_stats->best = b_stats->rating;
		b_stats->whenbest = time(NULL);
	}

	// Ratings are now saved to disk after each game
	player_save(garray[g].white);
	player_save(garray[g].black);

	// foxbat 3.11.95
	if (garray[g].type == TYPE_BLITZ) {
		Rb_count++;
		Rb_total += (w_stats->rating + b_stats->rating) / 2.0;
	} else if (garray[g].type == TYPE_STAND) {
		Rs_count++;
		Rs_total += (w_stats->rating + b_stats->rating) / 2.0;
	} else if (garray[g].type == TYPE_LIGHT) {
		Rl_count++;
		Rl_total += (w_stats->rating + b_stats->rating) / 2.0;
	} else if (garray[g].type == TYPE_WILD) {
		Rw_count++;
		Rw_total += (w_stats->rating + b_stats->rating) / 2.0;
	}

	// end add
	if (inprogress) {
		pprintf(garray[g].white, "--> %d\n", w_stats->rating);
		pprintf(garray[g].black, "--> %d\n", b_stats->rating);
	}

	save_ratings();

	UpdateRank(garray[g].type, parray[garray[g].white].name, w_stats,
	    parray[garray[g].white].name);
	UpdateRank(garray[g].type, parray[garray[g].black].name, b_stats,
	    parray[garray[g].black].name);
	return 0;
}

PUBLIC int
com_assess(int p, param_list param)
{
	double	newsterr1;
	double	newsterr2;
	int	p1 = p, p2;
	int	p1_connected = 1, p2_connected = 1;
	int	win1, draw1, loss1;
	int	win2, draw2, loss2;
	time_t	nowtime;

	nowtime = time(NULL);

	if (param[0].type == TYPE_NULL) {
		if (parray[p].game < 0) {
			pprintf(p, "You are not playing a game.\n");
			return COM_OK;
		} else if (garray[parray[p].game].status == GAME_EXAMINE) {
			if (!strcmp(garray[parray[p].game].black_name,
			    parray[p].name)) {
				pcommand(p, "assess %s\n",
				    garray[parray[p].game].white_name);
			} else {
				pcommand(p, "assess %s %s\n",
				    garray[parray[p].game].white_name,
				    garray[parray[p].game].black_name);
			}

			return COM_OK;
		} else {
			p2 = parray[p].opponent;
		}
	} else {
		if (!FindPlayer(p, param[0].val.word, &p2, &p2_connected)) {
			pprintf(p, "No user named \"%s\" was found.\n",
			    param[0].val.word);
			return COM_OK;
		}

		if (param[1].type != TYPE_NULL) {
			p1 = p2;
			p1_connected = p2_connected;

			if (!FindPlayer(p, param[1].val.word, &p2,
			    &p2_connected)) {
				pprintf(p, "No user named \"%s\" was found.\n",
				    param[1].val.word);

				if (!p1_connected)
					player_remove(p1);
				return COM_OK;
			}
		}
	}

	if (p1 == p2) {
		pprintf(p, "You can't assess the same players.\n");

		if (!p1_connected)
			player_remove(p1);
		if (!p2_connected)
			player_remove(p2);
		return COM_OK;
	}

	rating_sterr_delta(p1, p2, TYPE_BLITZ, nowtime, RESULT_WIN, &win1,
	    &newsterr1);
	rating_sterr_delta(p1, p2, TYPE_BLITZ, nowtime, RESULT_DRAW, &draw1,
	    &newsterr1);
	rating_sterr_delta(p1, p2, TYPE_BLITZ, nowtime, RESULT_LOSS, &loss1,
	    &newsterr1);
	rating_sterr_delta(p2, p1, TYPE_BLITZ, nowtime, RESULT_WIN, &win2,
	    &newsterr2);
	rating_sterr_delta(p2, p1, TYPE_BLITZ, nowtime, RESULT_DRAW, &draw2,
	    &newsterr2);
	rating_sterr_delta(p2, p1, TYPE_BLITZ, nowtime, RESULT_LOSS, &loss2,
	    &newsterr2);

	pprintf(p, "\nBlitz\n   %10s (%4s, RD: %5.1f)  %10s (%4s, RD: %5.1f)\n",
	    parray[p1].name,
	    ratstrii(parray[p1].b_stats.rating, parray[p1].registered),
	    parray[p1].b_stats.sterr,
	    parray[p2].name,
	    ratstrii(parray[p2].b_stats.rating, parray[p2].registered),
	    parray[p2].b_stats.sterr);

	pprintf(p, "Win :         %4d                         %4d\n",
	    win1,
	    loss2);
	pprintf(p, "Draw:         %4d                         %4d\n",
	    draw1,
	    draw2);
	pprintf(p, "Loss:         %4d                         %4d\n",
	    loss1,
	    win2);
	pprintf(p, "New RD:        %5.1f                        %5.1f\n",
	    newsterr1,
	    newsterr2);

	rating_sterr_delta(p1, p2, TYPE_STAND, nowtime, RESULT_WIN, &win1,
	    &newsterr1);
	rating_sterr_delta(p1, p2, TYPE_STAND, nowtime, RESULT_DRAW, &draw1,
	    &newsterr1);
	rating_sterr_delta(p1, p2, TYPE_STAND, nowtime, RESULT_LOSS, &loss1,
	    &newsterr1);
	rating_sterr_delta(p2, p1, TYPE_STAND, nowtime, RESULT_WIN, &win2,
	    &newsterr2);
	rating_sterr_delta(p2, p1, TYPE_STAND, nowtime, RESULT_DRAW, &draw2,
	    &newsterr2);
	rating_sterr_delta(p2, p1, TYPE_STAND, nowtime, RESULT_LOSS, &loss2,
	    &newsterr2);

	pprintf(p, "\nStandard\n   %10s (%4s, RD: %5.1f)  %10s "
	    "(%4s, RD: %5.1f)\n",
	    parray[p1].name,
	    ratstrii(parray[p1].s_stats.rating, parray[p1].registered),
	    parray[p1].s_stats.sterr,
	    parray[p2].name,
	    ratstrii(parray[p2].s_stats.rating, parray[p2].registered),
	    parray[p2].s_stats.sterr);

	pprintf(p, "Win :         %4d                         %4d\n",
	    win1,
	    loss2);
	pprintf(p, "Draw:         %4d                         %4d\n",
	    draw1,
	    draw2);
	pprintf(p, "Loss:         %4d                         %4d\n",
	    loss1,
	    win2);
	pprintf(p, "New RD:        %5.1f                        %5.1f\n",
	    newsterr1,
	    newsterr2);

	rating_sterr_delta(p1, p2, TYPE_LIGHT, nowtime, RESULT_WIN, &win1,
	    &newsterr1);
	rating_sterr_delta(p1, p2, TYPE_LIGHT, nowtime, RESULT_DRAW, &draw1,
	    &newsterr1);
	rating_sterr_delta(p1, p2, TYPE_LIGHT, nowtime, RESULT_LOSS, &loss1,
	    &newsterr1);
	rating_sterr_delta(p2, p1, TYPE_LIGHT, nowtime, RESULT_WIN, &win2,
	    &newsterr2);
	rating_sterr_delta(p2, p1, TYPE_LIGHT, nowtime, RESULT_DRAW, &draw2,
	    &newsterr2);
	rating_sterr_delta(p2, p1, TYPE_LIGHT, nowtime, RESULT_LOSS, &loss2,
	    &newsterr2);

	pprintf(p, "\nLightning\n   %10s (%4s, RD: %5.1f)  %10s "
	    "(%4s, RD: %5.1f)\n",
	    parray[p1].name,
	    ratstrii(parray[p1].l_stats.rating, parray[p1].registered),
	    parray[p1].l_stats.sterr,
	    parray[p2].name,
	    ratstrii(parray[p2].l_stats.rating, parray[p2].registered),
	    parray[p2].l_stats.sterr);

	pprintf(p, "Win :         %4d                         %4d\n",
	    win1,
	    loss2);
	pprintf(p, "Draw:         %4d                         %4d\n",
	    draw1,
	    draw2);
	pprintf(p, "Loss:         %4d                         %4d\n",
	    loss1,
	    win2);
	pprintf(p, "New RD:        %5.1f                        %5.1f\n",
	    newsterr1,
	    newsterr2);

	rating_sterr_delta(p1, p2, TYPE_WILD, nowtime, RESULT_WIN, &win1,
	    &newsterr1);
	rating_sterr_delta(p1, p2, TYPE_WILD, nowtime, RESULT_DRAW, &draw1,
	    &newsterr1);
	rating_sterr_delta(p1, p2, TYPE_WILD, nowtime, RESULT_LOSS, &loss1,
	    &newsterr1);

	rating_sterr_delta(p2, p1, TYPE_WILD, nowtime, RESULT_WIN, &win2,
	    &newsterr2);
	rating_sterr_delta(p2, p1, TYPE_WILD, nowtime, RESULT_DRAW, &draw2,
	    &newsterr2);
	rating_sterr_delta(p2, p1, TYPE_WILD, nowtime, RESULT_LOSS, &loss2,
	    &newsterr2);

	pprintf(p, "\nWild\n   %10s (%4s, RD: %5.1f)  %10s (%4s, RD: %5.1f)\n",
	    parray[p1].name,
	    ratstrii(parray[p1].w_stats.rating, parray[p1].registered),
	    parray[p1].w_stats.sterr,
	    parray[p2].name,
	    ratstrii(parray[p2].w_stats.rating, parray[p2].registered),
	    parray[p2].w_stats.sterr);

	pprintf(p, "Win :         %4d                         %4d\n",
	    win1,
	    loss2);
	pprintf(p, "Draw:         %4d                         %4d\n",
	    draw1,
	    draw2);
	pprintf(p, "Loss:         %4d                         %4d\n",
	    loss1,
	    win2);
	pprintf(p, "New RD:        %5.1f                        %5.1f\n",
	    newsterr1,
	    newsterr2);

	if (!p1_connected)
		player_remove(p1);
	if (!p2_connected)
		player_remove(p2);
	return COM_OK;
}

PUBLIC int
com_best(int p, param_list param)
{
	return Best(p, param, 1);
}

PUBLIC int
com_hbest(int p, param_list param)
{
	return Best(p, param, 0);
}

PUBLIC int
com_statistics(int p, param_list param)
{
	pprintf(p, "                Standard       Blitz   Lightning        "
	    "Wild\n");
	pprintf(p, "average:         %7.2f     %7.2f     %7.2f     %7.2f\n",
	    Ratings_S_Average,
	    Ratings_B_Average,
	    Ratings_L_Average,
	    Ratings_W_Average);
	pprintf(p, "std dev:         %7.2f     %7.2f     %7.2f     %7.2f\n",
	    Ratings_S_StdDev,
	    Ratings_B_StdDev,
	    Ratings_L_StdDev,
	    Ratings_W_StdDev);
	pprintf(p, "number :      %7d     %7d     %7d     %7d\n",
	    Rs_count, Rb_count,  Rl_count, Rw_count);
	return COM_OK;
}

PUBLIC int
com_fixrank(int p, param_list param)
{
	int	p1, connected;

	if (!FindPlayer(p, param[0].val.word, &p1, &connected))
		return COM_OK;

	UpdateRank(TYPE_BLITZ, parray[p1].name, &parray[p1].b_stats,
	    parray[p1].name);
	UpdateRank(TYPE_STAND, parray[p1].name, &parray[p1].s_stats,
	    parray[p1].name);
	UpdateRank(TYPE_WILD, parray[p1].name, &parray[p1].w_stats,
	    parray[p1].name);

	if (!connected)
		player_remove(p1);
	return COM_OK;
}

PUBLIC int
com_rank(int p, param_list param)
{
	return DisplayRank(p, param, 1);
}

PUBLIC int
com_hrank(int p, param_list param)
{
	return DisplayRank(p, param, 0);
}

PUBLIC int
DisplayRank(int p, param_list param, int showComputers)
{
	int	show = (SHOW_BLITZ|SHOW_STANDARD|SHOW_WILD);
	int	start, end, target, connected;

	if (param[0].type == TYPE_NULL) {
		DisplayTargetRank(p, parray[p].name, show, showComputers);
		return COM_OK;
	} else if (isdigit(param[0].val.word[0])) {
		end = -1;
		sscanf(param[0].val.word, "%d-%d", &start, &end);

		if (end > 0 && (param[1].type != TYPE_NULL))
			show = ShowFromString(param[1].val.word);

		DisplayRankedPlayers(p, start, end, show, showComputers);
		return COM_OK;
	} else {
		target = player_search(p, param[0].val.word);

		if (target == 0) {
			pprintf(p, "Target %s not found.\n", param[0].val.word);
			return COM_OK;
		}

		connected = (target > 0);

		if (!connected)
			target = -target - 1;
		else
			target--;

		if (param[1].type != TYPE_NULL)
			show = ShowFromString(param[1].val.word);

		DisplayTargetRank(p, parray[target].name, show, showComputers);

		if (!connected)
			player_remove(target);
		return COM_OK;
	}
}

/*
 * CompareStats() returns:
 *   - 1 if s1 comes first,
 *   - -1 if s2 comes first,
 *   - and 0 if neither takes precedence.
 */
PRIVATE int
CompareStats(char *name1, statistics *s1,
	     char *name2, statistics *s2)
{
	int	i, l1;

	if (s1 == NULL) {
		if (s2 == NULL)
			return 0;
		else
			return -1;
	} else if (s2 == NULL)
		return 1;

	if (s1->rating > s2->rating)
		return 1;
	if (s1->rating < s2->rating)
		return -1;

	l1 = strlen(name1);

	for (i = 0; i < l1; i++) {
		if (name2[i] == '\0')
			return -1;
		if (tolower(name1[i]) < tolower(name2[i]))
			return 1;
		if (tolower(name1[i]) > tolower(name2[i]))
			return -1;
	}

	if (name2[i] != '\0')
		return 1;

	fprintf(stderr, "Duplicate entries found: %s.\n", name1);
	return 0;
}

PRIVATE int
GetRankFileName(char *out, const size_t size, int type)
{
	switch (type) {
	case TYPE_BLITZ:
		snprintf(out, size, "%s/rank.blitz", sdir);
		return type;
	case TYPE_STAND:
		snprintf(out, size, "%s/rank.std", sdir);
		return type;
	case TYPE_WILD:
		snprintf(out, size, "%s/rank.wild", sdir);
		return type;
	default:
		return -1;
	}
}

PUBLIC void
UpdateRank(int type, char *addName, statistics *sNew, char *delName)
{
	FILE		*fp;
	FILE		*fptemp;
	char		 RankFile[MAX_FILENAME_SIZE];
	char		 TmpRankFile[MAX_FILENAME_SIZE];
	char		 command[MAX_STRING_LENGTH];
	char		 line[MAX_RANK_LINE] = { '\0' };
	char		 login[MAX_LOGIN_NAME] = { '\0' };
	int		 comp;
	statistics	 sCur;

	if (GetRankFileName(RankFile, sizeof RankFile, type) < 0)
		return;

	if ((fp = fopen(RankFile, "r")) == NULL) {
		warn("%s: can't open rank file to update", __func__);
		return;
	}

	snprintf(TmpRankFile, sizeof TmpRankFile, "%s/tmpRank", sdir);

	if ((fptemp = fopen(TmpRankFile, "w")) == NULL) {
		warn("%s: unable to open rank file for updating", __func__);
		fclose(fp);
		return;
	}

	while (fgets(line, MAX_RANK_LINE - 1, fp)) {
		sscanf(line, "%s %d %d %d", login, &sCur.rating, &sCur.num,
		    &comp);

		if (delName != NULL &&
		    !strcasecmp(delName, login)) {	// Kill name.
			delName = NULL;
			continue;
		}

		if (addName != NULL &&
		    CompareStats(addName, sNew, login, &sCur) > 0) {
			int computer = in_list(-1, L_COMPUTER, addName);

			fprintf(fptemp, "%s %d %d %d\n", addName, sNew->rating,
			    sNew->num, computer);
			addName = NULL;
		}

		fprintf(fptemp, "%s %d %d %d\n", login, sCur.rating, sCur.num,
		    comp);
	}

	fclose(fptemp);
	fclose(fp);

	// XXX
#define NASH_CODE 0
#if NASH_CODE
	snprintf(command, sizeof command, "mv %s %s", TmpRankFile, RankFile);
	system(command);
#else
	if (rename(TmpRankFile, RankFile) == -1)
		warn("%s: rename()", __func__);
	UNUSED_VAR(command);
#endif
}

PRIVATE void
DisplayRankHead(int p, int show)
{
	char Line[MAX_STRING_LENGTH] = { '\0' };

	if (CheckFlag(show, SHOW_BLITZ))
		strlcat(Line, "         Blitz          ", sizeof(Line));
	if (CheckFlag(show, SHOW_STANDARD))
		strlcat(Line, "       Standard          ", sizeof(Line));
	if (CheckFlag(show, SHOW_WILD))
		strlcat(Line, "          Wild", sizeof(Line));

	pprintf(p, "%s\n\n", Line);
}

PRIVATE int
CountRankLine(int countComp, char *loginName, int num, int is_computer)
{
	if (loginName == NULL || loginName[0] == '\0')
		return 0;
	return (countComp || !is_computer) && (is_active(num));
}

PRIVATE int
GetRank(FILE *fp, char *target, int countComp)
{
	char	line[MAX_RANK_LINE] = { '\0' };
	char	login[MAX_LOGIN_NAME] = { '\0' };
	int	count = 0;
	int	nGames, is_computer;
	int	playerFound = 0;

	while (fgets(line, MAX_RANK_LINE - 1, fp) &&
	    !playerFound) {
		sscanf(line, "%s %*d %d %d", login, &nGames, &is_computer);

		if ((playerFound = !strcasecmp(login, target)) ||
		    CountRankLine(countComp, login, nGames, is_computer))
			count++;
	}

	return (playerFound ? count : -1);
}

PRIVATE void
PositionFilePtr(FILE *fp, int count, int *last, int *nTied, int showComp)
{
	char	line[MAX_RANK_LINE] = { '\0' };
	char	login[MAX_LOGIN_NAME] = { '\0' };
	int	rating, nGames, is_computer;

	if (fp == NULL)
		return;

	rating = nGames = is_computer = 0;
	rewind(fp);

	for (int i = 1; i < count; i++) {
		do {
			fgets(line, MAX_RANK_LINE - 1, fp);

			if (feof(fp))
				break;
			sscanf(line, "%s %d %d %d", login, &rating, &nGames,
			    &is_computer);
		} while (!CountRankLine(showComp, login, nGames, is_computer));

		if (rating != *last) {
			*nTied = 1;
			*last = rating;
		} else
			(*nTied)++;
	}
}

PRIVATE int
ShowRankEntry(int p, FILE *fp, int count, int comp, char *target,
    int *lastRating, int *nTied)
{
	char	login[MAX_LOGIN_NAME] = { '\0' };
	char	newLine[MAX_RANK_LINE] = { '\0' };
	int	rating, findable, nGames, is_comp;

	// XXX
	rating		= 0;
	findable	= (count > 0 && !feof(fp));
	nGames		= 0;
	is_comp		= 0;

	if (findable) {
		do {
			fgets(newLine, MAX_RANK_LINE - 1, fp);

			if (feof(fp)) {
				findable = 0;
			} else if (newLine[0] != '\0') {
				sscanf(newLine, "%s %d %d %d", login, &rating,
				    &nGames, &is_comp);
			} else {
				login[0] = '\0';
			}
		} while (!CountRankLine(comp, login, nGames, is_comp) &&
		    findable &&
		    strcasecmp(login, target));
	}

	if (findable) {
		if (!strcasecmp(login, target) && !CountRankLine(comp, login,
		    nGames, is_comp)) {
			pprintf_highlight(p, "----  %-12.12s %4s", login,
			    ratstr(rating));
			pprintf(p, "  ");
			return 0;
		} else if (*lastRating == rating && *nTied < 1) {
			pprintf(p, "      ");

			if (!strcasecmp(login, target)) {
				pprintf_highlight(p, "%-12.12s %4s", login,
				    ratstr(rating));
			} else {
				pprintf(p, "%-12.12s %4s", login,
				    ratstr(rating));
			}

			pprintf(p, "  ");
			return 1;
		} else {
			if (*nTied >= 1) {
				if (*lastRating == rating)
					count -= *nTied;
				*nTied = -1;
			}

			if (!strcasecmp(login, target)) {
				pprintf_highlight(p, "%4d. %-12.12s %4s", count,
				    login, ratstr(rating));
			} else {
				pprintf(p, "%4d. %-12.12s %4s", count, login,
				    ratstr(rating));
			}

			pprintf(p, "  ");
			*lastRating = rating;
			return 1;
		}
	} else {
		pprintf(p, "%25s", "");
		return 1;
	}
}

PRIVATE int
CountAbove(int num, int blitz, int std, int wild, int which)
{
	int	max = blitz;

	if (max < std)
		max = std;
	if (max < wild)
		max = wild;
	return (max <= (num + 1) / 2 ? max - 1 : (num + 1) / 2);
}

PRIVATE int
ShowRankLines(int p, FILE *fb, FILE *fs, FILE *fw, int bCount, int sCount,
    int wCount, int n, int showComp, int show, char *target)
{
	int	lastBlitz = 9999, nTiedBlitz = 0;
	int	lastStd = 9999, nTiedStd = 0;
	int	lastWild = 9999, nTiedWild = 0;

	if (n <= 0)
		return 0;

	if (CheckFlag(show, SHOW_BLITZ)) {
		PositionFilePtr(fb, bCount, &lastBlitz, &nTiedBlitz, showComp);

		if (feof(fb))
			ClearFlag(show, SHOW_BLITZ);
	}

	if (CheckFlag(show, SHOW_STANDARD)) {
		PositionFilePtr(fs, sCount, &lastStd, &nTiedStd, showComp);

		if (feof(fs))
			ClearFlag(show, SHOW_STANDARD);
	}

	if (CheckFlag(show, SHOW_WILD)) {
		PositionFilePtr(fw, wCount, &lastWild, &nTiedWild, showComp);

		if (feof(fw))
			ClearFlag(show, SHOW_WILD);
	}

	if (!CheckFlag(show, (SHOW_BLITZ | SHOW_STANDARD | SHOW_WILD)))
		return 0;

	DisplayRankHead(p, show);

	for (int i = 0; i < n && show; i++) {
		if (CheckFlag(show, SHOW_BLITZ)) {
			bCount += ShowRankEntry(p, fb, bCount, showComp, target,
			    &lastBlitz, &nTiedBlitz);
		}
		if (CheckFlag(show, SHOW_STANDARD)) {
			sCount += ShowRankEntry(p, fs, sCount, showComp, target,
			    &lastStd, &nTiedStd);
		}
		if (CheckFlag(show, SHOW_WILD)) {
			wCount += ShowRankEntry(p, fw, wCount, showComp, target,
			    &lastWild, &nTiedWild);
		}
		pprintf(p, "\n");
	}

	return 1;
}

PUBLIC int
DisplayTargetRank(int p, char *target, int show, int showComp)
{
	FILE	*fb = NULL;
	FILE	*fs = NULL;
	FILE	*fw = NULL;
	char	 Path[MAX_FILENAME_SIZE] = { '\0' };
	int	 numAbove;
	int	 numToShow = 20;
	int	 blitzRank = -1, blitzCount;
	int	 stdRank = -1, stdCount;
	int	 wildRank = -1, wildCount;

	if (CheckFlag(show, SHOW_BLITZ)) {
		GetRankFileName(Path, sizeof Path, TYPE_BLITZ);

		if ((fb = fopen(Path, "r")) != NULL)
			blitzRank = GetRank(fb, target, showComp);
		if (blitzRank < 0)
			ClearFlag(show, SHOW_BLITZ);
	}

	if (CheckFlag(show, SHOW_STANDARD)) {
		GetRankFileName(Path, sizeof Path, TYPE_STAND);

		if ((fs = fopen(Path, "r")) != NULL)
			stdRank = GetRank(fs, target, showComp);
		if (stdRank < 0)
			ClearFlag(show, SHOW_STANDARD);
	}

	if (CheckFlag(show, SHOW_WILD)) {
		GetRankFileName(Path, sizeof Path, TYPE_WILD);

		if (CheckFlag(show, SHOW_WILD))
			fw = fopen(Path, "r");
		if (fw != NULL)
			wildRank = GetRank(fw, target, showComp);
		if (wildRank < 0)
			ClearFlag(show, SHOW_WILD);
	}

	if (!CheckFlag(show, (SHOW_BLITZ | SHOW_STANDARD | SHOW_WILD))) {
		pprintf(p, "No ratings to show.\n");

		if (fb != NULL)
			fclose(fb);
		if (fs != NULL)
			fclose(fs);
		if (fw != NULL)
			fclose(fw);
		return 0;
	}

	numAbove = CountAbove(numToShow, blitzRank, stdRank, wildRank, show);

	blitzCount	= (blitzRank - numAbove);
	stdCount	= (stdRank - numAbove);
	wildCount	= (wildRank - numAbove);

	ShowRankLines(p, fb, fs, fw, blitzCount, stdCount, wildCount, numToShow,
	    showComp, show, target);

	if (fb != NULL)
		fclose(fb);
	if (fs != NULL)
		fclose(fs);
	if (fw != NULL)
		fclose(fw);
	return 1;
}

PUBLIC int
DisplayRankedPlayers(int p, int start, int end, int show, int showComp)
{
	FILE	*fb = NULL;
	FILE	*fs = NULL;
	FILE	*fw = NULL;
	char	 Path[MAX_FILENAME_SIZE] = { '\0' };
	int	 num = (end - start + 1);

	if (start <= 0)
		start = 1;
	if (num <= 0)
		return 0;
	if (num > 100)
		num = 100;

	if (CheckFlag(show, SHOW_BLITZ)) {
		GetRankFileName(Path, sizeof Path, TYPE_BLITZ);

		if ((fb = fopen(Path, "r")) == NULL)
			ClearFlag(show, SHOW_BLITZ);
	}
	if (CheckFlag(show, SHOW_STANDARD)) {
		GetRankFileName(Path, sizeof Path, TYPE_STAND);

		if ((fs = fopen(Path, "r")) == NULL)
			ClearFlag(show, SHOW_STANDARD);
	}
	if (CheckFlag(show, SHOW_WILD)) {
		GetRankFileName(Path, sizeof Path, TYPE_WILD);

		if ((fw = fopen(Path, "r")) == NULL)
			ClearFlag(show, SHOW_WILD);
	}

	ShowRankLines(p, fb, fs, fw, start, start, start, num, showComp, show,
	    "");

	if (fb)
		fclose(fb);
	if (fs)
		fclose(fs);
	if (fw)
		fclose(fw);
	return 1;
}

PUBLIC int
ShowFromString(char *s)
{
	int	len = strlen(s ? s : "");
	int	show = 0;

	if (s == NULL || s[0] == '\0')
		return (SHOW_BLITZ | SHOW_STANDARD | SHOW_WILD);

	for (int i = 0; i < len; i++) {
		switch (s[i]) {
		case 'b':
			SetFlag(show, SHOW_BLITZ);
			break;
		case 's':
			SetFlag(show, SHOW_STANDARD);
			break;
		case 'w':
			SetFlag(show, SHOW_WILD);
			break;
		}
	}

	return show;
}

PUBLIC int
Best(int p, param_list param, int ShowComp)
{
	int show = (SHOW_BLITZ | SHOW_STANDARD | SHOW_WILD);

	if (param[0].type != TYPE_NULL)
		show = ShowFromString(param[0].val.word);

	DisplayRankedPlayers(p, 1, 20, show, ShowComp);
	return COM_OK;
}

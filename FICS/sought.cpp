// SPDX-FileCopyrightText: 2024 Markus Uhlin <maxxe@rpblc.net>
// SPDX-License-Identifier: ISC

#include "stdinclude.h"
#include "common.h"

#include "sought.h"

/*
 * Usage: sought [all]
 *
 * The "sought" command can be used in two ways: (a) typing "sought
 * all" will display all current ads including your own; (b) typing
 * "sought" alone will display only those current ads for which you
 * are eligible based on any formula you might have (default). An
 * example output is as follows:
 *
 *     0 1900 Hawk        blitz      5  0 rated           1800-2000 f
 *     1 1700 Friar       wild7      2 12 unrated [white]    0-9999
 *     4 1500 loon        standard   5  0 unrated            0-9999 m
 *
 * The various columns have this information:
 *
 *     Ad index number
 *     Player's rating
 *     Player's handle
 *     Type of chess match
 *     Time at start
 *     Increment per move
 *     Rated/unrated
 *     Color (if specified)
 *     Rating range
 *     Auto start/manual start and whether formula will be checked
 */
PUBLIC int
com_sought(int p, param_list param)
{
	UNUSED_PARAM(p);
	UNUSED_PARAM(param);
	return COM_OK;
}

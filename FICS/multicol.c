/* multicol.c
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
   Richard Nash                 93/10/22	Created
   Markus Uhlin                 23/12/20	Revised
*/

#include "stdinclude.h"
#include "common.h"

#include <err.h>

#include "multicol.h"
#include "rmalloc.h"
#include "utils.h"

PUBLIC multicol *
multicol_start(int maxArray)
{
	multicol *m;

	m = rmalloc(sizeof(multicol));
	m->arraySize	= maxArray;
	m->num		= 0;
	m->strArray	= reallocarray(NULL, sizeof(char *), m->arraySize);

	if (m->strArray == NULL)
		err(1, "%s: reallocarray", __func__);
	else
		malloc_count++;

	for (int i = 0; i < m->arraySize; i++)
		m->strArray[i] = NULL;
	return m;
}

PUBLIC int
multicol_store(multicol *m, char *str)
{
	if (m == NULL || str == NULL || m->num >= m->arraySize)
		return -1;
	m->strArray[m->num] = xstrdup(str);
	m->num++;
	return 0;
}

PUBLIC int
multicol_store_sorted(multicol *m, char *str)
{	// Use this instead of multicol_store() to print a list sorted.
	int found = 0;

	if (m == NULL || str == NULL || m->num >= m->arraySize)
		return -1;

	for (int i = m->num; i > 0 && !found; i--) {
		if (strcasecmp(str, m->strArray[i - 1]) >= 0) {
			found = 1;
			m->strArray[i] = xstrdup(str);
		} else {
			m->strArray[i] = m->strArray[i - 1];
		}
	}

	if (!found)
		m->strArray[0] = xstrdup(str);

	m->num++;
	return 0;
}

PUBLIC int
multicol_pprint(multicol *m, int player, int cols, int space)
{
	char	*tempptr;
	int	 done;
	int	 i;
	int	 maxWidth = 0;
	int	 numLines;
	int	 numPerLine;
	int	 on, theone, len;
	int	 temp;

	pprintf(player, "\n");

	for (i = 0; i < m->num; i++) {
		tempptr = m->strArray[i];
		temp = strlen(tempptr); // loon: yes, this is pathetic

		for (; *tempptr; tempptr++) {
			if (*tempptr == '\033')
				temp -= 4;
		}

		if (temp > maxWidth)
			maxWidth = temp;
	}

	maxWidth += space;

	numPerLine	= (cols / maxWidth);
	numLines	= (m->num / numPerLine);

	if ((numLines * numPerLine) < m->num)
		numLines++;

	on = 0;
	done = 0;

	while (!done) {
		for (i = 0; i < numPerLine; i++) {
			if ((theone = on + numLines * i) >= m->num)
				break;

			tempptr = m->strArray[theone];
			temp = strlen(tempptr);   // loon: yes, still pathetic

			for (; *tempptr; tempptr++) {
				if (*tempptr == '\033')
					temp -= 4;
			}

			len = maxWidth - temp;

			if (i == (numPerLine - 1))
				len -= space;

			pprintf(player, "%s", m->strArray[theone]);

			while (len) {
				pprintf(player, " ");
				len--;
			}
		}

		pprintf(player, "\n");
		on += 1;

		if (on >= numLines)
			break;
	}

	return 0;
}

PUBLIC int
multicol_end(multicol *m)
{
	for (int i = 0; i < m->num; i++)
		rfree(m->strArray[i]);
	rfree(m->strArray);
	rfree(m);
	return 0;
}

/* channel.c
 *
 */

/* THIS SOURCE FILE IS NOW OBSOLETE - DAV */

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

#include "stdinclude.h"

#include "common.h"
#include "channel.h"
#include "network.h"
#include "rmalloc.h"
#include "playerdb.h"

#if 0   /* ARGH!!! 256 * 256 * 4  = 256K! */
/* old cheesy (as loon would put it :) ) code removed */
PUBLIC int *channels[MAX_CHANNELS];
PUBLIC int numOn[MAX_CHANNELS];

PUBLIC void channel_init()
{
  int i;
  for (i = 0; i < MAX_CHANNELS; i++) {
    channels[i] = rmalloc(max_connections * sizeof(int));
    numOn[i] = 0;
  }
}

PUBLIC int on_channel(int ch, int p)
{
  int i;

  for (i = 0; i < numOn[ch]; i++)
    if (p == channels[ch][i])
      return 1;
  return 0;
}

PUBLIC int channel_remove(int ch, int p)
{
  int i, found;

  found = -1;
  for (i = 0; i < numOn[ch] && found < 0; i++)
    if (p == channels[ch][i])
      found = i;
  if (found < 0)
    return 1;
  for (i = found; i < numOn[ch] - 1; i++)
    channels[ch][i] = channels[ch][i + 1];
  numOn[ch] = numOn[ch] - 1;
  --parray[p].nochannels;
  return 0;
}

PUBLIC int channel_add(int ch, int p)
{
  if (numOn[ch] >= MAX_CHANNELS)
    return 1;
  if (on_channel(ch, p))
    return 2;
  if ((parray[p].nochannels == MAX_INCHANNELS) && (parray[p].adminLevel == 0)) {
    return 3;
  }
  channels[ch][numOn[ch]] = p;
  numOn[ch]++;
  parray[p].nochannels++;
  return 0;
}

/* this piece of replacement code appears in talkproc.c */
PUBLIC int on_channel(int ch, int p)
{
 char tmp[10];  /* 9 digits ought to be enough :) */
 
 sprintf (tmp,"%d",ch);
 return in_list(p, L_CHANNEL,ch )  /* since needs ch converted to a string keep
					hidden from view */
}

#endif

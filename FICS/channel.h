/* channel.h
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

#ifndef _CHANNEL_H
#define _CHANNEL_H

#define MAX_CHANNELS 256

extern int *channels[MAX_CHANNELS];
extern int numOn[MAX_CHANNELS];

extern void channel_init(void);
extern int on_channel(int, int);
extern int channel_remove(int, int);
extern int channel_add(int, int);

#endif /* _CHANNEL_H */

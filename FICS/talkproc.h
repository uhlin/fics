/* talkproc.h
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
   name				yy/mm/dd	Change
   hersco and Marsalis		95/07/24	Created
   Markus Uhlin			23/12/10	Deleted declarations
   Markus Uhlin			23/12/10	Sorted the declarations
*/

#ifndef _TALKPROC_H
#define _TALKPROC_H

#include "command.h" /* param_list */

#define MAX_CHANNELS 256

extern int quota_time;

extern int com_clearmessages();
extern int com_cshout();
extern int com_inchannel(int, param_list);
extern int com_it();
extern int com_kibitz();
extern int com_mailmess(int, param_list);
extern int com_messages();
extern int com_ptell();
extern int com_qtell();
extern int com_say();
extern int com_sendmessage();
extern int com_shout();
extern int com_tell();
extern int com_whisper();
extern int com_xtell();
extern int com_znotify();
extern int on_channel(int, int);

#endif /* _TALKPROC_H */

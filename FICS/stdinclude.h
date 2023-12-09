/* stdinclude.h
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

#ifndef _STDINCLUDE_H
#define _STDINCLUDE_H

#include <sys/types.h>

/* Set up system specific defines */
#if defined(SYSTEM_NEXT)

#define HASMALLOCSIZE
#include <sys/vfs.h>

#elif defined(SYSTEM_ULTRIX)

#include <sys/param.h>
#include <sys/mount.h>

#endif

#ifdef SYSTEM_USL
# define NO_TM_ZONE
#endif

#ifdef SYSTEM_SUN4
# define USE_VARARGS
#endif

#ifdef SGI
#define _BSD_SIGNALS
#include <fcntl.h>
#include <bstring.h>
#endif

#if defined(SYSTEM_RS6K)
#include <sys/select.h>
#include <dirent.h>
#define USE_DIRENT
#endif

/* These are included into every .c file */
#if defined(SYSTEM_SUN5)
#define USE_RLIMIT
#define USE_TIMES
#define USE_WAITPID
#define GOOD_STDIO
#define NO_TM_ZONE
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/filio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define direct dirent

#else
#include <strings.h>
#include <sys/dir.h>
#endif

#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/errno.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

extern size_t malloc_size(void *ptr); /* XXX */

#endif /* _STDINCLUDE_H */

/* network.h
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
   Markus Uhlin                 23/12/13	Cleaned up the file
*/

#ifndef _NETWORK_H
#define _NETWORK_H

#include <stdbool.h>

#include "command.h"	/* For MAX_STRING_LENGTH */

#define NET_NETERROR		0
#define NET_NEW			1
#define NET_DISCONNECT		2
#define NET_READLINE		3
#define NET_TIMEOUT		4
#define NET_NOTCOMPLETE		5

#define LINE_WIDTH	80

#ifndef O_NONBLOCK
#define O_NONBLOCK	00004
#endif

#define NETSTAT_EMPTY		0
#define NETSTAT_CONNECTED	1
#define NETSTAT_IDENT		2

typedef struct _connection {
	int		fd;
	int		outFd;
	int		status;
	unsigned int	fromHost;

#ifdef TIMESEAL
	char	sys[512];
	char	user[512];
	int	time;
	int	timeseal;
#endif

	/* Input buffering */
	int		numPending;
	int		processed;
	unsigned char	inBuf[MAX_STRING_LENGTH];

	/* Output buffering */
	char	*sndbuf; /* our send buffer, or NULL if none yet */
	int	 outPos; /* column count */
	int	 sndbufpos; /* position in send buffer */
	int	 sndbufsize; /* size of send buffer (this changes) */
	int	 state; /* 'telnet state' */

	/* identd stuff */
	char	ident[20];
	int	mypal;
} connection;

extern connection con[512];

extern int	 no_file;
extern int	 max_connections;

extern int	 findConnection(int);
extern int	 net_addConnection(int, unsigned int);
extern int	 net_consize(void);
extern int	 net_init(int);
extern int	 net_send_string(int, char *, int);
extern int	 readline2(comstr_t *, int);
extern bool	 net_connected_host(int, unsigned int *);
extern void	 net_close(void);
extern void	 net_close_connection(int);
extern void	 ngc2(comstr_t *, int);
extern void	 turn_echo_off(int);
extern void	 turn_echo_on(int);
#endif    /* _NETWORK_H */

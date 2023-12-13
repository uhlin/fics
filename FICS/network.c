/* network.c
 *
 */

#include "stdinclude.h"

#include <sys/socket.h>

#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <netinet/in.h>

#include "common.h"
#include "config.h"
#include "ficsmain.h"
#include "network.h"
#include "playerdb.h"
#include "rmalloc.h"
#ifdef TIMESEAL
#include "timeseal.h"
#endif
#include "utils.h"

extern int errno;

PRIVATE int sockfd = 0;		/* The socket */
PRIVATE int numConnections = 0;
/* Sparse array */
PUBLIC connection con[512];

PUBLIC int no_file;
PUBLIC int max_connections;

/* Index == fd, for sparse array, quick lookups! wasted memory :( */
PUBLIC int findConnection(int fd)
{
  if (con[fd].status == NETSTAT_EMPTY)
    return -1;
  else
    return fd;
}

PUBLIC int net_addConnection(int fd, unsigned int fromHost)
{
  int noblock = 1;

  if (findConnection(fd) >= 0) {
    fprintf(stderr, "FICS: FD already in connection table!\n");
    return -1;
  }
  if (numConnections >= max_connections)
    return -1;
  if (ioctl(fd, FIONBIO, &noblock) == -1) {
    fprintf(stderr, "Error setting nonblocking mode errno=%d\n", errno);
  }
  con[fd].fd = fd;
  if (fd != 0)
    con[fd].outFd = fd;
  else
    con[fd].outFd = 1;
  con[fd].fromHost = fromHost;
  con[fd].status = NETSTAT_CONNECTED;
#ifdef TIMESEAL
  con[fd].user[0]='\0';
  con[fd].sys[0]='\0';
  con[fd].timeseal = 0;
  con[fd].time = 0;
#endif
  con[fd].numPending = 0;
  con[fd].processed = 0;
  con[fd].outPos = 0;
  if (con[fd].sndbuf == NULL) {
#ifdef DEBUG
    fprintf(stderr, "FICS: nac(%d) allocating sndbuf.\n", fd);
#endif
    con[fd].sndbufpos = 0;
    con[fd].sndbufsize = MAX_STRING_LENGTH;
    con[fd].sndbuf = rmalloc(MAX_STRING_LENGTH);
  } else {
#ifdef DEBUG
    fprintf(stderr, "FICS: nac(%d) reusing old sndbuf size %d pos %d.\n", fd, con[fd].sndbufsize, con[fd].sndbufpos);
#endif
  }
  con[fd].state = 0;
  numConnections++;

#ifdef DEBUG
  fprintf(stderr, "FICS: fd: %d connections: %d  descriptors: %d \n", fd, numConnections, getdtablesize());	/* sparky 3/13/95 */
#endif

  return 0;
}

PRIVATE int remConnection(int fd)
{
  int which;
  if ((which = findConnection(fd)) < 0) {
    return -1;
  }
  numConnections--;
  con[fd].status = NETSTAT_EMPTY;
  if (con[fd].sndbuf == NULL) {
    fprintf(stderr, "FICS: remcon(%d) SNAFU, this shouldn't happen.\n", fd);
  } else {
    if (con[fd].sndbufsize > MAX_STRING_LENGTH) {
      con[fd].sndbufsize = MAX_STRING_LENGTH;
      con[fd].sndbuf = rrealloc(con[fd].sndbuf, MAX_STRING_LENGTH);
    }
    if (con[fd].sndbufpos) {	/* didn't send everything, bummer */
      con[fd].sndbufpos = 0;
    }
  }
  return 0;
}

PRIVATE void net_flushme(int which)
{
  int sent;

  sent = send(con[which].outFd, con[which].sndbuf, con[which].sndbufpos, 0);
  if (sent == -1) {
    if (errno != EPIPE)		/* EPIPE = they've disconnected */
      fprintf(stderr, "FICS: net_flushme(%d) couldn't send, errno=%d.\n", which, errno);
    con[which].sndbufpos = 0;
  } else {
    con[which].sndbufpos -= sent;
    if (con[which].sndbufpos)
      memmove(con[which].sndbuf, con[which].sndbuf + sent, con[which].sndbufpos);
  }
  if (con[which].sndbufsize > MAX_STRING_LENGTH && con[which].sndbufpos < MAX_STRING_LENGTH) {
    /* time to shrink the buffer */
    con[which].sndbuf = rrealloc(con[which].sndbuf, MAX_STRING_LENGTH);
    con[which].sndbufsize = MAX_STRING_LENGTH;
  }
}

PRIVATE void
net_flush_all_connections(void)
{
	fd_set		 writefds;
	int		 which;
	struct timeval	 to;

	FD_ZERO(&writefds);

	for (which = 0; which < MAX_PLAYER; which++) {
		if (con[which].status == NETSTAT_CONNECTED &&
		    con[which].sndbufpos)
			FD_SET(con[which].outFd, &writefds);
	}

	to.tv_usec = 0;
	to.tv_sec = 0;

	select(no_file, NULL, &writefds, NULL, &to);

	for (which = 0; which < MAX_PLAYER; which++) {
		if (FD_ISSET(con[which].outFd, &writefds))
			net_flushme(which);
	}
}

PRIVATE void
net_flush_connection(int fd)
{
	fd_set		 writefds;
	int		 which;
	struct timeval	 to;

	if ((which = findConnection(fd)) >= 0 && con[which].sndbufpos) {
		FD_ZERO(&writefds);
		FD_SET(con[which].outFd, &writefds);

		to.tv_usec = 0;
		to.tv_sec = 0;

		select(no_file, NULL, &writefds, NULL, &to);

		if (FD_ISSET(con[which].outFd, &writefds))
			net_flushme(which);
	}
}

PRIVATE int
sendme(int which, char *str, int len)
{
	fd_set		 writefds;
	int		 i, count;
	struct timeval	 to;

	count = len;

	while ((i = ((con[which].sndbufsize - con[which].sndbufpos) < len) ?
	    (con[which].sndbufsize - con[which].sndbufpos) : len) > 0) {
		memmove(con[which].sndbuf + con[which].sndbufpos, str, i);
		con[which].sndbufpos += i;

		if (con[which].sndbufpos == con[which].sndbufsize) {
			FD_ZERO(&writefds);
			FD_SET(con[which].outFd, &writefds);

			to.tv_usec	= 0;
			to.tv_sec	= 0;

			select(no_file, NULL, &writefds, NULL, &to);

			if (FD_ISSET(con[which].outFd, &writefds)) {
				net_flushme(which);
			} else {
				// time to grow the buffer
				con[which].sndbufsize += MAX_STRING_LENGTH;
				con[which].sndbuf = rrealloc(con[which].sndbuf,
				    con[which].sndbufsize);
			}
		}

		str += i;
		len -= i;
	}

	return count;
}

/*
 * Put LF after every CR and put '\' at the end of overlength lines.
 *
 * Doesn't send anything unless the buffer fills and output waits
 * until flushed.
 *
 * '-1' for an error other than 'EWOULDBLOCK'.
 */
PUBLIC int
net_send_string(int fd, char *str, int format)
{
	int which, i, j;

	if ((which = findConnection(fd)) < 0)
		return -1;
	while (*str) {
		for (i = 0; str[i] >= ' '; i++) {
			/* null */;
		}

		if (i) {
			if (format &&
			    (i >= (j = LINE_WIDTH - con[which].outPos))) {
				// word wrap

				i = j;

				while (i > 0 && str[i - 1] != ' ')
					i--;
				while (i > 0 && str[i - 1] == ' ')
					i--;
				if (i == 0)
					i = j - 1;
				sendme(which, str, i);
				sendme(which, "\n\r\\   ", 6);
				con[which].outPos = 4;

				while (str[i] == ' ') {	// eat the leading
							// spaces after we wrap
					i++;
				}
			} else {
				sendme(which, str, i);
				con[which].outPos += i;
			}
			str += i;
		} else { // non-printable stuff handled here
			switch (*str) {
			case '\t':
				sendme(which, "        ",
				    8 - (con[which].outPos & 7));
				con[which].outPos &= ~7;
				if (con[which].outPos += 8 >= LINE_WIDTH)
					con[which].outPos = 0;
				break;
			case '\n':
				sendme(which, "\n\r", 2);
				con[which].outPos = 0;
				break;
			case '\033':
				con[which].outPos -= 3;
			default:
				sendme(which, str, 1);
			}
			str++;
		}
	}
	return 0;
}

/*
 * A) if we get a complete line (something terminated by '\n'), copy it
 * to com and return 1.
 *
 * B) if we don't get a complete line, but there is no error, return 0.
 *
 * C) if some error, return -1.
 */
PUBLIC int
readline2(char *com, int who)
{
	int		 howmany, state, fd, pending;
	unsigned char	*start, *s, *d;

	static unsigned char	ayt[] = "[Responding to AYT: Yes, I'm here.]\n";
	static unsigned char	will_sga[] = { IAC, WILL, TELOPT_SGA, '\0' };
	static unsigned char	will_tm[] = { IAC, WILL, TELOPT_TM, '\0' };

	state = con[who].state;

	if (state == 2 || state > 4) {
		fprintf(stderr, "FICS: state screwed for con[%d], "
		    "this is a bug.\n", who);
		state = 0;
	}

	s = start = con[who].inBuf;
	pending = con[who].numPending;
	fd = con[who].fd;

	if ((howmany = recv(fd, start + pending, MAX_STRING_LENGTH - 1 -
	    pending, 0)) == 0) { // error: they've disconnected
		return -1;
	} else if (howmany == -1) {
		if (errno != EWOULDBLOCK) { // some other error
			return -1;
		} else if (con[who].processed) { // nothing new and nothing old
			return 0;
		} else {	// nothing new
				// but some unprocessed old
			howmany = 0;
		}
	}

	if (con[who].processed)
		s += pending;
	else
		howmany += pending;
	d = s;

	for (; howmany-- > 0; s++) {
		switch (state) {
		case 0:	// haven't skipped over any control chars or
			// telnet commands
			if (*s == IAC) {
				d = s;
				state = 1;
			} else if (*s == '\n') {
				*s = '\0';
				strcpy(com, start);
				if (howmany)
					bcopy(s + 1, start, howmany);
				con[who].state		= 0;
				con[who].numPending	= howmany;
				con[who].processed	= 0;
				con[who].outPos		= 0;
				return 1;
			} else if (*s > (0xff - 0x20) || *s < 0x20) {
				d = s;
				state = 2;
			}
			break;
		case 1: // got telnet IAC
			if (*s == IP) {
				return -1; // ^C = logout
			} else if (*s == DO) {
				state = 4;
			} else if (*s == WILL || *s == DONT || *s == WONT) {
				state = 3;	// this is cheesy
						// but we aren't using em
			} else if (*s == AYT) {
				send(fd, (char *)ayt, strlen((char *)ayt), 0);
				state = 2;
			} else if (*s == EL) {	// erase line
				d = start;
				state = 2;
			} else {	// dunno what it is
					// so ignore it
				state = 2;
			}
			break;
		case 2:	// we've skipped over something
			// need to shuffle processed chars down
			if (*s == IAC)
				state = 1;
			else if (*s == '\n') {
				*d = '\0';
				strcpy(com, start);
				if (howmany)
					memmove(start, s + 1, howmany);
				con[who].state		= 0;
				con[who].numPending	= howmany;
				con[who].processed	= 0;
				con[who].outPos		= 0;
				return 1;
			} else if (*s >= ' ')
				*(d++) = *s;
			break;
		case 3: // some telnet junk we're ignoring
			state = 2;
			break;
		case 4: // got IAC DO
			if (*s == TELOPT_TM) {
				send(fd, (char *)will_tm,
				    strlen((char *)will_tm), 0);
			} else if (*s == TELOPT_SGA) {
				send(fd, (char *)will_sga,
				    strlen((char *)will_sga), 0);
			}
			state = 2;
			break;
		}
	}

	if (state == 0)
		d = s;
	else if (state == 2)
		state = 0;

	con[who].state = state;
	con[who].numPending = d - start;
	con[who].processed = 1;

	if (con[who].numPending == MAX_STRING_LENGTH - 1) { // buffer full
		*d = '\0';
		strcpy(com, start);
		con[who].state		= 0;
		con[who].numPending	= 0;
		con[who].processed	= 0;
		return 1;
	}
	return 0;
}

PUBLIC int
net_init(int port)
{
	int			 opt;
	struct linger		 lingeropt;
	struct sockaddr_in	 serv_addr;

	/*
	 * Although we have 256 descriptors to work with for opening
	 * files, we can only use 126 for sockets under SunOS 4.x.x
	 * socket libs. Using glibc can get you up to 256 again. Many
	 * OS's can do more than that!
	 * Sparky 9/20/95
	 */

	if ((no_file = getdtablesize()) > MAX_PLAYER + 6)
		no_file = MAX_PLAYER + 6;
	max_connections = no_file - 6;

	for (int i = 0; i < no_file; i++) {
		con[i].status = NETSTAT_EMPTY;
		con[i].sndbuf = NULL;
		con[i].sndbufsize = con[i].sndbufpos = 0;
	}

	/* Open a TCP socket (an Internet stream socket) */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "FICS: can't open stream socket\n");
		return -1;
	}

	/* Bind our local address so that the client can send to us */
	memset(&serv_addr, 0, sizeof serv_addr);
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= htonl(INADDR_ANY);
	serv_addr.sin_port		= htons(port);

	/*
	 * Attempt to allow rebinding to the port...
	 */

	opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof opt);

	opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char *) &opt, sizeof opt);

	lingeropt.l_onoff	= 0;
	lingeropt.l_linger	= 0;
	setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (char *) &lingeropt,
	    sizeof(lingeropt));

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof serv_addr) < 0) {
		fprintf(stderr, "FICS: can't bind local address.  errno=%d\n",
		    errno);
		return -1;
	}

	opt = 1;
	ioctl(sockfd, FIONBIO, &opt);
	listen(sockfd, 5);
	return 0;
}

PUBLIC void
net_close(void)
{
	for (int i = 0; i < no_file; i++) {
		if (con[i].status != NETSTAT_EMPTY)
			net_close_connection(con[i].fd);
	}
}

PUBLIC void
net_close_connection(int fd)
{
	if (con[fd].status == NETSTAT_CONNECTED)
		net_flush_connection(fd);
	if (!remConnection(fd)) {
		if (fd > 2)
			close(fd);
	}
}

PUBLIC void
turn_echo_on(int fd)
{
	static unsigned char wont_echo[] = { IAC, WONT, TELOPT_ECHO, '\0' };
	send(fd, (char *) wont_echo, strlen((char *) wont_echo), 0);
}

PUBLIC void
turn_echo_off(int fd)
{
	static unsigned char will_echo[] = { IAC, WILL, TELOPT_ECHO, '\0' };
	send(fd, (char *) will_echo, strlen((char *) will_echo), 0);
}

PUBLIC unsigned int
net_connected_host(int fd)
{
	int which;

	if ((which = findConnection(fd)) < 0) {
		fprintf(stderr, "FICS: FD not in connection table!\n");
		return -1;
	}
	return con[which].fromHost;
}

PUBLIC void
ngc2(char *com, int timeout)
{
	fd_set			 readfds;
	int			 fd, loop, nfound, lineComplete;
	socklen_t		 cli_len = sizeof(struct sockaddr_in);
	struct sockaddr_in	 cli_addr;
	struct timeval		 to;

	while ((fd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len)) !=
	    -1) {
		if (net_addConnection(fd, cli_addr.sin_addr.s_addr)) {
			fprintf(stderr, "FICS is full.  fd = %d.\n", fd);
			psend_raw_file(fd, mess_dir, MESS_FULL);
			close(fd);
		} else {
			process_new_connection(fd, net_connected_host(fd));
		}
	}

	if (errno != EWOULDBLOCK) {
		fprintf(stderr, "FICS: Problem with accept().  errno=%d\n",
		    errno);
	}
	net_flush_all_connections();

	FD_ZERO(&readfds);
	for (loop = 0; loop < no_file; loop++) {
		if (con[loop].status != NETSTAT_EMPTY)
			FD_SET(con[loop].fd, &readfds);
	}

	to.tv_usec	= 0;
	to.tv_sec	= timeout;

	nfound = select(no_file, &readfds, NULL, NULL, &to);

	/* XXX: unused */
	(void) nfound;

	for (loop = 0; loop < no_file; loop++) {
		if (con[loop].status != NETSTAT_EMPTY) {
			fd = con[loop].fd;

			if ((lineComplete = readline2(com, fd)) == 0) {
				// partial line: do nothing
				continue;
			}

			if (lineComplete > 0) { // complete line: process it
#ifdef TIMESEAL
				if (!parseInput(com, &con[loop]))
					continue;
#endif
				if (process_input(fd, com) != COM_LOGOUT) {
					net_flush_connection(fd);
					continue;
				}
			}

			/*
			 * Disconnect anyone who gets here
			 */
			process_disconnection(fd);
			net_close_connection(fd);
		}
	}
}

PUBLIC int
net_consize(void)
{
	int total = sizeof con;

	for (int i = 0; i < no_file; i++)
		total += con[i].sndbufsize;
	return total;
}

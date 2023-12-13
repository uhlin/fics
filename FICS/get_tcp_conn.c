/*
** Routines to open a TCP connection
**
** New version that supports the old (pre 4.2 BSD) socket calls,
** and systems with the old (pre 4.2 BSD) hostname lookup stuff.
** Compile-time options are:
**
**	OLDSOCKET	- different args for socket() and connect()
**
** Erik E. Fair <fair@ucbarpa.berkeley.edu>
**
*/

/* Uncomment the following line if you are using old socket calls */
/* #define OLDSOCKET */

#include "stdinclude.h"

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "get_tcp_conn.h"
#include "get_tcp_conn.proto.h"

extern int errno;
extern int close();



/*
** Take the name of an internet host in ASCII (this may either be its
** official host name or internet number (with or without enclosing
** backets [])), and return a list of internet addresses.
**
** returns NULL for failure to find the host name in the local database,
** or for a bad internet address spec.
*/
unsigned long **
 name_to_address(char *host)
{
  static u_long *host_addresses[2];
  static u_long haddr;

  if (host == (char *) NULL) {
    return ((u_long **) NULL);
  }
  host_addresses[0] = &haddr;
  host_addresses[1] = (u_long *) NULL;

  /* * Is this an ASCII internet address? (either of [10.0.0.78] or *
     10.0.0.78). We get away with the second test because hostnames * and
     domain labels are not allowed to begin in numbers. * (cf. RFC952,
     RFC882). */
  if (*host == '[' || isdigit(*host)) {
    char namebuf[128];
    register char *cp = namebuf;

    /* * strip brackets [] or anything else we don't want. */
    while (*host != '\0' && cp < &namebuf[sizeof(namebuf)]) {
      if (isdigit(*host) || *host == '.')
	*cp++ = *host++;	/* copy */
      else
	host++;			/* skip */
    }
    *cp = '\0';
    haddr = inet_addr(namebuf);
    return (&host_addresses[0]);
  } else {
    struct hostent *hstp = gethostbyname(host);

    if (hstp == NULL) {
      return ((u_long **) NULL);/* no such host */
    }
    if (hstp->h_length != sizeof(u_long))
      abort();			/* this is fundamental */
    return ((u_long **) hstp->h_addr_list);
  }
}

u_short
gservice(char *serv, char *proto)
{
  if (serv == (char *) NULL || proto == (char *) NULL)
    return ((u_short) 0);

  if (isdigit(*serv)) {
    return (htons((u_short) (atoi(serv))));
  } else {
    struct servent *srvp = getservbyname(serv, proto);

    if (srvp == (struct servent *) NULL)
      return ((u_short) 0);
    return ((u_short) srvp->s_port);
  }
}

/*
** given a host name (either name or internet address) and port number
** give us a TCP connection to the
** requested service at the requested host (or give us FAIL).
*/
int get_tcp_conn(host, port)
char *host;
int port;
{
  register int sock;
  u_long **addrlist;
  struct sockaddr_in sadr;
#ifdef	OLDSOCKET
  struct sockproto sp;

  sp.sp_family = (u_short) AF_INET;
  sp.sp_protocol = (u_short) IPPROTO_TCP;
#endif

  if ((addrlist = name_to_address(host)) == (u_long **) NULL) {
    return (NOHOST);
  }
  sadr.sin_family = (u_short) AF_INET;	/* Only internet for now */
  sadr.sin_port = htons(port);

  for (; *addrlist != (u_long *) NULL; addrlist++) {
    bcopy((caddr_t) * addrlist, (caddr_t) & sadr.sin_addr,
	  sizeof(sadr.sin_addr));

#ifdef	OLDSOCKET
    if ((sock = socket(SOCK_STREAM, &sp, (struct sockaddr *) NULL, 0)) < 0)
      return (FAIL);

    if (connect(sock, (struct sockaddr *) & sadr) < 0) {
#else
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
      return (FAIL);

    if (connect(sock, (struct sockaddr *) & sadr, sizeof(sadr)) < 0) {
#endif
      int e_save = errno;

      fprintf(stderr, "%s [%s] port %d: %d\n", host,
	      inet_ntoa(sadr.sin_addr), port, errno);
      (void) close(sock);	/* dump descriptor */
      errno = e_save;
    } else
      return (sock);
  }
  return (FAIL);
}

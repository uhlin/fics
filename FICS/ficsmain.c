/* ficsmain.c
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
   Markus Uhlin                 23/12/10	Fixed the includes
*/

#include "stdinclude.h"

#include "board.h"
#include "command.h"
#include "common.h"
#include "comproc.h"
#include "config.h"
#ifndef IGNORE_ECO
#include "eco.h"
#endif
#include "ficsmain.h"
#include "network.h"
#include "playerdb.h"
#include "ratings.h"
#include "shutdown.h"
#include "talkproc.h"
#include "utils.h"

#include <sys/param.h>

/* Arguments */
PUBLIC int port;
PUBLIC int withConsole;

/* grimm */
void player_array_init(void);
/* for warning */

PRIVATE void usage(char *progname)
{
  fprintf(stderr, "Usage: %s [-p port] [-C] [-h]\n", progname);
  fprintf(stderr, "\t\t-p port\t\tSpecify port.  (Default=5000)\n");
  fprintf(stderr, "\t\t-C\t\tStart with console player connected to stdin.\n");
  fprintf(stderr, "\t\t-h\t\tDisplay this information.\n");
  exit(1);
}

PRIVATE void GetArgs(int argc, char *argv[])
{
  int i;

  port = DEFAULT_PORT;
  withConsole = 0;

  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
      case 'p':
	if (i == argc - 1)
	  usage(argv[0]);
	i++;
	if (sscanf(argv[i], "%d", &port) != 1)
	  usage(argv[0]);
	break;
      case 'C':
	fprintf(stderr, "-C Not implemented!\n");
	exit(1);
	withConsole = 1;
	break;
      case 'h':
	usage(argv[0]);
	break;
      }
    } else {
      usage(argv[0]);
    }
  }
}

PRIVATE void main_event_loop(void) {
int current_socket;
char command_string[MAX_STRING_LENGTH];

  while (1) {
    ngc2(command_string, HEARTBEATTIME);
    if (process_heartbeat(&current_socket) == COM_LOGOUT) {
      process_disconnection(current_socket);
      net_close_connection(current_socket);
    }
  }
}

void TerminateServer(int sig)
{
  fprintf(stderr, "FICS: Got signal %d\n", sig);
  output_shut_mess();
  TerminateCleanup();
  net_close();
  exit(1);
}

void BrokenPipe(int sig)
{
  fprintf(stderr, "FICS: Got Broken Pipe\n");
}

PUBLIC int main(int argc, char *argv[])
{

#ifdef DEBUG
#ifdef HAVE_MALLOC_DEBUG
  malloc_debug(16);
#endif
#endif
  GetArgs(argc, argv);
  signal(SIGTERM, TerminateServer);
  signal(SIGINT, TerminateServer);
  signal(SIGPIPE, BrokenPipe);
  if (net_init(port)) {
    fprintf(stderr, "FICS: Network initialize failed on port %d.\n", port);
    exit(1);
  }
  startuptime = time(0);
  strcpy ( fics_hostname, SERVER_HOSTNAME );
  quota_time = 60;
  player_high = 0;
  game_high = 0;
  srand(startuptime);
  fprintf(stderr, "FICS: Initialized on port %d at %s.\n", port, strltime(&startuptime));
  fprintf(stderr, "FICS: commands_init()\n");
  commands_init();
  fprintf(stderr, "FICS: rating_init()\n");
  rating_init();
  fprintf(stderr, "FICS: wild_init()\n");
  wild_init();
#ifndef IGNORE_ECO
  fprintf(stderr, "FICS: book init()\n");
  BookInit();
#endif
  fprintf(stderr, "FICS: player_array_init()\n");
  player_array_init();
  player_init(withConsole);
  main_event_loop();
  fprintf(stderr, "FICS: Closing down.\n");
  output_shut_mess();
  net_close();
  return 0;
}

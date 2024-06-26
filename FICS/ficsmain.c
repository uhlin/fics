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
   Markus Uhlin                 23/12/12	Revamped the file
   Markus Uhlin                 24/03/16	Fixed unbounded string copying
						and marked usage() '__dead'.
   Markus Uhlin                 24/06/01	Added command-line option 'l'
*/

#include "stdinclude.h"
#include "common.h"

#include <sys/param.h>

#include "board.h"
#include "command.h"
#include "comproc.h"
#include "config.h"
#ifndef IGNORE_ECO
#include "eco.h"
#endif
#include "ficsmain.h"
#include "legal.h"
#include "network.h"
#include "playerdb.h"
#include "ratings.h"
#include "shutdown.h"
#include "talkproc.h"
#include "utils.h"

#if __linux__
#include <bsd/string.h>
#endif

/* Arguments */
PUBLIC int	port;
PUBLIC int	withConsole;

PRIVATE __dead void usage(char *);

PRIVATE void
BrokenPipe(int sig)
{
	fprintf(stderr, "FICS: Got Broken Pipe (%d)\n", sig);
}

PRIVATE void
GetArgs(int argc, char *argv[])
{
	port = DEFAULT_PORT;
	withConsole = 0;

	for (int i = 1; i < argc; i++) {
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
			case 'l':
				puts(legalNotice);
				exit(0);
			}
		} else {
			usage(argv[0]);
		}
	}
}

PRIVATE __dead void
TerminateServer(int sig)
{
	fprintf(stderr, "FICS: Got signal %d\n", sig);
	output_shut_mess();
	TerminateCleanup();
	net_close();
	exit(EXIT_FAILURE);
}

PRIVATE void
main_event_loop(void)
{
	char	 command_string[MAX_STRING_LENGTH];
	int	 sockfd = -1;

	while (1) {
		ngc2(command_string, HEARTBEATTIME);

		if (process_heartbeat(&sockfd) == COM_LOGOUT && sockfd != -1) {
			process_disconnection(sockfd);
			net_close_connection(sockfd);
		}

		sockfd = -1;
	}
}

PRIVATE __dead void
usage(char *progname)
{
	fprintf(stderr, "Usage: %s [-p port] [-C] [-h] [-l]\n", progname);
	fprintf(stderr, "\t\t-p port\t\tSpecify port.  (Default=%d)\n",
	    DEFAULT_PORT);
	fprintf(stderr, "\t\t-C\t\tStart with console player connected "
	    "to stdin.\n");
	fprintf(stderr, "\t\t-h\t\tDisplay this information.\n");
	fprintf(stderr, "\t\t-l\t\tDisplay the legal notice and exit.\n");
	exit(EXIT_FAILURE);
}

PUBLIC int
main(int argc, char *argv[])
{
#ifdef DEBUG
#ifdef HAVE_MALLOC_DEBUG
	malloc_debug(16);
#endif
#endif
	GetArgs(argc, argv);

	signal(SIGINT, TerminateServer);
	signal(SIGPIPE, BrokenPipe);
	signal(SIGTERM, TerminateServer);

	if (net_init(port)) {
		fprintf(stderr, "FICS: Network initialize failed on port %d.\n",
		    port);
		return EXIT_FAILURE;
	}

	startuptime = time(NULL);
	strlcpy(fics_hostname, SERVER_HOSTNAME, sizeof fics_hostname);
	game_high = 0;
	player_high = 0;
	quota_time = 60;
	srand(startuptime);

	fprintf(stderr, "FICS: Initialized on port %d at %s.\n", port,
	    strltime(&startuptime));
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
	fprintf(stderr, "FICS: player_init(withConsole=%d)\n", withConsole);
	player_init(withConsole);

	main_event_loop();

	fprintf(stderr, "FICS: Closing down.\n");
	output_shut_mess();
	net_close();

	return EXIT_SUCCESS;
}

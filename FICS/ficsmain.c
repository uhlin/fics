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
   Markus Uhlin                 24/08/03	Added command-line option 'd'
*/

#include "stdinclude.h"
#include "common.h"

#include <sys/param.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#include "board.h"
#include "command.h"
#include "comproc.h"
#include "config.h"
#ifndef IGNORE_ECO
#include "eco.h"
#endif
#include "ficsmain.h"
#include "legal.h"
#include "legal2.h"
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
daemonize(void)
{
	int	fd[2];
	mode_t	mode[2];

	mode[0] = (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	mode[1] = (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (file_exists(DAEMON_LOCKFILE)) {
		errx(1, "%s: %s: already exists\ndelete the file manually and "
		    "try again after making sure that\nno copy of the program "
		    "is already running in the background",
		    __func__,
		    DAEMON_LOCKFILE);
	} else if ((fd[0] = open(DAEMON_LOCKFILE, (O_CREAT | O_RDWR),
	    mode[0])) == -1) {
		err(1, "%s: open(%s, ...)", __func__, DAEMON_LOCKFILE);
	} else if ((fd[1] = open(DAEMON_LOGFILE, (O_APPEND | O_CREAT | O_RDWR),
	    mode[1])) == -1) {
		err(1, "%s: open(%s, ...)", __func__, DAEMON_LOGFILE);
	} else if (daemon(1, 1) == -1) {
		int i;

		i = errno;
		close(fd[0]);
		close(fd[1]);
		errno = i;

		err(1, "%s: failed to run in the background", __func__);
	}

	dprintf(fd[0], "%jd\n", (intmax_t)getpid());
	close(fd[0]);

	(void) dup2(fd[1], STDIN_FILENO);
	(void) dup2(fd[1], STDOUT_FILENO);
	(void) dup2(fd[1], STDERR_FILENO);

	if (fd[1] > 2)
		close(fd[1]);
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
			case 'd':
				daemonize();
				break;
			case 'h':
				usage(argv[0]);
				break;
			case 'l':
				puts(legalNotice);
				puts(legalNotice2);
				exit(0);
			case 'v':
				printf("%s %s\n", VERS_NUM, COMP_DATE);
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
	comstr_t str;
	int sockfd = -1;

	while (1) {
		ngc2(&str, HEARTBEATTIME);

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
	fprintf(stderr, "Usage: %s [-p port] [-C] [-dhl]\n", progname);
	fprintf(stderr, "\t\t-p port\t\tSpecify port.  (Default=%d)\n",
	    DEFAULT_PORT);
	fprintf(stderr, "\t\t-C\t\tStart with console player connected "
	    "to stdin.\n");
	fprintf(stderr, "\t\t-d\t\tRun in the background.\n");
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

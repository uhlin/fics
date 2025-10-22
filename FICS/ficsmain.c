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
   Markus Uhlin                 24/12/04	Added command-line option 'v'
   Markus Uhlin                 25/10/14	Added usage of unveil() and
						pledge() (OpenBSD only).
*/

#include "stdinclude.h"
#include "common.h"

#include <sys/param.h>

#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "board.h"
#include "command.h"
#include "comproc.h"
#include "config.h"
#ifndef IGNORE_ECO
#include "eco.h"
#endif
#include "ficsmain.h"
#include "ficspaths.h"
#include "legal.h"
#include "legal2.h"
#include "network.h"
#include "playerdb.h"
#include "ratings.h"
#include "settings.h"
#include "shutdown.h"
#include "talkproc.h"
#include "utils.h"

#if __linux__
#include <bsd/string.h>
#endif

PUBLIC const int g_open_flags[4] = {
	[OPFL_APPEND] = (O_WRONLY|O_CREAT|O_APPEND),
	[OPFL_WRITE] = (O_WRONLY|O_CREAT|O_TRUNC),
	[OPFL_APLUS] = (O_RDWR|O_CREAT|O_APPEND),
	[OPFL_WPLUS] = (O_RDWR|O_CREAT|O_TRUNC),
};
PUBLIC const mode_t g_open_modes = (S_IWUSR | S_IRUSR);

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

#if defined(OpenBSD) && OpenBSD >= 201811
PRIVATE void
unveil_doit(void)
{
	struct whitelist_tag {
		const char	*path;
		const char	*permissions;
	} whitelist[] = {
#ifdef MAILPROGRAM
		{MAILPROGRAM, "rx"},
#endif
#ifdef SENDMAILPROG
		{SENDMAILPROG, "rx"},
#endif

		{DEFAULT_ADHELP, ""},		// data/admin
		{DEFAULT_ADJOURNED, "rwc"},	// games/adjourned
		{DEFAULT_BOARDS, ""},		// data/boards
		{DEFAULT_BOOK, ""},		// data/book
		{DEFAULT_COMHELP, ""},		// data/com_help
		{DEFAULT_CONFIG, "rwc"},	// config
		{DEFAULT_HELP, ""},		// data/help
		{DEFAULT_HISTORY, "rwc"},	// games/history
		{DEFAULT_INDEX, ""},		// data/index
		{DEFAULT_INFO, ""},		// data/info
		{DEFAULT_JOURNAL, "rwc"},	// games/journal
		{DEFAULT_LISTS, ""},		// data/lists
		{DEFAULT_MESS, ""},		// data/messages
		{DEFAULT_NEWS, ""},		// data/news
		{DEFAULT_PLAYERS, ""},		// players
		{DEFAULT_SOURCE, "r"},		// FICS
		{DEFAULT_STATS, ""},		// data/stats
		{DEFAULT_USAGE, ""},		// data/usage
		{DEFAULT_USCF, ""},		// data/uscf

		{MESS_FULL, "r"},		// data/messages/full
		{MESS_FULL_UNREG, "r"},		// data/messages/full_unreg

		{DAEMON_LOCKFILE, "rw"},	// fics.pid
		{DAEMON_LOGFILE, "rwc"},	// fics.log
	};

	if (unveil(FICS_PREFIX, "rwc") == -1)
		err(1, "unveil");

	for (struct whitelist_tag *wl_p = &whitelist[0];
	    wl_p < &whitelist[ARRAY_SIZE(whitelist)];
	    wl_p++) {
		errno = 0;

		if (strcmp(wl_p->path, "") == 0 ||
		    strcmp(wl_p->permissions, "") == 0)
			continue;
		if (unveil(wl_p->path, wl_p->permissions) == -1 &&
		    errno != ENOENT)
			err(1, "unveil(%s, %s)", wl_p->path, wl_p->permissions);
	}
}
#endif

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
	fprintf(stderr, "Usage: %s [-p port] [-C] [-dhlv]\n", progname);
	fprintf(stderr, "\t\t-p port\t\tSpecify port.  (Default=%d)\n",
	    DEFAULT_PORT);
	fprintf(stderr, "\t\t-C\t\tStart with console player connected "
	    "to stdin.\n");
	fprintf(stderr, "\t\t-d\t\tRun in the background.\n");
	fprintf(stderr, "\t\t-h\t\tDisplay this information.\n");
	fprintf(stderr, "\t\t-l\t\tDisplay the legal notice and exit.\n");
	fprintf(stderr, "\t\t-v\t\tDisplay version.\n");
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

	settings_init();
	settings_read_conf(FICS_SETTINGS);

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

#if defined(OpenBSD) && OpenBSD >= 201811
	unveil_doit();
#endif

#if defined(OpenBSD) && OpenBSD >= 201605
	if (pledge("cpath rpath wpath dns inet stdio tty", NULL) == -1) {
		warn("pledge");
		return EXIT_FAILURE;
	}
#endif

	main_event_loop();

	fprintf(stderr, "FICS: Closing down.\n");
	output_shut_mess();
	net_close();

	return EXIT_SUCCESS;
}

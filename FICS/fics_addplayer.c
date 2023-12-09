/* fics_addplayer.c
 *
 */

/*
    fics - An internet chess server.
    Copyright (C) 1994  Richard V. Nash

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
   Richard Nash	              	94/03/07	Created
*/

#include "stdinclude.h"

#include "command.h"
#include "common.h"
#include "playerdb.h"
#include "utils.h"

#define PASSLEN 4

PRIVATE char *funame = NULL;
PRIVATE char *fname = NULL;
PRIVATE char *email = NULL;

PRIVATE int local = 1;

PRIVATE void
usage(char *progname)
{
	fprintf(stderr, "Usage: %s [-l] [-n] UserName FullName EmailAddress\n",
	    progname);
	exit(1);
}

PUBLIC int
main(int argc, char *argv[])
{
	char	 password[PASSLEN + 1];
	char	 salt[3];
	char	 text[2048];
	int	 i;
	int	 p;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'l':
				local = 1;
				break;
			case 'n':
				local = 0;
				break;
			default:
				usage(argv[0]);
				break;
			}
		} else {
			if (!funame)
				funame = argv[i];
			else if (!fname)
				fname = argv[i];
			else if (!email)
				email = argv[i];
			else
				usage(argv[0]);
		}
	}
	if (!funame || !fname || !email)
		usage(argv[0]);

	/* Add the player here */
	if (strlen(funame) >= MAX_LOGIN_NAME) {
		fprintf(stderr, "Player name is too long\n");
		exit(0);
	}
	if (strlen(funame) < 3) {
		fprintf(stderr, "Player name is too short\n");
		exit(0);
	}
	if (!alphastring(funame)) {
		fprintf(stderr, "Illegal characters in player name. "
		    "Only A-Za-z allowed.\n");
		exit(0);
	}

	player_init(0);
	srand(time(0));
	p = player_new();
	if (!player_read(p, funame)) {
		fprintf(stderr, "%s already exists.\n", funame);
		exit(0);
	}
	parray[p].name = strdup(funame);
	parray[p].login = strdup(funame);
	stolower(parray[p].login);
	parray[p].fullName = strdup(fname);
	parray[p].emailAddress = strdup(email);
	for (i = 0; i < PASSLEN; i++) {
		password[i] = 'a' + rand() % 26;
	}
	password[i] = '\0';
	salt[0] = 'a' + rand() % 26;
	salt[1] = 'a' + rand() % 26;
	salt[2] = '\0';
	parray[p].passwd = strdup(crypt(password, salt));
	parray[p].registered = 1;
//	parray[p].network_player = !local;
	parray[p].rated = 1;
	player_save(p);

	printf("Added player account: >%s< >%s< >%s< >%s<\n",
	       funame, fname, email, password);

	sprintf(text, "\nYour player account has been created.\n\n"
	    "Login Name: %s\n"
	    "Full Name: %s\n"
	    "Email Address: %s\n"
	    "Initial Password: %s\n\n"

	    "If any of this information is incorrect, please\n"
	    "contact the administrator to get it corrected.\n\n"

	    "You may change your password with the password\n"
	    "command on the server.\n\n"

	    "Please be advised that if this is an unauthorized\n"
	    "duplicate account for you, by using it you take\n"
	    "the risk of being banned from accessing this chess\n"
	    "server.\n\n"

	    "To connect to the server and use this account:\n\n"
	    "\ttelnet %s 5000\n\n"
	    "and enter your handle name and password.\n\n"

	    "Regards,\n\nThe FICS admins\n", funame, fname, email, password,
	    fics_hostname);

	mail_string_to_address(email, "FICS Account Created", text);
	exit(0);
}

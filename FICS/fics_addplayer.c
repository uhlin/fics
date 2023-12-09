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

PRIVATE void
usage(char *progname)
{
	fprintf(stderr, "Usage: %s [-l] [-n] UserName FullName EmailAddress\n",
	    progname);
	exit(1);
}

/* Parameters */
int local = 1;
char *funame = NULL, *fname = NULL, *email = NULL;

PUBLIC int main(int argc, char *argv[])
{
  int i;
  int p;
  char password[PASSLEN + 1];
  char salt[3];
  char text[2048];

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
    fprintf(stderr, "Illegal characters in player name. Only A-Za-z allowed.\n");
    exit(0);
  }

/* loon: see if we can deliver mail to this email address. */
/*
  printf("Verifying email address %s...\n", email);
  if (check_emailaddr(email)) {
    fprintf(stderr, "The email address %s looks bad.\n", email);
    exit(0);
  }
*/
/*
  if (local)
    iamserver = 0;
  else
    iamserver = 1;
*/
/*  if (hostinfo_init()) {
    if (iamserver) {
      fprintf(stderr, "Can't read from hostinfo file.\n");
      fprintf(stderr, "Remember you need -l for local.\n");
      exit(1);
    } else {
    }
  } */
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
/*  parray[p].network_player = !local; */
  parray[p].rated = 1;
  player_save(p);
/*  changed by Sparky  12/15/95  Dropped reference to 'Network' players
    and spiffed it up  */
/*
  printf("Added %s player >%s< >%s< >%s< >%s<\n", local ? "local" : "network",
	 funame, fname, email, password);
  sprintf(text, "\nYou have been added as a %s player.\nIf this is a network account it may take a while to show up on all of the\nclients.\n\nLogin Name: %s\nFull Name: %s\nEmail Address: %s\nInitial Password: %s\n\nIf any of this information is incorrect, please contact the administrator\nto get it corrected.\nPlease write down your password, as it will be your initial passoword\non all of the servers.\n", local ? "local" : "network", funame, fname, email, password);
*/

  printf("Added player account: >%s< >%s< >%s< >%s<\n",
         funame, fname, email, password);

  sprintf(text, "\nYour player account has been created.\n\n"
   "Login Name: %s\nFull Name: %s\nEmail Address: %s\nInitial Password: %s\n\n"
   "If any of this information is incorrect, please contact the administrator\n"
   "to get it corrected.\n\n"
   "You may change your password with the password command on the the server.\n"
   "\nPlease be advised that if this is an unauthorized duplicate account for\n"
   "you, by using it you take the risk of being banned from accessing this\n"
   "chess server.\n\nTo connect to the server and use this account:\n\n"
   "	telnet %s 5000\n\nand enter your handle name and password.\n\n"
   "Regards,\n\nThe FICS admins\n",
        funame, fname, email, password, fics_hostname);

  mail_string_to_address(email, "FICS Account Created", text);
  exit(0);
}

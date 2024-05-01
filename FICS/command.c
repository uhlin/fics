/* command.c
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
   Markus Uhlin                 23/12/17	Fixed compiler warnings
   Markus Uhlin                 23/12/19	Usage of 'time_t'
   Markus Uhlin                 23/12/23	Fixed crypt()
*/

#include "stdinclude.h"
#include "common.h"

#include <sys/param.h>

#include "command.h"
#include "command_list.h"
#include "config.h"
#include "fics_getsalt.h"
#include "ficsmain.h"
#include "gamedb.h"
#include "gameproc.h"
#include "movecheck.h"
#include "network.h"
#include "obsproc.h"
#include "playerdb.h"
#include "ratings.h"
#include "rmalloc.h"
#include "shutdown.h"
#include "utils.h"
#include "vers.h"

PUBLIC char	*adhelp_dir	= DEFAULT_ADHELP;
PUBLIC char	*adj_dir	= DEFAULT_ADJOURNED;
PUBLIC char	*board_dir	= DEFAULT_BOARDS;
PUBLIC char	*comhelp_dir	= DEFAULT_COMHELP;
PUBLIC char	*def_prompt	= DEFAULT_PROMPT;
PUBLIC char	*help_dir[NUM_LANGS] = {
	DEFAULT_HELP,
	HELP_SPANISH,
	HELP_FRENCH,
	HELP_DANISH
};
PUBLIC char	*hist_dir	= DEFAULT_HISTORY;
PUBLIC char	*index_dir	= DEFAULT_INDEX;
PUBLIC char	*info_dir	= DEFAULT_INFO;
PUBLIC char	*journal_dir	= DEFAULT_JOURNAL;
PUBLIC char	*lists_dir	= DEFAULT_LISTS;
PUBLIC char	*mess_dir	= DEFAULT_MESS;
PUBLIC char	*news_dir	= DEFAULT_NEWS;
PUBLIC char	*player_dir	= DEFAULT_PLAYERS;
PUBLIC char	*source_dir	= DEFAULT_SOURCE;
PUBLIC char	*stats_dir	= DEFAULT_STATS;
PUBLIC char	*usage_dir[NUM_LANGS] = {
	DEFAULT_USAGE,
	USAGE_SPANISH,
	USAGE_FRENCH,
	USAGE_DANISH
};
PUBLIC char	*uscf_dir	= DEFAULT_USCF;

PUBLIC char	*hadmin_handle = HADMINHANDLE;
PRIVATE char	*hadmin_email = HADMINEMAIL;
PRIVATE char	*reg_addr = REGMAIL;

PUBLIC char	 fics_hostname[81];
PUBLIC int	 MailGameResult;
PUBLIC int	 game_high;
PUBLIC int	 player_high;
PUBLIC time_t	 startuptime;

/*
 * The player whose command you're in
 */
PUBLIC int commanding_player = -1;

PRIVATE int lastCommandFound = -1;

/*
 * Copies command into 'comm'.
 */
PRIVATE int
parse_command(char *com_string, char **comm, char **parameters)
{
	*comm = com_string;
	*parameters = eatword(com_string);

	if (**parameters != '\0') {
		**parameters = '\0';
		(*parameters)++;
		*parameters = eatwhite(*parameters);
	}

	if (strlen(*comm) >= MAX_COM_LENGTH)
		return COM_BADCOMMAND;
	return COM_OK;
}

PUBLIC int
alias_lookup(char *tmp, alias_type *alias_list, int numalias)
{
	for (int i = 0; (alias_list[i].comm_name && i < numalias); i++) {
		if (!strcmp(tmp, alias_list[i].comm_name))
			return i;
	}
	return -1; /* not found */
}

PUBLIC int
alias_count(alias_type *alias_list)
{
	int i;

	for (i = 0; alias_list[i].comm_name; i++) {
		/* null */;
	}
	return i;
}

/* Puts alias substitution into alias_string */
PRIVATE void alias_substitute(alias_type *alias_list, int num_alias,
			       char *com_str, char outalias[])
{
  char *s = com_str;
  char name[MAX_COM_LENGTH];
  char *t = name;
  int i = 0;
  char *atpos, *aliasval;

  /* Get first word of command, terminated by whitespace or by containing
     punctuation */
  while (*s && !iswhitespace(*s)) {
    if (i++ >= MAX_COM_LENGTH) {
      strcpy(outalias, com_str);
      return;
    }
    if (ispunct(*t++ = *s++))
      break;
  }
  *t = '\0';
  if (*s && iswhitespace(*s))
    s++;

  i = alias_lookup(name, alias_list, num_alias);
  if (i < 0) {
    strcpy(outalias, com_str);
    return;
  }
  aliasval = alias_list[i].alias;

  /* See if alias contains an @ */
  atpos = strchr(aliasval, '@');
  if (atpos != NULL) {
    strncpy(outalias, aliasval, atpos - aliasval);
    outalias[atpos - aliasval] = '\0';
    strcat(outalias, s);
    strcat(outalias, atpos + 1);
  } else {
    strcpy(outalias, aliasval);
    if (*s) {
      strcat(outalias, " ");
      strcat(outalias, s);
    }
  }
}

/* Returns pointer to command that matches */
PRIVATE int match_command(char *comm, int p)
{
  int i = 0;
  int gotIt = -1;
  int len = strlen(comm);

  while (command_list[i].comm_name) {
    if (!strncmp(command_list[i].comm_name, comm, len)
	&& parray[p].adminLevel >= command_list[i].adminLevel) {
      if (gotIt >= 0)
	return -COM_AMBIGUOUS;
      gotIt = i;
    }
    i++;
  }
  if (in_list(p, L_REMOVEDCOM, command_list[gotIt].comm_name)) {
    pprintf(p, "Due to a bug - this command has been temporarily removed.\n");
    return -COM_FAILED;
  }
  if (gotIt >= 0) {
    lastCommandFound = gotIt;
    return gotIt;
  }
  return -COM_FAILED;
}

/*
 * Gets the parameters for this command
 */
PRIVATE int
get_parameters(int command, char *parameters, param_list params)
{
	char		c;
	int		i, parlen;
	int		paramLower;
	static char	punc[2];

	punc[1] = '\0'; // Holds punc parameters

	for (i = 0; i < MAXNUMPARAMS; i++)
		(params)[i].type = TYPE_NULL; // Set all parameters to NULL

	parlen = strlen(command_list[command].param_string);

	for (i = 0; i < parlen; i++) {
		c = command_list[command].param_string[i];

		if (isupper(c)) {
			paramLower = 0;
			c = tolower(c);
		} else {
			paramLower = 1;
		}

		switch (c) {
		case 'w':
		case 'o':	// word or optional word
			parameters = eatwhite(parameters);

			if (!*parameters)
				return (c == 'o' ? COM_OK : COM_BADPARAMETERS);

			(params)[i].val.word = parameters;
			(params)[i].type = TYPE_WORD;

			if (ispunct(*parameters)) {
				punc[0] = *parameters;

				(params)[i].val.word = punc;

				parameters++;

				if (*parameters && iswhitespace(*parameters))
					parameters++;
			} else {
				parameters = eatword(parameters);

				if (*parameters != '\0') {
					*parameters = '\0';
					parameters++;
				}
			}

			if (paramLower)
				stolower((params)[i].val.word);

			break;
		case 'd':
		case 'p':	// optional or required integer
			parameters = eatwhite(parameters);

			if (!*parameters)
				return (c == 'p' ? COM_OK : COM_BADPARAMETERS);

			if (sscanf(parameters, "%d", &(params)[i].val.integer)
			    != 1)
				return COM_BADPARAMETERS;

			(params)[i].type = TYPE_INT;

			parameters = eatword(parameters);

			if (*parameters != '\0') {
				*parameters = '\0';
				parameters++;
			}

			break;
		case 'i':
		case 'n':	// optional or required word or integer
			parameters = eatwhite(parameters);

			if (!*parameters)
				return (c == 'n' ? COM_OK : COM_BADPARAMETERS);

			if (sscanf(parameters, "%d", &(params)[i].val.integer)
			    != 1) {
				(params)[i].val.word = parameters;
				(params)[i].type = TYPE_WORD;
			} else {
				(params)[i].type = TYPE_INT;
			}

			if (ispunct(*parameters)) {
				punc[0] = *parameters;

				(params)[i].val.word = punc;
				(params)[i].type = TYPE_WORD;

				parameters++;

				if (*parameters && iswhitespace(*parameters))
					parameters++;
			} else {
				parameters = eatword(parameters);

				if (*parameters != '\0') {
					*parameters = '\0';
					parameters++;
				}
			}

			if ((params)[i].type == TYPE_WORD)
				if (paramLower)
					stolower((params)[i].val.word);
			break;
		case 's':
		case 't':	// optional or required string to end
			if (!*parameters)
				return (c == 't' ? COM_OK : COM_BADPARAMETERS);

			(params)[i].val.string = parameters;
			(params)[i].type = TYPE_STRING;

			while (*parameters)
				parameters = nextword(parameters);
			if (paramLower)
				stolower((params)[i].val.string);
			break;
		} /* switch */
	} /* for */

	if (*parameters)
		return COM_BADPARAMETERS;
	else
		return COM_OK;
}

PRIVATE void
printusage(int p, char *command_str)
{
	char	*filenames[1000]; // enough for all usage names
	char	 c;
	int	 command;
	int	 i, parlen, UseLang = parray[p].language;

	if ((command = match_command(command_str, p)) < 0) {
		pprintf(p, "  UNKNOWN COMMAND\n");
		return;
	}

	/*
	 * Usage added by DAV 11/19/95.
	 * First lets check if we have a text usage file for it.
	 */

	i = search_directory(usage_dir[UseLang], command_str, filenames,
	    ARRAY_SIZE(filenames));

	if (i == 0) {
		if (UseLang != LANG_DEFAULT) {
			i += search_directory(usage_dir[LANG_DEFAULT],
			    command_str, filenames, ARRAY_SIZE(filenames));

			if (i > 0) {
				pprintf(p, "No usage available in %s; "
				    "using %s instead.\n",
				    Language(UseLang),
				    Language(LANG_DEFAULT));
				UseLang = LANG_DEFAULT;
			}
		}
	}

	if (i != 0) {
		if (i == 1 ||
		    !strcmp(*filenames, command_str)) { // found it?
			if (psend_file(p, usage_dir[UseLang], *filenames)) {
				/*
				 * We should never reach this unless
				 * the file was just deleted.
				 */
				pprintf(p, "Usage file %s could not be found! ",
				    *filenames);
				pprintf(p, "Please inform an admin of this. "
				    "Thank you.\n");

				/*
				 * No need to print 'system' usage -
				 * should never happen.
				 */
			}

			return;
		}
	}

	/*
	 * Print the default 'system' usage files (which aren't much
	 * help!)
	 */
	pprintf(p, "Usage: %s", command_list[lastCommandFound].comm_name);
	parlen = strlen(command_list[command].param_string);

	for (i = 0; i < parlen; i++) {
		c = command_list[command].param_string[i];

		if (isupper(c))
			c = tolower(c);

		switch (c) {
		case 'w':	// word
			pprintf(p, " word");
			break;
		case 'o':	// optional word
			pprintf(p, " [word]");
			break;
		case 'd':	// integer
			pprintf(p, " integer");
			break;
		case 'p':	// optional integer
			pprintf(p, " [integer]");
			break;
		case 'i':	// word or integer
			pprintf(p, " {word, integer}");
			break;
		case 'n':	// optional word or integer
			pprintf(p, " [{word, integer}]");
			break;
		case 's':	// string to end
			pprintf(p, " string");
			break;
		case 't':	// optional string to end
			pprintf(p, " [string]");
			break;
		}
	}

	pprintf(p, "\nSee 'help %s' for a complete description.\n",
	    command_list[lastCommandFound].comm_name);
}

PUBLIC int
process_command(int p, char *com_string, char **cmd)
{
	char		*comm, *parameters;
	int		 which_command, retval;
	param_list	 params;
	static char	 alias_string1[MAX_STRING_LENGTH * 4] = { '\0' };
	static char	 alias_string2[MAX_STRING_LENGTH * 4] = { '\0' };

#ifdef DEBUG
	if (strcasecmp(parray[p].name, parray[p].login)) {
		fprintf(stderr, "FICS: PROBLEM Name=%s, Login=%s\n",
		    parray[p].name,
		    parray[p].login);
	}
#endif

	if (!com_string)
		return COM_FAILED;

#ifdef DEBUG
	fprintf(stderr, "%s, %s, %d: >%s<\n", parray[p].name, parray[p].login,
	    parray[p].socket, com_string);
#endif

	alias_substitute(parray[p].alias_list, parray[p].numAlias, com_string,
	    alias_string1);
	alias_substitute(g_alias_list, 999, alias_string1, alias_string2);

#ifdef DEBUG
	if (strcmp(com_string, alias_string2) != 0) {
		fprintf(stderr, "%s -alias-: >%s<\n", parray[p].name,
		    alias_string2);
	}
#endif

	if ((retval = parse_command(alias_string2, &comm, &parameters)))
		return retval;
	if (is_move(comm))
		return COM_ISMOVE;

	stolower(comm); // All commands are case-insensitive
	*cmd = comm;

	if ((which_command = match_command(comm, p)) < 0)
		return -which_command;
	if (parray[p].adminLevel < command_list[which_command].adminLevel)
		return COM_RIGHTS;

	if ((retval = get_parameters(which_command, parameters, params)))
		return retval;
	return command_list[which_command].comm_func(p, params);
}

PRIVATE int
process_login(int p, char *loginname)
{
	int	problem = 1;

	loginname = eatwhite(loginname);

	if (!*loginname) {
		/* do something in here? */;
	} else {
		char	*loginnameii = xstrdup(loginname);

		stolower(loginname);

		if (!alphastring(loginname)) {
			pprintf(p, "\nSorry, names can only consist of lower "
			    "and upper case letters. Try again.\n");
		} else if (strlen(loginname) < 3) {
			pprintf(p, "\nA name should be at least three "
			    "characters long! Try again.\n");
		} else if (strlen(loginname) > 17) {
			pprintf(p, "\nSorry, names may be at most 17 "
			    "characters long. Try again.\n");
		} else if (in_list(p, L_BAN, loginnameii)) {
			pprintf(p, "\nPlayer \"%s\" is banned.\n", loginname);
			rfree(loginnameii);
			return COM_LOGOUT;
		} else if ((!in_list(p, L_ADMIN, loginnameii)) &&
		    (player_count(0) >= max_connections - 10)) {
			psend_raw_file(p, mess_dir, MESS_FULL);
			rfree(loginnameii);
			return COM_LOGOUT;
		} else {
			problem = 0;

			if (player_read(p, loginname)) {
				strcpy(parray[p].name, loginnameii);

				if (in_list(p, L_FILTER,
				    dotQuad(parray[p].thisHost))) {
					pprintf(p, "\nDue to abusive behavior, "
					    "nobody from your site may login.\n");
					pprintf(p, "If you wish to use this "
					    "server please email %s\n",
					    reg_addr);
					pprintf(p, "Include details of a "
					    "nick-name to be called here, "
					    "e-mail address and your real name."
					    "\n");
					pprintf(p, "We will send a password "
					    "to you. Thanks.\n");
					rfree(loginnameii);
					return COM_LOGOUT;
				}

				if ((player_count(0)) >=
				    MAX(max_connections - 60, 200)) {
					psend_raw_file(p, mess_dir,
					    MESS_FULL_UNREG);
					rfree(loginnameii);
					return COM_LOGOUT;
				}

				pprintf_noformat(p, "\n\"%s\" is not a "
				    "registered name. You may use this name "
				    "to play unrated games.\n(After logging in,"
				    "do \"help register\" for more info on "
				    "how to register.)\n\nPress return to "
				    "enter the FICS as \"%s\":",
				    parray[p].name,
				    parray[p].name);
			} else {
				pprintf_noformat(p, "\n\"%s\" is a registered "
				    "name. If it is yours, type the password.\n"
				    "If not, just hit return to try another "
				    "name.\n\npassword: ", parray[p].name);
			}

			parray[p].status = PLAYER_PASSWORD;
			turn_echo_off(parray[p].socket);
			rfree(loginnameii);

			if (strcasecmp(loginname, parray[p].name)) {
				pprintf(p, "\nYou've got a bad name field in "
				    "your playerfile -- please report this to "
				    "an admin!\n");
				rfree(loginnameii);
				return COM_LOGOUT;
			}

			if (parray[p].adminLevel != 0 &&
			    !in_list(p, L_ADMIN, parray[p].name)) {
				pprintf(p, "\nYou've got a bad playerfile -- "
				    "please report this to an admin!\n");
				pprintf(p, "Your handle is missing!");
				pprintf(p, "Please log on as an unreg until "
				    "an admin can correct this.\n");
				rfree(loginnameii);
				return COM_LOGOUT;
			}

			if (parray[p].registered &&
			    parray[p].fullName == NULL) {
				pprintf(p, "\nYou've got a bad playerfile -- "
				    "please report this to an admin!\n");
				pprintf(p, "Your FullName is missing!");
				pprintf(p, "Please log on as an unreg until "
				    "an admin can correct this.\n");
				rfree(loginnameii);
				return COM_LOGOUT;
			}

			if (parray[p].registered &&
			    parray[p].emailAddress == NULL) {
				pprintf(p, "\nYou've got a bad playerfile -- "
				    "please report this to an admin!\n");
				pprintf(p, "Your Email address is missing\n");
				pprintf(p, "Please log on as an unreg until "
				    "an admin can correct this.\n");
				rfree(loginnameii);
				return COM_LOGOUT;
			}
		}
	}

	if (problem) {
		psend_raw_file(p, mess_dir, MESS_LOGIN);
		pprintf(p, "login: ");
	}

	return 0;
}

void
boot_out(int p, int p1)
{
	int	fd;

	pprintf(p, "\n **** %s is already logged in - kicking them out. ****\n",
	    parray[p].name);
	pprintf(p1, "**** %s has arrived - you can't both be logged in. ****\n",
	    parray[p].name);

	fd = parray[p1].socket;
	process_disconnection(fd);
	net_close_connection(fd);
}

PUBLIC void
rscan_news(FILE *fp, int p, int lc)
{
	char		*junkp;
	char		 count[10];
	char		 junk[MAX_LINE_SIZE];
	long int	 lval;
	time_t		 crtime;

	fgets(junk, MAX_LINE_SIZE, fp);

	if (feof(fp))
		return;

	sscanf(junk, "%ld %s", &lval, count);
	crtime = lval;

	if ((crtime - lc) < 0)
		return;
	else {
		rscan_news(fp, p, lc);

		junkp = junk;
		junkp = nextword(junkp);
		junkp = nextword(junkp);

		pprintf(p, "%3s (%s) %s", count, fix_time(strltime(&crtime)),
		    junkp);
	}
}

PRIVATE void
check_news(int p, int admin)
{
	FILE		*fp;
	char		 count[10];
	char		 filename[MAX_FILENAME_SIZE];
	char		 junk[MAX_LINE_SIZE];
	char		*junkp;
	long int	 lval;
	time_t		 crtime;
	time_t		 lc = player_lastconnect(p);

	if (admin) {
		sprintf(filename, "%s/newadminnews.index", news_dir);

		if ((fp = fopen(filename, "r")) == NULL) {
			fprintf(stderr, "Can't find admin news index.\n");
			return;
		}

		if (num_anews == -1) {
			num_anews = count_lines(fp);
			fclose(fp);
			fp = fopen(filename, "r");
		}

		fgets(junk, MAX_LINE_SIZE, fp);
		sscanf(junk, "%ld %s", &lval, count);
		crtime = lval;

		if ((crtime - lc) < 0) {
			pprintf(p, "There are no new admin news items since "
			    "your last login.\n\n");
			fclose(fp);
			return;
		} else {
			pprintf(p, "Index of new admin news items:\n");
			rscan_news(fp, p, lc);

			junkp = junk;
			junkp = nextword(junkp);
			junkp = nextword(junkp);

			pprintf(p, "%3s (%s) %s", count,
			    fix_time(strltime(&crtime)), junkp);
			pprintf(p, "(\"anews %d\" will display the most recent "
			    "admin news file)\n", num_anews);
		}
	} else {
		sprintf(filename, "%s/newnews.index", news_dir);

		if ((fp = fopen(filename, "r")) == NULL) {
			fprintf(stderr, "Can't find news index.\n");
			return;
		}

		if (num_news == -1) {
			num_news = count_lines(fp);
			fclose(fp);
			if ((fp = fopen(filename, "r")) == NULL) {
				fprintf(stderr, "Can't find news index.\n");
				return;
			}
		}

		fgets(junk, MAX_LINE_SIZE, fp);
		sscanf(junk, "%ld %s", &lval, count);
		crtime = lval;

		if ((crtime - lc) < 0) {
			pprintf(p, "There are no new news items since your "
			    "last login (%s).\n", strltime(&lc));
			fclose(fp);
			return;
		} else {
			pprintf(p, "Index of new news items:\n");
			rscan_news(fp, p, lc);

			junkp = junk;
			junkp = nextword(junkp);
			junkp = nextword(junkp);

			pprintf(p, "%3s (%s) %s", count,
			    fix_time(strltime(&crtime)), junkp);
			pprintf(p, "(\"news %d\" will display the most recent "
			    "admin news file)\n", num_news);
		}
	}

	fclose(fp);
}

PRIVATE int
process_password(int p, char *password)
{
	char		 salt[FICS_SALT_SIZE];
	int		 dummy;    // to hold a return value
	int		 fd;
	int		 messnum;
	int		 p1;
	unsigned int	 fromHost;

	turn_echo_on(parray[p].socket);

	if (parray[p].passwd && parray[p].registered) {
		strncpy(salt, &(parray[p].passwd[0]), sizeof salt - 1);
		salt[sizeof salt - 1] = '\0';

		if (strcmp(crypt(password, salt), parray[p].passwd)) {
			fd		= parray[p].socket;
			fromHost	= parray[p].thisHost;

			player_clear(p);
			parray[p].logon_time = parray[p].last_command_time =
			    time(0);
			parray[p].status = PLAYER_LOGIN;
			parray[p].socket = fd;
			parray[p].thisHost = fromHost;

			if (*password) {
				pprintf(p, "\n\n**** Invalid password! ****"
				    "\n\n");
			}
			return COM_LOGOUT;
		}
	}

	for (p1 = 0; p1 < p_num; p1++) {
		if (parray[p1].name != NULL) {
			if (!strcasecmp(parray[p].name, parray[p1].name) &&
			    p != p1) {
				if (parray[p].registered == 0) {
					pprintf(p, "\n*** Sorry %s is already "
					    "logged in ***\n", parray[p].name);
					return COM_LOGOUT;
				}
				boot_out(p, p1);
			}
		}
	}

	if (parray[p].adminLevel > 0) {
		psend_raw_file(p, mess_dir, MESS_ADMOTD);
	} else {
		psend_raw_file(p, mess_dir, MESS_MOTD);
	}

	if (!parray[p].passwd && parray[p].registered) {
		pprintf(p, "\n*** You have no password. Please set one with "
		    "the password command.");
	}
	if (!parray[p].registered)
		psend_raw_file(p, mess_dir, MESS_UNREGISTERED);

	parray[p].status = PLAYER_PROMPT;
	player_write_login(p);

	for (p1 = 0; p1 < p_num; p1++) {
		if (p1 == p)
			continue;
		if (parray[p1].status != PLAYER_PROMPT)
			continue;
		if (!parray[p1].i_login)
			continue;

		if (parray[p1].adminLevel > 0) {
			pprintf_prompt(p1, "\n[%s (%s: %s) has connected.]\n",
			    parray[p].name,
			    (parray[p].registered ? "R" : "U"),
			    dotQuad(parray[p].thisHost));
		} else {
			pprintf_prompt(p1, "\n[%s has connected.]\n",
			    parray[p].name);
		}
	}

	parray[p].num_comments = player_num_comments(p);
	messnum = player_num_messages(p);

	/*
	 * Don't send unreg any news. When you change this, feel free
	 * to put all the news junk in one source file. No reason for
	 * identical code in 'command.c' and 'comproc.c'.
	 */

	if (parray[p].registered) {
		check_news(p, 0);

		if (parray[p].adminLevel > 0) {
			pprintf(p, "\n");
			check_news(p, 1);
		}
	}

	if (messnum) {
		pprintf(p, "\nYou have %d messages.\nUse \"messages\" to "
		    "display them, or \"clearmessages\" to remove them.\n",
		    messnum);
	}

	player_notify_present(p);
	player_notify(p, "arrived", "arrival");
	showstored(p);

	if (parray[p].registered &&
	    parray[p].lastHost != 0 &&
	    parray[p].lastHost != parray[p].thisHost) {
		pprintf(p, "\nPlayer %s: Last login: %s ", parray[p].name,
		    dotQuad(parray[p].lastHost));
		pprintf(p, "This login: %s", dotQuad(parray[p].thisHost));
	}

	parray[p].lastHost = parray[p].thisHost;

	if (parray[p].registered && !parray[p].timeOfReg)
		parray[p].timeOfReg = time(0);

	parray[p].logon_time = parray[p].last_command_time = time(0);

	dummy = check_and_print_shutdown(p);	// Tells the user if we are
						// going to shutdown

	// XXX: unused
	(void) dummy;

	pprintf(p, "\n%s", parray[p].prompt);
	return 0;
}

PRIVATE int
process_prompt(int p, char *command)
{
	char	*cmd = "";
	int	 i, len;
	int	 retval;

	command = eattailwhite(eatwhite(command));

	if (!*command) {
		pprintf(p, "%s", parray[p].prompt);
		return COM_OK;
	}

	retval = process_command(p, command, &cmd);

	switch (retval) {
	case COM_OK:
		retval = COM_OK;
		pprintf(p, "%s", parray[p].prompt);
		break;
	case COM_OK_NOPROMPT:
		retval = COM_OK;
		break;
	case COM_ISMOVE:
		retval = COM_OK;

#ifdef TIMESEAL
		if (parray[p].game >= 0 &&
		    garray[parray[p].game].status == GAME_ACTIVE &&
		    parray[p].side == garray[parray[p].game].game_state.onMove &&
		    garray[parray[p].game].flag_pending != FLAG_NONE) {
			ExecuteFlagCmd(parray[p].game,
			    con[parray[p].socket].time);
		}
#endif

		process_move(p, command);
		pprintf(p, "%s", parray[p].prompt);
		break;
	case COM_RIGHTS:
		pprintf(p, "%s: Insufficient rights.\n", cmd);
		pprintf(p, "%s", parray[p].prompt);
		retval = COM_OK;
		break;
	case COM_AMBIGUOUS:
		i = 0;
		len = strlen(cmd);

		pprintf(p, "Ambiguous command. Matches:");

		while (command_list[i].comm_name) {
			if (!strncmp(command_list[i].comm_name, cmd, len) &&
			    parray[p].adminLevel >= command_list[i].adminLevel)
				pprintf(p, " %s", command_list[i].comm_name);
			i++;
		}

		pprintf(p, "\n%s", parray[p].prompt);
		retval = COM_OK;
		break;
	case COM_BADPARAMETERS:
		printusage(p, command_list[lastCommandFound].comm_name);
		pprintf(p, "%s", parray[p].prompt);
		retval = COM_OK;
		break;
	case COM_FAILED:
	case COM_BADCOMMAND:
		pprintf(p, "%s: Command not found.\n", cmd);
		retval = COM_OK;
		pprintf(p, "%s", parray[p].prompt);
		break;
	case COM_LOGOUT:
		retval = COM_LOGOUT;
		break;
	}

	return retval;
}

/* Return 1 to disconnect */
PUBLIC int
process_input(int fd, char *com_string)
{
	int	p = player_find(fd);
	int	retval = 0;

	if (p < 0) {
		fprintf(stderr, "FICS: Input from a player not in array!\n");
		return -1;
	}

	commanding_player = p;
	parray[p].last_command_time = time(0);

	switch (parray[p].status) {
	case PLAYER_EMPTY:
		fprintf(stderr, "FICS: Command from an empty player!\n");
		break;
	case PLAYER_NEW:
		fprintf(stderr, "FICS: Command from a new player!\n");
		break;
	case PLAYER_INQUEUE:
		// Ignore input from player in queue
		break;
	case PLAYER_LOGIN:
		retval = process_login(p, com_string);

		if (retval == COM_LOGOUT && com_string != NULL) {
			fprintf(stderr, "%s tried to log in and failed.\n",
			    com_string);
		}

		break;
	case PLAYER_PASSWORD:
		retval = process_password(p, com_string);
		break;
	case PLAYER_PROMPT:
		parray[p].busy[0] = '\0';

		// added this to stop buggy admin levels; shane
		if (parray[p].adminLevel < 10)
			parray[p].adminLevel = 0;

		retval = process_prompt(p, com_string);
		break;
	}

	commanding_player = -1;
	return retval;
}

PUBLIC int
process_new_connection(int fd, unsigned int fromHost)
{
	int p = player_new();

	parray[p].status	= PLAYER_LOGIN;
	parray[p].socket	= fd;
	parray[p].thisHost	= fromHost;
	parray[p].logon_time	= time(0);

	psend_raw_file(p, mess_dir, MESS_WELCOME);
	pprintf(p, "Head admin : %s   Complaints to : %s\n",
	    hadmin_handle,
	    hadmin_email);
	pprintf(p, "Server location: %s   Server version : %s\n", fics_hostname,
	    VERS_NUM);
	psend_raw_file(p, mess_dir, MESS_LOGIN);
	pprintf(p, "login: ");
	return 0;
}

PUBLIC int
process_disconnection(int fd)
{
	int p = player_find(fd);

	if (p < 0) {
		fprintf(stderr, "FICS: Disconnect from a player not in array!"
		    "\n");
		return -1;
	}

	if (parray[p].game >= 0 && garray[parray[p].game].status ==
	    GAME_EXAMINE)
		pcommand(p, "unexamine");
	if (parray[p].game >= 0 && in_list(p, L_ABUSER, parray[p].name))
		pcommand(p, "resign");

	if (parray[p].status == PLAYER_PROMPT) {
		for (int p1 = 0; p1 < p_num; p1++) {
			if (p1 == p)
				continue;
			if (parray[p1].status != PLAYER_PROMPT)
				continue;
			if (!parray[p1].i_login)
				continue;
			pprintf_prompt(p1, "\n[%s has disconnected.]\n",
			    parray[p].name);
		}

		player_notify(p, "departed", "departure");
		player_notify_departure(p);
		player_write_logout(p);

		if (parray[p].registered) {
			parray[p].totalTime += time(0) - parray[p].logon_time;
			player_save(p);
		} else {	// delete unreg history file
			char fname[MAX_FILENAME_SIZE] = { '\0' };

			(void) snprintf(fname, sizeof fname,
			    "%s/player_data/%c/%s.games",
			    stats_dir,
			    parray[p].login[0],
			    parray[p].login);
			unlink(fname);
		}
	}

	player_remove(p);
	return 0;
}

/* Called every few seconds */
PUBLIC int
process_heartbeat(int *fd)
{
	int		 now = time(0);
	int		 time_since_last;
	static int	 last_comfile = 0;
	static int	 last_space = 0;
	static int	 lastcalled = 0;

	if (lastcalled == 0)
		time_since_last = 0;
	else
		time_since_last = now - lastcalled;
	lastcalled = now;

	// XXX: unused
	(void) time_since_last;

	/*
	 * Check for timed out connections
	 */

	for (int p = 0; p < p_num; p++) {
		if ((parray[p].status == PLAYER_LOGIN ||
		    parray[p].status == PLAYER_PASSWORD) &&
		    player_idle(p) > MAX_LOGIN_IDLE) {
			pprintf(p, "\n**** LOGIN TIMEOUT ****\n");
			*fd = parray[p].socket;
			return COM_LOGOUT;
		}

		if (parray[p].status == PLAYER_PROMPT &&
		    player_idle(p) > MAX_IDLE_TIME &&
		    parray[p].adminLevel == 0 &&
		    !in_list(p, L_TD, parray[p].name)) {
			pprintf(p, "\n**** Auto-logout because you were idle "
			    "more than one hour. ****\n");
			*fd = parray[p].socket;
			return COM_LOGOUT;
		}
	}

	/*
	 * Check for the communication file from mail updates every 10
	 * minutes. (That is probably too often, but who cares.)
	 */

	if (MailGameResult) {
		if (last_comfile == 0) {
			last_comfile = now;
		} else {
			if ((last_comfile + 10 * 60) < now)
				last_comfile = now;
		}
	}

	if (last_space == 0) {
		last_space = now;
	} else {
		if ((last_space + 60) < now) {	// Check the disk space every
						// minute
			last_space = now;

			if (available_space() < 1000000) {
				server_shutdown(60, "    **** Disk space is "
				    "dangerously low!!! ****\n");
			}
		}
	}

	ShutHeartBeat();
	return COM_OK;
}

PUBLIC void
commands_init(void)
{
	FILE	*fp, *afp;
	char	 fname[MAX_FILENAME_SIZE];
	int	 i = 0;

	snprintf(fname, sizeof fname, "%s/commands", comhelp_dir);

	if ((fp = fopen(fname, "w")) == NULL) {
		fprintf(stderr, "FICS: Could not write commands help file.\n");
		return;
	}

	snprintf(fname, sizeof fname, "%s/admin_commands", adhelp_dir);

	if ((afp = fopen(fname, "w")) == NULL) {
		fprintf(stderr, "FICS: Could not write admin_commands help "
		    "file.\n");
		fclose(fp);
		return;
	}

	while (command_list[i].comm_name) {
		if (command_list[i].adminLevel >= ADMIN_ADMIN)
			fprintf(afp, "%s\n", command_list[i].comm_name);
		else
			fprintf(fp, "%s\n", command_list[i].comm_name);
		i++;
	}

	fclose(fp);
	fclose(afp);
}

/* Need to save rated games */
PUBLIC void
TerminateCleanup(void)
{
	for (int g = 0; g < g_num; g++) {
		if (garray[g].status != GAME_ACTIVE)
			continue;
		if (garray[g].rated)
			game_ended(g, WHITE, END_ADJOURN);
	}

	for (int p1 = 0; p1 < p_num; p1++) {
		if (parray[p1].status == PLAYER_EMPTY)
			continue;

		pprintf(p1, "\n    **** Server shutting down immediately. "
		    "****\n\n");

		if (parray[p1].status != PLAYER_PROMPT) {
			close(parray[p1].socket);
		} else {
			pprintf(p1, "Logging you out.\n");
			psend_raw_file(p1, mess_dir, MESS_LOGOUT);
			player_write_logout(p1);
			if (parray[p1].registered) {
				parray[p1].totalTime +=
				    (time(0) - parray[p1].logon_time);
			}
			player_save(p1);
		}
	}
}

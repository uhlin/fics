/* command.h
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
   Markus Uhlin                 23/12/19	Cleaned up the file
   Markus Uhlin                 24/04/07	Added missing parameter lists
*/

#ifndef _COMMAND_H
#define _COMMAND_H

#include "variable.h"
#include "stdinclude.h"

/*
 * Maximum length of a login name
 */
#define MAX_LOGIN_NAME 20

/*
 * Maximum number of parameters per command
 */
#define MAXNUMPARAMS 10

/*
 * Maximum string length of a single command word
 */
#define MAX_COM_LENGTH 50

/*
 * Maximum string length of the whole command line
 */
#define MAX_STRING_LENGTH 1024

#define COM_OK                0
#define COM_FAILED            1
#define COM_ISMOVE            2
#define COM_AMBIGUOUS         3
#define COM_BADPARAMETERS     4
#define COM_BADCOMMAND        5
#define COM_LOGOUT            6
#define COM_FLUSHINPUT        7
#define COM_RIGHTS            8
#define COM_OK_NOPROMPT       9

#define ADMIN_USER	0
#define ADMIN_ADMIN	10
#define ADMIN_MASTER	20
#define ADMIN_DEMIGOD	60
#define ADMIN_GOD	100

#define TYPE_NULL	0
#define TYPE_WORD	1
#define TYPE_STRING	2
#define TYPE_INT	3
typedef struct u_parameter {
	int type;

	union {
		char	*word;
		char	*string;
		int	 integer;
	} val;
} parameter;

typedef parameter param_list[MAXNUMPARAMS];

typedef struct s_command_type {
	char *comm_name;
	char *param_string;
	int (*comm_func)(int, param_list);
	int adminLevel;
} command_type;

typedef struct s_alias_type {
	char	*comm_name;
	char	*alias;
} alias_type;

extern char	*adhelp_dir;
extern char	*adj_dir;
extern char	*board_dir;
extern char	*comhelp_dir;
extern char	*config_dir;
extern char	*def_prompt;
extern char	*help_dir[NUM_LANGS];
extern char	*hist_dir;
extern char	*index_dir;
extern char	*info_dir;
extern char	*journal_dir;
extern char	*lists_dir;
extern char	*mess_dir;
extern char	*news_dir;
extern char	*player_dir;
extern char	*source_dir;
extern char	*stats_dir;
extern char	*usage_dir[NUM_LANGS];
extern char	*uscf_dir;

extern char *hadmin_handle;

extern char	 fics_hostname[81];
extern int	 MailGameResult;
extern int	 game_high;
extern int	 player_high;
extern time_t	 startuptime;

/*
 * The player whose command you're in
 */
extern int commanding_player;

extern int	 alias_lookup(char *, alias_type *, int);
extern int	 process_command(int, char *, char **);
extern int	 process_disconnection(int);
extern int	 process_heartbeat(int *);
extern int	 process_input(int, char *);
extern int	 process_new_connection(int, unsigned int);
extern void	 TerminateCleanup(void);
extern void	 commands_init(void);

/* From variable.c */
extern int	 com_partner(int, param_list);
extern int	 com_variables(int, param_list);

#endif /* _COMMAND_H */

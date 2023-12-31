/* playerdb.h
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
   Markus Uhlin                 23/12/13	Cleaned up the file
*/

#include "command.h"
#include "lists.h"

#ifndef _PLAYERDB_H
#define _PLAYERDB_H

#define PLAYER_VERSION 1

#define MAX_ALIASES	30
#define MAX_CENSOR	50
#define MAX_FORMULA	9
#define MAX_INCHANNELS	16
#define MAX_MESSAGES	40
#define MAX_NOTIFY	80
#define MAX_OBSERVE	30
#define MAX_PENDING	10
#define MAX_PLAN	10
#define MAX_PLAYER	500
#define MAX_SIMUL	30

#define PLAYER_EMPTY          0
#define PLAYER_NEW            1
#define PLAYER_INQUEUE        2
#define PLAYER_LOGIN          3
#define PLAYER_PASSWORD       4
#define PLAYER_PROMPT         5

#define P_LOGIN		0
#define P_LOGOUT	1

#define SORT_BLITZ	0
#define SORT_STAND	1
#define SORT_ALPHA	2
#define SORT_WILD	3

typedef struct _statistics {
	int	 num, win, los, dra, rating, ltime, best, whenbest;
	double	 sterr;
} statistics;

#define PEND_MATCH	0
#define PEND_DRAW	1
#define PEND_ABORT	2
#define PEND_TAKEBACK	3
#define PEND_ADJOURN	4
#define PEND_SWITCH	5
#define PEND_SIMUL	6
#define PEND_PAUSE	7
#define PEND_PARTNER	8
#define PEND_BUGHOUSE	9
#define PEND_ALL -1

#define PEND_TO		0
#define PEND_FROM	1

typedef struct _pending {
	int	 type;
	int	 whoto;
	int	 whofrom;
	int	 param1, param2, param3, param4, param5, param6;
	char	 char1[50];
	char	 char2[50];
} pending;

typedef struct _simul_info_t {
	int	 numBoards;
	int	 onBoard;
	int	 results[MAX_SIMUL];
	int	 boards[MAX_SIMUL];
} simul_info_t;

typedef struct _player {
	/*
	 * This first block is not saved between logins
	 */
	List		*lists;
	char		 busy[100];
	char		*identptr;
	char		*last_file;
	char		*login;
	int		 flip;
	int		 game;
	int		 i_admin;
	int		 kiblevel;
	int		 lastColor;
	int		 last_channel;
	int		 last_command_time;
	int		 last_opponent;
	int		 last_tell;
	int		 lastshout_a;
	int		 lastshout_b;
	int		 logon_time;
	int		 num_comments; // number of lines in comments file
	int		 num_from;
	int		 num_observe;
	int		 num_to;
	int		 observe_list[MAX_OBSERVE];
	int		 opponent; // Only valid if game is >= 0
	int		 side; // Only valid if game is >= 0
	int		 partner;
	int		 registered;
	int		 socket;
	int		 sopen;
	int		 status;
	int		 timeOfReg;
	int		 totalTime;
	long		 last_file_byte;
	pending		 p_from_list[MAX_PENDING];
	pending		 p_to_list[MAX_PENDING];
	simul_info_t	 simul_info;
	unsigned int	 thisHost;

	/*
	 * All of this is saved between logins
	 */
	char		*name;
	char		*emailAddress;
	char		*fullName;
	char		*passwd;
	char		*prompt;
	statistics	 b_stats;
	statistics	 l_stats;
	statistics	 s_stats;
	statistics	 w_stats;
	statistics	 bug_stats;
	alias_type	 alias_list[MAX_ALIASES];
	char		*formula;
	char		*formulaLines[MAX_FORMULA];
	char		*planLines[MAX_PLAN];
	int		 adminLevel;
	int		 automail;
	int		 bell;
	int		 d_height;
	int		 d_inc;
	int		 d_time;
	int		 d_width;
	int		 highlight;
	int		 i_cshout;
	int		 i_game;
	int		 i_kibitz;
	int		 i_login;
	int		 i_mailmess;
	int		 i_shout;
	int		 i_tell;
	int		 jprivate;
	int		 language;
	int		 nochannels;
	int		 notifiedby;
	int		 numAlias;
	int		 num_black;
	int		 num_formula;
	int		 num_plan;
	int		 num_white;
	int		 open;
	int		 pgn;
	int		 private;
	int		 promote;
	int		 rated;
	int		 ropen;
	int		 style;
	unsigned int	 lastHost;
} player;

typedef struct _textlist {
	char	*text;
	int	 index;
	struct _textlist *next;
} textlist;

#define PARRAY_SIZE (MAX_PLAYER + 50)

extern player	 parray[PARRAY_SIZE];
extern int	 p_num;

extern int	 ClearMsgsBySender(int, param_list);
extern int	 ClrMsgRange(int, int, int);
extern int	 ShowMsgRange(int, int, int);
extern int	 ShowMsgsBySender(int, param_list);
extern int	 showstored(int);
extern textlist	*ClearTextListEntry(textlist *);
extern void	 ClearTextList(textlist *);
extern void	 SaveTextListEntry(textlist **, char *, int);

extern int	 player_add_comment(int, int, char *);
extern int	 player_add_message(int, int, char *);
extern int	 player_add_observe(int, int);
extern int	 player_add_request(int, int, int, int);
extern int	 player_censored(int, int);
extern int	 player_clear(int);
extern int	 player_clear_messages(int);
extern int	 player_count(int);
extern int	 player_decline_offers(int, int, int);
extern int	 player_delete(int);
extern int	 player_find(int);
extern int	 player_find_bylogin(char *);
extern int	 player_find_part_login(char *);
extern int	 player_find_pendfrom(int, int, int);
extern int	 player_find_pendto(int, int, int);
extern int	 player_free(int);
extern int	 player_game_ended(int);
extern int	 player_goto_board(int, int);
extern int	 player_goto_next_board(int);
extern int	 player_goto_prev_board(int);
extern int	 player_goto_simulgame_bynum(int, int);
extern int	 player_idle(int);
extern int	 player_is_observe(int, int);
extern int	 player_ishead(int);
extern int	 player_kill(char *);
extern int	 player_markdeleted(int);
extern int	 player_new(void);
extern int	 player_new_pendfrom(int);
extern int	 player_new_pendto(int);
extern int	 player_notified(int, int);
extern int	 player_notified_departure(int);
extern int	 player_notify(int, char *, char *);
extern int	 player_notify_present (int);
extern int	 player_num_active_boards(int);
extern int	 player_num_comments(int);
extern int	 player_num_messages(int);
extern int	 player_num_results(int, int);
extern int	 player_ontime(int);
extern int	 player_raise(char *);
extern int	 player_read(int, char *);
extern int	 player_reincarn(char *, char *);
extern int	 player_remove(int);
extern int	 player_remove_observe(int, int);
extern int	 player_remove_pendfrom(int, int, int);
extern int	 player_remove_pendto(int, int, int);
extern int	 player_remove_request(int, int, int);
extern int	 player_rename(char *, char *);
extern int	 player_save(int);
extern int	 player_search(int, char *);
extern int	 player_show_comments(int, int);
extern int	 player_show_messages(int);
extern int	 player_simul_over(int, int, int);
extern int	 player_withdraw_offers(int, int, int);
extern int	 player_zero(int);
extern time_t	 player_lastconnect(int);
extern time_t	 player_lastdisconnect(int);
extern void	 player_array_init(void);
extern void	 player_init(int);
extern void	 player_notify_departure(int);
extern void	 player_pend_print(int, pending *);
extern void	 player_write_login(int);
extern void	 player_write_logout(int);

#endif /* _PLAYERDB_H */

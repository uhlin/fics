/* lists.h
 *
 * Revised by maxxe 23/12/15
 */

#include <stdio.h>

#ifndef _LISTS_H
#define _LISTS_H

enum ListWhich {
	L_ADMIN = 0,
	L_REMOVEDCOM,
	L_FILTER,
	L_BAN,
	L_ABUSER,
	L_MUZZLE,
	L_CMUZZLE,
	L_FM,
	L_IM,
	L_GM,
	L_BLIND,
	L_TEAMS,
	L_COMPUTER,
	L_TD,
	L_CENSOR,
	L_GNOTIFY,
	L_NOPLAY,
	L_NOTIFY,
	L_CHANNEL
};

enum ListPerm {
	P_HEAD = 0,
	P_GOD,
	P_ADMIN,
	P_PUBLIC,
	P_PERSONAL
};

typedef struct {
	enum ListPerm	 rights;
	char		*name;
} ListTable;

/*
 * Max names in one list.
 */
#define MAX_GLOBAL_LIST_SIZE 200

typedef struct _List List;

struct _List {
	enum ListWhich	 which;
	int		 numMembers;
	char		*member[MAX_GLOBAL_LIST_SIZE];
	struct _List	*next;
};

extern int	com_addlist();
extern int	com_showlist();
extern int	com_sublist();

extern int	in_list(int, enum ListWhich, char *);
extern int	list_add(int, enum ListWhich, char *);
extern int	list_addsub(int, char *, char *, int);
extern int	list_channels(int, int);
extern int	list_size(int, enum ListWhich);
extern int	list_sub(int, enum ListWhich, char *);
extern int	titled_player(int, char *);
extern void	list_free(List *);
extern void	list_print(FILE *, int, enum ListWhich);

#endif    /* _LISTS_H */

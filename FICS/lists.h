/* lists.h */

#include <stdio.h>

#ifndef _LISTS_H
#define _LISTS_H

/* yes, it's all cheesy..  there is no significance to the order, but make
   sure it matches the order in lists.c */

enum ListWhich {L_ADMIN = 0, L_REMOVEDCOM, L_FILTER, L_BAN, L_ABUSER,
L_MUZZLE, L_CMUZZLE, L_FM, L_IM, L_GM, L_BLIND, L_TEAMS, L_COMPUTER,
L_TD, L_CENSOR, L_GNOTIFY, L_NOPLAY, L_NOTIFY, L_CHANNEL};

enum ListPerm {P_HEAD = 0, P_GOD, P_ADMIN, P_PUBLIC, P_PERSONAL};

typedef struct {enum ListPerm rights; char *name;} ListTable;

/* max. names in one list: 200 */
#define MAX_GLOBAL_LIST_SIZE 200

typedef struct _List List;
struct _List {
  enum ListWhich which;
  int numMembers;
  char *member[MAX_GLOBAL_LIST_SIZE];
  struct _List *next;
};

extern int com_addlist();
extern int com_sublist();
extern int com_showlist();

extern void list_free(List *);
extern int in_list(int, enum ListWhich, char *);
extern int list_size(int p, enum ListWhich);
extern int list_add(int, enum ListWhich, char *);
extern int list_addsub (int,char*,char*,int);
extern int list_sub(int, enum ListWhich, char *);
extern void list_print(FILE *, int, enum ListWhich);
extern int titled_player(int, char *);
extern int list_channels(int,int);

#endif   /* _LISTS_H */

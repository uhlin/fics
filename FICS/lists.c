/* lists.c  --  new global lists code
 *
 * Added by Shaney, 29 May 1995.
 * This file is part of FICS.
 */

#include "stdinclude.h"

#include <string.h>

#include "command.h"
#include "common.h"
#include "comproc.h"
#include "gamedb.h"
#include "lists.h"
#include "multicol.h"
#include "playerdb.h"
#include "ratings.h"
#include "rmalloc.h"
#include "talkproc.h"
#include "utils.h"

List *firstGlobalList = NULL;

PRIVATE ListTable ListArray[] =
{{P_HEAD, "admin"},
{P_GOD, "removedcom"},
{P_ADMIN, "filter"},
{P_ADMIN, "ban"},
{P_ADMIN, "abuser"},
{P_ADMIN, "muzzle"},
{P_ADMIN, "cmuzzle"},
{P_PUBLIC, "fm"},
{P_PUBLIC, "im"},
{P_PUBLIC, "gm"},
{P_PUBLIC, "blind"},
{P_PUBLIC, "teams"},
{P_PUBLIC, "computer"},
{P_PUBLIC, "td"},
{P_PERSONAL, "censor"},
{P_PERSONAL, "gnotify"},
{P_PERSONAL, "noplay"},
{P_PERSONAL, "notify"},
{P_PERSONAL, "channel"},
{0, NULL}};

/* find a list.  loads from disk if not in memory. */
PRIVATE List *list_find(int p, enum ListWhich l)
{
  List *prev, *tempList, **starter;
  int personal;
  int count = 0;

  personal = ListArray[l].rights == P_PERSONAL;
  starter = personal ? &(parray[p].lists) : &firstGlobalList;

  for (prev = NULL, tempList = *starter; tempList != NULL; tempList = tempList->next) {
    if (l == tempList->which) {
      if (prev != NULL) {
	prev->next = tempList->next;
	tempList->next = *starter;
	*starter = tempList;
      }
      return tempList;
    }
    prev = tempList;
  }

  tempList = rmalloc(sizeof(List));
  if (tempList == NULL)
    return NULL;

  if (!personal) {		/* now we have to load the list */
    FILE *fp;
    char listmember[100];
    char filename[MAX_FILENAME_SIZE];

    /* fprintf(stderr,"SHANEDEBUG: Adding %s list\n", ListArray[l].name); */

    sprintf(filename, "%s/%s", lists_dir, ListArray[l].name);
    fp = fopen(filename, "r");
    if (!fp) {
      rfree(tempList);
      return NULL;
    }
    while (!feof(fp)) {
      if (fgets(listmember, 100, fp) != NULL) {
	listmember[strlen(listmember) - 1] = '\0';
	tempList->member[count++] = xstrdup(listmember);
      }
    }
    fclose(fp);
  }
  tempList->which = l;
  tempList->numMembers = count;
  tempList->next = *starter;
  *starter = tempList;
  return tempList;
}

/* add item to list */
PUBLIC int list_add(int p, enum ListWhich l, char *s)
{
  List *gl = list_find(p, l);

  if (gl) {
    if (gl->numMembers < MAX_GLOBAL_LIST_SIZE) {
      gl->member[gl->numMembers] = xstrdup(s);
      gl->numMembers++;
      return 0;
    } else {
      return 1;
    }
  } else {
    return 1;
  }
}

/* remove item from list */
PUBLIC int list_sub(int p, enum ListWhich l, char *s)
{
  List *gl = list_find(p, l);

  if (gl) {
    int i, found = -1;
    for (i = 0; i < gl->numMembers; i++)
      if (!strcasecmp(s, gl->member[i])) {
	found = i;
	break;
      }
    if (found == -1)
      return 1;
    rfree(gl->member[found]);
    for (i = found; i < (gl->numMembers - 1); i++)
      gl->member[i] = gl->member[i + 1];
    gl->numMembers--;
    return 0;
  } else {
    return 1;
  }
}

/* pretty cheesy: print each member of a list, 1 per line */
PUBLIC void list_print(FILE * fp, int p, enum ListWhich l)
{
  int i;
  List *gl = list_find(p, l);

  if (gl) {
    for (i = 0; i < gl->numMembers; i++)
      fprintf(fp, "%s\n", gl->member[i]);
  }
}

/* return size of a list */
PUBLIC int list_size(int p, enum ListWhich l)
{
  List *gl = list_find(p, l);

  if (gl)
    return (gl->numMembers);
  else
    return 0;
}

/* find list by name, doesn't have to be the whole name */
PRIVATE List *list_findpartial(int p, char *which, int gonnado)
{
  List *gl;
  int i, foundit, slen;

  slen = strlen(which);
  for (i = 0, foundit = -1; ListArray[i].name != NULL; i++) {
    if (!strncasecmp(ListArray[i].name, which, slen)) {
      if (foundit == -1)
	foundit = i;
      else
	return NULL;		/* ambiguous */
    }
  }

  if (foundit != -1) {
    int rights = ListArray[foundit].rights;
    int youlose = 0;

    switch (rights) {		/* check rights */
    case P_HEAD:
      if (gonnado && !player_ishead(p))
	youlose = 1;
      break;
    case P_GOD:
      if ((gonnado && (parray[p].adminLevel < ADMIN_GOD)) ||
	  (!gonnado && (parray[p].adminLevel < ADMIN_ADMIN)))
	youlose = 1;
      break;
    case P_ADMIN:
      if (parray[p].adminLevel < ADMIN_ADMIN)
	youlose = 1;
      break;
    case P_PUBLIC:
      if (gonnado && (parray[p].adminLevel < ADMIN_ADMIN))
	youlose = 1;
      break;
    }
    if (youlose) {
      pprintf(p, "\"%s\" is not an appropriate list name or you have insufficient rights.\n", which);
      return NULL;
    }
    gl = list_find(p, foundit);
  } else {
    pprintf(p, "\"%s\" does not match any list name.\n", which);
    return NULL;
  }
  return gl;
}

/* see if something is in a list */
PUBLIC int in_list(int p, enum ListWhich which, char *member)
{
  List *gl;
  int i;
  int filterList = (which == L_FILTER);

  gl = list_find(p, which);
  if ((gl == NULL) || (member == NULL))
    return 0;
  for (i = 0; i < gl->numMembers; i++) {
    if (filterList) {
      if (!strncasecmp(member, gl->member[i], strlen(gl->member[i])))
	return 1;
    } else {
      if (!strcasecmp(member, gl->member[i]))
	return 1;
    }
  }
  return 0;
}

/* add or subtract something to/from a list */
PUBLIC int list_addsub(int p, char* list, char* who, int addsub)
{
  int p1, connected, loadme, personal, ch;
  char *listname, *member;
  List *gl;
  char *yourthe, *addrem;

  gl = list_findpartial(p, list, addsub);
  if (!gl)
    return COM_OK;

  personal = ListArray[gl->which].rights == P_PERSONAL;
  loadme = (gl->which != L_FILTER) && (gl->which != L_REMOVEDCOM) && (gl->which != L_CHANNEL);
  listname = ListArray[gl->which].name;
  yourthe = personal ? "your" : "the";
  addrem = (addsub == 1) ? "added to" : "removed from";

  if (loadme) {
    if (!FindPlayer(p, who, &p1, &connected)) {
      if (addsub == 1)
        return COM_OK;
      member = who;		/* allow sub removed/renamed player */
      loadme = 0;
    } else
      member = parray[p1].name;
  } else {
    member = who;
  }

  if (addsub == 1) {		/* add to list */

   if (gl->which == L_CHANNEL) {

    if (sscanf (who,"%d",&ch) == 1) {

     if ((!in_list(p,L_ADMIN,parray[p].name)) && (ch == 0)) {
      pprintf(p, "Only admins may join channel 0.\n");
      return COM_OK;
     }

     if (ch > (MAX_CHANNELS - 1)) {
      pprintf(p,"The maximum channel number is %d.\n",MAX_CHANNELS-1);
      return COM_OK;
     }
    } else {
     pprintf (p,"Your channel to add must be a number between 0 and %d.\n",MAX_CHANNELS - 1);
     return COM_OK;
    }
   }
   if (in_list(p, gl->which, member)) {
     pprintf(p, "[%s] is already on %s %s list.\n", member, yourthe, listname);
     if (loadme && !connected)
       player_remove(p1);
     return COM_OK;
    }
    if (list_add(p, gl->which, member)) {
      pprintf(p, "Sorry, %s %s list is full.\n", yourthe, listname);
      if (loadme && !connected)
	player_remove(p1);
      return COM_OK;
    }
  } else if (addsub == 2) {	/* subtract from list */
    if (!in_list(p, gl->which, member)) {
      pprintf(p, "[%s] is not in %s %s list.\n", member, yourthe, listname);
      if (loadme && !connected)
	player_remove(p1);
      return COM_OK;
    }
    list_sub(p, gl->which, member);
  }
  pprintf(p, "[%s] %s %s %s list.\n", member, addrem, yourthe, listname);

  if (!personal) {
    FILE *fp;
    char filename[MAX_FILENAME_SIZE];

    switch (gl->which) {
    case L_MUZZLE:
    case L_CMUZZLE:
    case L_ABUSER:
    case L_BAN:
      pprintf(p, "Please leave a comment to explain why %s was %s the %s list.\n", member, addrem, listname);
      pcommand(p, "addcomment %s %s %s list.\n", member, addrem, listname);
      break;
    case L_COMPUTER:
      if (parray[p1].b_stats.rating > 0)
	UpdateRank(TYPE_BLITZ, member, &parray[p1].b_stats, member);
      if (parray[p1].s_stats.rating > 0)
	UpdateRank(TYPE_STAND, member, &parray[p1].s_stats, member);
      if (parray[p1].w_stats.rating > 0)
	UpdateRank(TYPE_WILD, member, &parray[p1].w_stats, member);
      break;
    case L_ADMIN:
      if (addsub == 1) {	/* adding to list */
	parray[p1].adminLevel = 10;
	pprintf(p, "%s has been given an admin level of 10 - change with asetadmin.\n", member);
      } else {
	parray[p1].adminLevel = 0;
      }
      break;
    case L_FILTER:
      pprintf(p, "Please leave a message for filter to explain why site %s was %s filter list.\n", member, addrem);
      break;
    case L_REMOVEDCOM:
      pprintf(p, "Please leave a message on anews to explain why %s was %s removedcom list.\n", member, addrem);
      break;
    default:
      break;
    }
    if (loadme && connected) {
      pprintf_prompt(p1, "You have been %s the %s list by %s.\n", addrem, listname, parray[p].name);
    }
    sprintf(filename, "%s/%s", lists_dir, listname);
    fp = fopen(filename, "w");
    if (fp == NULL) {
      fprintf(stderr, "Couldn't save %s list.\n", listname);
    } else {
      int i;
      for (i = 0; i < gl->numMembers; i++)
	fprintf(fp, "%s\n", gl->member[i]);
      fclose(fp);
    }
  }
  if (loadme || (gl->which == L_ADMIN)) {
    player_save(p1);
  }
  if (loadme && !connected) {
    player_remove(p1);
  }
  return COM_OK;
}

PUBLIC int com_addlist(int p, param_list param)
{
  return list_addsub(p, param[0].val.word, param[1].val.word, 1);
}

PUBLIC int com_sublist(int p,param_list param)
{
  return list_addsub(p, param[0].val.word, param[1].val.word, 2);
}

PUBLIC int
com_showlist(int p, param_list param)
{
	List	*gl;
	char	*rightnames[] = {
		"EDIT HEAD, READ ADMINS",
		"EDIT GODS, READ ADMINS",
		"READ/WRITE ADMINS",
		"PUBLIC",
		"PERSONAL"
	};
	int	 i, rights;

	if (param[0].type == 0) { // Show all lists
		pprintf(p, "Lists:\n\n");

		for (i = 0; ListArray[i].name != NULL; i++) {
			if ((rights = ListArray[i].rights) > P_ADMIN ||
			    parray[p].adminLevel >= ADMIN_ADMIN) {
				pprintf(p, "%-20s is %s\n", ListArray[i].name,
				    rightnames[rights]);
			}
		}
	} else { // find match in index
		if ((gl = list_findpartial(p, param[0].val.word, 0)) == NULL)
			return COM_OK;

		rights = ListArray[gl->which].rights;

		{ // display the list
			multicol *m = multicol_start(gl->numMembers);

			pprintf(p, "-- %s list: %d %s --",
			    ListArray[gl->which].name,
			    gl->numMembers,
			    ((!strcmp(ListArray[gl->which].name, "filter"))
			    ? "ips"
			    : (!strcmp(ListArray[gl->which].name, "removedcom"))
			    ? "commands"
			    : (!strcmp(ListArray[gl->which].name, "channel"))
			    ? "channels"
			    : "names"));
			for (i = 0; i < gl->numMembers; i++)
				multicol_store_sorted(m, gl->member[i]);
			multicol_pprint(m, p, 78, 2);
			multicol_end(m);
		}
	}

	return COM_OK;
}

PUBLIC int
list_channels(int p, int p1)
{
	List	*gl;

	if ((gl = list_findpartial(p1, "channel", 0)) == NULL)
		return 1;

	if (gl->numMembers == 0)
		return 1;

	{
		multicol *m = multicol_start(gl->numMembers);

		for (int i = 0; i < gl->numMembers; i++)
			multicol_store_sorted(m, gl->member[i]);
		multicol_pprint(m, p, 78, 1);
		multicol_end(m);
	}

	return 0;
}

/*
 * Free the memory used by a list.
 */
PUBLIC void
list_free(List *gl)
{
	List *temp;

	while (gl) {
		for (int i = 0; i < gl->numMembers; i++)
			strfree(gl->member[i]);
		temp = gl->next;
		rfree(gl);
		gl = temp;
	}
}

PUBLIC int
titled_player(int p, char *name)
{
	if (in_list(p, L_FM, name) ||
	    in_list(p, L_IM, name) ||
	    in_list(p, L_GM, name))
		return 1;
	return 0;
}

/* variable.c
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
   loon                         95/03/17	Added figure_boolean()
   hersco                       95/04/10	Replaced figure_boolean() with
						set_boolean_var()
   DAV                          95/19/11	Moved variable command to here
						Added jprivate
   Markus Uhlin                 23/12/27	Fixed the includes
*/

#include "stdinclude.h"
#include "common.h"

#include "board.h"
#include "command.h"
#include "comproc.h"
#include "config.h"
#include "ficsmain.h"
#include "formula.h" /* SetValidFormula() */
#include "playerdb.h"
#include "rmalloc.h"
#include "talkproc.h"
#include "utils.h"
#include "variable.h"

#if __linux__
#include <bsd/string.h>
#endif

PRIVATE int set_boolean_var(int *var, char *val)
{
  int v = -1;

  if (val == NULL)
    return (*var = !*var);

  if (sscanf(val, "%d", &v) != 1) {
    stolower(val);
    if (!strcmp(val, "off"))
      v = 0;
    if (!strcmp(val, "false"))
      v = 0;
    if (!strcmp(val, "on"))
      v = 1;
    if (!strcmp(val, "true"))
      v = 1;
  }
  if ((v == 0) || (v == 1))
    return (*var = v);
  else
    return (-1);
}

PRIVATE int set_open(int p, char *var, char *val)
{
  int v = set_boolean_var(&parray[p].open, val);

  if (v < 0)
    return VAR_BADVAL;
  if (v > 0)
    pprintf(p, "You are now open to receive match requests.\n");
  else {
    player_decline_offers(p, -1, PEND_MATCH);
    player_withdraw_offers(p, -1, PEND_MATCH);
    pprintf(p, "You are no longer receiving match requests.\n");
  }
  return VAR_OK;
}

PRIVATE int set_sopen(int p, char *var, char *val)
{
  int v = set_boolean_var(&parray[p].sopen, val);

  if (v < 0)
    return VAR_BADVAL;
  pprintf(p, "sopen set to %d.\n", parray[p].sopen);

  if (v > 0)
    pprintf(p, "You are now open to receive simul requests.\n");
  else
    pprintf(p, "You are no longer receiving simul requests.\n");
  player_decline_offers(p, -1, PEND_SIMUL);
  return VAR_OK;
}

PRIVATE int set_ropen(int p, char *var, char *val)
{
  if (set_boolean_var(&parray[p].ropen, val) < 0)
    return VAR_BADVAL;
  pprintf(p, "ropen set to %d.\n", parray[p].ropen);
  return VAR_OK;
}

PRIVATE int set_rated(int p, char *var, char *val)
{
  if (!parray[p].registered) {
    pprintf(p, "You cannot change your rated status.\n");
    return VAR_OK;
  }
  if (set_boolean_var(&parray[p].rated, val) < 0)
    return VAR_BADVAL;
  pprintf(p, "rated set to %d.\n", parray[p].rated);
  return VAR_OK;
}

PRIVATE int set_shout(int p, char *var, char *val)
{
  if (set_boolean_var(&parray[p].i_shout, val) < 0)
    return VAR_BADVAL;
  if (parray[p].i_shout)
    pprintf(p, "You will now hear shouts.\n");
  else
    pprintf(p, "You will not hear shouts.\n");
  return VAR_OK;
}

PRIVATE int set_cshout(int p, char *var, char *val)
{
  if (set_boolean_var(&parray[p].i_cshout, val) < 0)
    return VAR_BADVAL;
  if (parray[p].i_cshout)
    pprintf(p, "You will now hear cshouts.\n");
  else
    pprintf(p, "You will not hear cshouts.\n");
  return VAR_OK;
}

PRIVATE int set_kibitz(int p, char *var, char *val)
{
  if (set_boolean_var(&parray[p].i_kibitz, val) < 0)
    return VAR_BADVAL;
  if (parray[p].i_kibitz)
    pprintf(p, "You will now hear kibitzes.\n");
  else
    pprintf(p, "You will not hear kibitzes.\n");
  return VAR_OK;
}
PRIVATE int set_kiblevel(int p, char *var, char *val)
{
  int v = -1;

  if (!val)
    return VAR_BADVAL;
  if (sscanf(val, "%d", &v) != 1)
    return VAR_BADVAL;
  if ((v < 0) || (v > 9999))
    return VAR_BADVAL;
  parray[p].kiblevel = v;
  pprintf(p, "Kibitz level now set to: %d.\n", v);
  return VAR_OK;
}

PRIVATE int set_tell(int p, char *var, char *val)
{
  if (set_boolean_var(&parray[p].i_tell, val) < 0)
    return VAR_BADVAL;
  if (parray[p].i_tell)
    pprintf(p, "You will now hear tells.\n");
  else
    pprintf(p, "You will not hear tells.\n");
  return VAR_OK;
}

PRIVATE int set_notifiedby(int p, char *var, char *val)
{
  if (set_boolean_var(&parray[p].notifiedby, val) < 0)
    return VAR_BADVAL;
  if (parray[p].notifiedby)
    pprintf(p, "You will now hear if people notify you, but you don't notify them.\n");
  else
    pprintf(p, "You will not hear if people notify you, but you don't notify them.\n");
  return VAR_OK;
}

PRIVATE int set_pinform(int p, char *var, char *val)
{
  if (set_boolean_var(&parray[p].i_login, val) < 0)
    return VAR_BADVAL;
  if (parray[p].i_login)
    pprintf(p, "You will now hear logins/logouts.\n");
  else
    pprintf(p, "You will not hear logins/logouts.\n");
  return VAR_OK;
}

PRIVATE int set_ginform(int p, char *var, char *val)
{
  if (set_boolean_var(&parray[p].i_game, val) < 0)
    return VAR_BADVAL;
  if (parray[p].i_game)
    pprintf(p, "You will now hear game results.\n");
  else
    pprintf(p, "You will not hear game results.\n");
  return VAR_OK;
}

PRIVATE int set_private(int p, char *var, char *val)
{
  if (set_boolean_var(&parray[p].private, val) < 0)
    return VAR_BADVAL;
  if (parray[p].private)
    pprintf(p, "Your games will be private.\n");
  else
    pprintf(p, "Your games may not be private.\n");
  return VAR_OK;
}

PRIVATE int set_jprivate(int p, char *var, char *val)
{
  if (!parray[p].registered) {
    pprintf(p, "Unregistered players may not keep a journal.\n");
    return VAR_OK;
  }

  if (set_boolean_var(&parray[p].jprivate, val) < 0)
    return VAR_BADVAL;
  if (parray[p].jprivate)
    pprintf(p, "Your journal will be private.\n");
  else
    pprintf(p, "Your journal will not be private.\n");
  return VAR_OK;
}

PRIVATE int set_automail(int p, char *var, char *val)
{
  if (set_boolean_var(&parray[p].automail, val) < 0)
    return VAR_BADVAL;
  if (parray[p].automail)
    pprintf(p, "Your games will be mailed to you.\n");
  else
    pprintf(p, "Your games will not be mailed to you.\n");
  return VAR_OK;
}

PRIVATE int set_mailmess(int p, char *var, char *val)
{
  if (!parray[p].registered) {
    pprintf(p, "Unregistered players may not receive messages.\n");
    return VAR_OK;
  }
  if (set_boolean_var(&parray[p].i_mailmess, val) < 0)
    return VAR_BADVAL;
  if (parray[p].i_mailmess)
    pprintf(p, "Your messages will be mailed to you.\n");
  else
    pprintf(p, "Your messages will not be mailed to you.\n");
  return VAR_OK;
}

PRIVATE int set_pgn(int p, char *var, char *val)
{
  if (set_boolean_var(&parray[p].pgn, val) < 0)
    return VAR_BADVAL;
  if (parray[p].pgn)
    pprintf(p, "Games will now be mailed to you in PGN.\n");
  else
    pprintf(p, "Games will now be mailed to you in FICS format.\n");
  return VAR_OK;
}

PRIVATE int set_bell(int p, char *var, char *val)
{
  if (set_boolean_var(&parray[p].bell, val) < 0)
    return VAR_BADVAL;
  if (parray[p].bell)
    pprintf(p, "Bell on.\n");
  else
    pprintf(p, "Bell off.\n");
  return VAR_OK;
}

PRIVATE int set_highlight(int p, char *var, char *val)
{
/*  if (set_boolean_var (&parray[p].highlight, val) < 0) return VAR_BADVAL;
 */
  int v = -1;

  if (!val)
    return VAR_BADVAL;
  if (sscanf(val, "%d", &v) != 1)
    return VAR_BADVAL;
  if ((v < 0) || (v > 15))
    return VAR_BADVAL;

  if ((parray[p].highlight = v)) {
    pprintf(p, "Highlight is now style ");
    pprintf_highlight(p, "%d", v);
    pprintf(p, ".\n");
  } else
    pprintf(p, "Highlight is off.\n");
  return VAR_OK;
}

PRIVATE int
set_style(int p, char *var, char *val)
{
	int v = -1;

	if (!val)
		return VAR_BADVAL;
	if (sscanf(val, "%d", &v) != 1)
		return VAR_BADVAL;
	if (v < 1 || v > MAX_STYLES)
		return VAR_BADVAL;

	parray[p].style = (v - 1);
	pprintf(p, "Style %d set.\n", v);
	return VAR_OK;
}

PRIVATE int
set_flip(int p, char *var, char *val)
{
	if (set_boolean_var(&parray[p].flip, val) < 0)
		return VAR_BADVAL;
	if (parray[p].flip)
		pprintf(p, "Flip on.\n");
	else
		pprintf(p, "Flip off.\n");
	return VAR_OK;
}

PRIVATE int
set_time(int p, char *var, char *val)
{
	int v = -1;

	if (!val)
		return VAR_BADVAL;
	if (sscanf(val, "%d", &v) != 1)
		return VAR_BADVAL;
	if (v < 0 || v > 240)
		return VAR_BADVAL;

	parray[p].d_time = v;
	pprintf(p, "Default time set to %d.\n", v);
	return VAR_OK;
}

PRIVATE int
set_inc(int p, char *var, char *val)
{
	int v = -1;

	if (!val)
		return VAR_BADVAL;
	if (sscanf(val, "%d", &v) != 1)
		return VAR_BADVAL;
	if (v < 0 || v > 300)
		return VAR_BADVAL;

	parray[p].d_inc = v;
	pprintf(p, "Default increment set to %d.\n", v);
	return VAR_OK;
}

PRIVATE int
set_height(int p, char *var, char *val)
{
	int v = -1;

	if (!val)
		return VAR_BADVAL;
	if (sscanf(val, "%d", &v) != 1)
		return VAR_BADVAL;
	if (v < 5 || v > 240)
		return VAR_BADVAL;

	parray[p].d_height = v;
	pprintf(p, "Height set to %d.\n", v);
	return VAR_OK;
}

PRIVATE int
set_width(int p, char *var, char *val)
{
	int v = -1;

	if (!val)
		return VAR_BADVAL;
	if (sscanf(val, "%d", &v) != 1)
		return VAR_BADVAL;
	if (v < 32 || v > 240)
		return VAR_BADVAL;

	parray[p].d_width = v;
	pprintf(p, "Width set to %d.\n", v);
	return VAR_OK;
}

PUBLIC char *
Language(int i)
{ // XXX
	static char *Lang[NUM_LANGS] = {
		"English",
		"Spanish",
		"French",
		"Danish"
	};
	return Lang[i];
}

PRIVATE int
set_language(int p, char *var, char *val)
{
	int	len, gotIt = -1;

	if (!val)
		return VAR_BADVAL;

	len = strlen(val);

	for (int i = 0; i < NUM_LANGS; i++) {
		if (strncasecmp(val, Language(i), len))
			continue;
		if (gotIt >= 0)
			return VAR_BADVAL;
		else
			gotIt = i;
	}

	if (gotIt < 0)
		return VAR_BADVAL;

	parray[p].language = gotIt;

	pprintf(p, "Language set to %s.\n", Language(gotIt));
	return VAR_OK;
}

PRIVATE int
set_promote(int p, char *var, char *val)
{
	if (!val)
		return VAR_BADVAL;

	stolower(val);

	switch (val[0]) {
	case 'q':
		parray[p].promote = QUEEN;
		pprintf(p, "Promotion piece set to QUEEN.\n");
		break;
	case 'r':
		parray[p].promote = ROOK;
		pprintf(p, "Promotion piece set to ROOK.\n");
		break;
	case 'b':
		parray[p].promote = BISHOP;
		pprintf(p, "Promotion piece set to BISHOP.\n");
		break;
	case 'n':
	case 'k':
		parray[p].promote = KNIGHT;
		pprintf(p, "Promotion piece set to KNIGHT.\n");
		break;
	default:
		return VAR_BADVAL;
	}

	return VAR_OK;
}

PRIVATE int
set_prompt(int p, char *var, char *val)
{
	if (!val) {
		if (parray[p].prompt && parray[p].prompt != def_prompt)
			rfree(parray[p].prompt);
		parray[p].prompt = def_prompt;
		return VAR_OK;
	}

	if (!printablestring(val))
		return VAR_BADVAL;

	if (parray[p].prompt != def_prompt)
		rfree(parray[p].prompt);

	const size_t size = strlen(val) + 2;
	parray[p].prompt = rmalloc(size);

	strlcpy(parray[p].prompt, val, size);
	strlcat(parray[p].prompt, " ", size);

	return VAR_OK;
}

PRIVATE int
RePartner(int p, int new)
{
	int	pOld;

	if (p < 0)
		return -1;

	pOld = parray[p].partner;

	if (parray[pOld].partner == p) {
		if (new >= 0) {
			pprintf_prompt(pOld, "Your partner has just chosen "
			    "a new partner.\n");
		} else {
			pprintf_prompt(pOld, "Your partner has just unset "
			    "his/her partner.\n");
		}

		player_withdraw_offers(pOld, -1, PEND_BUGHOUSE);
		player_decline_offers(pOld, -1, PEND_BUGHOUSE);

		player_withdraw_offers(p, -1, PEND_BUGHOUSE);
		player_decline_offers(p, -1, PEND_BUGHOUSE);
	}

	player_withdraw_offers(p, -1, PEND_PARTNER);
	player_decline_offers(p, -1, PEND_PARTNER);

	parray[pOld].partner	= -1;
	parray[p].partner	= new;

	return new;
}

PUBLIC int
com_partner(int p, param_list param)
{
	int pNew;

	if (param[0].type == TYPE_NULL) {
		RePartner(p, -1);
		return COM_OK;
	}

	// OK, we're trying to set a new partner.
	pNew = player_find_part_login(param[0].val.word);

	if (pNew < 0 ||
	    parray[pNew].status == PLAYER_PASSWORD ||
	    parray[pNew].status == PLAYER_LOGIN) {
		pprintf(p, "No user named \"%s\" is logged in.\n",
		    param[0].val.word);
		return COM_OK;
	}

	if (pNew == p) {
		pprintf(p, "You can't be your own bughouse partner.\n");
		return COM_OK;
	}

	/*
	 * Now we know a legit partner has been chosen. Is an offer
	 * pending?
	 */
	if (player_find_pendfrom(p, pNew, PEND_PARTNER) >= 0) {
		pprintf(p, "You agree to be %s's partner.\n",
		    parray[pNew].name);
		pprintf_prompt(pNew, "%s agrees to be your partner.\n",
		    parray[p].name);
		player_remove_request(pNew, p, PEND_PARTNER);

		// Make the switch.
		RePartner(p, pNew);
		RePartner(pNew, p);

		return COM_OK;
	}

	// This is just an offer. Make sure a new partner is needed.
	if (parray[pNew].partner >= 0) {
		pprintf(p, "%s already has a partner.\n", parray[pNew].name);
		return COM_OK;
	}

	pprintf(pNew, "\n");
	pprintf_highlight(pNew, "%s", parray[p].name);
	pprintf(pNew, " offers to be your bughouse partner; ");
	pprintf_prompt(pNew, "type \"partner %s\" to accept.\n",
	    parray[p].name);
	pprintf(p, "Making a partnership offer to %s.\n", parray[pNew].name);

	player_add_request(p, pNew, PEND_PARTNER, 0);
	return COM_OK;
}

PRIVATE int
set_partner(int p, char *var, char *val)
{
	if (!val)
		pprintf(p, "Command is obsolete; type \"partner\" to clear "
		    "your partner\n");
	else
		pprintf(p, "Command is obsolete; type \"partner %s\" to "
		    "change your partner\n", val);
	return VAR_OK;
}

PRIVATE int
set_busy(int p, char *var, char *val)
{
	if (!val) {
		parray[p].busy[0] = '\0';
		pprintf(p, "Your \"busy\" string was cleared.\n");
		return VAR_OK;
	}

	if (val && !printablestring(val))
		return VAR_BADVAL;

	if (strlen(val) > 50) {
		pprintf(p, "That string is too long.\n");
		return VAR_BADVAL;
	}

	strlcpy(parray[p].busy, val, sizeof(parray[p].busy));

	pprintf(p, "Your \"busy\" string was set to \" %s\"\n", parray[p].busy);
	return VAR_OK;
}

PRIVATE int
set_plan(int p, char *var, char *val)
{
	int	i;
	int	which;

	if (val && !printablestring(val))
		return VAR_BADVAL;
	if ((which = atoi(var)) > MAX_PLAN)
		return VAR_BADVAL;
	if (which > parray[p].num_plan)
		which = parray[p].num_plan + 1;

	if (which == 0) { // shove from top
		if (parray[p].num_plan >= MAX_PLAN) // free the bottom string
			strfree(parray[p].planLines[parray[p].num_plan - 1]);

		if (parray[p].num_plan) {
			i = (parray[p].num_plan >= MAX_PLAN ? MAX_PLAN - 1 :
			    parray[p].num_plan);
			for (; i > 0; i--) {
				parray[p].planLines[i] =
				    parray[p].planLines[i - 1];
			}
		}

		if (parray[p].num_plan < MAX_PLAN)
			parray[p].num_plan++;

		parray[p].planLines[0] = (val == NULL ? NULL : xstrdup(val));

		pprintf(p, "\nPlan variable %d changed to '%s'.\n", (which + 1),
		    parray[p].planLines[which]);
		pprintf(p, "All other variables moved down.\n");
		return VAR_OK;
	}

	if (which > parray[p].num_plan) {              // new line at bottom
		if (parray[p].num_plan >= MAX_PLAN) {  // shove the old lines up
			if (parray[p].planLines[0] != NULL)
				rfree(parray[p].planLines[0]);
			for (i = 0; i < parray[p].num_plan; i++) {
				parray[p].planLines[i] =
				    parray[p].planLines[i + 1];
			}
		} else {
			parray[p].num_plan++;
		}

		parray[p].planLines[which - 1] = (val == NULL ? NULL :
		    xstrdup(val));
		pprintf(p, "\nPlan variable %d changed to '%s'.\n", which,
		    parray[p].planLines[which - 1]);
		return VAR_OK;
	}

	which--;

	if (parray[p].planLines[which] != NULL)
		rfree(parray[p].planLines[which]);

	if (val != NULL) {
		parray[p].planLines[which] = xstrdup(val);
		pprintf(p, "\nPlan variable %d changed to '%s'.\n",
		    (which + 1),
		    parray[p].planLines[which]);
	} else {
		parray[p].planLines[which] = NULL;

		if (which == (parray[p].num_plan - 1)) {	// clear nulls
								// from bottom
			while (parray[p].num_plan > 0 &&
			    parray[p].planLines[parray[p].num_plan - 1] ==
			    NULL) {
				parray[p].num_plan--;
				pprintf(p, "\nPlan variable %d cleared.\n",
				    (which + 1));
			}
		} else if (which == 0) { // clear nulls from top
			while (which < parray[p].num_plan &&
			       parray[p].planLines[which] == NULL)
				which++;

			if (which != parray[p].num_plan) {
				for (i = which; i < parray[p].num_plan; i++) {
					parray[p].planLines[i - which] =
					    parray[p].planLines[i];
				}
			}

			parray[p].num_plan -= which;
		}
	}

	return VAR_OK;
}

PRIVATE int
set_formula(int p, char *var, char *val)
{
	int	 which;
	player	*me = &parray[p];

#ifdef NO_FORMULAS
	pprintf(p, "Sorry -- not available because of a bug\n");
	return COM_OK;
#else
	if (isdigit(var[1]))
		which = var[1] - '1';
	else
		which = MAX_FORMULA;

	if (val != NULL) {
		val = eatwhite(val);

		if (val[0] == '\0')
			val = NULL;
	}

	if (!SetValidFormula(p, which, val))
		return VAR_BADVAL;

	if (which < MAX_FORMULA) {
		if (val != NULL) {
			while (me->num_formula < which) {
				me->formulaLines[me->num_formula] = NULL;
				(me->num_formula)++;
			}

			if (me->num_formula <= which)
				me->num_formula = (which + 1);

			pprintf(p, "Formula variable f%d set to %s.\n",
			    (which + 1),
			    me->formulaLines[which]);
			return VAR_OK;
		}

		pprintf(p, "Formula variable f%d unset.\n", (which + 1));

		if (which + 1 >= me->num_formula) {
			while (which >= 0 && me->formulaLines[which] == NULL)
				which--;
			me->num_formula = (which + 1);
		}
	} else {
		if (me->formula != NULL)
			pprintf(p, "Formula set to %s.\n", me->formula);
		else
			pprintf(p, "Formula unset.\n");
	}

	return VAR_OK;
#endif
}

PUBLIC var_list variables[] = {
	{"automail",     set_automail},
	{"bell",         set_bell},
	{"busy",         set_busy},
	{"cshout",       set_cshout},
	{"flip",         set_flip},
	{"ginform",      set_ginform},
	{"height",       set_height},
	{"highlight",    set_highlight},
	{"i_game",       set_ginform},
	{"i_login",      set_pinform},
	{"inc",          set_inc},
	{"jprivate",     set_jprivate},
	{"kibitz",       set_kibitz},
	{"kiblevel",     set_kiblevel},
	{"language",     set_language},
	{"mailmess",     set_mailmess},
	{"notifiedby",   set_notifiedby},
	{"open",         set_open},
	{"partner",      set_partner},
	{"pgn",          set_pgn},
	{"pinform",      set_pinform},
	{"private",      set_private},
	{"promote",      set_promote},
	{"prompt",       set_prompt},
	{"rated",        set_rated},
	{"ropen",        set_ropen},
	{"shout",        set_shout},
	{"simopen",      set_sopen},
	{"style",        set_style},
	{"tell",         set_tell},
	{"time",         set_time},
	{"width",        set_width},
	{"0",  set_plan},
	{"1",  set_plan},
	{"2",  set_plan},
	{"3",  set_plan},
	{"4",  set_plan},
	{"5",  set_plan},
	{"6",  set_plan},
	{"7",  set_plan},
	{"8",  set_plan},
	{"9",  set_plan},
	{"10", set_plan},
	{"f1",      set_formula},
	{"f2",      set_formula},
	{"f3",      set_formula},
	{"f4",      set_formula},
	{"f5",      set_formula},
	{"f6",      set_formula},
	{"f7",      set_formula},
	{"f8",      set_formula},
	{"f9",      set_formula},
	{"formula", set_formula},
	{NULL, NULL}
};

PRIVATE int
set_find(char *var)
{
	int	gotIt = -1;
	int	i = 0;
	int	len = strlen(var);

	while (variables[i].name) {
		if (!strncmp(variables[i].name, var, len)) {
			if (len == strlen(variables[i].name))
				return i;
			else if (gotIt >= 0)
				return -VAR_AMBIGUOUS;

			gotIt = i;
		}

		i++;
	}

	if (gotIt >= 0)
		return gotIt;
	return -VAR_NOSUCH;
}

PUBLIC int
var_set(int p, char *var, char *val, int *wh)
{
	int which;

	if (!var)
		return VAR_NOSUCH;
	if ((which = set_find(var)) < 0)
		return -which;

	*wh = which;

	return variables[which].var_func(p, (isdigit(*variables[which].name) ?
	    var : variables[which].name), val);
}

PUBLIC int
com_variables(int p, param_list param)
{
	int	i;
	int	p1, connected;

	if (param[0].type == TYPE_WORD) {
		if (!FindPlayer(p, param[0].val.word, &p1, &connected))
			return COM_OK;
	} else {
		p1 = p;
		connected = 1;
	}

	pprintf(p, "Variable settings of %s:\n", parray[p1].name);

	pprintf(p, "   time=%-3d    inc=%-3d    private=%d  jprivate=%d  "
	    "Lang=%s\n",
	    parray[p1].d_time,
	    parray[p1].d_inc,
	    parray[p1].private,
	    parray[p1].jprivate,
	    Language(parray[p1].language));
	pprintf(p, "   rated=%d     ropen=%d    open=%d     simopen=%d\n",
	    parray[p1].rated,
	    parray[p1].ropen,
	    parray[p1].open,
	    parray[p1].sopen);
	pprintf(p, "   shout=%d     cshout=%d   kib=%d      tell=%d      "
	    "notifiedby=%d\n",
	    parray[p1].i_shout,
	    parray[p1].i_cshout,
	    parray[p1].i_kibitz,
	    parray[p1].i_tell,
	    parray[p1].notifiedby);
	pprintf(p, "   pin=%d       gin=%d      style=%-3d  flip=%d      "
	    "kiblevel=%d\n",
	    parray[p1].i_login,
	    parray[p1].i_game,
	    (parray[p1].style + 1),
	    parray[p1].flip,
	    parray[p1].kiblevel);
	pprintf(p, "   highlight=%d bell=%d     auto=%d     mailmess=%d  "
	    "pgn=%d\n",
	    parray[p1].highlight,
	    parray[p1].bell,
	    parray[p1].automail,
	    parray[p1].i_mailmess,
	    parray[p1].pgn);
	pprintf(p, "   width=%-3d   height=%-3d\n",
	    parray[p1].d_width,
	    parray[p1].d_height);

	if (parray[p1].prompt && parray[p1].prompt != def_prompt)
		pprintf(p, "   Prompt: %s\n", parray[p1].prompt);
	if (parray[p1].partner >= 0) {
		pprintf(p, "   Bughouse partner: %s\n",
		    parray[parray[p1].partner].name);
	}

	if (parray[p1].num_formula) {
		pprintf(p, "\n");

		for (i = 0; i < parray[p1].num_formula; i++) {
			if (parray[p1].formulaLines[i] != NULL)
				pprintf(p, " f%d: %s\n", (i + 1),
				    parray[p1].formulaLines[i]);
			else
				pprintf(p, " f%d:\n", (i + 1));
		}
	}

	if (parray[p1].formula != NULL)
		pprintf(p, "\nFormula: %s\n", parray[p1].formula);
	if (!connected)
		player_remove(p1);
	return COM_OK;
}

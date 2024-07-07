/* command_list.h
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
   Richard Nash			93/10/22	Created
   Markus Uhlin			23/12/10	Cleaned up the file a bit
*/

#ifndef _COMMAND_LIST_H
#define _COMMAND_LIST_H

#include "adminproc.h"
#include "comproc.h"
#include "eco.h"
#include "gameproc.h"
#include "lists.h"
#include "matchproc.h"
#include "obsproc.h"
#include "playerdb.h"
#include "rating_conv.h"
#include "ratings.h"
#include "shutdown.h"
#include "talkproc.h"

extern command_type	command_list[];
extern alias_type	g_alias_list[];

/*
  Parameter string format
  w - a word
  o - an optional word
  d - integer
  p - optional integer
  i - word or integer
  n - optional word or integer
  s - string to end
  t - optional string to end

  If the parameter option is given in lower case then the parameter is
  converted to lower case before being passsed to the function. If it is
  in upper case, then the parameter is passed as typed.
 */
/* Try to keep this list in alpha order, that is the way it is shown to
 * the 'help commands' command.
 */
 /* Name	Options	Functions	Security */
PUBLIC command_type command_list[] = {

  {"abort",             "",     com_abort,      ADMIN_USER },
  {"accept",		"n",	com_accept,	ADMIN_USER },
  {"addlist",           "ww",   com_addlist,    ADMIN_USER },
  {"adjourn",           "",     com_adjourn,    ADMIN_USER },
  {"alias",		"oT",	com_alias,	ADMIN_USER },
  {"allobservers",	"n",	com_allobservers,	ADMIN_USER },
  {"assess",		"oo",	com_assess,	ADMIN_USER },
  {"backward",          "p",    com_backward,   ADMIN_USER },
  {"bell",		"",	com_bell,	ADMIN_USER },
  {"best",		"o",	com_best,	ADMIN_USER },
  {"boards",		"o",	com_boards,	ADMIN_USER },
  {"clearmessages",	"n",	com_clearmessages,	ADMIN_USER },
  {"convert_bcf",	"d",	com_CONVERT_BCF,	ADMIN_USER },
  {"convert_elo",	"d",	com_CONVERT_ELO,	ADMIN_USER },
  {"convert_uscf",	"d",	com_CONVERT_USCF,	ADMIN_USER },
  {"cshout",            "S",    com_cshout,     ADMIN_USER },
  {"date",		"",	com_date,	ADMIN_USER },
  {"decline",		"n",	com_decline,	ADMIN_USER },
  {"draw",		"",	com_draw,	ADMIN_USER },
  {"eco",               "n",    com_eco,        ADMIN_USER },
  {"examine",           "on",   com_examine,    ADMIN_USER },
  {"finger",		"o",	com_stats,	ADMIN_USER },
  {"flag",		"",	com_flag,	ADMIN_USER },
  {"flip",		"",	com_flip,	ADMIN_USER },
  {"forward",           "p",    com_forward,    ADMIN_USER },
  {"games",		"o",	com_games,	ADMIN_USER },
  {"getpi",             "w",    com_getpi,      ADMIN_USER },
  {"goboard",           "w",    com_goboard,    ADMIN_USER },
  {"gonum",             "d",    com_gonum,      ADMIN_USER },
  {"handles",           "w",    com_handles,    ADMIN_USER },
  {"hbest",             "o",    com_hbest,      ADMIN_USER },
  {"help",		"o",	com_help,	ADMIN_USER },
  {"history",		"o",	com_history,	ADMIN_USER },
  {"hrank",		"oo",	com_hrank,	ADMIN_USER },
  {"inchannel",		"n",	com_inchannel,	ADMIN_USER },
  {"index",		"o",	com_index,	ADMIN_USER },
  {"info",              "",	com_info,       ADMIN_USER },
  {"it", 		"T",	com_it,		ADMIN_USER },
  {"journal",		"o",	com_journal,	ADMIN_USER },
  {"jsave",		"wwi",	com_jsave,	ADMIN_USER },
  {"kibitz",		"S",	com_kibitz,	ADMIN_USER },
  {"limits",            "",     com_limits,     ADMIN_USER },
  {"llogons",           "",     com_llogons,    ADMIN_USER },
/*  {"load",		"ww",	com_load,	ADMIN_USER },  */
  {"logons",		"o",	com_logons,	ADMIN_USER },
  {"mailhelp",          "o",    com_mailhelp,   ADMIN_USER },
  {"mailmess",          "",     com_mailmess,   ADMIN_USER },
  {"mailmoves",		"n",	com_mailmoves,	ADMIN_USER },
  {"mailoldmoves",	"o",	com_mailoldmoves, ADMIN_USER },
  {"mailsource",        "o",    com_mailsource, ADMIN_USER },
  {"mailstored",	"wi",	com_mailstored,	ADMIN_USER },
  {"match",		"wt",	com_match,	ADMIN_USER },
  {"messages",		"nT",	com_messages,	ADMIN_USER },
  {"mexamine",          "w",    com_mexamine,   ADMIN_USER },
  {"moretime",          "d",    com_moretime,   ADMIN_USER },
  {"moves",		"n",	com_moves,	ADMIN_USER },
  {"news",	        "o",    com_news,       ADMIN_USER },
  {"next", 		"",	com_more, 	ADMIN_USER },
  {"observe",		"n",	com_observe,	ADMIN_USER },
  {"oldmoves",		"o",	com_oldmoves,	ADMIN_USER },
  {"open",		"",	com_open,	ADMIN_USER },
  {"partner",		"o",	com_partner,	ADMIN_USER },
  {"password",		"WW",	com_password,	ADMIN_USER },
  {"pause",		"",	com_pause,	ADMIN_USER },
  {"pending",		"",	com_pending,	ADMIN_USER },
  {"prefresh",		"",	com_prefresh,	ADMIN_USER },
  {"promote",		"w",	com_promote,	ADMIN_USER },
  {"ptell",		"S",	com_ptell,	ADMIN_USER },
  {"qtell",             "iS",   com_qtell,      ADMIN_USER },
  {"quit",		"",	com_quit,	ADMIN_USER },
  {"rank",		"oo",	com_rank,	ADMIN_USER },
  {"refresh",		"n",	com_refresh,	ADMIN_USER },
  {"revert",            "",     com_revert,     ADMIN_USER },
  {"resign",		"o",	com_resign,	ADMIN_USER },
  {"say",		"S",	com_say,	ADMIN_USER },
  {"servers",		"",	com_servers,	ADMIN_USER },
  {"set",		"wT",	com_set,	ADMIN_USER },
  {"shout",		"T",	com_shout,	ADMIN_USER },
  {"showlist",          "o",    com_showlist,   ADMIN_USER },
  {"simabort",		"",	com_simabort,	ADMIN_USER },
  {"simallabort",	"",	com_simallabort,ADMIN_USER },
  {"simadjourn",	"",	com_simadjourn,	ADMIN_USER },
  {"simalladjourn",	"",	com_simalladjourn,ADMIN_USER },
  {"simgames",		"o",	com_simgames,	ADMIN_USER },
  {"simmatch",		"w",	com_simmatch,	ADMIN_USER },
  {"simnext",		"",	com_simnext,	ADMIN_USER },
  {"simopen",		"",	com_simopen,	ADMIN_USER },
  {"simpass",		"",	com_simpass,	ADMIN_USER },
  {"simprev",           "",     com_simprev,    ADMIN_USER },
  {"smoves",		"wi",	com_smoves,	ADMIN_USER },
  {"sposition",		"ww",	com_sposition,	ADMIN_USER },
  {"statistics",	"",	com_statistics,	ADMIN_USER },
  {"stored",		"o",	com_stored,	ADMIN_USER },
  {"style",		"d",	com_style,	ADMIN_USER },
  {"sublist",           "ww",   com_sublist,    ADMIN_USER },
  {"switch",		"",	com_switch,	ADMIN_USER },
  {"takeback",		"p",	com_takeback,	ADMIN_USER },
  {"tell",		"nS",	com_tell,	ADMIN_USER },
  {"time",		"n",	com_time,	ADMIN_USER },
  {"unalias",		"w",	com_unalias,	ADMIN_USER },
  {"unexamine",         "",     com_unexamine,  ADMIN_USER },
  {"unobserve",		"n",	com_unobserve,	ADMIN_USER },
  {"unpause",		"",	com_unpause,	ADMIN_USER },
  {"uptime",		"",	com_uptime,	ADMIN_USER },
  {"uscf",              "o",    com_uscf,       ADMIN_USER },
  {"variables",		"o",	com_variables,	ADMIN_USER },
  {"whenshut",		"",	com_whenshut,	ADMIN_USER },
  {"whisper",		"S",	com_whisper,	ADMIN_USER },
  {"who",               "T",    com_who,        ADMIN_USER },
  {"withdraw",		"n",	com_withdraw,	ADMIN_USER },
  {"xtell",             "wS",   com_xtell,      ADMIN_USER },
  {"znotify",		"",	com_znotify,	ADMIN_USER },

  {"addcomment",	"wS",	com_addcomment,    ADMIN_ADMIN },
  {"addplayer",		"WWS",	com_addplayer,	   ADMIN_ADMIN },
  {"adjudicate",	"www",	com_adjudicate,	   ADMIN_ADMIN },
  {"ahelp",             "o",    com_adhelp,        ADMIN_ADMIN },
  {"admin",             "",     com_admin,         ADMIN_ADMIN },
  {"anews",             "o",    com_anews,         ADMIN_ADMIN },
  {"announce",		"S",	com_announce,      ADMIN_ADMIN },
  {"annunreg",          "S",    com_annunreg,      ADMIN_ADMIN },
  {"asetv",             "wS",   com_asetv,         ADMIN_ADMIN },
  {"asetadmin",         "wd",   com_asetadmin,     ADMIN_ADMIN },
  {"asetblitz",         "wdpppp",com_asetblitz,    ADMIN_ADMIN },
  {"asetemail",		"wO",	com_asetemail,	   ADMIN_ADMIN },
  {"asethandle",        "WW",   com_asethandle,    ADMIN_ADMIN },
  {"asetlight",         "wdpppp",com_asetlight,    ADMIN_ADMIN },
  {"asetpasswd",        "wW",   com_asetpasswd,    ADMIN_ADMIN },
  {"asetrealname",      "wT",   com_asetrealname,  ADMIN_ADMIN },
  {"asetstd",           "wdpppp",com_asetstd,      ADMIN_ADMIN },
  {"asetwild",          "wdpppp",com_asetwild,     ADMIN_ADMIN },
  {"chkip",             "w",    com_checkIP,       ADMIN_ADMIN },
  {"chkgame",           "i",    com_checkGAME,     ADMIN_ADMIN },
  {"chkpl",             "w",    com_checkPLAYER,   ADMIN_ADMIN },
  {"chksc",             "d",    com_checkSOCKET,   ADMIN_ADMIN },
  {"chkts",		"",	com_checkTIMESEAL, ADMIN_ADMIN },
  {"cmuzzle",           "o",    com_cmuzzle,       ADMIN_ADMIN },
  {"cnewsi",            "S",    com_cnewsi,        ADMIN_ADMIN },
  {"cnewsf",            "dS",   com_cnewsf,        ADMIN_ADMIN },
  {"canewsi",           "S",    com_canewsi,       ADMIN_ADMIN },
  {"canewsf",           "dS",   com_canewsf,       ADMIN_ADMIN },
  {"muzzle",		"o",	com_muzzle,	   ADMIN_ADMIN },
  {"nuke",              "w",    com_nuke,          ADMIN_ADMIN },
  {"pose",		"wS",	com_pose,	   ADMIN_GOD   },
  {"asetmaxplayers",    "p",    com_asetmaxplayer, ADMIN_ADMIN },
  {"quota",             "p",    com_quota,         ADMIN_ADMIN },
  {"raisedead",         "WO",   com_raisedead,     ADMIN_ADMIN },
  {"remplayer",         "w",    com_remplayer,     ADMIN_ADMIN },
  {"rerank",		"w",	com_fixrank,	   ADMIN_ADMIN },
  {"showcomment",	"w",	com_showcomment,   ADMIN_ADMIN },
  {"shutdown",          "oT",    com_shutdown,      ADMIN_ADMIN },
  {"summon",            "w",	com_summon,        ADMIN_ADMIN },

  {NULL, NULL, NULL, ADMIN_USER}
};

PUBLIC alias_type g_alias_list[] = {
  {"comment",   "addcomment"},
  {"adhelp",    "ahelp"},
  {"w",		"who"},
  {"h",		"help"},
  {"t",		"tell"},
  {"m",		"match"},
  {"go",        "goboard"},
  {"goto",      "goboard"},
  {"f",		"finger"},
  {"a",		"accept"},
  {"saa",       "simallabort"},
  {"saab",	"simallaabort"},
  {"sab",       "simabort"},
  {"sadj",      "simadjourn"},
  {"saadj",     "simalladjourn"},
  {"sh",	"shout"},
  {"sn",	"simnext"},
  {"sp",        "simprev"},
  {"vars",	"variables"},
  {"g",		"games"},
  {"players",	"who a"},
  {"p",		"who a"},
  {"pl",	"who a"},
  {"o",		"observe"},
  {"r",		"refresh"},
  {"re",        "refresh"}, /* So r/re doesn't resign! */
  {"ch",	"channel"},
  {"cls",       "help cls"},
  {"in",	"inchannel"},
  {".",		"tell ."},
  {",",		"tell ,"},
  {"`",         "tell ."},
  {"!",		"shout"},
  {"I",		"it"},
  {"i",		"it"},
  {":",		"it"},
  {"?",		"help"},
  {"exit",	"quit"},
  {"logout",	"quit"},
  {"bye",       "quit"},
  {"*",      	"kibitz"},
  {"#",         "whisper"},
  {"ma",	"match"},
  {"more",      "next"},
  {"n",         "next"},
  {"znotl",     "znotify"},
  {"+",         "addlist"},
  {"-",         "sublist"},
  {"=",         "showlist"},
  {NULL, NULL}
};

#endif /* _COMMAND_LIST_H */

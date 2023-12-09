/* adminproc.h
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
   Markus Uhlin			23/12/09	Sorted the declarations
*/

#ifndef _ADMINPROC_H
#define _ADMINPROC_H

extern int num_anews;

extern int com_addcomment();
extern int com_addplayer();
extern int com_adjudicate(int, param_list);
extern int com_admin();
extern int com_anews();
extern int com_announce();
extern int com_annunreg();
extern int com_asetadmin();
extern int com_asetblitz();
extern int com_asetemail();
extern int com_asethandle();
extern int com_asetlight();
extern int com_asetmaxplayer();
extern int com_asetpasswd();
extern int com_asetrealname();
extern int com_asetstd();
extern int com_asetv();
extern int com_asetwild();
extern int com_canewsf();
extern int com_canewsi();
extern int com_checkGAME();
extern int com_checkIP();
extern int com_checkPLAYER();
extern int com_checkSOCKET();
extern int com_checkTIMESEAL();
extern int com_cmuzzle();
extern int com_cnewsf();
extern int com_cnewsi();
extern int com_createadmnews();
extern int com_muzzle();
extern int com_nuke();
extern int com_pose();
extern int com_quota();
extern int com_raisedead();
extern int com_remplayer();
extern int com_showcomment();
extern int com_summon();
extern int server_shutdown();
extern int strcmpwild();

#endif /* _ADMINPROC_H */

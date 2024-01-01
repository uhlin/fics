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
   Markus Uhlin			24/01/01	Added argument lists
*/

#ifndef _ADMINPROC_H
#define _ADMINPROC_H

#include "command.h"

extern int num_anews;

extern int	com_addcomment(int, param_list);
extern int	com_addplayer(int, param_list);
extern int	com_adjudicate(int, param_list);
extern int	com_admin(int, param_list);
extern int	com_anews(int, param_list);
extern int	com_announce(int, param_list);
extern int	com_annunreg(int, param_list);
extern int	com_asetadmin(int, param_list);
extern int	com_asetblitz(int, param_list);
extern int	com_asetemail(int, param_list);
extern int	com_asethandle(int, param_list);
extern int	com_asetlight(int, param_list);
extern int	com_asetmaxplayer(int, param_list);
extern int	com_asetpasswd(int, param_list);
extern int	com_asetrealname(int, param_list);
extern int	com_asetstd(int, param_list);
extern int	com_asetv(int, param_list);
extern int	com_asetwild(int, param_list);
extern int	com_canewsf(int, param_list);
extern int	com_canewsi(int, param_list);
extern int	com_checkGAME(int, param_list);
extern int	com_checkIP(int, param_list);
extern int	com_checkPLAYER(int, param_list);
extern int	com_checkSOCKET(int, param_list);
extern int	com_checkTIMESEAL(int, param_list);
extern int	com_cmuzzle(int, param_list);
extern int	com_cnewsf(int, param_list);
extern int	com_cnewsi(int, param_list);
extern int	com_muzzle(int, param_list);
extern int	com_nuke(int, param_list);
extern int	com_pose(int, param_list);
extern int	com_quota(int, param_list);
extern int	com_raisedead(int, param_list);
extern int	com_remplayer(int, param_list);
extern int	com_showcomment(int, param_list);
extern int	com_summon(int, param_list);
extern int	strcmpwild(char *, char *);

#endif /* _ADMINPROC_H */

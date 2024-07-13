// SPDX-FileCopyrightText: 2024 Markus Uhlin <maxxe@rpblc.net>
// SPDX-License-Identifier: ISC

#include "stdinclude.h"
#include "common.h"

#include "iset.h"
#include "utils.h"

PUBLIC int
com_iset(int p, param_list param)
{
	if (param[0].type != TYPE_WORD ||
	    param[1].type != TYPE_WORD) {
		pprintf(p, "%s: bad parameters\n", __func__);
		return COM_BADPARAMETERS;
	}

	pprintf(p, "%s: %s %s\n", __func__,
	    param[0].val.word,
	    param[1].val.word);
	return COM_OK;
}

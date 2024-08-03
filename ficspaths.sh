#!/bin/sh
# SPDX-FileCopyrightText: 2023 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC

if [ $# -ne 1 ]; then
	echo "bogus number of args"
	exit 1
fi

HDRPATH=include/ficspaths.h

PREFIX=$1
shift

cat <<EOF >${HDRPATH}
#ifndef _FICSPATHS_H_
#define _FICSPATHS_H_

#define DEFAULT_ADHELP        "${PREFIX}/data/admin"
#define DEFAULT_ADJOURNED     "${PREFIX}/games/adjourned"
#define DEFAULT_BOARDS        "${PREFIX}/data/boards"
#define DEFAULT_BOOK          "${PREFIX}/data/book"
#define DEFAULT_COMHELP       "${PREFIX}/data/com_help"
#define DEFAULT_CONFIG        "${PREFIX}/config"
#define DEFAULT_HELP          "${PREFIX}/data/help"
#define DEFAULT_HISTORY       "${PREFIX}/games/history"
#define DEFAULT_INDEX         "${PREFIX}/data/index"
#define DEFAULT_INFO          "${PREFIX}/data/info"
#define DEFAULT_JOURNAL       "${PREFIX}/games/journal"
#define DEFAULT_LISTS         "${PREFIX}/data/lists"
#define DEFAULT_MESS          "${PREFIX}/data/messages"
#define DEFAULT_NEWS          "${PREFIX}/data/news"
#define DEFAULT_PLAYERS       "${PREFIX}/players"
#define DEFAULT_SOURCE        "${PREFIX}/FICS"
#define DEFAULT_STATS         "${PREFIX}/data/stats"
#define DEFAULT_USAGE         "${PREFIX}/data/usage"
#define DEFAULT_USCF          "${PREFIX}/data/uscf"

#define HELP_DANISH      "${PREFIX}/data/Danish"
#define HELP_FRENCH      "${PREFIX}/data/French"
#define HELP_SPANISH     "${PREFIX}/data/Spanish"

#define MESS_FULL        "${PREFIX}/data/messages/full"
#define MESS_FULL_UNREG  "${PREFIX}/data/messages/full_unreg"

#define USAGE_DANISH     "${PREFIX}/data/usage_danish"
#define USAGE_FRENCH     "${PREFIX}/data/usage_french"
#define USAGE_SPANISH    "${PREFIX}/data/usage_spanish"

#define DAEMON_LOCKFILE "${PREFIX}/fics.pid"
#define DAEMON_LOGFILE "${PREFIX}/fics.log"

#endif
EOF

if [ ! -r ${HDRPATH} ]; then
	echo "fatal: error creating ${HDRPATH}"
	exit 1
fi

echo "created ${HDRPATH}"

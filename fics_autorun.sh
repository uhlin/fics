#!/bin/sh
# SPDX-FileCopyrightText: 2024 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC
#
# This script is suitable to run as a cron job for the dedicated chess
# user.

# You need to change these variables if FICS was built with a
# non-standard home location.
FICS_LOCKFILE=/home/chess/config/fics.pid
FICS_PROG=/home/chess/bin/fics

run_fics()
{
	if [ -x ${FICS_PROG} ]; then
		${FICS_PROG} -d || exit 1
		exit 0
	else
		echo "cannot find the program"
		exit 1
	fi
}

if [ -r $FICS_LOCKFILE ]; then
	PID="$(cat $FICS_LOCKFILE)"

	case "$PID" in
	""|*[!0-9]*)
		echo "pid not numeric"
		exit 1
		;;
	*)
		;;
	esac

	if [ ${#PID} -gt 20 ]; then
		echo "pid too long"
		exit 1
	fi

	if ps -p "$PID" > /dev/null; then
		# already running (good)
		exit 0
	else
		echo "$PID not running. restarting..."
		rm -f $FICS_LOCKFILE
		run_fics
	fi
else
	run_fics
fi

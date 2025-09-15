#!/bin/sh
# SPDX-FileCopyrightText: 2023 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC

i_data_usage () {
	local _src_prefix _dest _files

	_src_prefix=${1}
	_dest=${2}

	_files="
abort
accept
addlist
adjourn
alias
allobservers
assess
backward
bell
best
boards
censor
channel
clearmessages
cshout
date
decline
draw
eco
examine
finger
flag
flip
forward
games
goboard
gonum
handles
hbest
history
hrank
inchannel
it
kibitz
limits
llogons
logons
mailhelp
mailmess
mailmoves
mailoldmoves
mailsource
mailstored
match
messages
mexamine
moretime
moves
news
next
notify
observe
oldmoves
open
password
pause
pending
promote
quit
rank
refresh
resign
revert
say
set
shout
showlist
simabort
simadjourn
simallabort
simalladjourn
simgames
simmatch
simnext
simopen
simpass
simprev
smoves
sposition
statistics
stored
style
sublist
switch
takeback
tell
time
unalias
uncensor
unexamine
unnotify
unobserve
unpause
uptime
variables
whisper
who
withdraw
xtell
znotify
"

	echo "installing..."

	for file in ${_files}; do
		printf "%s -> %s: " "${_src_prefix}/${file}" "${_dest}/${file}"
		if [ -r "${_src_prefix}/${file}" ]; then
			install -m 0644 "${_src_prefix}/${file}" "${_dest}"
			if [ $? -eq 0 ]; then
				echo "ok"
			else
				echo "error"
			fi
		else
			echo "not found"
		fi
	done
}

i_data_usage "${1}" "${2}"

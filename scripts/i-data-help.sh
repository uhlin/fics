#!/bin/sh
# SPDX-FileCopyrightText: 2023 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC

i_data_help () {
	local _src_prefix _dest _files

	_src_prefix=${1}
	_dest=${2}

# login -> ../messages/login*
# logout -> ../messages/logout*
# motd -> ../messages/motd*
# newstuff -> /home/chess/FICS/5001/README.NEW
# register.dist*
	_files="
abort
abuser
accept
addlist
addresses
adjourn
adjournments
adjudication
adm_info
admins
adm_new
alias
allobservers
analysis
assess
backward
bell
best
blindfold
blitz
boards
bughouse
bughouse_not
bughouse_strat
busy
censor
chan_1
channel
channel_list
clearmessages
cls
cls_info
commands
computers
convert_bcf
convert_elo
convert_uscf
courtesyabort
credit
cshout
date
decline
draw
eco
eggo
etiquette
examine
ficsfaq
fics_lingo
finger
fixes
flag
flip
formula
forward
ftp_hints
games
glicko
gm_game
gnotify
goboard
gonum
handle
handles
hbest
_help
help
highlight
history
hrank
inchannel
index
inetchesslib
interfaces
intro_basics
intro_general
intro_information
intro_moving
intro_playing
intro_settings
intro_special
intro_talking
intro_welcome
it
journal
jsave
kibitz
kiblevel
lag
lecture1
lightning
limits
lists
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
motd_help
moves
newrating
news
next
noplay
notes
notify
observe
oldmoves
open
partner
password
pause
pending
policy
prefresh
private
promote
ptell
quit
rank
ratings
refresh
register
resign
revert
say
servers
set
shout
shout_abuse
shout_quota
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
simuls
smoves
soapbox
sourcecode
sposition
standard
statistics
stored
style
sublist
switch
takeback
team
teamgames
tell
time
timeseal
totals
unalias
unexamine
unobserve
unpause
untimed
uptime
uscf_faq
variables
wcmatch
whenshut
whisper
who
wild
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

i_data_help "${1}" "${2}"

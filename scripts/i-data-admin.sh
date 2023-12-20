#!/bin/sh
# SPDX-FileCopyrightText: 2023 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC

i_data_admin () {
	local _src_prefix _dest _files

	_src_prefix=${1}
	_dest=${2}

# adjrequests -> ../stats/player_data/a/adjudicate.messages
# amotd -> ../messages/admotd*
# bug -> ../stats/player_data/b/bug.messages
# computerlist -> ../stats/player_data/c/computerlist.messages
# events -> ../stats/player_data/e/event.messages
# filtered -> ../stats/player_data/f/filter.messages
# helpfiles -> ../stats/player_data/h/helpfiles.messages
# multiaccounts -> ../stats/player_data/m/multiaccounts.messages
# suggestion -> ../stats/player_data/s/suggestion.messages
# vacation -> ../stats/player_data/v/vacation.messages
	_files="
accounts
addaccounts
addcomment
addplayer
adjudicate
adjud_info
admin
admin_commands
admin_hierarchy
ahelp
anews
announce
annunreg
asetadmin
asetblitz
asetemail
asethandle
asetmaxplayer
asetpasswd
asetrealname
asetstd
asetv
asetwild
ban
buglist
canewsf
canewsi
chkgame
chkip
chkpl
chksc
chkts
cmuzzle
cnewsf
cnewsi
createanews
filter
finger
lists
muzzle
nuke
pose
projects
quota
raisedead
register
remplayer
rerank
server_com
showcomment
shutdown
summon
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

i_data_admin "${1}" "${2}"

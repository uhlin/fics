#!/bin/sh
# SPDX-FileCopyrightText: 2023 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC

i_data_lists () {
	local _path _files

	_path="${1}"

# HelpToIndex
# index
# README
	_files="
abuser
admin
ban
blind
cmuzzle
computer
filter
fm
gm
im
muzzle
programmer
removedcom
td
teams
"

	echo "installing..."

	if [ ! -r "${_path}/index" ]; then
		cat <<EOF > "${_path}/index"
abuser 0
admin -1
ban 0
blind 1
cmuzzle 0
computer 1
filter 0
fm 1
gm 1
im 1
muzzle 0
programmer -1
removedcom -1
td -1
teams 1
EOF
		if [ $? -eq 0 ]; then
			echo "created ${_path}/index"
		fi
	fi

	for file in ${_files}; do
		printf "creating '%s': " "${_path}/${file}"
		if [ -r "${_path}/${file}" ]; then
			echo "already exists (good)"
		else
			cat /dev/null > "${_path}/${file}"
			if [ $? -eq 0 ]; then
				echo "ok"
			else
				echo "error"
			fi
		fi
	done
}

i_data_lists "${1}"

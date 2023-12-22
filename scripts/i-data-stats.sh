#!/bin/sh
# SPDX-FileCopyrightText: 2023 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC

i_data_stats () {
	local _path _files

	_path="${1}"
	_files="
logons.log
newratingsV2_data
rank.blitz
rank.std
rank.wild
"

	echo "installing..."

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

i_data_stats "${1}"

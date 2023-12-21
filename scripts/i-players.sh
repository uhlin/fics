#!/bin/sh
# SPDX-FileCopyrightText: 2023 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC

i_players () {
	local _path _dirs

	_path="${1}"
	_dirs="
a b c d e f g h i j k l m
n o p q r s t u v w x y z
"

	echo "installing..."

	for dir in ${_dirs}; do
		printf "creating '%s': " "${_path}/${dir}"
		if [ -d "${_path}/${dir}" ]; then
			echo "already exists (good)"
		else
			install -d "${_path}/${dir}"
			if [ $? -eq 0 ]; then
				echo "done"
			else
				echo "error"
			fi
		fi
	done
}

i_players "${1}"

#!/bin/sh
# SPDX-FileCopyrightText: 2023 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC

i_data_boards_openings () {
	local _src_prefix _dest _files

	_src_prefix=${1}
	_dest=${2}

	_files="
double-d-pawn
double-e-pawn
giouco-piano
kings-gambita
kings-gambita-be7
kings-gambita-fischer
kings-gambita-nc6
kings-gambita-schallop
kings-gambitd-bc5
kings-gambitd-d6
kings-gambitd-falkbeer
kings-gambitd-nf6
kings-gambitd-nimzo
ruy-lopez
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

i_data_boards_openings "${1}" "${2}"

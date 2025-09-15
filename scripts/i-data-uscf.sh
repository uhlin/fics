#!/bin/sh
# SPDX-FileCopyrightText: 2023 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC

i_data_uscf () {
	local _src_prefix _dest _files

	_src_prefix=${1}
	_dest=${2}

	_files="
ak
al
ar
az
beginner
ca_n
ca_s
co
ct
dc
de
fl
foreign
ga
gran_prix
help
hi
ia
id
il
in
ks
ky
la
ma
md
me
mi
mn
mo
ms
mt
national
nc
nd
ne
nh
nj
nm
nv
ny
oh
ok
or
pa
ri
sc
sd
tn
tx
ut
va
vt
wa
wi
wv
wy
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

i_data_uscf "${1}" "${2}"

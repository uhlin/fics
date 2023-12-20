# SPDX-FileCopyrightText: 2023 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC

# Don't provide a default value for DESTDIR. It should be empty.
DESTDIR ?=

BINDIR = $(DESTDIR)$(PREFIX)/bin

install:
	install -d $(BINDIR)
	install -m 0755 fics $(BINDIR)
	install -m 0755 fics_addplayer $(BINDIR)
	install -m 0755 makerank $(BINDIR)
	install -d $(DESTDIR)$(FICS_HOME)/data
	install -d $(DESTDIR)$(FICS_HOME)/data/admin
	$(ROOT)scripts/i-data-admin.sh $(ROOT)data/admin \
	    $(DESTDIR)$(FICS_HOME)/data/admin

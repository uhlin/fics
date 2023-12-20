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
	install -d $(DESTDIR)$(FICS_HOME)/data/boards
	install -d $(DESTDIR)$(FICS_HOME)/data/book
	$(ROOT)scripts/i-data-book.sh $(ROOT)data/book \
	    $(DESTDIR)$(FICS_HOME)/data/book
	install -d $(DESTDIR)$(FICS_HOME)/data/com_help
	install -m 0644 $(ROOT)data/commands $(DESTDIR)$(FICS_HOME)/data
	install -d $(DESTDIR)$(FICS_HOME)/data/help
	$(ROOT)scripts/i-data-help.sh $(ROOT)data/help \
	    $(DESTDIR)$(FICS_HOME)/data/help
	install -d $(DESTDIR)$(FICS_HOME)/data/index
	install -d $(DESTDIR)$(FICS_HOME)/data/info
	install -d $(DESTDIR)$(FICS_HOME)/data/lists
	install -d $(DESTDIR)$(FICS_HOME)/data/messages
	$(ROOT)scripts/i-data-messages.sh $(ROOT)data/messages \
	    $(DESTDIR)$(FICS_HOME)/data/messages
	install -d $(DESTDIR)$(FICS_HOME)/data/news
	install -d $(DESTDIR)$(FICS_HOME)/data/Spanish
	install -d $(DESTDIR)$(FICS_HOME)/data/stats
	install -d $(DESTDIR)$(FICS_HOME)/data/usage
	install -d $(DESTDIR)$(FICS_HOME)/data/uscf

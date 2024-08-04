# SPDX-FileCopyrightText: 2023-2024 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC

# Don't provide a default value for DESTDIR. It should be empty.
DESTDIR ?=

BINDIR = $(DESTDIR)$(PREFIX)/bin
MANDIR = $(DESTDIR)/usr/local/man/man1

install-init:
	install -d $(BINDIR)
	install -m 0755 fics $(BINDIR)
	install -m 0755 fics_addplayer $(BINDIR)
	install -m 0755 fics_autorun.sh $(BINDIR)
	install -m 0755 makerank $(BINDIR)
	install -d $(DESTDIR)$(FICS_HOME)/data
	install -d $(DESTDIR)$(FICS_HOME)/data/admin
	install -d $(DESTDIR)$(FICS_HOME)/data/boards
	install -d $(DESTDIR)$(FICS_HOME)/data/boards/blitz
	install -d $(DESTDIR)$(FICS_HOME)/data/boards/lightning
	install -d $(DESTDIR)$(FICS_HOME)/data/boards/openings
	install -d $(DESTDIR)$(FICS_HOME)/data/boards/standard
	install -m 0644 $(ROOT)data/boards/standard/standard \
	    $(DESTDIR)$(FICS_HOME)/data/boards/standard
	install -m 0644 $(ROOT)data/boards/std.board \
	    $(DESTDIR)$(FICS_HOME)/data/boards
	install -d $(DESTDIR)$(FICS_HOME)/data/boards/wild
	install -d $(DESTDIR)$(FICS_HOME)/data/book
	install -d $(DESTDIR)$(FICS_HOME)/data/com_help
	install -m 0755 $(ROOT)data/com_help/makelinks.sh \
	    $(DESTDIR)$(FICS_HOME)/data/com_help
	install -m 0644 $(ROOT)data/commands $(DESTDIR)$(FICS_HOME)/data
	install -d $(DESTDIR)$(FICS_HOME)/data/help
	install -d $(DESTDIR)$(FICS_HOME)/data/index
	install -d $(DESTDIR)$(FICS_HOME)/data/info
	install -d $(DESTDIR)$(FICS_HOME)/data/lists
	install -d $(DESTDIR)$(FICS_HOME)/data/messages
	install -d $(DESTDIR)$(FICS_HOME)/data/news
	install -d $(DESTDIR)$(FICS_HOME)/data/Spanish
	install -d $(DESTDIR)$(FICS_HOME)/data/stats
	install -d $(DESTDIR)$(FICS_HOME)/data/stats/player_data
	install -d $(DESTDIR)$(FICS_HOME)/data/usage
	install -d $(DESTDIR)$(FICS_HOME)/data/uscf
	install -d $(DESTDIR)$(FICS_HOME)/games
	install -d $(DESTDIR)$(FICS_HOME)/games/adjourned
	install -d $(DESTDIR)$(FICS_HOME)/games/history
	install -d $(DESTDIR)$(FICS_HOME)/games/journal
	install -d $(DESTDIR)$(FICS_HOME)/players

MP_DEPS = $(ROOT)manpages/fics.1\
	$(ROOT)manpages/fics_addplayer.1\
	$(ROOT)manpages/makerank.1

install-manpages: $(MP_DEPS)
	install -d $(MANDIR)
	install -m 0444 $(ROOT)manpages/fics.1 $(MANDIR)
	install -m 0444 $(ROOT)manpages/fics_addplayer.1 $(MANDIR)
	install -m 0444 $(ROOT)manpages/makerank.1 $(MANDIR)

install: install-init
	$(ROOT)scripts/i-data-admin.sh $(ROOT)data/admin \
	    $(DESTDIR)$(FICS_HOME)/data/admin
	$(ROOT)scripts/i-data-boards-openings.sh $(ROOT)data/boards/openings \
	    $(DESTDIR)$(FICS_HOME)/data/boards/openings
	$(ROOT)scripts/i-data-book.sh $(ROOT)data/book \
	    $(DESTDIR)$(FICS_HOME)/data/book
	$(ROOT)scripts/i-data-help.sh $(ROOT)data/help \
	    $(DESTDIR)$(FICS_HOME)/data/help
	$(ROOT)scripts/i-data-index.sh $(ROOT)data/index \
	    $(DESTDIR)$(FICS_HOME)/data/index
	$(ROOT)scripts/i-data-lists.sh $(DESTDIR)$(FICS_HOME)/data/lists
	$(ROOT)scripts/i-data-messages.sh $(ROOT)data/messages \
	    $(DESTDIR)$(FICS_HOME)/data/messages
	$(ROOT)scripts/i-data-stats.sh $(DESTDIR)$(FICS_HOME)/data/stats
	$(ROOT)scripts/i-players.sh \
	    $(DESTDIR)$(FICS_HOME)/data/stats/player_data
	$(ROOT)scripts/i-data-usage.sh $(ROOT)data/usage \
	    $(DESTDIR)$(FICS_HOME)/data/usage
	$(ROOT)scripts/i-data-uscf.sh $(ROOT)data/uscf \
	    $(DESTDIR)$(FICS_HOME)/data/uscf
	$(ROOT)scripts/i-games-adjourned.sh \
	    $(DESTDIR)$(FICS_HOME)/games/adjourned
	$(ROOT)scripts/i-games-history.sh $(DESTDIR)$(FICS_HOME)/games/history
	$(ROOT)scripts/i-games-journal.sh $(DESTDIR)$(FICS_HOME)/games/journal
	$(ROOT)scripts/i-players.sh $(DESTDIR)$(FICS_HOME)/players

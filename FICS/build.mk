# SPDX-FileCopyrightText: 2023 Markus Uhlin <maxxe@rpblc.net>
# SPDX-License-Identifier: ISC

SRC_DIR := FICS/

OBJS = $(SRC_DIR)adminproc.o\
	$(SRC_DIR)algcheck.o\
	$(SRC_DIR)board.o\
	$(SRC_DIR)channel.o\
	$(SRC_DIR)command.o\
	$(SRC_DIR)comproc.o\
	$(SRC_DIR)eco.o\
	$(SRC_DIR)ficsmain.o\
	$(SRC_DIR)formula.o\
	$(SRC_DIR)gamedb.o\
	$(SRC_DIR)gameproc.o\
	$(SRC_DIR)get_tcp_conn.o\
	$(SRC_DIR)legal.o\
	$(SRC_DIR)lists.o\
	$(SRC_DIR)matchproc.o\
	$(SRC_DIR)movecheck.o\
	$(SRC_DIR)multicol.o\
	$(SRC_DIR)network.o\
	$(SRC_DIR)obsproc.o\
	$(SRC_DIR)playerdb.o\
	$(SRC_DIR)rating_conv.o\
	$(SRC_DIR)ratings.o\
	$(SRC_DIR)rmalloc.o\
	$(SRC_DIR)shutdown.o\
	$(SRC_DIR)talkproc.o\
	$(SRC_DIR)utils.o\
	$(SRC_DIR)variable.o\
	$(SRC_DIR)vers.o

AP_OBJS = $(SRC_DIR)fics_addplayer.o\
	$(OBJS)
AP_LDFLAGS =
AP_LDLIBS = -lcrypt

MR_OBJS = $(SRC_DIR)makerank.o
MR_LDFLAGS =
MR_LDLIBS =
# dfree
# memmove

fics: $(OBJS)
	$(E) "  LINK    " $@
	$(Q) $(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)
fics_addplayer: $(AP_OBJS)
	strip --strip-symbol=main $(SRC_DIR)ficsmain.o
	$(E) "  LINK    " $@
	$(Q) $(CXX) $(CXXFLAGS) -o $@ $(AP_OBJS) $(AP_LDFLAGS) $(AP_LDLIBS)
makerank: $(MR_OBJS)
	$(E) "  LINK    " $@
	$(Q) $(CXX) $(CXXFLAGS) -o $@ $(MR_OBJS) $(MR_LDFLAGS) $(MR_LDLIBS)
# EOF

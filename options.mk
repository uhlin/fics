# options.mk

# Locations of the data, players and games directories.
FICS_HOME ?= /home/chess/config

PREFIX ?= /home/chess

CC ?= cc
CFLAGS = -O2 -Wall -g -pipe -std=c11 \
	-Wformat-security \
	-Wshadow \
	-Wsign-compare \
	-Wstrict-prototypes

CXX ?= c++
CXXFLAGS = -O2 -Wall -g -pipe -std=c++17

# C preprocessor flags
CPPFLAGS = -D_DEFAULT_SOURCE=1 -D_FORTIFY_SOURCE=3

LDFLAGS =
LDLIBS = -lbsd -lcrypt
RM ?= @rm -f

E = @echo
Q = @

# addplayer
AP_LDFLAGS =
AP_LDLIBS = -lbsd -lcrypt

# makerank
MR_LDFLAGS =
MR_LDLIBS = -lbsd

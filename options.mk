# options.mk

FICS_HOME ?= /home/_fics
PREFIX ?= /usr/local

CC ?= cc
CFLAGS = -O2 -Wall -pipe -std=c11

CXX ?= c++
CXXFLAGS = -std=c++17

# C preprocessor flags
CPPFLAGS = -D_DEFAULT_SOURCE=1

LDFLAGS =
LDLIBS = -lcrypt
RM ?= @rm -f

E = @echo
Q = @

# addplayer
AP_LDFLAGS =
AP_LDLIBS = -lcrypt

# makerank
MR_LDFLAGS =
MR_LDLIBS =

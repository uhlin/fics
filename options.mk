# options.mk

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

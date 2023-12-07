# Makefile for FICS
# This was originally written under NeXTSTEP
#

# Turn on optimization
# With optimization on you may get errors during compiling.
OPTFLAGS=

# Turn on debugging
#DEBUGFLAGS=-DDEBUG 

# Anything else you care to flag?
#OTHERFLAGS=-traditional-cpp -Wall -g
OTHERFLAGS=-Wall -g -pg 

# Which architectures are you compiling for? (NeXTSTEP)
# ARCHFLAGS=-arch m68k -arch i386

PROGRAM=fics


CC=gcc
#cc doesn't seem to be supported at present.

CFLAGS=${SYSFLAG} ${OPTFLAGS} ${DEBUGFLAGS} ${OTHERFLAGS} ${ARCHFLAGS} 
LNFLAGS=-lm -lcompat -lcrypt

# dfree is temporary, for debugging only, and works only if you are using
# the GNU C library --mann 
#MORESRCS=dfree.c
#MOREOBJS=dfree.o
#you may have to hash out the above two

# This is just for versions distributed with timeseal.
#MORESRCS=timeseal.c
#MOREOBJS=timeseal.o

# Define the system below that you are compiling on.
# Set the flags for your system

SYSFLAG=-DNETBSD #-DTIMESEAL #-DIGNORE_ECO

# The following is needed when using gcc and glibc, as on caissa.
#MAKEDEPMORECFLAGS= -I/usr/local/include -I/usr/local/lib/gcc-lib/sparc-sun-sunos4.1.3/2.6.3/include 

include ./Makefile.common
# include ./Makedist

#run makedepend to fill out the info below
# DO NOT DELETE THIS LINE -- make depend depends on it.

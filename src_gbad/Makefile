# Makefile
#
# GBAD 3.3
#
# NOTE: This makefile is specifically for the GBAD version to be used in PLADS.
#
CC =		gcc
CFLAGS =	-Wall -O3
LDFLAGS =	-O3
# debug CFLAGS and LDFLAGS
#CFLAGS =	-g -pg -Wall -O3
#LDFLAGS =	-g -pg -O3

LDLIBS =	-lm
OBJS = 		compress.o discover.o dot.o evaluate.o extend.o graphmatch.o\
                graphops.o labels.o sgiso.o subops.o utility.o \
                gbad.o actions.o lex.yy.o y.tab.o  
TARGETS =	gbad graph2dot gm

all: $(TARGETS)

gm: gm_main.o $(OBJS)
	$(CC) $(LDFLAGS) -o gm gm_main.o $(OBJS) $(LDLIBS)

graph2dot: graph2dot_main.o $(OBJS)
	$(CC) $(LDFLAGS) -o graph2dot graph2dot_main.o $(OBJS) $(LDLIBS)

gbad: main.o $(OBJS)
	 $(CC) $(LDFLAGS) -o gbad main.o $(OBJS) $(LDLIBS)

%.o: %.c gbad.h
	$(CC) -c $(CFLAGS) $< -o $@

install:
	cp $(TARGETS) ../bin

clean:
	/bin/rm -f *.o $(TARGETS)


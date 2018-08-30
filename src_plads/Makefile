# Makefile
#
# PLADS 1.0
#
CC =		gcc
CFLAGS =	-Wall -O3
LDFLAGS =	-O3
# debug CFLAGS and LDFLAGS
#CFLAGS =	-g -pg -Wall -O3
#LDFLAGS =	-g -pg -O3

LDLIBS =	-lm -lrt
OBJS = 		utility.o plads.o
TARGETS =	plads

all: $(TARGETS)

plads: main.o $(OBJS)
	 $(CC) $(LDFLAGS) -o plads main.o $(OBJS) $(LDLIBS)

%.o: %.c plads.h
	$(CC) -c $(CFLAGS) $< -o $@

install:
	cp $(TARGETS) ../bin

clean:
	/bin/rm -f *.o $(TARGETS)


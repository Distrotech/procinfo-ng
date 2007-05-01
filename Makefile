### Makefile for procinfo-ng

prefix=/usr

CC = g++ 

CFLAGS = -O3 -fno-strict-aliasing -Wall -I/usr/include --pipe
LDFLAGS = -s

#LDLIBS = -levent

### Add to taste:

# CFLAGS  = -g 
# LDFLAGS = -g

# CFLAGS += -DPROC_DIR=\"extra2/\"

# CFLAGS += -DDEBUG
# LDLIBS += -ldmalloc

# CFLAGS += -pg
# LDFLAGS = -pg

### End of configurable options.

all:	procinfo

clean:
	rm -f procinfo procinfo.0 *.o *~ out

.PHONY: clean all

procinfo: procinfo.cpp routines.cpp Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) procinfo.cpp -o $@

#procinfo.o: procinfo.cpp procinfo.h
#	$(CC) $(CFLAGS) procinfo.cpp -o procinfo.o

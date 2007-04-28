### Makefile for procinfo-ng

prefix=/usr

CC = g++ 

CFLAGS = -Os -fno-strict-aliasing -Wall -I/usr/include
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

all:		procinfo

procinfo:	procinfo.o

clean:
	rm -f procinfo procinfo.0 *.o *~ out

procinfo.o : procinfo.cpp procinfo.h

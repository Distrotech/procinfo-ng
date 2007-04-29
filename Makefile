### Makefile for procinfo-ng

prefix=/usr

CC = g++ 

CFLAGS = -O0 -g3 -fno-strict-aliasing -Wall -I/usr/include
LDFLAGS =

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

procinfo: procinfo.cpp procinfo.h
	$(CC) $(CFLAGS) $(LDFLAGS) $@.cpp -o $@

#procinfo.o: procinfo.cpp procinfo.h
#	$(CC) $(CFLAGS) procinfo.cpp -o procinfo.o


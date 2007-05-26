### Makefile for procinfo-ng

prefix=/usr

CXX = g++

CFLAGS = -O3 --pipe
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
	rm -f procinfo procinfo.0 *.o *~ out config.*

distclean:
	rm -f procinfo procinfo.0 *.o *~ out config.* Makefile

.PHONY: clean all

procinfo: procinfo.cpp routines.cpp Makefile
	$(CXX) $(CFLAGS) $(LDFLAGS) procinfo.cpp -o $@

#procinfo.o: procinfo.cpp procinfo.h
#	$(XX) $(CFLAGS) procinfo.cpp -o procinfo.o

install: procinfo procinfo.8
	-mkdir -p $(prefix)/bin
	install procinfo $(prefix)/bin/procinfo
	-mkdir -p $(prefix)/man/man8
	install -m 644  procinfo.8 $(prefix)/man/man8/procinfo.8

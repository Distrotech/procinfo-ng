### Makefile for procinfo-ng

prefix=/usr

mandir= ${prefix}/share/man

CXX = g++

CFLAGS = -march=athlon64 -O99 --pipe
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
	rm -f procinfo procinfo.exe procinfo.0 *.o *~ out config.*

distclean:
	rm -f procinfo procinfo.exe procinfo.0 *.o *~ out config.* Makefile

.PHONY: clean all

procinfo: procinfo.cpp routines.cpp timeRoutines.cpp \
prettyPrint.cpp \
linux26_procstat.cpp linux26_rendercpupagestat.cpp \
cygwin_procstat.cpp cygwin_rendercpupagestat.cpp \
Makefile
	$(CXX) $(CFLAGS) $(LDFLAGS) procinfo.cpp prettyPrint.cpp -o $@

#procinfo.o: procinfo.cpp procinfo.h
#	$(XX) $(CFLAGS) procinfo.cpp -o procinfo.o

install: procinfo procinfo.8
	-mkdir -p $(DESTDIR)$(prefix)/bin
	install procinfo $(DESTDIR)$(prefix)/bin/procinfo
	-mkdir -p $(DESTDIR)$(mandir)/man8
	install -m 644  procinfo.8 $(DESTDIR)$(mandir)/man8/procinfo.8

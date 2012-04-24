CFLAGS = -O2 -Wall
LDFLAGS = -s
PREFIX = /usr/local

ifeq ($(shell which MagickWand-config>/dev/null && echo y),y)
	WANDCONFIG = MagickWand-config
else
	WANDCONFIG = Wand-config
endif

WANDCFLAGS = `$(WANDCONFIG) --cflags`
WANDCPPFLAGS = `$(WANDCONFIG) --cppflags`
WANDLDFLAGS = `$(WANDCONFIG) --ldflags`
WANDLIBS = `$(WANDCONFIG) --libs`

ifeq ($(shell echo "\#include <term.h>"|$(CC) $(CPPFLAGS) $(WANDCPPFLAGS) $(DEFS) -E - >/dev/null 2>/dev/null && echo y),y)
	DEFS = 
	LIBS = -lm -lncurses
else
	DEFS = -DNO_CURSES
	LIBS = -lm
endif

CFLAGS := $(CFLAGS) $(WANDCFLAGS)
CPPFLAGS := $(CPPFLAGS) $(WANDCPPFLAGS) $(DEFS)
LDFLAGS := $(LDFLAGS) $(WANDLDFLAGS)
LIBS := $(LIBS) $(WANDLIBS)

all: img2xterm man6/img2xterm.6.gz

img2xterm: img2xterm.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

.PHONY: all install clean

man6:
	mkdir man6

man6/img2xterm.6.gz: man6/img2xterm.6 | man6
	gzip -fc $< > $@

man6/img2xterm.6: img2xterm.c | man6 img2xterm
	help2man -s 6 -N -m " " --version-string="git" ./img2xterm -o $@

install: img2xterm
	install -D -m 0755 img2xterm $(PREFIX)/bin/img2xterm
	ln -fs $(PREFIX)/bin/img2xterm $(PREFIX)/bin/img2cow
	install -D -m 0644 man6/img2xterm.6.gz $(PREFIX)/share/man/man6/img2xterm.6.gz
	ln -fs $(PREFIX)/share/man/man6/img2xterm.6.gz $(PREFIX)/share/man/man6/img2cow.6.gz

clean:
	-rm -f img2xterm man6/img2xterm.6 man6/img2xterm.6.gz

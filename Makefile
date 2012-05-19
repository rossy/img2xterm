PREFIX = /usr/local
INSTALL = install
LN = ln -fs

ifeq ($(shell sh -c 'which gcc>/dev/null 2>/dev/null && echo y'), y)
	CC = gcc
	CFLAGS = -O2 -Wall
	LDFLAGS = -s
endif

ifeq ($(shell sh -c 'uname'), Darwin)
	CC = clang
	CFLAGS = -O2 -Wall
	LDFLAGS =
endif

ifeq ($(shell sh -c 'which ncurses5-config>/dev/null 2>/dev/null && echo y'), y)
	DEFS =
	CFLAGS += $(shell ncurses5-config --cflags)
	LIBS = -lm $(shell ncurses5-config --libs)
else
	ifeq ($(shell sh -c 'which ncursesw5-config>/dev/null 2>/dev/null && echo y'), y)
		DEFS =
		CFLAGS += $(shell ncursesw5-config --cflags)
		LIBS = -lm $(shell ncursesw5-config --libs)
	else
		DEFS = -DNO_CURSES
		LIBS = -lm
	endif
endif

LIBPNGCONFIG = libpng-config

CFLAGS := $(CFLAGS) $(shell $(LIBPNGCONFIG) --cflags)
CPPFLAGS := $(CPPFLAGS) $(DEFS) $(shell $(LIBPNGCONFIG) --cppflags)
LDFLAGS := $(LDFLAGS) $(shell $(LIBPNGCONFIG) --ldflags)
LIBS := $(LIBS) $(shell $(LIBPNGCONFIG) --libs) -lz

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
	$(INSTALL) -D -m 0755 img2xterm $(PREFIX)/bin/img2xterm
	$(LN) $(PREFIX)/bin/img2xterm $(PREFIX)/bin/img2cow
	$(INSTALL) -D -m 0644 man6/img2xterm.6.gz $(PREFIX)/share/man/man6/img2xterm.6.gz
	$(LN) $(PREFIX)/share/man/man6/img2xterm.6.gz $(PREFIX)/share/man/man6/img2cow.6.gz

clean:
	-$(RM) img2xterm man6/img2xterm.6.gz

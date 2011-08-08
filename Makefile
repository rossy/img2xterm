CFLAGS = -O2 -Wall
WANDCFLAGS = `Wand-config --cflags --cppflags`
LDFLAGS = -s
WANDLDFLAGS = `Wand-config --ldflags --libs`
DEFS = 
LIBS = -lm -lncurses
PREFIX = /usr/local

all: img2xterm man6/img2xterm.6.gz

img2xterm: img2xterm.c
	$(CC) $(CFLAGS) $(WANDCFLAGS) -o $@ $< $(WANDLDFLAGS) $(LDFLAGS) $(LIBS)

man6:
	mkdir man6

man6/img2xterm.6.gz: man6/img2xterm.6 | man6
	gzip -fc $< > $@

man6/img2xterm.6: img2xterm.c | man6 img2xterm
	help2man -s 6 -N -m " " --version-string="git" ./img2xterm -o $@

.PHONY: all install clean

install: img2xterm
	install -D -m 0755 img2xterm $(PREFIX)/bin/img2xterm
	ln -fs $(PREFIX)/bin/img2xterm $(PREFIX)/bin/img2cow
	install -D -m 0644 man6/img2xterm.6.gz $(PREFIX)/share/man/man6/img2xterm.6.gz
	ln -fs $(PREFIX)/share/man/man6/img2xterm.6.gz $(PREFIX)/share/man/man6/img2cow.6.gz

clean:
	-rm -f img2xterm man6/img2xterm.6 man6/img2xterm.6.gz

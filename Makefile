CFLAGS = -O2 -Wall
LDFLAGS = -s
LIBS = -lncurses
PREFIX = /usr/local

img2xterm: img2xterm.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@ $< `MagickWand-config --cflags --ldflags --libs`

.PHONY: all install clean

all: img2xterm

install: img2xterm img2cow
	install -m 0755 img2xterm $(PREFIX)/bin
	ln -fs $(PREFIX)/bin/img2xterm $(PREFIX)/bin/img2cow

clean:
	-rm -f img2xterm

CFLAGS = -O2 -Wall
LDFLAGS = -s
PREFIX = /usr/local

img2xterm: img2xterm.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< `MagickWand-config --cflags --ldflags --libs`

.PHONY: all install clean

all: img2xterm

install: img2xterm img2cow
	install -m 0755 img2xterm $(PREFIX)/bin
	install -m 0755 img2cow $(PREFIX)/bin

clean:
	-rm -f img2xterm

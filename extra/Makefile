INSTALL = install
COWDIR = /usr/share/cows

ifeq ($(shell which gcc>/dev/null && echo y),y)
	CC = gcc
	CFLAGS = -Wall
endif

ifeq ($(shell uname), Darwin)
	CC = clang
	CFLAGS = -Wall
endif

all: xterm-256color.gpl cows

.PHONY: all cows install-cows clean

xterm-256color.gpl: xterm2gimppal
	./xterm2gimppal > $@

%.cow: %.png ../img2xterm
	../img2xterm -ytcw 1.5 $< $@

../img2xterm:
	@echo "error: img2xterm must be compiled before the cowfiles can be made"
	@echo "       go to the containing directory and run \"make\" first"
	@false

cows: $(patsubst %.png,%.cow,$(wildcard cows/*.png))

install-cows: cows
	$(INSTALL) -D -m 644 cows/*.cow $(COWDIR)

clean:
	-$(RM) cows/*.cow
	-$(RM) xterm-256color.gpl xterm2gimppal

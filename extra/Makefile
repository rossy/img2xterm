
all: xterm-256color.gpl cows

.PHONY: all cows install-cows clean

xterm-256color.gpl: xterm2gimppal
	./xterm2gimppal > $@

%.cow: %.png ../img2xterm
	img2xterm -c $< $@

cows: $(patsubst %.png,%.cow,$(wildcard cows/*.png))

install-cows: cows
	install -m 644 cows/*.cow /usr/share/cows

clean:
	-rm -f xterm-256color.gpl xterm2gimppal

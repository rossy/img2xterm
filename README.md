img2xterm: display images on the terminal
=========================================

img2xterm is a program that can display bitmap images on 256-colour terminals
by converting them into Unicode block characters and xterm compatible control
sequences. Based on software by [lachs0r] [1] and Xebec for creating colourful
[cowfiles] [2], img2xterm improves on the colour selection and block printing
logic, providing cleaner output on terminals with nice bitmap fonts.

This is an example of a cowfile created with img2xterm's `--cow` option:

![Example of img2xterm in action.](http://img2xterm.sooaweso.me/demo.png)

img2xterm uses a modified version of the algorithm used in [xterm256-conv] [3]
in order to have an accurate representation of the upper 240 colours used in
xterm. Modification was needed in order to fix the range of the grey ramp.

[1]: http://srsfckn.biz/cows/img2cow.c
[2]: http://www.nog.net/~tony/warez/cowsay.shtml
[3]: http://frexx.de/xterm-256-notes

Dependencies
------------

Before compilation, make sure you have development versions of [ImageMagick]
[4] (for MagickWand) and [Ncurses] [5] (for terminfo support.)

[4]: http://www.imagemagick.org
[5]: http://www.gnu.org/software/ncurses/ncurses.html

Getting img2xterm
-----------------

  The GNU Autotools are not required. To compile and install from source,
  simply run:

    $ make
    # make install

  A [GIMP] [6] palette containing the upper 240 colours used in xterm is also
  available. It can be used for dithering images before conversion. To install,
  run:

    $ cd extra/
    $ make
    $ cp xterm-256color.gpl ~/.gimp-2.6/palettes/

[6]: http://www.gimp.org

Converting images
-----------------

  To display an image on a compatible 256-color terminal:

    $ img2xterm image.png

  `img2cow` is a symlink to the `img2xterm` command. When invoked in this way,
  the program behaves as if the `--cow` option was used.

  To generate a cowfile:

    $ img2cow image.png image.cow
    # cp image.cow /usr/share/cows

Known issues
------------

 * There is something wrong with the implementation of [CIE94 delta-E] [8].
 * ImageMagick can be glitchy. Perhaps using libpng would be a better idea.

[8]: https://en.wikipedia.org/wiki/Color_difference#CIE94

Copying
-------

To the extent possible under law, the author(s) have dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

[![CC0](http://i.creativecommons.org/p/zero/1.0/80x15.png)](http://creativecommons.org/publicdomain/zero/1.0/)

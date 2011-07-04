/*
 *                THE STRONGEST PUBLIC LICENSE
 *                  Draft 1, November 2010
 *
 * Everyone is permitted to copy and distribute verbatim or modified
 * copies of this license document, and changing it is allowed as long
 * as the name is changed.
 *
 *                  THE STRONGEST PUBLIC LICENSE
 *   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *  ⑨. This license document permits you to DO WHAT THE FUCK YOU WANT TO
 *      as long as you APPRECIATE CIRNO AS THE STRONGEST IN GENSOKYO.
 *
 * This program is distributed in the hope that it will be THE STRONGEST,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * USEFULNESS or FITNESS FOR A PARTICULAR PURPOSE.
 */

/* 
 * img2xterm -- convert images to 256 color block elements for use in cowfiles
 * written in the spirit of img2cow, with modified (bugfixed) code from
 * xterm256-conv2 by Wolfgang Frisch (xororand@frexx.de)
 *
 * compile with
 * cc img2xterm.c `MagickWand-config --cflags --ldflags --libs` -o img2xterm
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ImageMagick/wand/MagickWand.h>
#ifndef NO_CURSES
#include <term.h>
#endif

enum {
	color_undef,
	color_transparent,
};

unsigned char* colortable;
const unsigned char valuerange[] = { 0x00, 0x5F, 0x87, 0xAF, 0xD7, 0xFF };
unsigned long oldfg = color_undef;
unsigned long oldbg = color_undef;

#ifndef NO_CURSES
int use_terminfo = 0;
char* ti_setb;
char* ti_setf;
char* ti_op;
#endif

void xterm2rgb(unsigned char color, unsigned char* rgb)
{
	if (color < 232)
	{
		color -= 16;
		rgb[0] = valuerange[(color / 36) % 6];
		rgb[1] = valuerange[(color / 6) % 6];
		rgb[2] = valuerange[color % 6];
	}
	else
		rgb[0] = rgb[1] = rgb[2] = 8 + (color - 232) * 10;
}

unsigned long rgb2xterm(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned long i = 16, d, ret = 0, smallest_distance = UINT_MAX;
	
	for (; i < 256; i++)
	{
		d = (colortable[i * 3] - r) * (colortable[i * 3] - r) +
			(colortable[i * 3 + 1] - g) * (colortable[i * 3 + 1] - g) +
			(colortable[i * 3 + 2] - b) * (colortable[i * 3 + 2] - b);
		if (d < smallest_distance)
		{
			smallest_distance = d;
			ret = i;
		}
	}
	
	return ret;
}

void bifurcate(FILE* file, unsigned long color1, unsigned long color2)
{
	unsigned long fg = oldfg;
	unsigned long bg = oldbg;
	char* str = "\xe2\x96\x84";
	
	if (color1 == color2)
	{
		bg = color1;
		str = " ";
	}
	else
		if (color2 == color_transparent)
		{
			str = "\xe2\x96\x80";
			bg = color2;
			fg = color1;
		}
		else
		{
			bg = color1;
			fg = color2;
		}
	
#ifndef NO_CURSES
	if (use_terminfo)
	{
		if (bg != oldbg)
		{
			if (bg == color_transparent)
			{
				fputs(ti_op, file);
				oldfg = color_undef;
			}
			else
				fputs(tparm(ti_setb, bg), file);
		}
		
		if (fg != oldfg)
			fputs(tparm(ti_setf, fg), file);
	}
	else
#endif
	if (bg != oldbg)
	{
		if (bg == color_transparent)
			fputs("\e[49", file);
		else
			fprintf(file, "\e[48;5;%lu", bg);
		
		if (fg != oldfg)
			fprintf(file, ";38;5;%lum", fg);
		else
			fputc('m', file);
	}
	else if (fg != oldfg)
		fprintf(file, "\e[38;5;%lum", fg);
	
	oldbg = bg;
	oldfg = fg;
	
	fputs(str, file);
}

unsigned long fillrow(PixelWand** pixels, unsigned long* row,
	unsigned long width)
{
	unsigned long i = 0, lastpx = 0;
	
	for (; i < width; i ++)
	{
		if (PixelGetAlpha(pixels[i]) < 0.5)
			row[i] = color_transparent;
		else
		{
			row[i] = rgb2xterm((unsigned long)(PixelGetRed(pixels[i]) * 255.0),
				(unsigned long)(PixelGetGreen(pixels[i]) * 255.0),
				(unsigned long)(PixelGetBlue(pixels[i]) * 255.0));
			lastpx = i;
		}
	}
	
	return lastpx + 1;
}

void usage(int ret, char* binname)
{
	fprintf(ret ? stderr: stdout,
		"Usage: %s [ -chi ] [ -l stem-length ] [ -m margin ] [ -s stem-margin "
		"]\n    [ infile ] [ outfile ]\n"
		"Convert bitmap images to 256 color block elements suitable for"
		" display in xterm\nand similar terminal emulators.\n\n"
		"Options:\n"
		"  -c, --cow                   generate a cowfile header\n"
		"                              (default behavior if invoked as"
		" img2cow)\n"
		"  -h, --help                  display this message\n"
#ifndef NO_CURSES
		"  -i, --terminfo              use terminfo to set the colors of each"
		" block\n"
#endif
		"  -l, --stem-length <length>  length of the speech bubble stem when"
		" generating\n"
		"                              cowfiles (default: 4)\n"
		"  -m, --margin <width>        add a margin to the left of the image\n"
		"  -s, --stem-margin <width>   margin for the speech bubble stem when"
		" generating\n"
		"                              cowfiles (default: 11)\n"
		, binname);
	exit(ret);
}

int main(int argc, char** argv)
{
	char* stdin_str = "/dev/stdin";
	char* infile = stdin_str, * outfile_str = NULL, * binname = *argv, c;
	FILE* outfile = stdout;
	
	size_t width1, width2;
	unsigned long i, j, * row1, * row2, lastpx1, lastpx2, margin = 0;
	
	int cowheader = !strcmp(binname, "img2cow");
	unsigned long stemlen = 4, stemmargin = 11;
	
	MagickWand* science;
	PixelIterator* iterator;
	PixelWand** pixels;
	
	while (*++argv)
		if (**argv == '-')
		{
			while ((c = *++*argv))
				switch (c)
				{
					case '-':
						if (!strcmp("help", ++*argv))
							usage(0, binname);
						else if (!strcmp("cow", *argv))
							cowheader = 1;
						else if (!strcmp("stem-length", *argv))
						{
							if (!*++argv || !sscanf(*argv, "%lu", &stemlen))
								usage(1, binname);
						}
						else if (!strcmp("margin", *argv))
						{
							if (!*++argv || !sscanf(*argv, "%lu", &margin))
								usage(1, binname);
						}
						else if (!strcmp("stem-margin", *argv))
						{
							if (!*++argv || !sscanf(*argv, "%lu", &stemmargin))
								usage(1, binname);
						}
#ifndef NO_CURSES
						else if (!strcmp("terminfo", *argv))
							use_terminfo = 1;
#endif
						else
						{
							fprintf(stderr,
								"%s: unrecognised long option --%s\n",
								binname, *argv);
							usage(1, binname);
						}
						goto nextarg;
					case 'h':
						usage(0, binname);
						break;
					case 'c':
						cowheader = 1;
						break;
#ifndef NO_CURSES
					case 'i':
						use_terminfo = 1;
						break;
#endif
					case 'l':
						if (*++*argv || !*++argv || !sscanf(*argv, "%lu",
							&stemlen))
							usage(1, binname);
						goto nextarg;
					case 'm':
						if (*++*argv || !*++argv || !sscanf(*argv, "%lu",
							&margin))
							usage(1, binname);
						goto nextarg;
					case 's':
						if (*++*argv || !*++argv || !sscanf(*argv, "%lu",
							&stemmargin))
							usage(1, binname);
						goto nextarg;
					default:
						fprintf(stderr, "%s: unrecognised option -%c",
							binname, *--*argv);
						usage(1, binname);
				}
			nextarg:
			continue;
		}
		else if (infile == stdin_str)
			infile = *argv;
		else if (!outfile_str)
			outfile_str = *argv;
		else
			usage(1, binname);
	
#ifndef NO_CURSES
	if (use_terminfo)
	{
		if (setupterm(NULL, fileno(stdout), NULL))
			return 5;
		if (((ti_op = tigetstr("op")) == (void*)-1 &&
			(ti_op = tigetstr("sgr0")) == (void*)-1) ||
			(ti_setb = tigetstr("setb")) == (void*)-1 ||
			(ti_setf = tigetstr("setf")) == (void*)-1 ||
			!tiparm(ti_setb, 255) ||
			!tiparm(ti_setf, 255))
		{
			fprintf(stderr,
				"%s: terminal doesn't support required features\n", binname);
			return 5;
		}
	}
#endif
	
	MagickWandGenesis();
	atexit(MagickWandTerminus);
	science = NewMagickWand();
	
	if (!MagickReadImage(science, infile))
	{
		DestroyMagickWand(science);
		fprintf(stderr, "%s: couldn't open input file %s\n", binname,
			infile == stdin_str ? "<stdin>" : infile);
		return 3;
	}
	
	if (!(iterator = NewPixelIterator(science)))
	{
		DestroyMagickWand(science);
		fprintf(stderr, "%s: out of memory\n", binname);
		return 4;
	}
	
	if (outfile_str)
		if (!(outfile = fopen(outfile_str, "w")))
		{
			fprintf(stderr, "%s: couldn't open output file %s\n", binname,
				outfile_str);
			return 2;
		}
	
	colortable = malloc(256 * 3 * sizeof(unsigned char));
	for (i = 16; i < 256; i ++)
		xterm2rgb(i, colortable + i * 3);
	
	if (cowheader)
	{
		fputs("$the_cow =<<EOC;\n", outfile);
		for (i = 0; i < stemlen; i ++)
		{
			for (j = 0; j < stemmargin + i; j ++)
				fputc(' ', outfile);
			fputs("$thoughts\n", outfile);
		}
	}
	
	pixels = PixelGetNextIteratorRow(iterator, &width1);
	while (pixels)
	{
		row1 = malloc(width1 * sizeof(unsigned long));
		lastpx1 = fillrow(pixels, row1, width1);
		
		if ((pixels = PixelGetNextIteratorRow(iterator, &width2)))
		{
			row2 = malloc(width2 * sizeof(unsigned long));
			lastpx2 = fillrow(pixels, row2, width2);
			if (lastpx2 > lastpx1)
				lastpx1 = lastpx2;
		}
		else
			row2 = NULL;
		
		for (i = 0; i < margin; i ++)
			fputc(' ', outfile);
		
		for (i = 0; i < lastpx1; i ++)
			bifurcate(outfile, i < width1 ? row1[i] : color_transparent,
				i < width2 ? row2 ? row2[i] : color_transparent :
				color_transparent);
		
		free(row1);
		if (row2)
			free(row2);
		
		if ((pixels = PixelGetNextIteratorRow(iterator, &width1)))
#ifndef NO_CURSES
			if (use_terminfo)
			{
				fprintf(outfile, "%s\n", ti_op);
				oldbg = color_transparent;
				oldfg = color_undef;
			}
			else
#endif
			if (oldbg != color_transparent)
			{
				fputs("\e[49m\n", outfile);
				oldbg = color_transparent;
			}
			else
				fputc('\n', outfile);
#ifndef NO_CURSES
		else if (use_terminfo)
			fprintf(outfile, "%s\n", ti_op);
#endif
		else if (oldbg == color_transparent)
			fputs("\e[39m\n", outfile);
		else
			fputs("\e[39;49m\n", outfile);
	}
	
	if (cowheader)
		fputs("\nEOC\n", outfile);
	
	DestroyPixelIterator(iterator);
	DestroyMagickWand(science);
	free(colortable);
	if (outfile != stdout)
		fclose(outfile);
	
	return 0;
}

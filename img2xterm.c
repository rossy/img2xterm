/* img2xterm - convert images to 256 color block elements for use in cowfiles
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <MagickWand/MagickWand.h>

#ifndef NO_CURSES
#include <term.h>
#endif

#ifndef INFINITY
#include <float.h>
#define INFINITY DBL_MAX
#endif

enum {
	color_undef,
	color_transparent,
};

unsigned char* colortable;
double* labtable;

const unsigned char valuerange[] = { 0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff };
unsigned long oldfg = color_undef;
unsigned long oldbg = color_undef;
int perceptive = 0, cowheader;
double chroma_weight = 1.0;

#ifndef NO_CURSES
int use_terminfo = 0;
char* ti_setb;
char* ti_setf;
char* ti_op;
#endif

void srgb2lab(unsigned char red, unsigned char green, unsigned char blue, double* lightness, double* aa, double* bb)
{
	double r = (double)red / 255.0;
	double g = (double)green / 255.0;
	double b = (double)blue / 255.0;

	double rl = r <= 0.4045 ? r / 12.92 : pow((r + 0.055) / 1.055, 2.4);
	double gl = g <= 0.4045 ? g / 12.92 : pow((g + 0.055) / 1.055, 2.4);
	double bl = b <= 0.4045 ? b / 12.92 : pow((b + 0.055) / 1.055, 2.4);

	double x = 0.4124564 * rl + 0.3575761 * gl + 0.1804375 * bl;
	double y = 0.2126729 * rl + 0.7151522 * gl + 0.0721750 * bl;
	double z = 0.0193339 * rl + 0.1191920 * gl + 0.9503041 * bl;

	double xn = x / 0.95047;
	double yn = y;
	double zn = z / 1.08883;

	double fxn = xn > (216.0 / 24389.0) ? pow(xn, 1.0 / 3.0)
		: (841.0 / 108.0) * xn + (4.0 / 29.0);
	double fyn = yn > (216.0 / 24389.0) ? pow(yn, 1.0 / 3.0)
		: (841.0 / 108.0) * yn + (4.0 / 29.0);
	double fzn = zn > (216.0 / 24389.0) ? pow(zn, 1.0 / 3.0)
		: (841.0 / 108.0) * zn + (4.0 / 29.0);

	*lightness = 116.0 * fyn - 16.0;
	*aa = (500.0 * (fxn - fyn)) * chroma_weight;
	*bb = (200.0 * (fyn - fzn)) * chroma_weight;
}

void srgb2yiq(unsigned char red, unsigned char green, unsigned char blue, double* y, double* i, double* q)
{
	double r = (double)red / 255.0;
	double g = (double)green / 255.0;
	double b = (double)blue / 255.0;

	*y =   0.299    * r +  0.587    * g +  0.144    * b;
	*i =  (0.595716 * r + -0.274453 * g + -0.321263 * b) * chroma_weight;
	*q =  (0.211456 * r + -0.522591 * g +  0.311135 * b) * chroma_weight;
}

double cie94(double l1, double a1, double b1, double l2, double a2, double b2)
{
	const double kl = 1;
	const double k1 = 0.045;
	const double k2 = 0.015;

	double c1 = sqrt(a1 * a1 + b1 * b1);
	double c2 = sqrt(a2 * a2 + b2 * b2);
	double dl = l1 - l2;
	double dc = c1 - c2;
	double da = a1 - a2;
	double db = b1 - b2;
	double dh = sqrt(da * da + db * db - dc * dc);

	double t1 = dl / kl;
	double t2 = dc / (1 + k1 * c1);
	double t3 = dh / (1 + k2 * c1);

	return sqrt(t1 * t1 + t2 * t2 + t3 * t3);
}

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

unsigned long rgb2xterm_cie94(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned long i = 16, ret = 0;
	double d, smallest_distance = INFINITY;
	double l, aa, bb;

	srgb2lab(r, g, b, &l, &aa, &bb);

	for (; i < 256; i++)
	{
		d = cie94(l, aa, bb, labtable[i * 3], labtable[i * 3 + 1], labtable[i * 3 + 2]);
		if (d < smallest_distance)
		{
			smallest_distance = d;
			ret = i;
		}
	}

	return ret;
}

unsigned long rgb2xterm_yiq(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned long i = 16, ret = 0;
	double d, smallest_distance = INFINITY;
	double y, ii, q;

	srgb2yiq(r, g, b, &y, &ii, &q);

	for (; i < 256; i++)
	{
		d = (y - labtable[i * 3]) * (y - labtable[i * 3]) +
			(ii - labtable[i * 3 + 1]) * (ii - labtable[i * 3 + 1]) +
			(q - labtable[i * 3 + 2]) * (q - labtable[i * 3 + 2]);
		if (d < smallest_distance)
		{
			smallest_distance = d;
			ret = i;
		}
	}

	return ret;
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

void bifurcate(FILE* file, unsigned long color1, unsigned long color2, char* bstr)
{
	unsigned long fg = oldfg;
	unsigned long bg = oldbg;
	char* str = cowheader ? "\\N{U+2584}" : "\xe2\x96\x84";

	if (color1 == color2)
	{
		bg = color1;
		if (bstr && bg == color_transparent)
		{
			fg = color_undef;
			str = bstr;
		}
		else
			str = " ";
	}
	else
		if (color2 == color_transparent)
		{
			str = cowheader ? "\\N{U+2580}" : "\xe2\x96\x80";
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
			fputs(cowheader ? "\\e[49m" : "\e[49m", file);
		else
			fprintf(file, cowheader ? "\\e[48;5;%lum" : "\e[48;5;%lum", bg);
	}

	if (fg != oldfg)
	{
		if (fg == color_undef)
			fputs(cowheader ? "\\e[39m" : "\e[39m", file);
		else
			fprintf(file, cowheader ? "\\e[38;5;%lum" : "\e[38;5;%lum", fg);
	}

	oldbg = bg;
	oldfg = fg;

	fputs(str, file);
}

unsigned long fillrow(PixelWand** pixels, unsigned long* row, unsigned long width)
{
	unsigned long i = 0, lastpx = 0;

	switch (perceptive)
	{
		case 0:
			for (; i < width; i ++)
			{
				if (PixelGetAlpha(pixels[i]) < 0.5)
					row[i] = color_transparent;
				else
				{
					row[i] = rgb2xterm(
						(unsigned long)(PixelGetRed(pixels[i]) * 255.0),
						(unsigned long)(PixelGetGreen(pixels[i]) * 255.0),
						(unsigned long)(PixelGetBlue(pixels[i]) * 255.0));
					lastpx = i;
				}
			}
			break;
		case 1:
			for (; i < width; i ++)
			{
				if (PixelGetAlpha(pixels[i]) < 0.5)
					row[i] = color_transparent;
				else
				{
					row[i] = rgb2xterm_cie94(
						(unsigned long)(PixelGetRed(pixels[i]) * 255.0),
						(unsigned long)(PixelGetGreen(pixels[i]) * 255.0),
						(unsigned long)(PixelGetBlue(pixels[i]) * 255.0));
					lastpx = i;
				}
			}
			break;
		case 2:
			for (; i < width; i ++)
			{
				if (PixelGetAlpha(pixels[i]) < 0.5)
					row[i] = color_transparent;
				else
				{
					row[i] = rgb2xterm_yiq(
						(unsigned long)(PixelGetRed(pixels[i]) * 255.0),
						(unsigned long)(PixelGetGreen(pixels[i]) * 255.0),
						(unsigned long)(PixelGetBlue(pixels[i]) * 255.0));
					lastpx = i;
				}
			}
			break;
	}

	return lastpx + 1;
}

void usage(int ret, const char* binname)
{
	fprintf(ret ? stderr: stdout,
"\
Usage: %s [ options ] [ infile ] [ outfile ]\n\n\
Convert bitmap images to 256 color block elements suitable for display in xterm\n\
and similar terminal emulators.\n\n\
Options:\n\
  -c, --cow                   generate a cowfile header\n\
                              (default behavior if invoked as img2cow)\n\
  -h, --help                  display this message\n"
#ifndef NO_CURSES
"\
  -i, --terminfo              use terminfo to set the colors of each block\n\
"
#endif
"\
  -l, --stem-length [length]  length of the speech bubble stem when generating\n\
                              cowfiles (default: 4)\n\
  -m, --margin [width]        add a margin to the left of the image\n\
  -p, --perceptive            use the CIE94 color difference formula for color\n\
                              conversion instead of simple RGB linear distance\n\
  -s, --stem-margin [width]   margin for the speech bubble stem when generating\n\
                              cowfiles (default: 11)\n\
  -t, --stem-continue         continue drawing the speech bubble stem into the\n\
                              transparent areas of the image\n\
  -w, --chroma-weight [mult]  weighting for chroma in --perceptive and --yiq\n\
                              modes (default: 1)\n\
  -y, --yiq                   use linear distance in the YIQ color space\n\
                              instead of RGB for color conversion\n\
\n\
If the output file name is omitted the image is written to stdout. If both the\n\
input file name and the output file name are missing, img2xterm will act as a\n\
filter.\n\
\n\
Examples:\n\
  img2xterm -yw 2 image.png   display a bitmap image on the terminal using the\n\
                              YIQ color space for conversion with weighted\n\
                              chroma\n\
  img2xterm banner.png motd   save a bitmap image as a text file to display\n\
                              later\n\
  img2cow rms.png rms.cow     create a cowfile (assuming img2cow is a link to\n\
                              img2xterm)\n\
"
		, binname);
	exit(ret);
}

const char* basename2(const char* string)
{
	const char* ret = string;
	for (; *string; string++)
#if defined(WIN32) || defined(_WIN32)
		if (*string == '/' || *string == '\\')
#endif
		if (*string == '/')
			ret = string + 1;
	return ret;
}

int main(int argc, char** argv)
{
	const char stdin_str[] = "-", * infile = stdin_str, * outfile_str = NULL, * binname = *argv;
	char c;
	FILE* outfile = stdout;

	size_t width1, width2;
	unsigned long i, j, * row1, * row2, color1, color2, lastpx1, lastpx2, margin = 0;

	int background = 0;
	unsigned long stemlen = 4, stemmargin = 11;

	MagickWand* science;
	PixelIterator* iterator;
	PixelWand** pixels;

	cowheader = !memcmp(basename2(binname), "img2cow", 7);

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
						else if (!strcmp("perceptive", *argv))
							perceptive = 1;
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
						else if (!strcmp("stem-continue", *argv))
							background = 1;
#ifndef NO_CURSES
						else if (!strcmp("terminfo", *argv))
							use_terminfo = 1;
#endif
						else if (!strcmp("chroma-weight", *argv))
						{
							if (!*++argv || !sscanf(*argv, "%lf", &chroma_weight))
								usage(1, binname);
						}
						else if (!strcmp("yiq", *argv))
							perceptive = 2;
						else
						{
							fprintf(stderr, "%s: unrecognised long option --%s\n", binname, *argv);
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
						if (*++*argv || !*++argv || !sscanf(*argv, "%lu", &stemlen))
							usage(1, binname);
						goto nextarg;
					case 'm':
						if (*++*argv || !*++argv || !sscanf(*argv, "%lu", &margin))
							usage(1, binname);
						goto nextarg;
					case 'p':
						perceptive = 1;
						break;
					case 's':
						if (*++*argv || !*++argv || !sscanf(*argv, "%lu", &stemmargin))
							usage(1, binname);
						goto nextarg;
					case 't':
						background = 1;
						break;
					case 'w':
						if (*++*argv || !*++argv || !sscanf(*argv, "%lf", &chroma_weight))
							usage(1, binname);
						goto nextarg;
					case 'y':
						perceptive = 2;
						break;
					default:
						fprintf(stderr, "%s: unrecognised option -%c", binname, *--*argv);
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

	if (!cowheader && background == 1)
		background = 0;

#ifndef NO_CURSES
	if (use_terminfo)
	{
		if (setupterm(NULL, fileno(stdout), NULL))
			return 5;
		if (((ti_op = tigetstr("op")) == (void*)-1 &&
			(ti_op = tigetstr("sgr0")) == (void*)-1) ||
			(ti_setb = tigetstr("setb")) == (void*)-1 ||
			(ti_setf = tigetstr("setf")) == (void*)-1 ||
			!tparm(ti_setb, 255) ||
			!tparm(ti_setf, 255))
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
		fprintf(stderr, "%s: couldn't open input file %s\n", binname, infile == stdin_str ? "<stdin>" : infile);
		return 3;
	}

	if (!(iterator = NewPixelIterator(science)))
	{
		DestroyMagickWand(science);
		fprintf(stderr, "%s: out of memory\n", binname);
		return 4;
	}

	if (outfile_str && !(outfile = fopen(outfile_str, "w")))
	{
		fprintf(stderr, "%s: couldn't open output file %s\n", binname, outfile_str);
		return 2;
	}

	if (perceptive)
	{
		unsigned char rgb[3];
		double l, a, b;

		labtable = malloc(256 * 3 * sizeof(double));
		for (i = 16; i < 256; i ++)
		{
			xterm2rgb(i, rgb);
			if (perceptive == 1)
				srgb2lab(rgb[0], rgb[1], rgb[2], &l, &a, &b);
			else
				srgb2yiq(rgb[0], rgb[1], rgb[2], &l, &a, &b);
			labtable[i * 3] = l;
			labtable[i * 3 + 1] = a;
			labtable[i * 3 + 2] = b;
		}
	}
	else
	{
		colortable = malloc(256 * 3 * sizeof(unsigned char));
		for (i = 16; i < 256; i ++)
			xterm2rgb(i, colortable + i * 3);
	}
	if (cowheader)
	{
		fputs("binmode STDOUT, \":utf8\";\n$the_cow =<<EOC;\n", outfile);
		for (i = 0; i < stemlen; stemmargin ++, i ++)
		{
			for (j = 0; j < stemmargin; j ++)
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
			if (background && i == stemmargin)
				bifurcate(outfile, color_transparent, color_transparent, "$thoughts");
			else
				fputc(' ', outfile);

		if (background == 1 && lastpx1 < stemmargin + 1)
			lastpx1 = stemmargin + 1;

		for (i = 0; i < lastpx1; i ++)
		{
			color1 = i < width1 ? row1[i] : color_transparent;
			color2 = i < width2 ? row2 ? row2[i] : color_transparent : color_transparent;
			if (background == 1)
			{
				if (i + margin == stemmargin)
				{
					if (color1 == color_transparent && color2 == color_transparent)
					{
						bifurcate(outfile, color1, color2, "$thoughts");
						continue;
					}
					else
						background = 0;
				}
				else if (i + margin == stemmargin + 1 && (color1 != color_transparent || color2 != color_transparent))
					background = 0;
			}
			bifurcate(outfile, color1, color2, NULL);
		}

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
				fputs(cowheader ? "\\e[49m\n" : "\e[49m\n", outfile);
				oldbg = color_transparent;
			}
			else
				fputc('\n', outfile);
#ifndef NO_CURSES
		else if (use_terminfo)
			fprintf(outfile, "%s\n", ti_op);
#endif
		else if (oldbg == color_transparent)
			fputs(cowheader ? "\\e[39m\n" : "\e[39m\n", outfile);
		else
			fputs(cowheader ? "\\e[39;49m\n" : "\e[39;49m\n", outfile);

		stemmargin ++;
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

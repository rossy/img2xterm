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

unsigned char* colortable;
const unsigned char valuerange[] = { 0x00, 0x5F, 0x87, 0xAF, 0xD7, 0xFF };
const unsigned char basic16[16][3] = {
	{ 0x00, 0x00, 0x00 },
	{ 0xCD, 0x00, 0x00 },
	{ 0x00, 0xCD, 0x00 },
	{ 0xCD, 0xCD, 0x00 },
	{ 0x00, 0x00, 0xEE },
	{ 0xCD, 0x00, 0xCD },
	{ 0x00, 0xCD, 0xCD },
	{ 0xE5, 0xE5, 0xE5 },
	{ 0x7F, 0x7F, 0x7F },
	{ 0xFF, 0x00, 0x00 },
	{ 0x00, 0xFF, 0x00 },
	{ 0xFF, 0xFF, 0x00 },
	{ 0x5C, 0x5C, 0xFF },
	{ 0xFF, 0x00, 0xFF },
	{ 0x00, 0xFF, 0xFF },
	{ 0xFF, 0xFF, 0xFF },
};
unsigned long oldfg = 256;
unsigned long oldbg = 256;

void xterm2rgb(unsigned char color, unsigned char* rgb)
{
	if (color < 16)
	{
		rgb[0] = basic16[color][0];
		rgb[1] = basic16[color][1];
		rgb[2] = basic16[color][2];
	}
	else if (color < 232)
	{
		color -= 16;
		rgb[0] = valuerange[(color / 36) % 6];
		rgb[1] = valuerange[(color / 6) % 6];
		rgb[2] = valuerange[color % 6];
	}
	else if (color < 256)
		rgb[0] = rgb[1] = rgb[2] = 8 + (color - 232) * 10;
	else
		rgb[0] = rgb[1] = rgb[2] = 0;
}

unsigned long rgb2xterm(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned long i = 0, d, ret = 0, smallest_distance = UINT_MAX;
	
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
	char* str = "▄";
	
	if (color1 == color2)
	{
		bg = color1;
		str = " ";
	}
	else
		if (color2 == 256)
		{
			str = "▀";
			bg = color2;
			fg = color1;
		}
		else
		{
			bg = color1;
			fg = color2;
		}
	
	if (bg != oldbg)
	{
		if (bg == 256)
		{
			fputs("\e[0m", file);
			oldfg = 256;
		}
		else
			fprintf(file, "\e[48;5;%lum", bg);
	}
	
	if (fg != oldfg)
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
			row[i] = 256;
		else
		{
			row[i] = rgb2xterm((int)(PixelGetRed(pixels[i]) * 255.0),
				(int)(PixelGetGreen(pixels[i]) * 255.0),
				(int)(PixelGetBlue(pixels[i]) * 255.0));
			lastpx = i;
		}
	}
	
	return lastpx + 1;
}

int main(int argc, char** argv)
{
	if (argc > 3 || (argc == 2 && !strcmp(argv[1], "--help")))
	{
		fprintf(argc == 2 ? stdout : stderr,
			"%s -- convert images to 256 color block elements\n"
			"usage: %s [infile] [outfile]\n", argv[0], argv[0]);
		return argc == 2 ? 0 : 1;
	}
	
	char* infile = argc > 1 ? argv[1] : "/dev/stdin";
	FILE* outfile = argc > 2 ? fopen(argv[2], "w") : stdout;
	
	size_t width1, width2;
	unsigned long i, *row1, *row2, lastpx1, lastpx2;
	
	MagickWand* science;
	PixelIterator* iterator;
	PixelWand** pixels;
	
	if (!outfile)
	{
		fprintf(stderr, "%s: couldn't open output file %s\n", argv[0],
			argc > 2 ? argv[2] : "<stdout>");
		return 1;
	}
	
	MagickWandGenesis();
	atexit(MagickWandTerminus);
	science = NewMagickWand();
	
	if (!MagickReadImage(science, infile))
	{
		DestroyMagickWand(science);
		fprintf(stderr, "%s: couldn't open input file %s\n", argv[0],
			argc > 1 ? infile : "<stdin>");
		return 2;
	}
	
	if (!(iterator = NewPixelIterator(science)))
	{
		DestroyMagickWand(science);
		fprintf(stderr, "%s: out of memory\n", argv[0]);
		return 3;
	}
	
	colortable = malloc(256 * 3 * sizeof(unsigned char));
	for (i = 0; i < 256; i ++)
		xterm2rgb(i, colortable + i * 3);
	
	fputs("\e[0m", outfile);
	while ((pixels = PixelGetNextIteratorRow(iterator, &width1)))
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
		
		for (i = 0; i < lastpx1; i ++)
			bifurcate(outfile, i < width1 ? row1[i] : 256,
				i < width2 ? row2 ? row2[i] : 256 : 256);
		
		free(row1);
		if (row2)
			free(row2);
		
		fputs("\e[0m\n", outfile);
		oldfg = 256;
		oldbg = 256;
	}
	
	DestroyPixelIterator(iterator);
	DestroyMagickWand(science);
	free(colortable);
	
	return 0;
}

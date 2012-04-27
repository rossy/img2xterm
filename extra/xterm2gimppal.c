#include <stdio.h>

const unsigned char valuerange[] = { 0x00, 0x5F, 0x87, 0xAF, 0xD7, 0xFF };

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

int main()
{
	int i = 16;
	unsigned char c[3];
	
	puts("GIMP Palette");
	puts("Name: xterm-256color");
	puts("Columns: 12");
	puts("#");
	
	for (; i < 256; i ++)
	{
		xterm2rgb(i, c);
		printf("%3d %3d %3d #%d, ", c[0], c[1], c[2], i);
		if (i < 232)
			printf("color cube (%d, %d, %d)\n",
				   ((i - 16) / 36) % 6, ((i - 16) / 6) % 6, (i - 16) % 6);
		else
			printf("grey ramp (%d)\n", i - 232);
	}
	
	return 0;
}

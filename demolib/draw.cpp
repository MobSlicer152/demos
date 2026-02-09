#include "pch.h"
#include "demolib.h"

void SetPixel(uint32_t x, uint32_t y, BYTE color)
{
	x = x >= g_width ? g_width - 1 : x;
	y = y >= g_height ? g_height - 1 : y;
	g_framebuffer[y * g_fbStride + x] = color;
}

void DrawPalette(uint32_t width, uint32_t height, uint32_t xi, uint32_t yi, uint32_t perRow, uint32_t rows)
{
	for (int y = yi; y < height; y++)
	{
		for (int x = xi; x < width; x++)
		{
			SetPixel(x, y, (int)((float)y / height * rows) * perRow + (int)((float)x / width * perRow));
		}
	}
}

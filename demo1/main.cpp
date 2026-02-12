#include "../demolib/demolib.h"

void InitPalette()
{
	//for (uint32_t y = 0; y < STANDARD_PALETTE_ROWS; y++)
	//{
	//	for (uint32_t x = 0; x < STANDARD_PALETTE_COLUMNS; x++)
	//	{
	//		float r = (float)x / STANDARD_PALETTE_COLUMNS;
	//		float b = (float)y / STANDARD_PALETTE_ROWS;
	//		float g = 0.0f;
	//		SetColor(y * STANDARD_PALETTE_COLUMNS + x, r * 255, g * 255, b * 255);
	//	}
	//}
	InitStandardPalette();
}

void DrawDemo()
{
	for (uint32_t y = 0; y < FRAMEBUFFER_HEIGHT; y++)
	{
		for (uint32_t x = 0; x < FRAMEBUFFER_WIDTH; x++)
		{
			SetPixel(x, y, FindNearestColor((float)x / FRAMEBUFFER_WIDTH * MAXBYTE, 0, (float)y / FRAMEBUFFER_HEIGHT * MAXBYTE));
		}
	}
	//DrawPalette();
}

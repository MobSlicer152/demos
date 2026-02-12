#include "../demolib/demolib.h"

void InitPalette()
{
	InitStandardPalette();
}

void DrawDemo()
{
	for (uint32_t y = 0; y < FRAMEBUFFER_HEIGHT; y++)
	{
		for (uint32_t x = 0; x < FRAMEBUFFER_WIDTH; x++)
		{
			auto xf = (float)x / FRAMEBUFFER_WIDTH;
			auto yf = (float)y / FRAMEBUFFER_HEIGHT;
			float n = Noise(Vec2(xf * (sinf(g_elapsed) + 1.0f) * 0.5f, yf * 0.5f));
			SetPixel(x, y, Vec4(xf * n, 0.0f, yf * n, 0.0f));
		}
	}
	//DrawPalette();
}

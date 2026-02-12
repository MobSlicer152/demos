#include "../demolib/demolib.h"

void InitPalette()
{
	InitStandardPalette();
}

void DrawDemo()
{
#pragma omp parallel for
	for (int32_t y = 0; y < FRAMEBUFFER_HEIGHT; y++)
	{
		for (int32_t x = 0; x < FRAMEBUFFER_WIDTH; x++)
		{
			auto xf = (float)x / FRAMEBUFFER_WIDTH;
			auto yf = (float)y / FRAMEBUFFER_HEIGHT;
			auto f = 3.5f + sinf(g_elapsed);
			float n =
				(Perlin(Vec2(f * xf + ((sinf(g_elapsed) + 1.0f) * 0.5f), f * yf + (cosf(g_elapsed) + 1.0f) * 0.5f), 64) + 1.0f) *
				0.5f;
			SetPixel(x, y, Vec4(xf + n, n, yf + n, 0.0f));
		}
	}
	// DrawPalette();
}

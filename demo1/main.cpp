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
			auto f = 3.5f;
			auto speed = 0.33f;
			auto s = ((sinf(fmodf(g_elapsed * speed * 2 * PI, 2 * PI)) + 1.0f) * 0.5f);
			auto c = ((cosf(fmodf(g_elapsed * speed * 2 * PI, 2 * PI)) + 1.0f) * 0.5f);
			float n = (Perlin(
						   Vec2(
							   f * xf + s,
							   f * yf + c),
						   64) +
					   1.0f) *
				0.5f;
			SetPixel(x, y, Vec4((xf + n + c) * 0.5f, n, (yf + n + s) * 0.5f, 0.0f));
		}
	}
	//DrawPalette();
}

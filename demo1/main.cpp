#include "../demolib/demolib.h"

void InitPalette()
{
    InitStandardPalette();
}

void DrawDemo()
{
	for (uint32_t y = 0; y < g_height; y++)
	{
		for (uint32_t x = 0; x < g_width; x++)
		{
			SetPixel(x, y, FindNearestColor((float)x / g_width * 256, (float)y / g_height * 256, 0));
		}
	}
	DrawTriangle(
		Vec2(0.5, 0.0), Vec2(0.0, 1.0), Vec2(1.0, 1.0), STANDARD_COLOR(3, 5), STANDARD_COLOR(3, 15), STANDARD_COLOR(3, 20), DrawMode::Wireframe);
}

#include "pch.h"
#include "demolib.h"

void SetPixel(uint32_t x, uint32_t y, BYTE color)
{
	x = x >= g_width ? g_width - 1 : x;
	y = y >= g_height ? g_height - 1 : y;
	g_framebuffer[y * g_fbStride + x] = color;
}

void SetPixel(float x, float y, BYTE color)
{
	SetPixel((uint32_t)(x * g_width), (uint32_t)(y * g_height), color);
}

void DrawPalette(uint32_t width, uint32_t height, uint32_t xi, uint32_t yi, uint32_t perRow, uint32_t rows)
{
	for (uint32_t y = yi; y < height; y++)
	{
		for (uint32_t x = xi; x < width; x++)
		{
			SetPixel(x, y, (BYTE)((float)y / height * rows) * perRow + (BYTE)((float)x / width * perRow));
		}
	}
}

void DrawRectangle(const Vec3& a, const Vec3& b, BYTE color)
{
	auto ax = (uint32_t)(a.x * g_width);
	auto ay = (uint32_t)(a.y * g_height);
	auto bx = (uint32_t)(b.x * g_width);
	auto by = (uint32_t)(b.y * g_height);
	for (uint32_t y = ay; y < by; y++)
	{
		// TODO: use memset (just needs a proper bounds check but i'm lazy)
		for (uint32_t x = ax; x < bx; x++)
		{
			SetPixel(x, y, color);
		}
	}
}

void DrawLine(const Vec3& start, const Vec3& end, BYTE color)
{
	auto x0 = (int32_t)(start.x * g_width);
	auto y0 = (int32_t)(start.y * g_height);
	auto x1 = (int32_t)(end.x * g_width);
	auto y1 = (int32_t)(end.y * g_height);

	int32_t dx = abs(x1 - x0);
	int32_t sx = x0 < x1 ? 1 : -1;
	int32_t dy = -abs(y1 - y0);
	int32_t sy = y0 < y1 ? 1 : -1;

	// TODO: use memset for horizontal lines

	int32_t error = dx + dy;
	while (true)
	{
		SetPixel((uint32_t)x0, (uint32_t)y0, color);
		auto e2 = error * 2;
		if (e2 >= dy)
		{
			if (x0 == x1)
			{
				break;
			}
			error += dy;
			x0 += sx;
		}
		if (e2 <= dx)
		{
			if (y0 == y1)
			{
				break;
			}
			error += dx;
			y0 += sy;
		}
	}
}

static void RasterTriangle(const Vec2& p1, const Vec2& p2, const Vec3& p3, BYTE c1, BYTE c2, BYTE c3, uint32_t textureId)
{
	DrawLine(p1, p2, c1);
	DrawLine(p2, p3, c2);
	DrawLine(p3, p1, c3);
}

void DrawTriangle(const Vec4& p1, const Vec4& p2, const Vec4& p3, BYTE c1, BYTE c2, BYTE c3, DrawMode mode, uint32_t textureId)
{
	RasterTriangle(Vec2(p1.x, p1.y), Vec2(p2.x, p2.y), Vec3(p3.x, p3.y), c1, c2, c3, textureId);
}

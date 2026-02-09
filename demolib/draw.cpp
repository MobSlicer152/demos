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
			SetPixel(x, y, (BYTE)((float)y / height * rows) * perRow + (int)((float)x / width * perRow));
		}
	}
}

void DrawRectangle(const Vec3& a, const Vec3& b, BYTE color)
{
}

void DrawLine(const Vec3& start, const Vec3& end, BYTE color)
{
	const auto& s = start;
	const auto& e = end;

	float dx = abs(e.x - s.x);
	float sx = s.x < e.x ? 0.01 : -0.01;
	float dy = abs(e.y - s.y);
	float sy = s.y < e.y ? 0.01 : -0.01;

	float error = dx + dy;
	float x = s.x;
	float y = s.y;
	while (true)
	{
		SetPixel(x, y, color);
		float e2 = error * 2;
		if (e2 >= dy)
		{
			if (FloatEqual(x, e.x, sx))
			{
				break;
			}
			error += dy;
			x += sx;
		}
		if (e2 <= dx)
		{
			if (FloatEqual(y, e.y, sy))
			{
				break;
			}
			error += dx;
			y += sy;
		}
	}
}

static void RasterTriangle(const Vec2& p1, const Vec2& p2, const Vec3& p3, BYTE c1, BYTE c2, BYTE c3, uint32_t textureId)
{
	DrawLine(p1, p2, c1);
	DrawLine(p2, p3, c2);
	DrawLine(p3, p1, c3);
}

void DrawTriangle(const Vec3& p1, const Vec3& p2, const Vec3& p3, BYTE c1, BYTE c2, BYTE c3, DrawMode mode, uint32_t textureId)
{
	RasterTriangle(Vec2(p1.x, p1.y), Vec2(p2.x, p2.y), Vec3(p3.x, p3.y), c1, c2, c3, textureId);
}

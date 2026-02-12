#include "demolib.h"
#include "pch.h"

void SetPixel(uint32_t x, uint32_t y, BYTE color, bool dither)
{
	x = x >= g_width ? g_width - 1 : x;
	y = y >= g_height ? g_height - 1 : y;

	BYTE c = color;
	if (dither)
	{
		c = Dither(x, y, color);
	}

	g_framebuffer[y * g_fbStride + x] = c;
}

void SetPixel(float x, float y, BYTE color, bool dither)
{
	SetPixel((uint32_t)(x * g_width), (uint32_t)(y * g_height), color, dither);
}

void DrawPalette(uint32_t width, uint32_t height, uint32_t xi, uint32_t yi, uint32_t perRow, uint32_t rows)
{
	for (uint32_t y = yi; y < height; y++)
	{
		for (uint32_t x = xi; x < width; x++)
		{
			SetPixel(x, y, (BYTE)((float)y / height * rows) * perRow + (BYTE)((float)x / width * perRow), false);
		}
	}
}

void DrawRectangle(const Vec3& a, const Vec3& b, BYTE color)
{
	auto ax = (uint32_t)(std::min(a.x, b.x) * g_width);
	auto ay = (uint32_t)(std::min(a.y, b.y) * g_height);
	auto bx = (uint32_t)(std::max(a.x, b.x) * g_width);
	auto by = (uint32_t)(std::max(a.y, b.y) * g_height);
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

static float TriangleArea(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3)
{
	return ((y2 - y1) * (x2 + x1) + (y3 - y2) * (x3 + x2) + (y1 - y3) * (x1 + x3)) / 2.0f;
}

static void RasterTriangle(
	int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, BYTE c1, BYTE c2, BYTE c3, uint32_t textureId)
{
	auto minX = std::min(x1, std::min(x2, x3));
	auto minY = std::min(y1, std::min(y2, y3));
	auto maxX = std::max(x1, std::max(x2, x3));
	auto maxY = std::max(y1, std::max(y2, y3));

	float area = TriangleArea(x1, y1, x2, y2, x3, y3);
	//// get rid of small triangles
	// if (area < 1)
	//{
	//	return;
	// }

	for (auto y = minY; y <= maxY; y++)
	{
		for (auto x = minX; x <= maxX; x++)
		{
			float a = TriangleArea(x, y, x2, y2, x3, y3) / area;
			float b = TriangleArea(x, y, x3, y3, x1, y1) / area;
			float c = TriangleArea(x, y, x1, y1, x2, y2) / area;
			if (a < 0 || b < 0 || c < 0)
			{
				continue;
			}

			SetPixel((uint32_t)x, (uint32_t)y, c1);
		}
	}
}

void DrawTriangle(const Vec4& p1, const Vec4& p2, const Vec4& p3, BYTE c1, BYTE c2, BYTE c3, DrawMode mode, uint32_t textureId)
{
	switch (mode)
	{
	case DrawMode::Shaded:
		RasterTriangle(
			p1.x * g_width,
			p1.y * g_height,
			p2.x * g_width,
			p2.y * g_height,
			p3.x * g_width,
			p3.y * g_height,
			c1,
			c2,
			c3,
			textureId);
		break;
	case DrawMode::Flat:
		break;
	case DrawMode::Wireframe:
		DrawLine(Vec3(p1.x, p1.y, p1.z), Vec3(p2.x, p2.y, p2.z), c1);
		DrawLine(Vec3(p2.x, p2.y, p2.z), Vec3(p3.x, p3.y, p3.z), c2);
		DrawLine(Vec3(p3.x, p3.y, p3.z), Vec3(p1.x, p1.y, p1.z), c3);
		break;
	case DrawMode::Normals:
		break;
	case DrawMode::UV:
		break;
	default:
		break;
	}
}

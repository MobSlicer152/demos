#include "demolib.h"
#include "pch.h"

#define CLAMP_COORDS(x, y)                                                                                                       \
	x = std::clamp((int32_t)x, 0, FRAMEBUFFER_WIDTH - 1);                                                                        \
	y = std::clamp((int32_t)y, 0, FRAMEBUFFER_HEIGHT - 1);
#define FLOAT_TO_INT(p) (uint32_t)(p.x * FRAMEBUFFER_WIDTH), (uint32_t)(p.y * FRAMEBUFFER_HEIGHT)

void SetPixel(uint32_t x, uint32_t y, byte color)
{
	CLAMP_COORDS(x, y);
	g_framebuffer[y * g_fbStride + x] = color;
}

void SetPixel(uint32_t x, uint32_t y, const Vec4& color, bool dither)
{
	byte c = dither ? Dither(x, y, color) : FindNearestColor(color);
	SetPixel(x, y, c);
}

void SetPixel(const Vec2& p, const Vec4& color, bool dither)
{
	SetPixel(FLOAT_TO_INT(p), color, dither);
}

Vec4 GetPixel(uint32_t x, uint32_t y)
{
	CLAMP_COORDS(x, y);
	auto c = g_framebuffer[y * g_fbStride + x];
	byte r, g, b;
	GetColor(c, r, g, b);
	return Vec4(r / 255.0f, g / 255.0f, b / 255.0f);
}

Vec4 GetPixel(const Vec2& p)
{
	return GetPixel(FLOAT_TO_INT(p));
}

void SetDepthPixel(uint32_t x, uint32_t y, float z)
{
	CLAMP_COORDS(x, y);
	g_zBuffer[y * FRAMEBUFFER_WIDTH + x] = z;
}

void SetDepthPixel(const Vec2& p, float z)
{
	SetDepthPixel(FLOAT_TO_INT(p), z);
}

float GetDepthPixel(uint32_t x, uint32_t y)
{
	CLAMP_COORDS(x, y);
	return g_zBuffer[y * FRAMEBUFFER_WIDTH + x];
}

float GetDepthPixel(const Vec2& p)
{
	return GetDepthPixel(FLOAT_TO_INT(p));
}

void DrawPalette(uint32_t width, uint32_t height, uint32_t xi, uint32_t yi, uint32_t perRow, uint32_t rows)
{
	for (uint32_t y = yi; y < height; y++)
	{
		for (uint32_t x = xi; x < width; x++)
		{
			SetPixel(x, y, (byte)((float)y / height * rows) * perRow + (byte)((float)x / width * perRow));
		}
	}
}

void DrawRectangle(const Vec3& a, const Vec3& b, const Vec4& color)
{
	auto ax = (uint32_t)(std::min(a.x, b.x) * FRAMEBUFFER_WIDTH);
	auto ay = (uint32_t)(std::min(a.y, b.y) * FRAMEBUFFER_HEIGHT);
	auto bx = (uint32_t)(std::max(a.x, b.x) * FRAMEBUFFER_WIDTH);
	auto by = (uint32_t)(std::max(a.y, b.y) * FRAMEBUFFER_HEIGHT);
	for (uint32_t y = ay; y < by; y++)
	{
		// TODO: use memset (just needs a proper bounds check but i'm lazy)
		for (uint32_t x = ax; x < bx; x++)
		{
			SetPixel(x, y, color, false);
		}
	}
}

void ClearColor(const Vec4& c)
{
	DrawRectangle(Vec2(0.0f), Vec2(1.0f), c);
}

void ClearDepth(float z)
{
	for (uint32_t y = 0; y < FRAMEBUFFER_HEIGHT; y++)
	{
		// TODO: use memset (just needs a proper bounds check but i'm lazy)
		for (uint32_t x = 0; x < FRAMEBUFFER_WIDTH; x++)
		{
			SetDepthPixel(x, y, z);
		}
	}
}

void DrawLine(const Vec3& start, const Vec3& end, const Vec4& color)
{
	auto x0 = (int32_t)(start.x * FRAMEBUFFER_WIDTH);
	auto y0 = (int32_t)(start.y * FRAMEBUFFER_HEIGHT);
	auto x1 = (int32_t)(end.x * FRAMEBUFFER_WIDTH);
	auto y1 = (int32_t)(end.y * FRAMEBUFFER_HEIGHT);

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

DECLSPEC_ALIGN(64) struct TriangleInfo
{
	// in framebuffer space, post vertex shader
	int32_t x1 = 0;
	int32_t y1 = 0;
	float z1 = 0.0f;
	float w1 = 0.0f;
	int32_t x2 = 0;
	int32_t y2 = 0;
	float z2 = 0.0f;
	float w2 = 0.0f;
	int32_t x3 = 0;
	int32_t y3 = 0;
	float z3 = 0.0f;
	float w3 = 0.0f;

	Vec4 c1 = Vec4(1.0f);
	Vec4 c2 = Vec4(1.0f);
	Vec4 c3 = Vec4(1.0f);

	DrawMode mode = DrawMode::Shaded;
	uint32_t textureId = INVALID_TEXTURE_ID;

	Shader* shader;

	TriangleInfo() = default;

	TriangleInfo(
		const Vec4& p1,
		const Vec4& p2,
		const Vec4& p3,
		const Vec4& c1,
		const Vec4& c2,
		const Vec4& c3,
		DrawMode mode,
		uint32_t textureId,
		Shader* shader)
		: x1(p1.x * FRAMEBUFFER_WIDTH), y1(p1.y * FRAMEBUFFER_HEIGHT), z1(p1.z), w1(p1.w), x2(p2.x * FRAMEBUFFER_WIDTH),
		  y2(p2.y * FRAMEBUFFER_HEIGHT), z2(p2.z), w2(p2.w), x3(p3.x * FRAMEBUFFER_WIDTH), y3(p3.y * FRAMEBUFFER_HEIGHT),
		  z3(p3.z), w3(p3.w), c1(c1), c2(c2), c3(c3), mode(mode), textureId(textureId), shader(shader)
	{
	}
};

static void RasterTriangle(const TriangleInfo& t)
{
	auto minX = (uint32_t)std::min(t.x1, std::min(t.x2, t.x3));
	auto minY = (uint32_t)std::min(t.y1, std::min(t.y2, t.y3));
	auto maxX = (uint32_t)std::max(t.x1, std::max(t.x2, t.x3));
	auto maxY = (uint32_t)std::max(t.y1, std::max(t.y2, t.y3));

	float area = TriangleArea(t.x1, t.y1, t.x2, t.y2, t.x3, t.y3);
	// get rid of small triangles
	if (abs(area) < 1)
	{
		return;
	}

	for (uint32_t y = minY; y <= maxY; y++)
	{
		for (uint32_t x = minX; x <= maxX; x++)
		{
			float a = TriangleArea(x, y, t.x2, t.y2, t.x3, t.y3) / area;
			float b = TriangleArea(x, y, t.x3, t.y3, t.x1, t.y1) / area;
			float g = TriangleArea(x, y, t.x1, t.y1, t.x2, t.y2) / area;
			if (a < 0 || b < 0 || g < 0)
			{
				continue;
			}

			float z = a * t.z1 + b * t.z2 + g * t.z3;
			float w = a * t.w1 + b * t.w2 + g * t.w3;
			auto c = t.c1 * a + t.c2 * b + t.c3 * g;
			FragmentShaderOutput fso = {c}; // default
			if (t.shader && t.shader->fragment)
			{
				FragmentShaderInput fsi = {
					Vec4((float)x / FRAMEBUFFER_WIDTH, (float)y / FRAMEBUFFER_HEIGHT, z, w), g, t.shader->user};
				t.shader->fragment(fsi, fso);
			}

			if (z <= GetDepthPixel(x, y))
			{
				continue;
			}

			SetDepthPixel(x, y, z);
			SetPixel(x, y, BlendColor(fso.color, GetPixel(x, y)));
		}
	}
}

void DrawTriangle(
	const Vec4& p1,
	const Vec4& p2,
	const Vec4& p3,
	const Vec4& c1,
	const Vec4& c2,
	const Vec4& c3,
	DrawMode mode,
	uint32_t textureId,
	Shader* shader)
{
	auto t = TriangleInfo(p1, p2, p3, c1, c2, c3, mode, textureId, shader);
	switch (t.mode)
	{
	case DrawMode::Flat:
	case DrawMode::Shaded:
		RasterTriangle(t);
		break;
	case DrawMode::Wireframe:
		break;
	}
}

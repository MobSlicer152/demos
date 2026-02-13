#include "demolib.h"
#include "pch.h"

#define CLAMP_COORDS(x, y)                                                                                                       \
	x = std::clamp((int32_t)x, 0, FRAMEBUFFER_WIDTH - 1);                                                                        \
	y = std::clamp((int32_t)y, 0, FRAMEBUFFER_HEIGHT - 1);
#define FLOAT_TO_INT(p) (int32_t)((p.x + 1.0f) * 0.5f * FRAMEBUFFER_WIDTH), (int32_t)((p.y + 1.0f) * 0.5f * FRAMEBUFFER_HEIGHT)

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
	auto minX = std::min(t.x1, std::min(t.x2, t.x3));
	auto minY = std::min(t.y1, std::min(t.y2, t.y3));
	auto maxX = std::max(t.x1, std::max(t.x2, t.x3));
	auto maxY = std::max(t.y1, std::max(t.y2, t.y3));

	float area = TriangleArea(t.x1, t.y1, t.x2, t.y2, t.x3, t.y3);
	// get rid of small triangles
	if (abs(area) < 1)
	{
		return;
	}

	for (int32_t y = minY; y <= maxY; y++)
	{
		for (int32_t x = minX; x <= maxX; x++)
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

struct ProcessedTriangle
{
	Vec4 v1;
	Vec4 v2;
	Vec4 v3;
	Vec4 c1;
	Vec4 c2;
	Vec4 c3;

	ProcessedTriangle(
		const Vec4& p1,
		const Vec4& p2,
		const Vec4& p3,
		const Vec4& ci1,
		const Vec4& ci2,
		const Vec4& ci3,
		Mat4 mvp,
		Shader* shader)
	{
		v1 = p1;
		v2 = p2;
		v3 = p3;
		c1 = ci1;
		c2 = ci2;
		c3 = ci3;
		if (shader && shader->vertex)
		{
			VertexShaderInput vsi[3];
			vsi[0] = {v1, mvp, c1, shader->user};
			vsi[1] = {v2, mvp, c2, shader->user};
			vsi[2] = {v3, mvp, c3, shader->user};
			VertexShaderOutput vso[3];
			shader->vertex(vsi[0], vso[0]);
			shader->vertex(vsi[1], vso[1]);
			shader->vertex(vsi[2], vso[2]);
			v1 = vso[0].pos;
			c1 = vso[0].color;
			v2 = vso[1].pos;
			c2 = vso[1].color;
			v3 = vso[2].pos;
			c3 = vso[2].color;
		}
	}
};

void DrawTriangle(
	const Vec4& p1,
	const Vec4& p2,
	const Vec4& p3,
	const Vec4& c1,
	const Vec4& c2,
	const Vec4& c3,
	DrawMode mode,
	uint32_t textureId,
	Mat4 mvp,
	Shader* shader)
{
	auto pt = ProcessedTriangle(p1, p2, p3, c1, c2, c3, mvp, shader);
	auto t = TriangleInfo(pt.v1, pt.v2, pt.v3, pt.c1, pt.c2, pt.c3, mode, textureId, shader);
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

void DrawModel(const CMD2Model& model, DrawMode mode, uint32_t textureId, Mat4 mvp, Shader* shader)
{
}

void DrawModel(
	const Vec3* verts,
	uint32_t vertCount,
	const Vec3i* tris,
	uint32_t triCount,
	DrawMode mode,
	uint32_t textureId,
	Mat4 mvp,
	Shader* shader,
	const Vec3* normals,
	uint32_t normalCount,
	const Vec2* texCoords,
	uint32_t texCoordCount)
{
	for (uint32_t i = 0; i < triCount; i++)
	{
		const auto& tri = tris[i];
		ASSERT_MSG(tri[0] < vertCount && tri[1] < vertCount && tri[2] < vertCount, "Invalid triangle!");
		auto pt =
			ProcessedTriangle(verts[tri[0]], verts[tri[1]], verts[tri[2]], Vec4::WHITE, Vec4::WHITE, Vec4::WHITE, mvp, shader);
		auto t = TriangleInfo(pt.v1, pt.v2, pt.v3, pt.c1, pt.c2, pt.c3, mode, textureId, shader);
		RasterTriangle(t);
	}
}

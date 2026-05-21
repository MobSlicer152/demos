#include "../demolib/demolib.h"

struct Building
{
	byte color;
	byte height;
};

static Building s_city[64];

void GenerateCity()
{
	for (uint8_t y = 0; y < 8; y++)
	{
		for (uint8_t x = 0; x < 8; x++)
		{
			auto val = Perlin(Vec2(x, y) * 0.5, 8);
			auto& b = s_city[y * 8 + x];
			b.color = (byte)(256 - val * 255);
			b.height = (byte)(val * 255);
		}
	}
}

void InitDemoPalette()
{
	InitStandardPalette();
}

void InitDemo()
{
	GenerateCity();
}

void ShutdownDemo()
{
}

void VertexShader(const VertexShaderInput& in, VertexShaderOutput& out)
{
	out.pos = in.mvp * Transform(Vec3(), Quat(Vec3::UP, PI / 2) * Quat(Vec3::UP, g_elapsed)).GetMatrix() *
			  in.vert;
	out.color = in.color;
}

void FragmentShader(const FragmentShaderInput& in, FragmentShaderOutput& out)
{
	out.color = in.color;
}

void Spirograph(float R, float r, float a, float tMax, float scale)
{
	float Rr = R - r;
	float rR = r / R;
	for (auto t = 0.0f; t < tMax; t += 0.01f)
	{
		float x = Rr * cosf(rR * t) + a * cosf((1 - rR) * t);
		float y = Rr * sinf(rR * t) + a * sinf((1 - rR) * t);
		auto p = (Vec2(x, y) * scale + 1.0f) * 0.5f;
		auto x2 = p.x * FRAMEBUFFER_WIDTH;
		auto y2 = p.y * FRAMEBUFFER_HEIGHT;
		auto c = GetPixel(x2, y2);
		SetPixel(x2, y2, Vec4(1.0f - c.r, 1.0f - c.g, 1.0f - c.b, 1.0f));
	}
}

void DrawDemo()
{
	//auto vp = Mat4::Perspective(Deg2Rad(78.0f), g_aspect, 0.1f, 1000.0f) *
	//		  Mat4::LookAt(Vec3::BACKWARD * 5.0f, Vec3(0.0f)) * Mat4::Scale(4.0f);
	//Shader shader = {VertexShader, FragmentShader};
	//DrawTriangle(
	//	Vec4(-0.5, -0.5, 0.0, 1.0),
	//	Vec4(0.5, -0.5, 0.0, 1.0),
	//	Vec4(0.0, 0.5, 0.0, 1.0),
	//	Vec4::RED,
	//	Vec4::GREEN,
	//	Vec4::BLUE,
	//	DrawMode::Shaded,
	//	0,
	//	vp,
	//	&shader);

	auto c = (cosf(g_elapsed * 2 * PI * 0.05f) + 1.0f) * 0.5f;
	auto s = (sinf(g_elapsed * 2 * PI * 0.05f) + 1.0f) * 0.5f;
	auto c1 = HsvToRgb(Vec4(c * 2 * PI, 1.0, 1.0));
	auto c2 = HsvToRgb(Vec4(s * 2 * PI, 1.0, 1.0));

	for (UINT32 x = 0; x < FRAMEBUFFER_WIDTH; x++)
	{
		for (UINT32 y = 0; y < FRAMEBUFFER_HEIGHT; y++)
		{
			auto p = Vec2((float)x / FRAMEBUFFER_WIDTH, (float)y / FRAMEBUFFER_HEIGHT);
			p = p * 2 - 1;
			auto shift = Perlin(Vec2(p.x + s * 3, p.y + c * 3) * 2.0f, 8.0f);
			auto color = Lerp(c1, c2, shift + 0.75f) * 0.8f;
			SetPixel(x, y, color);
		}
	}

	auto sp = fabsf(sinf(g_elapsed * 2 * PI * 0.025f));
	Spirograph(5.0f * sp, 1.02f, 2.0f, sp * 360.0f, 0.15f);
	DrawString(32, 16, Vec4::WHITE, 4.0f, "DEMO BY");
	DrawString(216, FRAMEBUFFER_HEIGHT - 16 - 32, Vec4::WHITE, 4.0f, "MOBSLiCER152");
}

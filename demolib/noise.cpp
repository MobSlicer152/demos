#include "pch.h"

static byte s_perlinPermutation[512];

void InitNoise()
{
	for (int i = 0; i < 256; i++)
	{
		s_perlinPermutation[i] = i;
	}

	for (int i = 255; i > 0; i--)
	{
		std::swap(s_perlinPermutation[(byte)UniformRandom(0, i)], s_perlinPermutation[i]);
	}

	for (int i = 0; i < 256; i++)
	{
		s_perlinPermutation[256 + i] = s_perlinPermutation[i];
	}
}

static byte GetPerm(uint32_t x)
{
	return s_perlinPermutation[x & 255];
}

static constexpr int32_t PERLIN_REPEAT = 32;

static byte PerlinHash(int32_t x, int32_t y)
{
	x = (x % PERLIN_REPEAT + PERLIN_REPEAT) % PERLIN_REPEAT;
	y = (y % PERLIN_REPEAT + PERLIN_REPEAT) % PERLIN_REPEAT;
	return GetPerm(GetPerm(x) + y);
}

static Vec2 GetVector(byte v)
{
	auto h = v & 3;
	switch (h)
	{
	case 0:
		return Vec2(1.0f, 1.0f);
	case 1:
		return Vec2(-1.0f, 1.0f);
	case 2:
		return Vec2(-1.0f, -1.0f);
	default:
	case 3:
		return Vec2(1.0f, -1.0f);
	}
}

float Noise(const Vec2& p)
{
	auto x = fmodf(p.x, 1.0f);
	auto y = fmodf(p.y, 1.0f);
	auto xi = (int)floorf(x);
	auto yi = (int)floorf(y);
	float xf = x - xi;
	float yf = y - yi;

	auto tr = Vec2(xf - 1.0f, yf - 1.0f);
	auto tl = Vec2(xf, yf - 1.0f);
	auto br = Vec2(xf - 1.0f, yf);
	auto bl = Vec2(xf, yf);

	auto vtr = PerlinHash(xi + 1, yi + 1);
	auto vtl = PerlinHash(xi, yi + 1);
	auto vbr = PerlinHash(xi + 1, yi);
	auto vbl = PerlinHash(xi, yi);

	auto dtr = tr.Dot(GetVector(vtr));
	auto dtl = tl.Dot(GetVector(vtl));
	auto dbr = br.Dot(GetVector(vbr));
	auto dbl = bl.Dot(GetVector(vbl));

	auto u = Fade(xf);
	auto v = Fade(yf);

	return Lerp(v, Lerp(u, dbl, dtl), Lerp(u, dbr, dtr));
}

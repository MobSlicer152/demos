#include "pch.h"

static byte s_perlinPermutation[256];

void InitNoise()
{
	for (int i = 0; i < ArraySize(s_perlinPermutation); i++)
	{
		s_perlinPermutation[i] = i;
	}

	for (int i = 0; i < ArraySize(s_perlinPermutation); i++)
	{
		s_perlinPermutation[(byte)UniformRandom(0, ArraySize(s_perlinPermutation) - 1)] = s_perlinPermutation[i];
	}
}

static byte GetPerm(uint32_t x)
{
	return s_perlinPermutation[x % ArraySize(s_perlinPermutation)];
}

static byte PerlinHash(uint32_t x, uint32_t y)
{
	return GetPerm(GetPerm(x) + y);
}

static Vec2 GetVector(byte v)
{
	auto h = v & 3;
	switch (h)
	{
	case 0:
		return Vec2(1.0f, 1.0f);
	case 2:
		return Vec2(-1.0f, 1.0f);
	case 3:
		return Vec2(-1.0f, -1.0f);
	default:
	case 4:
		return Vec2(1.0f, -1.0f);
	}
}

float Noise(const Vec2& p)
{
	auto x = p.x;
	auto y = p.y;
	auto xi = floorf(p.x);
	auto yi = floorf(p.y);
	float xf = x - xi;
	float yf = y - yi;

	auto tr = Vec2(xf - 1.0f, yf - 1.0f);
	auto tl = Vec2(xf, yf - 1.0f);
	auto br = Vec2(xf - 1.0f, yf);
	auto bl = Vec2(xf, yf);

	auto vtr = PerlinHash(x + 1, y + 1);
	auto vtl = PerlinHash(x, y + 1);
	auto vbr = PerlinHash(x + 1, y);
	auto vbl = PerlinHash(x, y);

	auto dtr = tr.Dot(GetVector(vtr));
	auto dtl = tl.Dot(GetVector(vtl));
	auto dbr = br.Dot(GetVector(vbr));
	auto dbl = bl.Dot(GetVector(vbl));

	auto u = Fade(xf);
	auto v = Fade(yf);

	return Lerp(u, Lerp(v, dbl, dtl), Lerp(v, dbr, dtr));
}

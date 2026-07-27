#include "pch.h"

static byte s_perlinPermutation[512];
static Vec2 s_perlinDirs[256];

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

	for (int a = 0; a < 256; a++)
	{
		s_perlinDirs[a] = Vec2(cosf(a * 2.0f * PI / 256), sinf(a * 2.0f * PI / 256));
	}
}

static byte GetPerm(uint32_t x)
{
	return s_perlinPermutation[x & 255];
}

static byte PerlinHash(int32_t x, int32_t y, int32_t period)
{
	int32_t xi = ((x % period) + period) % period;
	int32_t yi = ((y % period) + period) % period;
	return GetPerm(GetPerm(xi) + yi);
}

// just like me fr
static float Poly(float v)
{
	return 1.0f - 6 * powf(v, 5) + 15 * powf(v, 4) - 10 * powf(v, 3);
}

float Perlin(const Vec2& p, uint32_t period)
{
	auto surflet = [&](int32_t gridX, int32_t gridY) {
		auto distX = abs(p.x - gridX);
		auto distY = abs(p.y - gridY);
		auto polyX = Poly(distX);
		auto polyY = Poly(distY);
		auto hashed = PerlinHash(gridX, gridY, period);
		auto grad = (p - Vec2(gridX, gridY)).Dot(s_perlinDirs[hashed]);
		return polyX * polyY * grad;
	};

	auto intX = (int32_t)floorf(p.x);
	auto intY = (int32_t)floorf(p.y);
	return surflet(intX + 0, intY + 0) + surflet(intX + 1, intY + 0) + surflet(intX + 0, intY + 1) + surflet(intX + 1, intY + 1);
}

float FBM(const Vec2& p, uint32_t period, uint32_t octave)
{
	auto v = 0.0f;
	float a = 1.0f;
	float f = 1.0f;
	for (uint32_t o = 0; o < octave; o++)
	{
		v += a * Perlin(Vec2(p.x * f, p.y * f), period * f);
		a *= 0.5f;
		f *= 2.0f;
	}
	return v;
}

uint64_t FNV(std::span<const byte> data)
{
	constexpr uint64_t OFFSET_BASIS = 0xcbf29ce484222325;
	constexpr uint64_t PRIME = 0x00000100000001B3;

	uint64_t hash = OFFSET_BASIS;
	for (size_t i = 0; i < data.size(); i++)
	{
		hash ^= data[i];
		hash *= PRIME;
	}

	return hash;
}

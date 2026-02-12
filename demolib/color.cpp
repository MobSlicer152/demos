#include "pch.h"

void SetColor(BYTE index, BYTE r, BYTE g, BYTE b)
{
	index = index % PALETTE_SIZE;
	auto& colors = g_bitmapInfo->bmiColors;
	colors[index].rgbRed = r;
	colors[index].rgbGreen = g;
	colors[index].rgbBlue = b;
}

void GetColor(BYTE index, BYTE& r, BYTE& g, BYTE& b)
{
	index = index % PALETTE_SIZE;
	auto& colors = g_bitmapInfo->bmiColors;
	r = colors[index].rgbRed;
	g = colors[index].rgbGreen;
	b = colors[index].rgbBlue;
}

// bits per colour in colour table, [1..8] are allowed. more than 6 is heavy on memory and startup (>1mb and a few second delay).
static constexpr uint32_t COLORTAB_BITS = 6;

// used for conversion and measurement
static constexpr uint32_t COLORTAB_SHIFT = 8 - COLORTAB_BITS;
static constexpr uint32_t COLORTAB_PERCOLOR = (1 << COLORTAB_BITS);

// get the index of a 24-bit colour
static constexpr uint32_t COLORTAB_INDEX(BYTE r, BYTE g, BYTE b)
{
	return ((r >> COLORTAB_SHIFT) << (2 * COLORTAB_BITS)) | ((g >> COLORTAB_SHIFT) << COLORTAB_BITS) | (b >> COLORTAB_SHIFT);
}

// RGBxxx -> palette table
static BYTE s_colorTable[COLORTAB_PERCOLOR * COLORTAB_PERCOLOR * COLORTAB_PERCOLOR];

BYTE FindNearestColor(BYTE r, BYTE g, BYTE b)
{
	return s_colorTable[COLORTAB_INDEX(r, g, b)];
}

static constexpr uint32_t BAYER_SIZE = 4;
static constexpr uint32_t BAYER_SQUARED_SIZE = BAYER_SIZE * BAYER_SIZE;

static float BayerMatrix(uint32_t x, uint32_t y)
{
	auto i = x % BAYER_SIZE;
	auto j = y % BAYER_SIZE;

	return Reverse32(Interleave32(i ^ j, i)) / (float)BAYER_SQUARED_SIZE;
}

BYTE Dither(uint32_t x, uint32_t y, BYTE color)
{
	static constexpr float R = PALETTE_SIZE / (COLORTAB_BITS / 3.0f);

	float bayer = BayerMatrix(x, y) - 0.5f;
	BYTE r, g, b;
	GetColor(color + R * bayer, r, g, b);
	return FindNearestColor(r, g, b);
}

void InitStandardPalette()
{
	Vec3 lo = Vec3(0.0f);
	Vec3 hi = Vec3(1.0f);
	for (uint32_t y = 0; y < STANDARD_PALETTE_ROWS; y++)
	{
		for (uint32_t x = 0; x < STANDARD_PALETTE_COLUMNS; x++)
		{
			auto p = lo.Lerp(hi, x);
			auto q = lo.Lerp(hi, y);
			auto c = p.Lerp(q, 0.5f);
			SetColor(y * STANDARD_PALETTE_COLUMNS + x, c.x * 255, c.y * 255, c.z * 255);
		}
	}

	//// first 32 are shades
	//// dont need any hsv for this
	//for (int v = 0; v < 32; v++)
	//{
	//	SetColor(v, v * 8, v * 8, v * 8);
	//}
	//
	//// can do 7 more rows. 5 for different value, 2 for different saturation
	//
	//int i = 32;
	//auto row = [&](int s, int v) {
	//	for (int h = 0; h < 32; h++)
	//	{
	//		Vec4 c = HsvToRgb(Vec4(h * (PI / 16), 1.0f / s, 1.0f / v, 1.0f)) * 255;
	//		SetColor(i, (BYTE)c.r, (BYTE)c.g, (BYTE)c.b);
	//		i++;
	//	}
	//};
	//
	//row(3, 1);
	//row(2, 1);
	//row(1, 1);
	//row(1, 2);
	//row(1, 3);
	//row(1, 6);
	//row(1, 8);
}

void InitColorTable()
{
#pragma omp parallel for collapse(4)
	for (int r = 0; r < COLORTAB_PERCOLOR; r++)
	{
		for (int g = 0; g < COLORTAB_PERCOLOR; g++)
		{
			for (int b = 0; b < COLORTAB_PERCOLOR; b++)
			{
				uint32_t lastDist = UINT32_MAX;
				// this loop runs COLORTAB_PERCOLOR^3 * PALETTE_SIZE times, or roughly 66 million times
				for (int c = 0; c < PALETTE_SIZE; c++)
				{
					BYTE cr, cg, cb;
					GetColor((BYTE)c, cr, cg, cb);
					uint32_t dist =
						abs(cr - (r << COLORTAB_SHIFT)) + abs(cg - (g << COLORTAB_SHIFT)) + abs(cb - (b << COLORTAB_SHIFT));
					if (dist < lastDist)
					{
						lastDist = dist;
						s_colorTable[COLORTAB_INDEX(r << COLORTAB_SHIFT, g << COLORTAB_SHIFT, b << COLORTAB_SHIFT)] = c;
					}
				}
			}
		}
	}
}
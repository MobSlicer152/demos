#include "pch.h"

void SetColor(BYTE index, BYTE r, BYTE g, BYTE b)
{
	//index = index % PALETTE_SIZE;
	auto& colors = g_bitmapInfo->bmiColors;
	colors[index].rgbRed = r;
	colors[index].rgbGreen = g;
	colors[index].rgbBlue = b;
}

void GetColor(BYTE index, BYTE& r, BYTE& g, BYTE& b)
{
	//index = index % PALETTE_SIZE;
	auto& colors = g_bitmapInfo->bmiColors;
	r = colors[index].rgbRed;
	g = colors[index].rgbGreen;
	b = colors[index].rgbBlue;
}

// bits per colour in colour table, [1..8] are allowed. more than 6 is heavy on memory and startup (>1mb and a few second delay).
#ifdef _DEBUG
static constexpr uint32_t COLORTAB_BITS = 6;
#else
static constexpr uint32_t COLORTAB_BITS = 7;
#endif

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

static constexpr Mat4 BAYER4 = Mat4(Vec4(0, 12, 3, 15), Vec4(8, 4, 11, 7), Vec4(2, 14, 1, 13), Vec4(10, 6, 9, 5)) / 16.0f;

static float BayerMatrix(uint32_t x, uint32_t y)
{
	auto i = x % 4;
	auto j = y % 4;
	return BAYER4[j][i];
}

BYTE Dither(uint32_t x, uint32_t y, BYTE color)
{
	static constexpr float R = 6.7f * 255.0f / (COLORTAB_PERCOLOR - 1);

	float bayer = BayerMatrix(x, y) - 0.5f;
	float dither = R * bayer;
	BYTE r, g, b;
	GetColor(color, r, g, b);
	r = std::clamp<int>(roundf(r + dither), 0, 255);
	g = std::clamp<int>(roundf(g + dither), 0, 255);
	b = std::clamp<int>(roundf(b + dither), 0, 255);
	return FindNearestColor(r, g, b);
}

void InitStandardPalette()
{
	int i = 0;

	// first 16 are shades
	// dont need any hsv for this
	for (; i < STANDARD_PALETTE_COLUMNS; i++)
	{
		static constexpr auto F = MAXBYTE / STANDARD_PALETTE_COLUMNS;
		SetColor(i, i * F, i * F, i * F);
	}
	
	auto row = [&](float s, float v) {
		for (int h = 0; h < STANDARD_PALETTE_COLUMNS; h++)
		{
			Vec4 c = HsvToRgb(Vec4(h * (PI / STANDARD_PALETTE_COLUMNS * 2.0f), 1.0f / s, 1.0f / v, 1.0f)) * 255;
			SetColor(i, (BYTE)c.r, (BYTE)c.g, (BYTE)c.b);
			i++;
		}
	};
	
	for (uint32_t r = 0; r < 15; r++)
	{
		// saturation only goes down (1 + ln(r + 1) / 4), value goes down 1 + r / 3
		// value wraps around for last two rows to get some brighter desaturated colours
		row(1.0f + logf(r + 1.0f) * 0.25f, 1.0f + fmodf(r * 0.3f, 3.5f));
	}
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
					auto dr = cr - (r << COLORTAB_SHIFT);
					auto dg = cg - (g << COLORTAB_SHIFT);
					auto db = cb - (b << COLORTAB_SHIFT);
					uint32_t dist = dr * dr + dg * dg + db * db;
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

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

static constexpr uint32_t COLORTAB_BITS = 6;
static constexpr uint32_t COLORTAB_SHIFT = 8 - COLORTAB_BITS;
static constexpr uint32_t COLORTAB_PERCOLOR = (1 << COLORTAB_BITS);
// RGB555 -> palette table
static BYTE s_colorTable[COLORTAB_PERCOLOR * COLORTAB_PERCOLOR * COLORTAB_PERCOLOR];

BYTE FindNearestColor(BYTE r, BYTE g, BYTE b)
{
	return s_colorTable
		[((r >> COLORTAB_SHIFT) << (2 * COLORTAB_BITS)) | ((g >> COLORTAB_SHIFT) << COLORTAB_BITS) | (b >> COLORTAB_SHIFT)];
}

void InitStandardPalette()
{
	// first 32 are shades
	// dont need any hsv for this
	for (int v = 0; v < 32; v++)
	{
		SetColor(v, v * 8, v * 8, v * 8);
	}

	// can do 7 more rows. 5 for different value, 2 for different saturation

	int i = 32;
	auto row = [&](int s, int v) {
		for (int h = 0; h < 32; h++)
		{
			Vec4 c = HsvToRgb(Vec4(h * (PI / 16), 1.0f / s, 1.0f / v, 1.0f)) * 255;
			SetColor(i, (BYTE)c.r, (BYTE)c.g, (BYTE)c.b);
			i++;
		}
	};

	row(3, 1);
	row(2, 1);
	row(1, 1);
	row(1, 2);
	row(1, 3);
	row(1, 6);
	row(1, 8);
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
				for (int c = 0; c < PALETTE_SIZE; c++)
				{
					BYTE cr, cg, cb;
					GetColor((BYTE)c, cr, cg, cb);
					cr >>= COLORTAB_SHIFT;
					cg >>= COLORTAB_SHIFT;
					cb >>= COLORTAB_SHIFT;
					uint32_t dist = abs(cr - r) + abs(cg - g) + abs(cb - b);
					if (dist < lastDist)
					{
						lastDist = dist;
						s_colorTable[(r << (2 * COLORTAB_BITS)) | (g << COLORTAB_BITS) | b] = c;
					}
				}
			}
		}
	}
}
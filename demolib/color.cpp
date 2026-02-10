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
	row(1, 4);
	row(1, 5);
}

void CalcDitherColors()
{
	float lastDist = FLT_MAX;
	for (BYTE c1 = 0; c1 < PALETTE_SIZE; c1++)
	{
		BYTE r1, g1, b1;
		GetColor(c1, r1, g1, b1);
		for (BYTE c2 = 0; c2 < PALETTE_SIZE; c2++)
		{
			BYTE r2, g2, b2;
			GetColor(c2, r2, g2, b2);
			Vec3 c1v = Vec3(r1, g1, b1);
			Vec3 c2v = Vec3(r2, g2, b2);
			Vec4 avg = (c1v + c2v) / 2;
			for (BYTE c3 = 0; c3 < PALETTE_SIZE; c3++)
			{
				BYTE r3, g3, b3;
				GetColor(c3, r3, g3, b3);
				auto dist = abs(avg.r - r3) + abs(avg.g - g3) + abs(avg.b - b3);
				if (dist < lastDist)
				{
					lastDist = dist;
					g_ditherTab[c1 * PALETTE_SIZE + c2] = c3;
				}
			}
		}
	}
}

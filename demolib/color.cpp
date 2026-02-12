#include "pch.h"

void SetColor(byte index, byte r, byte g, byte b)
{
	// index = index % PALETTE_SIZE;
	auto& colors = g_bitmapInfo->bmiColors;
	colors[index].rgbRed = r;
	colors[index].rgbGreen = g;
	colors[index].rgbBlue = b;
}

void GetColor(byte index, byte& r, byte& g, byte& b)
{
	// index = index % PALETTE_SIZE;
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
static constexpr uint32_t COLORTAB_INDEX(byte r, byte g, byte b)
{
	return ((r >> COLORTAB_SHIFT) << (2 * COLORTAB_BITS)) | ((g >> COLORTAB_SHIFT) << COLORTAB_BITS) | (b >> COLORTAB_SHIFT);
}

// RGBxxx -> palette table
static byte s_colorTable[COLORTAB_PERCOLOR * COLORTAB_PERCOLOR * COLORTAB_PERCOLOR];

byte FindNearestColor(byte r, byte g, byte b)
{
	return s_colorTable[COLORTAB_INDEX(r, g, b)];
}

byte FindNearestColor(const Vec4& color)
{
	return s_colorTable[COLORTAB_INDEX(color.r * 255, color.g * 255, color.b * 255)];
}

static constexpr byte BAYER_SIZE = 16;
static constexpr byte BAYER[BAYER_SIZE][BAYER_SIZE] = {
	{0, 128, 32, 160, 8, 136, 40, 168, 2, 130, 34, 162, 10, 138, 42, 170},
	{192, 64, 224, 96, 200, 72, 232, 104, 194, 66, 226, 98, 202, 74, 234, 106},
	{48, 176, 16, 144, 56, 184, 24, 152, 50, 178, 18, 146, 58, 186, 26, 154},
	{240, 112, 208, 80, 248, 120, 216, 88, 242, 114, 210, 82, 250, 122, 218, 90},
	{12, 140, 44, 172, 4, 132, 36, 164, 14, 142, 46, 174, 6, 134, 38, 166},
	{204, 76, 236, 108, 196, 68, 228, 100, 206, 78, 238, 110, 198, 70, 230, 102},
	{60, 188, 28, 156, 52, 180, 20, 148, 62, 190, 30, 158, 54, 182, 22, 150},
	{252, 124, 220, 92, 244, 116, 212, 84, 254, 126, 222, 94, 246, 118, 214, 86},
	{3, 131, 35, 163, 11, 139, 43, 171, 1, 129, 33, 161, 9, 137, 41, 169},
	{195, 67, 227, 99, 203, 75, 235, 107, 193, 65, 225, 97, 201, 73, 233, 105},
	{51, 179, 19, 147, 59, 187, 27, 155, 49, 177, 17, 145, 57, 185, 25, 153},
	{243, 115, 211, 83, 251, 123, 219, 91, 241, 113, 209, 81, 249, 121, 217, 89},
	{15, 143, 47, 175, 7, 135, 39, 167, 13, 141, 45, 173, 5, 133, 37, 165},
	{207, 79, 239, 111, 199, 71, 231, 103, 205, 77, 237, 109, 197, 69, 229, 101},
	{63, 191, 31, 159, 55, 183, 23, 151, 61, 189, 29, 157, 53, 181, 21, 149},
	{255, 127, 223, 95, 247, 119, 215, 87, 253, 125, 221, 93, 245, 117, 213, 85}};

static float BayerMatrix(uint32_t x, uint32_t y)
{
	auto i = x % BAYER_SIZE;
	auto j = y % BAYER_SIZE;
	return (float)BAYER[i][j] / (BAYER_SIZE * BAYER_SIZE);
}

byte Dither(uint32_t x, uint32_t y, const Vec4& color)
{
	static constexpr float R = 8.0f * 1.0f / (COLORTAB_PERCOLOR - 1);

	float bayer = BayerMatrix(x, y) - 0.5f;
	float dither = R * bayer;
	byte r = std::clamp<int>(roundf((color.r + dither) * 255), 0, 255);
	byte g = std::clamp<int>(roundf((color.g + dither) * 255), 0, 255);
	byte b = std::clamp<int>(roundf((color.b + dither) * 255), 0, 255);
	return FindNearestColor(r, g, b);
}

void InitStandardPalette()
{
	int i = 0;

	// first 16 are shades
	// dont need any hsv for this
	for (; i < STANDARD_PALETTE_COLUMNS; i++)
	{
		static constexpr auto F = UINT8_MAX / STANDARD_PALETTE_COLUMNS;
		SetColor(i, i * F, i * F, i * F);
	}

	auto row = [&](float s, float v) {
		for (int h = 0; h < STANDARD_PALETTE_COLUMNS; h++)
		{
			Vec4 c = HsvToRgb(Vec4(h * (PI / STANDARD_PALETTE_COLUMNS * 2.0f), 1.0f / s, 1.0f / v, 1.0f)) * 255;
			SetColor(i, (byte)c.r, (byte)c.g, (byte)c.b);
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
#pragma omp parallel for collapse(3)
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
					byte cr, cg, cb;
					GetColor((byte)c, cr, cg, cb);
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

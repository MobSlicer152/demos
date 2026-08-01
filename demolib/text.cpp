#include "pch.h"

#include "font8x8_basic.h"

void DrawChar(int32_t x, int32_t y, char c, const Vec4& color, float size)
{
	c &= 0x7F;
	const auto& b = font8x8_basic[c];
//#pragma omp parallel for
	for (uint32_t i = 0; i < (uint32_t)(ArraySize(b) * size); i++)
	{
		for (uint32_t j = 0; j < (uint32_t)(8 * size); j++)
		{
			SetPixel(x + j, y + i, b[(uint32_t)(i / size)] & (1 << (uint32_t)(j / size)) ? color : Vec4(0.0f, 0.0f, 0.0f, 0.0f));
		}
	}
}

// need 8 bytes for each row of each char too
static byte s_fontAtlas[sizeof(font8x8_basic) * 8];
static bool s_fontAtlasInitialized = false;
byte* CreateFontAtlas()
{
	if (s_fontAtlasInitialized)
	{
		return s_fontAtlas;
	}

	for (int c = 0; c < ArraySize(font8x8_basic); c++)
	{
		for (uint32_t x = 0; x < FONT_WIDTH; x++)
		{
			for (uint32_t y = 0; y < FONT_HEIGHT; y++)
			{
				auto& data = font8x8_basic[c];
				bool white = data[y] & (1 << x);
				uint32_t gx = c % 16 * FONT_WIDTH + x;
				uint32_t gy = c / 16 * FONT_HEIGHT + y;
				s_fontAtlas[gy * FONT_ATLAS_WIDTH + gx] = white ? 0xFF : 0x00;
			}
		}
	}

	s_fontAtlasInitialized = true;
	return s_fontAtlas;
}

void DrawString(int32_t x, int32_t y, const Vec4& color, float size, const char* s, ...)
{
	char buf[512] = {};
	va_list args;
	va_start(args, s);
	_vsnprintf_s(buf, ArraySize(buf), s, args);
	buf[ArraySize(buf) - 1] = 0;
	va_end(args);

	for (auto i = 0; i < strlen(buf); i++)
	{
		DrawChar(x + (i * 8 * size), y, buf[i], color, size);
	}
}

void Message(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	Message(msg, args);
	va_end(args);
}

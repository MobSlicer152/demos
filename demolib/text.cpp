#include "pch.h"

#include "font8x8_basic.h"

void DrawChar(int32_t x, int32_t y, char c, const Vec4& color, float size)
{
	c &= 0x7F;
	const auto& b = font8x8_basic[c];
	for (uint32_t i = 0; i < ArraySize(b) * size; i++)
	{
		for (uint32_t j = 0; j < 8 * size; j++)
		{
			SetPixel(x + j, y + i, b[(uint32_t)(i / size)] & (1 << (uint32_t)(j / size)) ? color : Vec4(0.0f, 0.0f, 0.0f, 0.0f));
		}
	}
}

void DrawString(int32_t x, int32_t y, const Vec4& color, float size, const char* s, ...)
{
	char buf[512] = {};
	va_list args;
	va_start(args, s);
	_vsnprintf_s(buf, ArraySize(buf), s, args);
	va_end(args);

	for (auto i = 0; i < strlen(buf); i++)
	{
		DrawChar(x + (i * 8 * size), y, buf[i], color, size);
	}
}

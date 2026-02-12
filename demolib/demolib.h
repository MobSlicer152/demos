#pragma once

#define _NO_CRT_STDIO_INLINE

#include "camera.h"
#include "mathfun.h"
#include "md2.h"
#include "misc.h"

extern HINSTANCE g_inst;
extern LPSTR g_cmdline;
extern bool g_running;
extern bool g_paused;

extern UINT64 g_lastTime;
extern UINT64 g_nowTime;
extern float g_delta;
extern float g_elapsed;
extern int g_targetFps;

#define DEFAULT_TARGET_FPS 60

extern ATOM g_wndClass;
extern HWND g_wnd;
extern uint32_t g_width;
extern uint32_t g_height;
extern float g_aspect;
extern RECT g_wndRect;

#define PALETTE_SIZE			 255
#define STANDARD_PALETTE_COLUMNS 32
#define STANDARD_PALETTE_ROWS 8
#define STANDARD_COLOR(row, col) ((row) * STANDARD_PALETTE_COLUMNS + (col))

extern HBITMAP g_bitmap;
extern PBITMAPINFO g_bitmapInfo;
extern PBYTE g_framebuffer;
extern uint32_t g_fbStride;
extern bool g_autoClear;
extern BYTE g_clearColor;

// demo functions, provided by individual demo

// here you initialize your palette, or call InitStandardPalette for a mode 13h-esque palette
extern void InitPalette();
// here, you draw the current frame of your demo
extern void DrawDemo();

// lib functions

static constexpr uint64_t InterleaveZeros32(uint32_t input)
{
	uint64_t word = input;
	word = (word ^ (word << 16)) & 0x0000ffff0000ffff;
	word = (word ^ (word << 8)) & 0x00ff00ff00ff00ff;
	word = (word ^ (word << 4)) & 0x0f0f0f0f0f0f0f0f;
	word = (word ^ (word << 2)) & 0x3333333333333333;
	word = (word ^ (word << 1)) & 0x5555555555555555;
	return word;
}

static constexpr uint64_t Interleave32(uint32_t a, uint32_t b)
{
	return InterleaveZeros32(a) | (InterleaveZeros32(b) << 1);
}

static constexpr uint32_t Reverse32(uint32_t x)
{
	return ((x * 0x0802LU & 0x22110LU) | (x * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

// sets a palette color
extern void SetColor(BYTE index, BYTE r, BYTE g, BYTE b);
extern void GetColor(BYTE index, BYTE& r, BYTE& g, BYTE& b);
extern BYTE FindNearestColor(BYTE r, BYTE g, BYTE b);

// dither a color
extern BYTE Dither(uint32_t x, uint32_t y, BYTE color);

// initializes a better version of roughly what mode 13h offers
extern void InitStandardPalette();

// set a pixel
extern void SetPixel(uint32_t x, uint32_t y, BYTE color, bool dither = true);
// set a pixel but 0..1 instead of 0..g_width/height
extern void SetPixel(float x, float y, BYTE color, bool dither = true);

// draw the palette
extern void DrawPalette(
	uint32_t width = g_width,
	uint32_t height = g_height,
	uint32_t x = 0,
	uint32_t y = 0,
	uint32_t perRow = STANDARD_PALETTE_COLUMNS,
	uint32_t rows = STANDARD_PALETTE_ROWS);

// draw a rectangle
extern void DrawRectangle(const Vec3& a, const Vec3& b, BYTE color);

// draw a line
extern void DrawLine(const Vec3& start, const Vec3& end, BYTE color);

// different modes of drawing
enum class DrawMode
{
	Shaded,	   // has light applied
	Flat,	   // fullbright
	Wireframe, // shaded but wireframe
	Normals,   // shows normals
	UV		   // shows texcoords
};

static constexpr uint32_t INVALID_TEXTURE_ID = 0;

// draw a triangle
extern void DrawTriangle(
	const Vec4& p1,
	const Vec4& p2,
	const Vec4& p3,
	BYTE c1 = 31,
	BYTE c2 = 31,
	BYTE c3 = 31,
	DrawMode mode = DrawMode::Shaded,
	uint32_t textureId = INVALID_TEXTURE_ID);

// display an error messagebox and exit the process
extern DECLSPEC_NORETURN void ErrorMessage(int code, const char* msg, ...);
extern DECLSPEC_NORETURN void ErrorMessage(int code, const char* msg, va_list args);

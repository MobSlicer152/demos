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
#define FRAMEBUFFER_WIDTH  640
#define FRAMEBUFFER_HEIGHT 480
#define MAX_TRIANGLES	   512

extern ATOM g_wndClass;
extern HWND g_wnd;
extern uint32_t g_width;
extern uint32_t g_height;
extern float g_aspect;
extern RECT g_wndRect;

#define PALETTE_SIZE			 255
#define STANDARD_PALETTE_COLUMNS 16
#define STANDARD_PALETTE_ROWS	 16
#define STANDARD_COLOR(row, col) ((row) * STANDARD_PALETTE_COLUMNS + (col))

extern HBITMAP g_bitmap;
extern PBITMAPINFO g_bitmapInfo;
extern byte* g_framebuffer;
extern float g_zBuffer[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT];
extern uint32_t g_fbStride;
extern bool g_autoClear;
extern Vec4 g_clearColor;

// demo functions, provided by individual demo

// here you initialize your palette, or call InitStandardPalette for a solid palette
extern void InitPalette();
// here, you draw the current frame of your demo
extern void DrawDemo();

// lib functions

static constexpr byte Reverse(byte b)
{
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

static constexpr uint16_t Reverse(uint16_t w)
{
	return ((uint16_t)Reverse((byte)(w & 0xFF)) << 8) | Reverse((byte)(w >> 8));
}

static constexpr uint16_t Interleave(byte a, byte b)
{
	return (uint16_t)(((a * 0x0101010101010101ULL & 0x8040201008040201ULL) * 0x0102040810204081ULL >> 49) & 0x5555 |
					  ((b * 0x0101010101010101ULL & 0x8040201008040201ULL) * 0x0102040810204081ULL >> 48) & 0xAAAA);
}

// generate a uniform random number between min and max
extern float UniformRandom(float min = 0.0f, float max = 1.0f);

// perlin noise
extern float Perlin(const Vec2& p, uint32_t period);
extern float FBM(const Vec2& p, uint32_t period, uint32_t octave);

// sets a palette color
extern void SetColor(byte index, byte r, byte g, byte b);
extern void GetColor(byte index, byte& r, byte& g, byte& b);
extern byte FindNearestColor(byte r, byte g, byte b);
extern byte FindNearestColor(const Vec4& color);
extern Vec4 BlendColor(const Vec4& fg, const Vec4& bg);

// dither a color
extern byte Dither(uint32_t x, uint32_t y, const Vec4& color);

// initializes a palette with a decent range of hsv values
extern void InitStandardPalette();

// set a pixel to a palette color
extern void SetPixel(uint32_t x, uint32_t y, byte color);
// set a pixel to an approximation of an rgb color
extern void SetPixel(uint32_t x, uint32_t y, const Vec4& color, bool dither = true);
// set a pixel but 0..1 instead of framebuffer size
extern void SetPixel(const Vec2& p, const Vec4& color, bool dither = true);
extern Vec4 GetPixel(uint32_t x, uint32_t y);
extern Vec4 GetPixel(const Vec2& p);

// like SetPixel, but for depth
extern void SetDepthPixel(uint32_t x, uint32_t y, float z);
extern void SetDepthPixel(const Vec2& p, float z);
extern float GetDepthPixel(uint32_t x, uint32_t y);
extern float GetDepthPixel(const Vec2& p);

// draw the palette
extern void DrawPalette(
	uint32_t width = FRAMEBUFFER_WIDTH,
	uint32_t height = FRAMEBUFFER_HEIGHT,
	uint32_t x = 0,
	uint32_t y = 0,
	uint32_t perRow = STANDARD_PALETTE_COLUMNS,
	uint32_t rows = STANDARD_PALETTE_ROWS);

// draw a rectangle
extern void DrawRectangle(const Vec3& a, const Vec3& b, const Vec4& color);

// clear color buffer
extern void ClearColor(const Vec4& c);
// clear depth buffer
extern void ClearDepth(float z);

// draw a line
extern void DrawLine(const Vec3& start, const Vec3& end, const Vec4& color);

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

struct VertexShaderInput
{
	Vec4 vert;
	Mat4 mvp;
	Vec4 color;
	void* user;
};

struct VertexShaderOutput
{
	Vec4 pos;
	Vec4 color;
};

typedef void (*VertexShaderCallback_t)(const VertexShaderInput& in, VertexShaderOutput& out);

struct FragmentShaderInput
{
	Vec4 pos;
	Vec4 color;
	void* user;
};

struct FragmentShaderOutput
{
	Vec4 color;
};

typedef void (*FragmentShaderCallback_t)(const FragmentShaderInput& in, FragmentShaderOutput& out);

struct Shader
{
	VertexShaderCallback_t vertex;
	FragmentShaderCallback_t fragment;
	void* user;
};

// draw a triangle
extern void DrawTriangle(
	const Vec4& p1,
	const Vec4& p2,
	const Vec4& p3,
	const Vec4& c1,
	const Vec4& c2,
	const Vec4& c3,
	DrawMode mode = DrawMode::Shaded,
	uint32_t textureId = INVALID_TEXTURE_ID,
	Mat4 mvp = Mat4(),
	Shader* shader = nullptr);

// draw an MD2 model
extern void DrawModel(
	const CMD2Model& model,
	DrawMode mode = DrawMode::Shaded,
	uint32_t textureId = INVALID_TEXTURE_ID,
	Mat4 mvp = Mat4(),
	Shader* shader = nullptr);
// draw a model from raw mesh data
extern void DrawModel(
	const Vec3* verts,
	uint32_t vertCount,
	const Vec3i* tris,
	uint32_t triCount,
	DrawMode mode = DrawMode::Shaded,
	uint32_t textureId = INVALID_TEXTURE_ID,
	Mat4 mvp = Mat4(),
	Shader* shader = nullptr,
	const Vec3* normals = nullptr,
	uint32_t normalCount = 0,
	const Vec2* texCoords = nullptr,
	uint32_t texCoordCount = 0);

// display an error messagebox and exit the process
extern DECLSPEC_NORETURN void ErrorMessage(int code, const char* msg, ...);
extern DECLSPEC_NORETURN void ErrorMessage(int code, const char* msg, va_list args);

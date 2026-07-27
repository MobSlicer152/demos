#pragma once

#define _NO_CRT_STDIO_INLINE

#include <span>

#include "camera.h"
#include "mathfun.h"
#include "md2.h"
#include "misc.h"

extern bool g_running;
extern bool g_paused;

extern uint64_t g_lastTime;
extern uint64_t g_nowTime;
extern float g_delta;
extern float g_elapsed;
extern int g_targetFps;
extern int g_framesSinceAverage;
extern uint64_t g_lastAverage;
extern float g_averageFps;

#define TIME_PER_FPS_AVERAGE 100
#define DEFAULT_TARGET_FPS 1000
#define FRAMEBUFFER_WIDTH  640
#define FRAMEBUFFER_HEIGHT 480

extern uint32_t g_width;
extern uint32_t g_height;
extern float g_aspect;

#define PALETTE_SIZE			 255
#define STANDARD_PALETTE_COLUMNS 16
#define STANDARD_PALETTE_ROWS	 16
#define STANDARD_COLOR(row, col) ((row) * STANDARD_PALETTE_COLUMNS + (col))

extern bool g_useFramebuffer;
extern byte* g_framebuffer;
extern float g_zBuffer[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT];
extern uint32_t g_fbStride;
extern bool g_autoClear;
extern Vec4 g_clearColor;

#define MINIMUM_FILE_TABLE_SIZE (sizeof(uint32_t) * 2) // origSize and totalSize
#define NO_FILE_TABLE extern "C" uint32_t g_fileTable[MINIMUM_FILE_TABLE_SIZE] = {};

#ifdef _WIN32
extern HINSTANCE g_inst;
extern LPSTR g_cmdline;
extern ATOM g_wndClass;
extern HWND g_wnd;
extern HBITMAP g_bitmap;
extern PBITMAPINFO g_bitmapInfo;
extern RECT g_wndRect;
#else
extern PaletteColor g_palette[PALETTE_SIZE];
#endif

// demo functions, provided by individual demo

// here you initialize your palette, or call InitStandardPalette for a solid palette
extern void InitDemoPalette();
// post-color init
extern void InitDemo();
// here, you draw the current frame of your demo
extern void DrawDemo();
// release any stuff for your demo
extern void ShutdownDemo();

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

// FNV-1a hash
extern uint64_t FNV(std::span<const byte> data);

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

// draw a character
extern void DrawChar(int32_t x, int32_t y, char c, const Vec4& color, float size = 1.0f);
// draw a string
extern void DrawString(int32_t x, int32_t y, const Vec4& color, float size, const char* s, ...);

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
	uint32_t vertIndex;
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
	const CMD2Model* model,
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

// print a message
extern void Message(const char* msg, ...);
extern void Message(const char* msg, va_list args);

// prepares space for files to be decompressed
extern void InitFileTable();

// get a file
extern std::span<const byte> GetFile(const char* name);

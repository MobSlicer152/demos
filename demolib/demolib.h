#pragma once

#define _NO_CRT_STDIO_INLINE

#include "camera.h"
#include "mathfun.h"
#include "misc.h"

extern HINSTANCE g_inst;
extern LPSTR g_cmdline;
extern bool g_running;
extern bool g_paused;

extern UINT64 g_lastTime;
extern UINT64 g_nowTime;
extern float g_delta;
extern float g_elapsed;

extern ATOM g_wndClass;
extern HWND g_wnd;
extern int g_width;
extern int g_height;
extern float g_aspect;
extern RECT g_wndRect;

extern HBITMAP g_bitmap;
extern PBITMAPINFO g_bitmapInfo;
extern PBYTE g_framebuffer;
extern int g_fbStride;

// demo functions

extern void DrawDemo();
extern void InitPalette();

// lib functions

extern void InitStandardPalette();
extern DECLSPEC_NORETURN void ErrorMessage(int code, const char* msg, ...);
extern DECLSPEC_NORETURN void ErrorMessage(int code, const char* msg, va_list args);

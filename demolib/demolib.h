#pragma once

#include "mathfun.h"
#include "misc.h"
#include "camera.h"

extern HINSTANCE g_inst;
extern LPSTR g_cmdline;
extern bool g_running;
extern bool g_paused;

extern UINT64 g_lastTime;
extern UINT64 g_nowTime;
extern float g_delta;

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

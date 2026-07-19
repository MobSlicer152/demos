// demolib.cpp : Defines the functions for the static library.
//

#include "demolib.h"
#include "pch.h"

HINSTANCE g_inst;
LPSTR g_cmdline;
bool g_running;
bool g_paused;

UINT64 g_lastTime;
UINT64 g_nowTime;
float g_delta;
float g_elapsed;
int g_targetFps = DEFAULT_TARGET_FPS;

ATOM g_wndClass;
HWND g_wnd;
uint32_t g_width;
uint32_t g_height;
float g_aspect;
RECT g_wndRect;

bool g_useFramebuffer = true;
HBITMAP g_bitmap;
static BYTE s_rawBitmapInfo[sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255];
PBITMAPINFO g_bitmapInfo = (PBITMAPINFO)s_rawBitmapInfo;
PBYTE g_framebuffer;
float g_zBuffer[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT];
uint32_t g_fbStride;
bool g_autoClear = true;
Vec4 g_clearColor = Vec4::BLACK;

#define CLASSNAME "DemoWnd"

static ULONG s_randomSeed = (ULONG)GetTickCount64();
float UniformRandom(float min, float max)
{
	return std::clamp(((float)RtlUniform(&s_randomSeed) / MAXLONG) * (max - min) + min, min, max);
}

static LRESULT WINAPI WindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
	case WM_QUIT: {
		g_running = false;
		break;
	}
	case WM_KEYDOWN: {
		auto key = wParam;
		switch (key)
		{
		case VK_ESCAPE:
			g_running = false;
			break;
		case VK_SPACE:
			g_paused = !g_paused;
			break;
		}
	}
	}

	return DefWindowProcA(wnd, msg, wParam, lParam);
}

static void InitWindow()
{
	WNDCLASSEXA wndClass = {};
	wndClass.cbSize = sizeof(WNDCLASSEXA);
	wndClass.hInstance = g_inst;
	wndClass.lpszClassName = CLASSNAME;
	wndClass.hCursor = LoadCursorW(g_inst, IDC_ARROW);
	wndClass.lpfnWndProc = WindowProc;
	g_wndClass = RegisterClassExA(&wndClass);
	if (!g_wndClass)
	{
		auto error = GetLastError();
		ErrorMessage(error, "failed to register window class: %d\n", error);
	}

	// nicely centre the window
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int frameWidth = GetSystemMetrics(SM_CXFRAME);
	int frameHeight = GetSystemMetrics(SM_CYFRAME);
	int captionHeight = GetSystemMetrics(SM_CYCAPTION);

	// make a nice 4:3 that's 3/4 the height of the monitor
	g_height = (uint32_t)((screenHeight * 0.75f) + frameHeight);
	g_width = (uint32_t)(g_height * 1.33333f);

	// centre and account for caption
	int x = (screenWidth - g_width) / 2;
	int y = (screenHeight - g_height) / 2 - captionHeight;

	g_wnd = CreateWindowExA(
		0,
		(LPSTR)g_wndClass,
		"Demo",
		(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX),
		x,
		y,
		g_width,
		g_height,
		nullptr,
		nullptr,
		g_inst,
		nullptr);
	if (!g_wnd)
	{
		auto error = GetLastError();
		ErrorMessage(error, "failed to create window: %d\n", error);
	}

	GetClientRect(g_wnd, &g_wndRect);
	g_width = g_wndRect.right - g_wndRect.left;
	g_height = g_wndRect.bottom - g_wndRect.top;
	g_aspect = (float)g_width / g_height;
}

extern DECLSPEC_NORETURN void ErrorMessage(int code, const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	ErrorMessage(code, msg, args);
}

extern DECLSPEC_NORETURN void ErrorMessage(int code, const char* msg, va_list args)
{
	char buf[512] = {}; // avoid allocation since it could be the cause of the error
	_vsnprintf_s(buf, ArraySize(buf), msg, args);
	buf[ArraySize(buf) - 1] = 0;
	MessageBoxA(g_wnd, buf, "Fatal error", MB_OK | MB_ICONERROR);
	ExitProcess(code);
}

static void InitFramebuffer()
{
	auto dc = GetDC(g_wnd);

	auto& header = g_bitmapInfo->bmiHeader;
	header.biSize = sizeof(BITMAPINFOHEADER);
	header.biWidth = FRAMEBUFFER_WIDTH;
	header.biHeight = FRAMEBUFFER_HEIGHT;
	header.biPlanes = 1;
	header.biBitCount = 8;
	header.biCompression = BI_RGB;
	g_fbStride = ((((header.biWidth * header.biBitCount) + 31) & ~31) >> 3);
	header.biSizeImage = g_fbStride * header.biHeight;
	g_bitmap = CreateDIBSection(dc, g_bitmapInfo, DIB_RGB_COLORS, (PVOID*)&g_framebuffer, nullptr, 0);
	if (!g_bitmap)
	{
		auto error = GetLastError();
		ErrorMessage(error, "failed to create bitmap: %d\n", error);
	}

	ReleaseDC(g_wnd, dc);
}

static void SleepToNextFrame()
{
	float spf = 1.0f / g_targetFps;
	float delay = spf - g_delta;
	if (delay > 0.0f)
	{
		Sleep((DWORD)(delay * 1000));
	}
}

static void WindowLoop()
{
	g_lastTime = GetTickCount64();
	g_running = true;
	while (g_running)
	{
		g_nowTime = GetTickCount64();
		MSG msg = {};
		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (g_autoClear && g_useFramebuffer)
		{
			DrawRectangle(Vec2(0.0f), Vec2(1.0f), g_clearColor);
			ClearColor(g_clearColor);
			ClearDepth(FLT_MIN);
		}

		DrawDemo();

		if (g_useFramebuffer)
		{
			auto dc = GetDC(g_wnd);
			StretchDIBits(
				dc,
				0,
				g_height - 1,
				g_width,
				-(int32_t)g_height,
				0,
				0,
				FRAMEBUFFER_WIDTH,
				FRAMEBUFFER_HEIGHT,
				g_framebuffer,
				g_bitmapInfo,
				DIB_RGB_COLORS,
				SRCCOPY);
			ReleaseDC(g_wnd, dc);
		}

		g_delta = g_paused ? 0.0f : (g_nowTime - g_lastTime) / 1000.0f;
		SleepToNextFrame();
		g_elapsed += g_delta;
		g_lastTime = g_nowTime;
	}
}

// prepares space for files to be decompressed
extern void InitFileTable();

// calculates similar colors for dithering
extern void InitColorTable();

// generates perlin permutation table
extern void InitNoise();

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prevInst, LPSTR cmdline, int show)
{
	g_inst = inst;
	g_cmdline = cmdline;

#ifdef _DEBUG
	//_putenv("OMP_DISPLAY_ENV=verbose");
#endif
	//_putenv("OMP_WAIT_POLICY=passive");

	InitFileTable();
	InitWindow();
	InitNoise();
	InitDemoPalette();
	if (g_useFramebuffer)
	{
		InitColorTable();
		InitFramebuffer();
	}
	InitDemo();
	ShowWindow(g_wnd, show);
	WindowLoop();
	ShutdownDemo();
	DestroyWindow(g_wnd);
	ExitProcess(0);
}

void main()
{
	WinMain(GetModuleHandleA(nullptr), nullptr, GetCommandLineA(), SW_SHOWNORMAL);
}

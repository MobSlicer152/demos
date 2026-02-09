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
int g_width;
int g_height;
float g_aspect;
RECT g_wndRect;

HBITMAP g_bitmap;
static BYTE s_rawBitmapInfo[sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255];
PBITMAPINFO g_bitmapInfo = (PBITMAPINFO)s_rawBitmapInfo;
PBYTE g_framebuffer;
int g_fbStride;

#define CLASSNAME "Demo1"

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

static void InitWindow(int show)
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
	g_height = (screenHeight * 0.75) + frameHeight;
	g_width = g_height * 1.33333f;

	// centre and account for caption
	int x = (screenWidth - g_width) / 2;
	int y = (screenHeight - g_height) / 2 - captionHeight;

	g_wnd = CreateWindowExA(
		0,
		(LPSTR)g_wndClass,
		"Demo1",
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

	ShowWindow(g_wnd, show);

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

extern void SetColor(BYTE index, BYTE r, BYTE g, BYTE b)
{
	index = index % 255;
	auto& colors = g_bitmapInfo->bmiColors;
	colors[index].rgbRed = r;
	colors[index].rgbGreen = g;
	colors[index].rgbBlue = b;
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

static void InitFramebuffer()
{
	InitPalette();

	auto& header = g_bitmapInfo->bmiHeader;
	header.biSize = sizeof(BITMAPINFOHEADER);
	header.biWidth = g_width;
	header.biHeight = g_height;
	header.biPlanes = 1;
	header.biBitCount = 8;
	header.biCompression = BI_RGB;
	g_fbStride = ((((header.biWidth * header.biBitCount) + 31) & ~31) >> 3);
	header.biSizeImage = g_fbStride * header.biHeight;
	g_bitmap = CreateDIBSection(GetDC(g_wnd), g_bitmapInfo, DIB_RGB_COLORS, (PVOID*)&g_framebuffer, nullptr, 0);
	if (!g_bitmap)
	{
		auto error = GetLastError();
		ErrorMessage(error, "failed to create bitmap: %d\n", error);
	}
}

static void SleepToNextFrame()
{
	float spf = 1.0 / g_targetFps;
	float delay = spf - g_delta;
	if (delay > 0.0)
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

		DrawDemo();
		StretchDIBits(
			GetDC(g_wnd),
			0,
			g_height,
			g_width,
			-g_height,
			0,
			0,
			g_width,
			g_height,
			g_framebuffer,
			g_bitmapInfo,
			DIB_RGB_COLORS,
			SRCCOPY);

		g_delta = g_paused ? 0.0f : (g_nowTime - g_lastTime) / 1000.0f;
		SleepToNextFrame();
		g_elapsed += g_delta;
		g_lastTime = g_nowTime;
	}
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prevInst, LPSTR cmdline, int show)
{
	g_inst = inst;
	g_cmdline = cmdline;

	InitWindow(show);
	InitFramebuffer();
	WindowLoop();
	DestroyWindow(g_wnd);
}

#ifdef _DEBUG
#define ENTRY mainCRTStartup
#else
#define ENTRY WinMainCRTStartup
#endif

extern "C" void ENTRY()
{
	WinMain(GetModuleHandleA(nullptr), nullptr, GetCommandLineA(), SW_SHOWNORMAL);
}

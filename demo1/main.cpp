#include <stdio.h>
#include <windows.h>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

#define CLASSNAME "Demo1"

HINSTANCE g_inst;
LPSTR g_cmdline;
bool g_running;
bool g_paused;

UINT64 g_lastTime;
UINT64 g_nowTime;
float g_delta;

ATOM g_wndClass;
HWND g_wnd;
int g_width;
int g_height;
RECT g_wndRect;

HBITMAP g_bitmap;
BYTE g_rawBitmapInfo[sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255];
PBITMAPINFO g_bitmapInfo = (PBITMAPINFO)g_rawBitmapInfo;
PBYTE g_framebuffer;
int g_fbStride;

LRESULT WINAPI WindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

void InitWindow(int show)
{
    WNDCLASSEXA wndClass = {};
    wndClass.cbSize = sizeof(WNDCLASSEXA);
    wndClass.hInstance = g_inst;
    wndClass.lpszClassName = CLASSNAME;
    wndClass.lpfnWndProc = WindowProc;
    g_wndClass = RegisterClassExA(&wndClass);
    if (!g_wndClass)
    {
        auto error = GetLastError();
        printf("[!] failed to register window class: %d\n", error);
        exit(error);
    }
    
    // nicely centre the window
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int frameWidth = GetSystemMetrics(SM_CXFRAME);
    int frameHeight = GetSystemMetrics(SM_CYFRAME);
    int captionHeight = GetSystemMetrics(SM_CYCAPTION);
    g_width = (screenWidth / 2) + frameWidth;
    g_height = (screenHeight / 2) + frameHeight;
    int x = (screenWidth - g_width) / 2;
    int y = (screenHeight - g_height) / 2 - captionHeight;
    
    g_wnd = CreateWindowExA(0, (LPSTR)g_wndClass, "Demo1", (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX),
                x, y, g_width, g_height, nullptr, nullptr, g_inst, nullptr);
    if (!g_wnd)
    {
        auto error = GetLastError();
        printf("[!] failed to create window: %d\n", error);
        exit(error);
    }
                
    ShowWindow(g_wnd, show);
    
    GetClientRect(g_wnd, &g_wndRect);
    g_width = g_wndRect.right - g_wndRect.left;
    g_height = g_wndRect.bottom - g_wndRect.top;
}

void InitPalette()
{
    
}

void InitFramebuffer()
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
        printf("[!] failed to create bitmap: %d\n", error);
        exit(error);
    }
}

void DrawDemo()
{
    memset(g_framebuffer, 0, g_bitmapInfo->bmiHeader.biSizeImage);
}

void WindowLoop()
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
        StretchDIBits(GetDC(g_wnd), 0, 0, g_width, g_height, 0, 0, g_width, g_height, g_framebuffer,
            g_bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
            
        g_delta = g_paused ? 0.0 : (g_nowTime - g_lastTime) / 1000.0f;
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

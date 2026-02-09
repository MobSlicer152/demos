#include "../demolib/demolib.h"

void InitPalette()
{
    InitStandardPalette();
}

void DrawDemo()
{
    memset(g_framebuffer, (int)(g_elapsed * 10) % 255, g_bitmapInfo->bmiHeader.biSizeImage);
}

#include "../demolib/demolib.h"

void InitPalette()
{
    InitStandardPalette();
}

void DrawDemo()
{
    memset(g_framebuffer, 0, g_bitmapInfo->bmiHeader.biSizeImage);
}

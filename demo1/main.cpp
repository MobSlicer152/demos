#include "../demolib/demolib.h"

void InitPalette()
{
    InitStandardPalette();
}

void DrawDemo()
{
	DrawTriangle(Vec2(0.0, 0.0), Vec2(0.0, 1.0), Vec2(1.0, 0.0));
}

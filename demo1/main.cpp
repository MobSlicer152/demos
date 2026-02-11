#include "../demolib/demolib.h"

void InitPalette()
{
    InitStandardPalette();
}

void DrawDemo()
{
	DrawPalette();
	DrawTriangle(
		Vec2(0.5, 0.0), Vec2(0.0, 1.0), Vec2(1.0, 1.0), STANDARD_COLOR(3, 5), STANDARD_COLOR(3, 15), STANDARD_COLOR(3, 20), DrawMode::Wireframe);
}

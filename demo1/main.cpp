#include "../demolib/demolib.h"

void InitPalette()
{
	InitStandardPalette();
}

void DrawDemo()
{
	DrawTriangle(
		Vec3(0.5f, 0.0f, 1.0f), Vec3(0.0f, 1.0f, 1.0f), Vec3(1.0f, 1.0f, 1.0f), Vec4::RED, Vec4::GREEN, Vec4::BLUE);
	DrawTriangle(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(1.0f, 1.0f, 0.0f), Vec4::BLUE, Vec4::GREEN, Vec4::RED);
	//DrawPalette();
}

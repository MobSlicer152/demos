#include "../demolib/demolib.h"

void InitPalette()
{
	InitStandardPalette();
}

void VertexShader(const VertexShaderInput& in, VertexShaderOutput& out)
{
	out.pos = in.mvp * in.vert;
	out.color = in.color;
}

void DrawDemo()
{
	auto mvp = Mat4::Perspective(Deg2Rad(78.0f), g_aspect, 0.1f, 1000.0f) * Mat4::LookAt(Vec3::BACKWARD * 3.0f, Vec3(0.0f));
	auto shader = Shader {VertexShader};
	DrawTriangle(
		Vec3(0.5f, 0.0f, 1.0f), Vec3(0.0f, 1.0f, 1.0f), Vec3(1.0f, 1.0f, 1.0f), Vec4::RED, Vec4::GREEN, Vec4::BLUE, DrawMode::Shaded, INVALID_TEXTURE_ID, mvp, &shader);
	//DrawPalette();
}

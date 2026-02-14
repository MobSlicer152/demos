#include "../demolib/demolib.h"
#include "heart.h"

static constexpr int32_t PLANE_SIZE = 32;
static constexpr int32_t PLANE_EDGE_VERTICES = PLANE_SIZE * 2;
static constexpr int32_t PLANE_QUAD_COUNT = PLANE_SIZE * PLANE_SIZE;
static constexpr int32_t PLANE_TRI_COUNT = PLANE_QUAD_COUNT * 2;

static Vec3 s_planeVerts[PLANE_TRI_COUNT * 3];
static Vec3i s_planeTris[PLANE_TRI_COUNT];
static void GeneratePlane();
static CMD2Model* s_heart;

void InitDemo()
{
	InitStandardPalette();
	GeneratePlane();
	s_heart = new CMD2Model(HEART_MODEL, sizeof(HEART_MODEL));
}

void ShutdownDemo()
{
	delete s_heart;
}

void GeneratePlane()
{
	auto v = 0;
	auto t = 0;
	for (int32_t x = -(PLANE_SIZE / 2); x < PLANE_SIZE / 2; x++)
	{
		for (int32_t y = -(PLANE_SIZE / 2); y < PLANE_SIZE / 2; y++)
		{
			auto scale = 1.0f / PLANE_SIZE;
			s_planeVerts[v++] = Vec3((-1.0f + x) * scale, 0.0f, (1.0f + y) * scale);
			s_planeVerts[v++] = Vec3((1.0f + x) * scale, 0.0f, (1.0f + y) * scale);
			s_planeVerts[v++] = Vec3((1.0f + x) * scale, 0.0f, (-1.0f + y) * scale);
			s_planeVerts[v++] = Vec3((-1.0f + x) * scale, 0.0f, (-1.0f + y) * scale);
			auto vb = v - 4;
			s_planeTris[t++] = Vec3i(vb + 0, vb + 1, vb + 2);
			s_planeTris[t++] = Vec3i(vb + 2, vb + 3, vb + 0);
		}
	}
}

void PlaneVertexShader(const VertexShaderInput& in, VertexShaderOutput& out)
{
	auto c = (cosf(g_elapsed * 2 * PI * 0.33f) + 1.0f) * 0.5f;
	auto s = (sinf(g_elapsed * 2 * PI * 0.33f) + 1.0f) * 0.5f;
	auto x = in.vert.x;
	auto y = in.vert.z;
	auto shift = Perlin(Vec2(x + s, y + c) * 2.0f, 8.0f);
	out.pos = in.mvp * (in.vert + Vec3::UP * shift * 0.25f);
	out.color = Lerp(Vec4::RED, Vec4(0.0f, 0.4f, 1.0f, 1.0f), shift + 0.75f);
}

void HeartVertexShader(const VertexShaderInput& in, VertexShaderOutput& out)
{
	out.pos = in.mvp * in.vert;
	out.color = Lerp(Vec4::WHITE, Vec4::RED, in.vert.z + 0.5f);
}

void FragmentShader(const FragmentShaderInput& in, FragmentShaderOutput& out)
{
	out.color = in.color;
}

void DrawDemo()
{
	auto vp = Mat4::Perspective(Deg2Rad(78.0f), g_aspect, 0.1f, 1000.0f) *
			  Mat4::LookAt(Vec3::BACKWARD * 1.5f + Vec3::UP * 0.75f, Vec3(0.0f)) * Mat4::Scale(4.0f);
	auto planeShader = Shader {PlaneVertexShader, FragmentShader};
	auto heartShader = Shader {HeartVertexShader, FragmentShader};
	// DrawTriangle(
	//	Vec3(0.0f, -0.5f, 0.0f),
	//	Vec3(0.5f, 0.5f, 0.0f),
	//	Vec3(-0.5f, 0.5f, 0.0f),
	//	Vec4::RED,
	//	Vec4::GREEN,
	//	Vec4::BLUE,
	//	DrawMode::Wireframe,
	//	INVALID_TEXTURE_ID,
	//	mvp,
	//	&shader);
	DrawModel(
		s_planeVerts,
		ArraySize(s_planeVerts),
		s_planeTris,
		ArraySize(s_planeTris),
		DrawMode::Shaded,
		INVALID_TEXTURE_ID,
		vp,
		&planeShader);
	DrawModel(
		s_heart,
		DrawMode::Wireframe,
		INVALID_TEXTURE_ID,
		vp * Transform(Vec3(0.0f, 0.05f, -0.5f), Quat(Vec3::LEFT, PI / 2) * Quat(Vec3::UP, g_elapsed), 0.05f).GetMatrix(),
		&heartShader);
	DrawString(sinf(g_elapsed) * 128 + FRAMEBUFFER_WIDTH / 2 - 150, 8, Vec4::RED, 4.0f, "I love you!");
	DrawString(cosf(-g_elapsed) * 128 + FRAMEBUFFER_WIDTH / 2 - 120, FRAMEBUFFER_HEIGHT - 5 * 8, Vec4::RED, 4.0f, "1225 & 802");
}

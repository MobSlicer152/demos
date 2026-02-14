// This is an implementation of MD2, Quake 2's model format.
// It's a lot simpler to parse than OBJ, and it even has animations.
// Structures from here:
// https://rapidq.phatcode.net/examples/OpenGL/md2%20Format%20Specification.htm

#pragma once

#include "mathfun.h"
#include "misc.h"

// NOTE: this is only valid for little endian, so this wouldn't work on 7th gen stuff
static constexpr int32_t MD2_MAGIC = ('I' | ('D' << 8) | ('P' << 16) | ('2' << 24));
static constexpr int32_t MD2_VERSION = 8;

struct MD2Header
{
	int32_t magic = MD2_MAGIC;
	int32_t version = MD2_VERSION;
	int32_t skinWidth;
	int32_t skinHeight;
	int32_t frameSize;
	int32_t numSkins;
	int32_t numVertices;
	int32_t numTexCoords;
	int32_t numTriangles;
	int32_t numGlCommands;
	int32_t numFrames;
	int32_t offsetSkins;
	int32_t offsetTexCoords;
	int32_t offsetTriangles;
	int32_t offsetFrames;
	int32_t offsetGlCommands;
	int32_t offsetEnd;
};

struct MD2Vertex
{
	byte vertex[3];
	byte normal;

	operator Vec3() const
	{
		return Vec3(vertex[0], vertex[1], vertex[2]);
	}
};

struct MD2Frame
{
	Vec3 scale;
	Vec3 translate;
	char name[16];
	MD2Vertex* vertices;
};

struct MD2Triangle
{
	int16_t vertexIndices[3];
	int16_t textureIndices[3];
};

struct MD2TextureCoordinate
{
	int16_t s;
	int16_t t;
};

struct MD2GLVertex
{
	float s;
	float t;
	int32_t vertexIndex;
};

class CMD2Model
{
  public:
	CMD2Model() = default;
	CMD2Model(const byte* data, size_t size);
	~CMD2Model();

	const MD2Header& GetHeader() const
	{
		return m_header;
	}

	const MD2Triangle* GetTriangles() const
	{
		return m_triangles;
	}

	size_t GetTriangleCount() const
	{
		return m_header.numTriangles;
	}

	const MD2TextureCoordinate* GetTexCoords() const
	{
		return m_texCoords;
	}

	size_t GetTexCoordCount() const
	{
		return m_header.numTexCoords;
	}

	Vec2 FixTexCoord(const MD2TextureCoordinate& coord) const
	{
		return Vec2((float)coord.s / m_header.skinWidth, (float)coord.t / m_header.skinHeight);
	}

	const MD2Frame* GetFrames() const
	{
		return m_frames;
	}

	size_t GetFrameCount() const
	{
		return m_header.numFrames;
	}

	static const Vec3 NORMALS[162];

  private:
	MD2Header m_header;
	MD2TextureCoordinate* m_texCoords;
	MD2Triangle* m_triangles;
	MD2Frame* m_frames;
	MD2Vertex* m_vertices;
};

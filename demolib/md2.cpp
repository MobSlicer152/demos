#include "pch.h"

// i don't think this counts as an external library, it's just
// a bunch of numbers i could realistically generate myself but
// didn't.
constexpr Vec3 CMD2Model::NORMALS[] = {
#include "anorms.h"
};

CMD2Model::CMD2Model(byte* data, size_t size)
{
	size_t offset = 0;
	size_t readSize = sizeof(MD2Header);
	ASSERT_MSG(size - offset >= readSize, "MD2 header is truncated!");
	memcpy(&m_header, data + offset, readSize);
	ASSERT_MSG(m_header.magic == MD2_MAGIC && m_header.version == MD2_VERSION, "MD2 header is invalid!");

	m_texCoords = new MD2TextureCoordinate[m_header.numTexCoords];
	m_triangles = new MD2Triangle[m_header.numTriangles];
	m_frames = new MD2Frame[m_header.numFrames];
	m_vertices = new MD2Vertex[m_header.numVertices * m_header.numFrames];

	offset = m_header.offsetTexCoords;
	readSize = m_header.numTexCoords * sizeof(MD2TextureCoordinate);
	ASSERT_MSG(size - offset >= readSize, "MD2 texture coordinates are truncated!");
	memcpy(m_texCoords, data + offset, readSize);

	offset = m_header.offsetTriangles;
	readSize = m_header.numTriangles * sizeof(MD2Triangle);
	ASSERT_MSG(size - offset >= readSize, "MD2 triangles are truncated!");
	memcpy(m_triangles, data, readSize);

	offset = m_header.offsetFrames;
	readSize = m_header.numFrames * sizeof(MD2Header) + m_header.numFrames * m_header.numVertices * sizeof(MD2Vertex);
	ASSERT_MSG(size - offset >= readSize)
	for (int32_t i = 0; i < m_header.numFrames; i++)
	{
		m_frames[i].vertices = m_vertices + i * m_header.numVertices;
		memcpy(&m_frames[i].scale, data + offset, sizeof(Vec3));
		offset += sizeof(Vec3);
		memcpy(&m_frames[i].translate, data + offset, sizeof(Vec3));
		offset += sizeof(Vec3);
		memcpy(m_frames[i].name, data + offset, ArraySize(m_frames[i].name));
		offset += ArraySize(m_frames[i].name);
		memcpy(m_frames[i].vertices, data + offset, m_header.numVertices * sizeof(MD2Vertex));
		offset += m_header.numVertices * sizeof(MD2Vertex);
	}
}

CMD2Model::~CMD2Model()
{
	delete[] m_texCoords;
	delete[] m_triangles;
	delete[] m_frames;
	delete[] m_vertices;
}

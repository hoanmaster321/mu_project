
#pragma once

#include "../CBMu_RenderConfig.h"
#include <vector>

struct CBMu_GLTerrainVertex
{
	float Position[3];
	float TexCoord[2];
	float Color[4];
};

struct CBMu_GLTerrainChunkDesc
{
	int OriginX;
	int OriginY;
	int TileCountX;
	int TileCountY;
};

class CBMu_GLTerrainChunk
{
public:
	CBMu_GLTerrainChunk();
	~CBMu_GLTerrainChunk();

	bool Initialize(const CBMu_GLTerrainChunkDesc& desc);
	void Shutdown();
	void ClearCpuData();
	void AddQuad(
		const CBMu_GLTerrainVertex& v0,
		const CBMu_GLTerrainVertex& v1,
		const CBMu_GLTerrainVertex& v2,
		const CBMu_GLTerrainVertex& v3);
	bool Upload();
	void Draw() const;

	bool IsReady() const;
	const CBMu_GLTerrainChunkDesc& GetDesc() const;

private:
	CBMu_GLTerrainChunkDesc m_Desc;
	GLuint m_VBO;
	GLuint m_IBO;
	bool m_Ready;
	std::vector<CBMu_GLTerrainVertex> m_Vertices;
	std::vector<unsigned short> m_Indices;
};

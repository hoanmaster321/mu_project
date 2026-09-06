
#pragma once

#include "../CBMu_RenderConfig.h"
#include <vector>

struct CBMu_GLSpriteVertex
{
	float Position[3];
	float TexCoord[2];
	float Color[4];
};

struct CBMu_GLSpriteBatchKey
{
	GLuint Texture;
	int BlendMode;
	bool DepthTest;
	bool DepthWrite;
	GLuint ShaderProgram;

	bool Equals(const CBMu_GLSpriteBatchKey& other) const;
};

class CBMu_GLSpriteBatch
{
public:
	CBMu_GLSpriteBatch();
	~CBMu_GLSpriteBatch();

	bool Initialize();
	void Shutdown();
	void BeginFrame();
	void EndFrame();
	void BeginBatch(const CBMu_GLSpriteBatchKey& key);
	void SubmitQuad(
		const CBMu_GLSpriteBatchKey& key,
		const CBMu_GLSpriteVertex& v0,
		const CBMu_GLSpriteVertex& v1,
		const CBMu_GLSpriteVertex& v2,
		const CBMu_GLSpriteVertex& v3);
	void Flush();
	bool IsReady() const;
	bool IsActive() const;

private:
	void ApplyKey(const CBMu_GLSpriteBatchKey& key);

	GLuint m_VAO;
	GLuint m_VBO;
	bool m_Initialized;
	bool m_Active;
	bool m_HasKey;
	CBMu_GLSpriteBatchKey m_CurrentKey;
	std::vector<CBMu_GLSpriteVertex> m_Vertices;
};

extern CBMu_GLSpriteBatch g_CBMuGLSpriteBatch;

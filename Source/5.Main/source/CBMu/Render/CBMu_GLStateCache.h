
#pragma once

#include "../CBMu_RenderConfig.h"

class CBMu_GLStateCache
{
public:
	CBMu_GLStateCache();

	void Reset();
	void SetTexture2D(GLuint texture);
	void SetBlendEnabled(bool enabled);
	void SetBlendFunc(GLenum src, GLenum dst);
	void SetDepthTest(bool enabled);
	void SetDepthWrite(bool enabled);
	void SetCullFace(bool enabled);
	void SetShaderProgram(GLuint program);

private:
	bool m_Texture2DValid;
	bool m_BlendEnabledValid;
	bool m_BlendFuncValid;
	bool m_DepthTestValid;
	bool m_DepthWriteValid;
	bool m_CullFaceValid;
	bool m_ShaderProgramValid;

	GLuint m_Texture2D;
	bool m_BlendEnabled;
	GLenum m_BlendSrc;
	GLenum m_BlendDst;
	bool m_DepthTest;
	bool m_DepthWrite;
	bool m_CullFace;
	GLuint m_ShaderProgram;
};

extern CBMu_GLStateCache g_CBMuGLStateCache;


#pragma once

#include "../CBMu_RenderConfig.h"

class CBMu_GLStateCache
{
public:
	CBMu_GLStateCache() {}

	void Reset() {}
	void SetTexture2D(GLuint texture) {}
	void SetBlendEnabled(bool enabled) {}
	void SetBlendFunc(GLenum src, GLenum dst) {}
	void SetDepthTest(bool enabled) {}
	void SetDepthWrite(bool enabled) {}
	void SetCullFace(bool enabled) {}
	void SetShaderProgram(GLuint program) {}
};

extern CBMu_GLStateCache g_CBMuGLStateCache;

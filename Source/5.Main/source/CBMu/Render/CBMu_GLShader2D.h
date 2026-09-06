
#pragma once

#include "../CBMu_RenderConfig.h"

class CBMu_GLShader2D
{
public:
	CBMu_GLShader2D();
	~CBMu_GLShader2D();

	bool Initialize();
	void Shutdown();
	bool IsReady() const;
	void Bind() const;
	void Unbind() const;
	void SetProjection(const float projection[16]) const;
	GLuint GetProgram() const;

private:
	GLuint CompileShader(GLenum type, const char* source) const;
	bool LinkProgram(GLuint vertexShader, GLuint fragmentShader);
	void CacheUniformLocations();

	GLuint m_Program;
	GLint m_ProjectionLocation;
	GLint m_TextureLocation;
};

extern CBMu_GLShader2D g_CBMuGLShader2D;

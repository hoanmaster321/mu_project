#pragma once
// ============================================================================
// VulkanGLStub.h - Zero-Dependency Legacy OpenGL Compatibility Stub
// Cho phép Client loại bỏ hoàn toàn opengl32.lib, glu32.lib, glew, glad.
// Mọi thao tác dựng hình thực tế được đảm nhiệm bởi Vulkan (GPUContext + BatchRenderer).
// ============================================================================

#include <cstdint>
#include <cmath>
#include <cstring>

// Base OpenGL Types
typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef signed char    GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef int            GLsizei;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef double         GLclampd;
typedef void           GLvoid;
typedef ptrdiff_t      GLintptr;
typedef ptrdiff_t      GLsizeiptr;

// Standard Constants
#define GL_FALSE                          0
#define GL_TRUE                           1

// Primitives
#define GL_POINTS                         0x0000
#define GL_LINES                          0x0001
#define GL_LINE_LOOP                      0x0002
#define GL_LINE_STRIP                     0x0003
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006
#define GL_QUADS                          0x0007
#define GL_QUAD_STRIP                     0x0008
#define GL_POLYGON                        0x0009

// Matrix Modes
#define GL_MATRIX_MODE                    0x0BA0
#define GL_MODELVIEW                      0x1700
#define GL_PROJECTION                     0x1701
#define GL_TEXTURE                        0x1702
#define GL_MODELVIEW_MATRIX               0x0BA6
#define GL_PROJECTION_MATRIX              0x0BA7
#define GL_VIEWPORT                       0x0BA2

// Depth & Blending & Capabilities
#define GL_DEPTH_TEST                     0x0B71
#define GL_BLEND                          0x0BE2
#define GL_ALPHA_TEST                     0x0BC0
#define GL_CULL_FACE                      0x0B44
#define GL_TEXTURE_2D                     0x0DE1
#define GL_FOG                            0x0B60
#define GL_LIGHTING                       0x0B50
#define GL_SCISSOR_TEST                   0x0C11

// Depth & Alpha Functions
#define GL_NEVER                          0x0200
#define GL_LESS                           0x0201
#define GL_EQUAL                          0x0202
#define GL_LEQUAL                         0x0203
#define GL_GREATER                        0x0204
#define GL_NOTEQUAL                       0x0205
#define GL_GEQUAL                         0x0206
#define GL_ALWAYS                         0x0207

// Blending Factors
#define GL_ZERO                           0
#define GL_ONE                            1
#define GL_SRC_COLOR                      0x0300
#define GL_ONE_MINUS_SRC_COLOR            0x0301
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_DST_ALPHA                      0x0304
#define GL_ONE_MINUS_DST_ALPHA            0x0305
#define GL_DST_COLOR                      0x0306
#define GL_ONE_MINUS_DST_COLOR            0x0307

// Clear bits
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_DEPTH_BUFFER_BIT               0x00000100

// Texture parameters & formats
#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
#define GL_NEAREST_MIPMAP_NEAREST         0x2700
#define GL_LINEAR_MIPMAP_NEAREST          0x2701
#define GL_NEAREST_MIPMAP_LINEAR          0x2702
#define GL_LINEAR_MIPMAP_LINEAR           0x2703
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_CLAMP                          0x2900
#define GL_REPEAT                         0x2901
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_BGR_EXT                        0x80E0
#define GL_BGRA_EXT                       0x80E1
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_FLOAT                          0x1406
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_UNSIGNED_INT                   0x1405
#define GL_TEXTURE0                       0x84C0

// Client states
#define GL_VERTEX_ARRAY                   0x8074
#define GL_NORMAL_ARRAY                   0x8075
#define GL_COLOR_ARRAY                    0x8076
#define GL_TEXTURE_COORD_ARRAY            0x8078

// Culling
#define GL_BACK                           0x0405
#define GL_FRONT                          0x0404
#define GL_FRONT_AND_BACK                 0x0408
#define GL_CW                             0x0900
#define GL_CCW                            0x0901

// Buffers & Fog & Extensions & Texture Env
#define GL_UNPACK_ROW_LENGTH              0x0CF2
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DEPTH_COMPONENT                0x1902
#define GL_FOG_MODE                       0x0B65
#define GL_FOG_DENSITY                    0x0B62
#define GL_FOG_COLOR                      0x0B66
#define GL_EXTENSIONS                     0x1F03
#define GL_TEXTURE_ENV                    0x2300
#define GL_TEXTURE_ENV_MODE               0x2200
#define GL_ADD                            0x0104
#define GL_MODULATE                       0x2100
#define GL_CURRENT_COLOR                  0x0B00
#define GL_VENDOR                         0x1F00
#define GL_RENDERER                       0x1F01
#define GL_VERSION                        0x1F02
#define GL_MAX_TEXTURE_SIZE               0x0D33
#define GL_MAX_VIEWPORT_DIMS              0x0D3A
#define GL_UNPACK_ALIGNMENT               0x0CF5
#define GL_ALPHA                          0x1906
#define GL_FOG_START                      0x0B63
#define GL_FOG_END                        0x0B64
#define GL_STENCIL_TEST                   0x0B90
#define GL_KEEP                           0x1E00
#define GL_INCR                           0x1E02
#define GL_DECR                           0x1E03
#define GL_POINT                          0x1B00
#define GL_LINE                           0x1B01
#define GL_FILL                           0x1B02
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_UNIFORM_BUFFER                 0x8A11
#define GL_STREAM_DRAW                    0x88E0
#define GL_INVALID_INDEX                  0xFFFFFFFFu

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#if !defined(__ANDROID__) && !defined(MU_IOS)
inline uint64_t MU_MobilePerfNow() { return 0; }
#endif
inline void glTexEnvi(GLenum target, GLenum pname, GLint param) {}
inline void glTexEnvf(GLenum target, GLenum pname, GLfloat param) {}
inline void glPixelStorei(GLenum pname, GLint param) {}
inline void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {}
inline void glStencilFunc(GLenum func, GLint ref, GLuint mask) {}
inline void glPolygonMode(GLenum face, GLenum mode) {}

// Global state tracking
inline float g_CurrentGLColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

// Inline Stubs for Legacy OpenGL Calls
inline void glColor4f(float r, float g, float b, float a) {
    g_CurrentGLColor[0] = r; g_CurrentGLColor[1] = g; g_CurrentGLColor[2] = b; g_CurrentGLColor[3] = a;
}
inline void glColor3f(float r, float g, float b) {
    g_CurrentGLColor[0] = r; g_CurrentGLColor[1] = g; g_CurrentGLColor[2] = b; g_CurrentGLColor[3] = 1.0f;
}
inline void glColor3fv(const float* v) {
    if (v) { g_CurrentGLColor[0] = v[0]; g_CurrentGLColor[1] = v[1]; g_CurrentGLColor[2] = v[2]; g_CurrentGLColor[3] = 1.0f; }
}
inline void glColor4fv(const float* v) {
    if (v) { g_CurrentGLColor[0] = v[0]; g_CurrentGLColor[1] = v[1]; g_CurrentGLColor[2] = v[2]; g_CurrentGLColor[3] = v[3]; }
}
inline void glColor3ub(uint8_t r, uint8_t g, uint8_t b) {
    g_CurrentGLColor[0] = r / 255.0f; g_CurrentGLColor[1] = g / 255.0f; g_CurrentGLColor[2] = b / 255.0f; g_CurrentGLColor[3] = 1.0f;
}
inline void glColor4ub(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    g_CurrentGLColor[0] = r / 255.0f; g_CurrentGLColor[1] = g / 255.0f; g_CurrentGLColor[2] = b / 255.0f; g_CurrentGLColor[3] = a / 255.0f;
}

inline void glBegin(GLenum mode) {}
inline void glEnd() {}
inline void glVertex2f(float x, float y) {}
inline void glVertex3f(float x, float y, float z) {}
inline void glVertex3fv(const float* v) {}
inline void glTexCoord2f(float u, float v) {}
inline void glTexCoord2fv(const float* v) {}
inline void glNormal3f(float x, float y, float z) {}
inline void glNormal3fv(const float* v) {}

inline void glEnable(GLenum cap) {}
inline void glDisable(GLenum cap) {}
inline void glBlendFunc(GLenum sfactor, GLenum dfactor) {}
inline void glDepthFunc(GLenum func) {}
inline void glAlphaFunc(GLenum func, GLclampf ref) {}
inline void glCullFace(GLenum mode) {}
inline void glFrontFace(GLenum mode) {}
inline void glDepthMask(GLboolean flag) {}
inline void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {}
inline void glClear(GLbitfield mask) {}
inline void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {}
inline void glClearDepth(GLclampd depth) {}
inline void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {}
inline void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {}

inline void glMatrixMode(GLenum mode) {}
inline void glPushMatrix() {}
inline void glPopMatrix() {}
inline void glLoadIdentity() {}
inline void glLoadMatrixf(const float* m) {}
inline void glMultMatrixf(const float* m) {}
inline void glTranslatef(float x, float y, float z) {}
inline void glRotatef(float angle, float x, float y, float z) {}
inline void glScalef(float x, float y, float z) {}
inline void glGetFloatv(GLenum pname, float* params) { if (params) std::memset(params, 0, 16 * sizeof(float)); }
inline void glGetIntegerv(GLenum pname, int* params) { if (params) std::memset(params, 0, 4 * sizeof(int)); }

inline void glVertex4f(float x, float y, float z, float w) {}
inline void glVertex4fv(const float* v) {}

inline void glBindBuffer(GLenum target, GLuint buffer) {}
inline void glBufferData(GLenum target, ptrdiff_t size, const void* data, GLenum usage) {}
inline void glGenBuffers(GLsizei n, GLuint* buffers) {
    if (buffers) for (int i = 0; i < n; ++i) buffers[i] = (GLuint)(i + 1);
}
inline void glDeleteBuffers(GLsizei n, const GLuint* buffers) {}
inline void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels) {
    if (pixels && width > 0 && height > 0) std::memset(pixels, 0, width * height * 4);
}
inline void glFogi(GLenum pname, GLint param) {}
inline void glFogf(GLenum pname, GLfloat param) {}
inline void glFogfv(GLenum pname, const GLfloat* params) {}
inline const GLubyte* glGetString(GLenum name) {
    return (const GLubyte*)"GL_ARB_vertex_buffer_object GL_EXT_texture_filter_anisotropic";
}

inline void glBindTexture(GLenum target, GLuint texture) {}
inline void glActiveTexture(GLenum texture) {}
inline void glGenTextures(GLsizei n, GLuint* textures) {
    static GLuint s_dummyTexId = 1000;
    if (textures) {
        for (int i = 0; i < n; ++i) textures[i] = ++s_dummyTexId;
    }
}
inline void glDeleteTextures(GLsizei n, const GLuint* textures) {}
inline void glTexParameteri(GLenum target, GLenum pname, GLint param) {}
inline void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) {}
inline void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {}

inline void glEnableClientState(GLenum array) {}
inline void glDisableClientState(GLenum array) {}
inline void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer) {}
inline void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer) {}
inline void glColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer) {}
inline void glNormalPointer(GLenum type, GLsizei stride, const void* pointer) {}
inline void glDrawArrays(GLenum mode, GLint first, GLsizei count) {}
inline void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {}
inline void glFlush() {}
inline void glFinish() {}

// GLU Utilities Stub
inline void gluPerspective(double fovy, double aspect, double zNear, double zFar) {}
inline void gluOrtho2D(double left, double right, double bottom, double top) {}
inline void gluLookAt(double eyeX, double eyeY, double eyeZ, double centerX, double centerY, double centerZ, double upX, double upY, double upZ) {}
inline int gluProject(double objX, double objY, double objZ, const double* model, const double* proj, const int* view, double* winX, double* winY, double* winZ) {
    if (winX) *winX = 0; if (winY) *winY = 0; if (winZ) *winZ = 0;
    return GL_TRUE;
}
inline int gluUnProject(double winX, double winY, double winZ, const double* model, const double* proj, const int* view, double* objX, double* objY, double* objZ) {
    if (objX) *objX = 0; if (objY) *objY = 0; if (objZ) *objZ = 0;
    return GL_TRUE;
}

// Modern OpenGL 3.0+ Stubs (GLAD replacement for Pure Vulkan builds)
inline GLuint glCreateShader(GLenum type) { return 1; }
inline void glShaderSource(GLuint shader, GLsizei count, const char* const* string, const GLint* length) {}
inline void glCompileShader(GLuint shader) {}
inline void glGetShaderiv(GLuint shader, GLenum pname, GLint* params) { if (params) *params = 1; }
inline void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog) { if (infoLog && bufSize > 0) infoLog[0] = 0; }
inline void glDeleteShader(GLuint shader) {}
inline GLuint glCreateProgram() { return 1; }
inline void glAttachShader(GLuint program, GLuint shader) {}
inline void glLinkProgram(GLuint program) {}
inline void glGetProgramiv(GLuint program, GLenum pname, GLint* params) { if (params) *params = 1; }
inline void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog) { if (infoLog && bufSize > 0) infoLog[0] = 0; }
inline void glDeleteProgram(GLuint program) {}
inline void glUseProgram(GLuint program) {}
inline void glGenVertexArrays(GLsizei n, GLuint* arrays) { if (arrays) for (int i=0; i<n; ++i) arrays[i] = 1; }
inline void glBindVertexArray(GLuint array) {}
inline void glDeleteVertexArrays(GLsizei n, const GLuint* arrays) {}
inline void glEnableVertexAttribArray(GLuint index) {}
inline void glDisableVertexAttribArray(GLuint index) {}
inline void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {}
inline void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) {}
inline void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {}
inline void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {}
inline GLuint glGetUniformBlockIndex(GLuint program, const char* uniformBlockName) { return 0; }
inline void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {}
inline GLint glGetUniformLocation(GLuint program, const char* name) { return 0; }
inline void glUniform1i(GLint location, GLint v0) {}
inline void glUniform1f(GLint location, GLfloat v0) {}
inline void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {}
inline void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {}
inline void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {}
inline void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {}
inline void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) {}
inline void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {}
inline void glVertexAttribDivisor(GLuint index, GLuint divisor) {}
inline void glVertexAttribI1ui(GLuint index, GLuint x) {}
inline void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices) {}

#define GL_TEXTURE_BUFFER                 0x8C2A
#define GL_TEXTURE3                       0x84C3
#define GL_RGBA32F                        0x8814
#define GL_READ_ONLY                      0x88B8
#define GL_WRITE_ONLY                     0x88B9
#define GL_READ_WRITE                     0x88BA

inline void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer) {}
inline void glUniform4fv(GLint location, GLsizei count, const GLfloat* value) {}
inline void* glMapBuffer(GLenum target, GLenum access) { return nullptr; }
inline GLboolean glUnmapBuffer(GLenum target) { return GL_TRUE; }



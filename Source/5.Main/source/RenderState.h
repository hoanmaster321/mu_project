#pragma once
// ============================================================================
// RenderState.h - Pure Native Vulkan Rendering State & Types
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include <atomic>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL3/SDL.h>

// Base Rendering Types (replaced OpenGL types with standard C++ typedefs)
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
#define GL_TEXTURE_ENV                    0x2300
#define GL_TEXTURE_ENV_MODE               0x2200
#define GL_MODULATE                       0x2100
#define GL_TEXTURE_MAX_LEVEL              0x813D
#define GL_UNPACK_ALIGNMENT               0x0CF5
#define GL_UNPACK_ROW_LENGTH              0x0CF2
#define GL_ALPHA                          0x1906
#define GL_CURRENT_COLOR                  0x0B00
#define GL_ADD                            0x0104

// Buffer & Shader constants
#define GL_INVALID_INDEX                  0xFFFFFFFFu
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_UNIFORM_BUFFER                 0x8A11
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_DEPTH_COMPONENT                0x1902

// Fog constants
#define GL_FOG_COLOR                      0x0B66
#define GL_FOG_DENSITY                    0x0B62
#define GL_FOG_START                      0x0B63
#define GL_FOG_END                        0x0B64
#define GL_FOG_MODE                       0x0B65
#define GL_EXP                            0x0800
#define GL_EXP2                           0x0801

// Culling & Polygon
#define GL_BACK                           0x0405
#define GL_FRONT                          0x0404
#define GL_FRONT_AND_BACK                 0x0408
#define GL_CW                             0x0900
#define GL_CCW                            0x0901
#define GL_POINT                          0x1B00
#define GL_LINE                           0x1B01
#define GL_FILL                           0x1B02

// Global Color State (Used across UI, Text, and Batch Renderers)
inline float g_CurrentGLColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

// Color Setting API
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

namespace RenderState
{
    inline void SetColor(float r, float g, float b, float a = 1.0f) {
        glColor4f(r, g, b, a);
    }
    inline void SetColor(const float* v) {
        glColor3fv(v);
    }
    inline void SetColor4(const float* v) {
        glColor4fv(v);
    }
    inline void SetColorUB(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        glColor4ub(r, g, b, a);
    }
}

// ============================================================================
// CPU Matrix Stack Implementation (Independent of OpenGL library)
// ============================================================================
namespace RenderMatrix
{
    inline GLenum s_CurrentMode = GL_MODELVIEW;
    inline std::vector<glm::mat4> s_ModelViewStack = { glm::mat4(1.0f) };
    inline std::vector<glm::mat4> s_ProjectionStack = { glm::mat4(1.0f) };
    inline std::vector<glm::mat4> s_TextureStack = { glm::mat4(1.0f) };

    inline std::vector<glm::mat4>& GetCurrentStack()
    {
        if (s_CurrentMode == GL_PROJECTION) return s_ProjectionStack;
        if (s_CurrentMode == GL_TEXTURE) return s_TextureStack;
        return s_ModelViewStack;
    }
}

inline void glMatrixMode(GLenum mode) {
    RenderMatrix::s_CurrentMode = mode;
}

inline void glPushMatrix() {
    auto& stack = RenderMatrix::GetCurrentStack();
    stack.push_back(stack.back());
}

inline void glPopMatrix() {
    auto& stack = RenderMatrix::GetCurrentStack();
    if (stack.size() > 1) {
        stack.pop_back();
    }
}

inline void glLoadIdentity() {
    RenderMatrix::GetCurrentStack().back() = glm::mat4(1.0f);
}

inline void glLoadMatrixf(const float* m) {
    if (m) RenderMatrix::GetCurrentStack().back() = glm::make_mat4(m);
}

inline void glMultMatrixf(const float* m) {
    if (m) RenderMatrix::GetCurrentStack().back() *= glm::make_mat4(m);
}

inline void glTranslatef(float x, float y, float z) {
    RenderMatrix::GetCurrentStack().back() = glm::translate(RenderMatrix::GetCurrentStack().back(), glm::vec3(x, y, z));
}

inline void glRotatef(float angle, float x, float y, float z) {
    RenderMatrix::GetCurrentStack().back() = glm::rotate(RenderMatrix::GetCurrentStack().back(), glm::radians(angle), glm::vec3(x, y, z));
}

inline void glScalef(float x, float y, float z) {
    RenderMatrix::GetCurrentStack().back() = glm::scale(RenderMatrix::GetCurrentStack().back(), glm::vec3(x, y, z));
}

inline void glGetFloatv(GLenum pname, float* params) {
    if (!params) return;
    if (pname == GL_MODELVIEW_MATRIX) {
        memcpy(params, glm::value_ptr(RenderMatrix::s_ModelViewStack.back()), 16 * sizeof(float));
    } else if (pname == GL_PROJECTION_MATRIX) {
        memcpy(params, glm::value_ptr(RenderMatrix::s_ProjectionStack.back()), 16 * sizeof(float));
    } else if (pname == GL_CURRENT_COLOR) {
        memcpy(params, g_CurrentGLColor, 4 * sizeof(float));
    } else {
        memset(params, 0, 16 * sizeof(float));
    }
}

inline void gluPerspective(float fovy, float aspect, float zNear, float zFar) {
    RenderMatrix::s_ProjectionStack.back() *= glm::perspective(glm::radians(fovy), aspect, zNear, zFar);
}

inline void gluOrtho2D(float left, float right, float bottom, float top) {
    RenderMatrix::s_ProjectionStack.back() *= glm::ortho(left, right, bottom, top);
}

// ============================================================================
// State Control Wrappers & Vulkan Native Operations
// ============================================================================

// Bridge functions implemented in GPUContext.cpp
void VulkanSetClearColor(float r, float g, float b, float a);
void VulkanClearDepthBuffer();
void VulkanSetViewport(float x, float y, float width, float height);
void VulkanSetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

inline void glEnable(GLenum) {}
inline void glDisable(GLenum) {}
inline void glAlphaFunc(GLenum, float) {}
inline void glDepthFunc(GLenum) {}
inline void glDepthMask(GLboolean) {}
inline void glBlendFunc(GLenum, GLenum) {}
inline void glPolygonMode(GLenum, GLenum) {}
inline void glFlush() {}
inline void glFogf(GLenum, float) {}
inline void glFogfv(GLenum, const float*) {}
inline void glReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*) {}
inline const GLubyte* glGetString(GLenum) { return (const GLubyte*)"Vulkan Native"; }

inline void glClear(GLbitfield mask) {
    if (mask & GL_DEPTH_BUFFER_BIT) {
        VulkanClearDepthBuffer();
    }
}

inline void glClearColor(float r, float g, float b, float a) {
    VulkanSetClearColor(r, g, b, a);
}

inline void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    VulkanSetViewport(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
}

inline void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    VulkanSetScissor(x, y, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

#if !defined(__ANDROID__) && !defined(MU_IOS)
// High performance timer available globally across Windows & Android
inline uint64_t MU_MobilePerfNow() {
    return SDL_GetPerformanceCounter();
}
#endif

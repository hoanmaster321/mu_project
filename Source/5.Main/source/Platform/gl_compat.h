#pragma once
// =============================================================================
// Platform/gl_compat.h
// Pure Vulkan native stub - OpenGL ES completely removed.
// =============================================================================

#if defined(__ANDROID__) || defined(MU_IOS)

#include <stdint.h>
#include "VulkanGLStub.h"

inline void GL_Compat_Init() {}
inline void GL_Compat_Shutdown() {}
inline void GL_FlushPending() {}
inline void GL_DrawQuadsBulk(const float* vertexData, int quadCount) {}
inline void GL_DrawTrisBulk(const float* vertexData, int triCount) {}

inline void GL_BatchAppendTriangles(const float* positions, const float* normals, const float* texcoords, int numVerts, float alpha = 1.0f, float texOffsetU = 0.0f, float texOffsetV = 0.0f) {}
inline void GL_BatchAppendTrianglesLitTex(const float* positions, const float* normals, const float* texcoords, int numVerts, float alpha = 1.0f, float texOffsetU = 0.0f, float texOffsetV = 0.0f) {}
inline void GL_BatchAppendTrianglesUnlitTex(const float* positions, const float* texcoords, int numVerts, float alpha = 1.0f, float texOffsetU = 0.0f, float texOffsetV = 0.0f) {}
inline void GL_BatchAppendTrianglesConstColor(const float* positions, const float* texcoords, int numVerts, const float color[4], float texOffsetU = 0.0f, float texOffsetV = 0.0f) {}
inline void GL_BatchAppendIndexedTrianglesLitTex(const float* positions3, const float* lights3, const float* texcoords2, const short* vertexIndexBase, const short* normalIndexBase, const short* texCoordIndexBase, int triangleStrideBytes, int triangleCount, float alpha = 1.0f, float texOffsetU = 0.0f, float texOffsetV = 0.0f) {}
inline void GL_BatchAppendIndexedTrianglesConstColor(const float* positions3, const float* texcoords2, const short* vertexIndexBase, const short* texCoordIndexBase, int triangleStrideBytes, int triangleCount, const float color[4], float texOffsetU = 0.0f, float texOffsetV = 0.0f) {}

#endif // __ANDROID__

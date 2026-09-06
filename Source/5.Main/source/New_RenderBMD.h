#pragma once

struct RenderUniformBlock {
    float uProj[16];
    float uView[16];
    float u_bodyLight[4];
    float u_lightPosition[4];
    float u_meshUV[4];
    float u_setting1[4];
    float u_setting2[4];
    int   u_enableLight;
    int   uRenderFlags;
    float uAlpha;
    int   pad; // align 16 bytes cho std140
};

#if CB_SHADER330_TEST

#include "CBMu/CBMu_RenderConfig.h"
#include "New_ModelBMD.h"
#include "UniformLocationCache.h"
#include <cstddef>
#include <cstring>
#include <unordered_map>
#include <vector>

class BMD;
class CGMNewRenderBMD
{
public:
    virtual void Render(OGL330MODEL::RenderMeshVAO&) = 0;
    virtual bool RenderInstanced(const OGL330MODEL::MeshVAO&, std::size_t, std::size_t) { return false; }
};

class UniformBlockCache {
public:
    UniformBlockCache() {
#if CBMu_ENABLE_GL_BMD_UBO_RING
    }
    ~UniformBlockCache() {
        for (int i = 0; i < 3; ++i) {
            if (ubos[i]) {
                glDeleteBuffers(1, &ubos[i]);
            }
        }
#else
        glGenBuffers(1, &ubo);
    }
    ~UniformBlockCache() {
        if (ubo) glDeleteBuffers(1, &ubo);
#endif
    }

    void Bind(GLuint shaderID, const char* blockName, GLuint bindingPoint = 0) {
#if CBMu_ENABLE_GL_BMD_UBO_CACHE
        // CBMu_BEGIN: Cache uniform block BMD để tránh tra cứu chuỗi trong mỗi mesh.
        BlockBinding& binding = bindings[shaderID];
        if (!binding.Queried) {
            binding.Index = glGetUniformBlockIndex(shaderID, blockName);
            binding.Valid = binding.Index != GL_INVALID_INDEX;
            binding.Queried = true;
        }
        if (!binding.Valid) {
            return;
        }
        if (!binding.Bound || binding.BindingPoint != bindingPoint) {
            glUniformBlockBinding(shaderID, binding.Index, bindingPoint);
            binding.Bound = true;
            binding.BindingPoint = bindingPoint;
        }
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, ubo);
        // CBMu_END: Cache uniform block BMD để tránh tra cứu chuỗi trong mỗi mesh.
#else
        GLuint blockIndex = glGetUniformBlockIndex(shaderID, blockName);
        if (blockIndex != GL_INVALID_INDEX) {
            glUniformBlockBinding(shaderID, blockIndex, bindingPoint);
            glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, ubo);
        }
#endif
    }

    void Update(const RenderUniformBlock& data) {
#if CBMu_ENABLE_GL_BMD_UBO_UPDATE_SKIP
        if (hasLastBlock && std::memcmp(&lastBlock, &data, sizeof(RenderUniformBlock)) == 0) {
            return;
        }
#endif
#if CBMu_ENABLE_GL_BMD_UBO_RING
        SelectNextUBO();
#endif
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
#if CBMu_ENABLE_GL_BMD_UBO_CACHE
        // Cấp phát UBO một lần, sau đó chỉ cập nhật dữ liệu để giảm chi phí mỗi mesh.
#if CBMu_ENABLE_GL_BMD_UBO_RING && CBMu_ENABLE_GL_BMD_UBO_RING_ORPHAN
        // Orphan & upload in one atomic call to avoid uninitialized GPU read.
        glBufferData(GL_UNIFORM_BUFFER, sizeof(RenderUniformBlock), &data, GL_DYNAMIC_DRAW);
        MarkCurrentAllocated();
#else
        if (!IsCurrentAllocated()) {
            glBufferData(GL_UNIFORM_BUFFER, sizeof(RenderUniformBlock), nullptr, GL_DYNAMIC_DRAW);
            MarkCurrentAllocated();
        }
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(RenderUniformBlock), &data);
#endif
#else
        glBufferData(GL_UNIFORM_BUFFER, sizeof(RenderUniformBlock), &data, GL_DYNAMIC_DRAW);
#endif
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
#if CBMu_ENABLE_GL_BMD_UBO_UPDATE_SKIP
        std::memcpy(&lastBlock, &data, sizeof(RenderUniformBlock));
        hasLastBlock = true;
#endif
    }

private:
    struct BlockBinding {
        GLuint Index = GL_INVALID_INDEX;
        GLuint BindingPoint = 0;
        bool Queried = false;
        bool Valid = false;
        bool Bound = false;
    };

    GLuint ubo = 0;
#if CBMu_ENABLE_GL_BMD_UBO_RING
    // CBMu_BEGIN: Dùng vòng UBO RenderBlock để giảm stall khi cập nhật nhiều mesh BMD.
    GLuint ubos[3] = {};
    bool allocatedRing[3] = {};
    unsigned int uboCursor = 0;
    unsigned int currentUBOIndex = 0;

    void SelectNextUBO() {
        currentUBOIndex = uboCursor++ % 3u;
        if (ubos[currentUBOIndex] == 0) {
            glGenBuffers(1, &ubos[currentUBOIndex]);
        }
        ubo = ubos[currentUBOIndex];
    }

    bool IsCurrentAllocated() const {
        return allocatedRing[currentUBOIndex];
    }

    void MarkCurrentAllocated() {
        allocatedRing[currentUBOIndex] = true;
    }
    // CBMu_END: Dùng vòng UBO RenderBlock để giảm stall khi cập nhật nhiều mesh BMD.
#else
    bool allocated = false;

    bool IsCurrentAllocated() const {
        return allocated;
    }

    void MarkCurrentAllocated() {
        allocated = true;
    }
#endif
    std::unordered_map<GLuint, BlockBinding> bindings;
#if CBMu_ENABLE_GL_BMD_UBO_UPDATE_SKIP
    RenderUniformBlock lastBlock = {};
    bool hasLastBlock = false;
#endif
};

void CBMu_BindBMDVAO(GLuint vao);
void CBMu_UnbindBMDVAOAfterDraw();
void CBMu_ResetBMDVAOBindCache();
void CBMu_ResetBMDBoneTextureBufferCache();
void CBMu_ResetBMDWearableCompositeTextureUnits();

// Helper điền dữ liệu block từ r
inline void FillRenderBlock(RenderUniformBlock& b, const OGL330MODEL::RenderMeshVAO& r) {
#if CBMu_ENABLE_GL_BMD_MATRIX_CACHE
    if (r.m_HasCachedMatrices) {
        memcpy(b.uProj, r.m_ProjectionMatrix, sizeof(b.uProj));
        memcpy(b.uView, r.m_ViewMatrix, sizeof(b.uView));
    }
    else
#endif
    {
        GetActiveProjectionMatrix(b.uProj);
        GetActiveViewMatrix(b.uView);
    }

    b.u_bodyLight[0] = r.m_bodyLight.x;     b.u_bodyLight[1] = r.m_bodyLight.y;
    b.u_bodyLight[2] = r.m_bodyLight.z;     b.u_bodyLight[3] = r.m_bodyLight.w;

    b.u_lightPosition[0] = r.m_lightPosition.x; b.u_lightPosition[1] = r.m_lightPosition.y;
    b.u_lightPosition[2] = r.m_lightPosition.z; b.u_lightPosition[3] = r.m_lightPosition.w;

    b.u_meshUV[0] = r.m_meshUV.x; b.u_meshUV[1] = r.m_meshUV.y; b.u_meshUV[2] = r.m_meshUV.z; b.u_meshUV[3] = r.m_meshUV.w;

    b.u_setting1[0] = r.m_setting1.x; b.u_setting1[1] = r.m_setting1.y; b.u_setting1[2] = r.m_setting1.z; b.u_setting1[3] = r.m_setting1.w;
    b.u_setting2[0] = r.m_setting2.x; b.u_setting2[1] = r.m_setting2.y; b.u_setting2[2] = r.m_setting2.z; b.u_setting2[3] = r.m_setting2.w;

    b.u_enableLight = r.m_isLight ? 1 : 0;
    b.uRenderFlags = r.m_FlagRender;
    b.uAlpha = r.m_isAlpha;
    b.pad = 0;
}

extern CGMNewRenderBMD* g_NewRenderBMD;

class CGMShaderBMD : public CGMNewRenderBMD
{
public:

    bool RenderInstanced(const OGL330MODEL::MeshVAO& data, std::size_t start, std::size_t count) override;

    struct RenderStateCache {
        bool alphaBlend = false;
        bool depthTest = true;
        bool depthMask = true;
        bool textureEnabled = false;
        GLuint boundTexture = 0;
    };

    inline void SetAlphaBlend(RenderStateCache& s, bool enable) {
        if (s.alphaBlend != enable) {
            if (enable) EnableAlphaBlend(); else DisableAlphaBlend();
            s.alphaBlend = enable;
        }
    }
    inline void SetAlphaBlendMinus(RenderStateCache& s) {
        EnableAlphaBlendMinus();
        s.alphaBlend = true;
    }
    inline void SetDepthTest(RenderStateCache& s, bool enable) {
        if (s.depthTest != enable) {
            if (enable) EnableDepthTest(); else DisableDepthTest();
            s.depthTest = enable;
        }
    }
    inline void SetDepthMask(RenderStateCache& s, bool enable) {
        if (s.depthMask != enable) {
            if (enable) EnableDepthMask(); else DisableDepthMask();
            s.depthMask = enable;
        }
    }
    inline void SetTexture(RenderStateCache& s, bool enable) {
        if (s.textureEnabled != enable) {
            if (enable) glEnable(GL_TEXTURE_2D); else DisableTexture();
            s.textureEnabled = enable;
        }
    }
    inline void BindTextureCached(RenderStateCache& s, GLuint tex) {
        if (!s.textureEnabled) {
            glEnable(GL_TEXTURE_2D);
            s.textureEnabled = true;
        }
        if (s.boundTexture != tex) {
            glActiveTexture(GL_TEXTURE0);
            BindTexture(tex);
            s.boundTexture = tex;
        }
    }

    inline void SendExtraUniforms(GLuint shaderID, int renderFlags, float alpha) {
        static UniformLocationCache uniformCache;
        GLint loc;
        if ((loc = uniformCache.GetLocation(shaderID, "uRenderFlags")) != -1) {
            glUniform1i(loc, renderFlags);
        }
        if ((loc = uniformCache.GetLocation(shaderID, "uAlpha")) != -1) {
            glUniform1f(loc, alpha);
        }
    }


    CGMShaderBMD() {}
    ~CGMShaderBMD() {}
    void Render(OGL330MODEL::RenderMeshVAO&);
    void RenderOLD(OGL330MODEL::RenderMeshVAO&);
};

#endif

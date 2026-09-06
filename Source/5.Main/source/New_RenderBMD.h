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
    UniformBlockCache() {}
    ~UniformBlockCache() {}
    void Bind(GLuint shaderID, const char* blockName, GLuint bindingPoint = 0) {}
    void Update(const RenderUniformBlock& data) {}
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
        s.textureEnabled = enable;
    }
    inline void BindTextureCached(RenderStateCache& s, GLuint tex) {
        s.textureEnabled = true;
        s.boundTexture = tex;
    }

    inline void SendExtraUniforms(GLuint shaderID, int renderFlags, float alpha) {
    }


    CGMShaderBMD() {}
    ~CGMShaderBMD() {}
    void Render(OGL330MODEL::RenderMeshVAO&);
    void RenderOLD(OGL330MODEL::RenderMeshVAO&);
};

#endif

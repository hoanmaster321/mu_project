#pragma once

#include <cstdint>
#include "volk.h"

// ============================================================================
// Render State & Blend Mode Flags (Matching legacy definitions)
// ============================================================================
#ifndef RENDER_ALPHA_BLEND_MASK
#define RENDER_ALPHA_BLEND_MASK        0x00000038
#define RENDER_ALPHA_BLEND_SHIFT       3
#define RENDER_ALPHA_BLEND_TYPE_NONE   (0 << RENDER_ALPHA_BLEND_SHIFT)
#define RENDER_ALPHA_BLEND_TYPE_NORMAL (1 << RENDER_ALPHA_BLEND_SHIFT)
#define RENDER_ALPHA_BLEND_TYPE_ADD    (2 << RENDER_ALPHA_BLEND_SHIFT)
#define RENDER_ALPHA_BLEND_TYPE_SUB    (3 << RENDER_ALPHA_BLEND_SHIFT)

#define RENDER_DEPTH_MASK_ENABLE       0x00000040
#define RENDER_DEPTH_MASK_DISABLE      0x00000080
#define RENDER_CULL_FACE_ENABLE        0x00000100
#define RENDER_TILE_REPEAT             0x00000200
#define RENDER_ALPHA_TEST_ENABLE       0x00000400
#endif

enum class GPUBlendMode
{
    None = 0,
    AlphaBlend,      // src * srcAlpha + dst * (1 - srcAlpha)
    Additive,        // src * srcAlpha + dst * 1
    Multiply,        // src * dst
    Subtract,
    Count
};

enum class GPUCullMode
{
    None = 0,
    Front,
    Back,
    FrontAndBack
};

struct GPURenderState
{
    bool depthTest = true;
    bool depthWrite = true;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    GPUCullMode cullMode = GPUCullMode::None;
    GPUBlendMode blendMode = GPUBlendMode::None;

    bool alphaTest = false;
    float alphaRef = 0.0f;

    VkViewport viewport{};
    VkRect2D scissor{};

    static GPUBlendMode FromRenderFlags(uint32_t flags)
    {
        uint32_t blendType = (flags & RENDER_ALPHA_BLEND_MASK) >> RENDER_ALPHA_BLEND_SHIFT;
        switch (blendType)
        {
        case 1: return GPUBlendMode::AlphaBlend;
        case 2: return GPUBlendMode::Additive;
        case 3: return GPUBlendMode::Subtract;
        default: return GPUBlendMode::None;
        }
    }

    static bool DepthWriteFromFlags(uint32_t flags)
    {
        if (flags & RENDER_DEPTH_MASK_DISABLE) return false;
        return true;
    }
};

// =============================================================================
// UIMobileCommon.h
// Common utilities, helpers, and styling for MU Online Mobile UI.
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "ZzzOpenglUtil.h"
#include "ZzzTexture.h"
#include "UIControls.h"
#include "NewUICommon.h"
#include "_TextureIndex.h"
#include "NewUIMessageBox.h"

#if defined(__ANDROID__) || defined(MU_IOS) || (defined(__has_include) && __has_include(<SDL3/SDL.h>))
#include <SDL3/SDL.h>
#else
typedef int64_t SDL_FingerID;
struct SDL_TouchFingerEvent {
    SDL_FingerID fingerID;
    float x;
    float y;
};
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

extern int DisplayWin;
extern int DisplayHeight;
extern float g_fScreenRate_x;
extern float g_fScreenRate_y;

#ifndef RGBA
#define RGBA(r, g, b, a) (((DWORD)(a) << 24) | ((DWORD)(b) << 16) | ((DWORD)(g) << 8) | ((DWORD)(r)))
#endif

namespace UIMobile
{
    // Screen utilities
    namespace Screen
    {
        inline int GetWidth() { return (DisplayWin > 0) ? DisplayWin : 640; }
        inline int GetHeight() { return (DisplayHeight > 0) ? DisplayHeight : 480; }
    }

    struct UIRect
    {
        float x, y, w, h;
        bool Contains(float px, float py) const
        {
            return (px >= x && px <= x + w && py >= y && py <= y + h);
        }
    };

    // Screen coordinate conversion
    inline void TouchToVirtual(float normX, float normY, float& outX, float& outY)
    {
        const float winW = static_cast<float>(Screen::GetWidth());
        const float winH = static_cast<float>(Screen::GetHeight());
        outX = normX * winW;
        outY = normY * winH;
    }

    inline bool HitTestRect(float tx, float ty, float rx, float ry, float rw, float rh)
    {
        return (tx >= rx && tx <= rx + rw && ty >= ry && ty <= ry + rh);
    }

    inline bool HitTestCircle(float tx, float ty, float cx, float cy, float radius)
    {
        const float dx = tx - cx;
        const float dy = ty - cy;
        return (dx * dx + dy * dy) <= (radius * radius);
    }

    // Color definitions
    struct ColorRGBA
    {
        float r, g, b, a;
        constexpr ColorRGBA(float _r = 1.0f, float _g = 1.0f, float _b = 1.0f, float _a = 1.0f)
            : r(_r), g(_g), b(_b), a(_a) {}
    };

    namespace Colors
    {
        constexpr ColorRGBA PanelBg(0.06f, 0.09f, 0.14f, 0.94f);
        constexpr ColorRGBA PanelBorder(0.24f, 0.42f, 0.65f, 0.85f);
        constexpr ColorRGBA PanelBorderGold(0.85f, 0.70f, 0.20f, 0.95f);
        constexpr ColorRGBA HeaderBg(0.10f, 0.15f, 0.24f, 0.95f);
        constexpr ColorRGBA SlotBg(0.08f, 0.12f, 0.18f, 0.85f);
        constexpr ColorRGBA SlotBorder(0.28f, 0.38f, 0.52f, 0.65f);
        constexpr ColorRGBA SlotHighlight(0.95f, 0.80f, 0.15f, 1.0f);
        constexpr ColorRGBA BtnPrimary(0.16f, 0.46f, 0.82f, 0.92f);
        constexpr ColorRGBA BtnPrimaryPressed(0.10f, 0.32f, 0.60f, 0.95f);
        constexpr ColorRGBA BtnSuccess(0.16f, 0.62f, 0.32f, 0.92f);
        constexpr ColorRGBA BtnSuccessPressed(0.10f, 0.44f, 0.22f, 0.95f);
        constexpr ColorRGBA BtnDanger(0.72f, 0.20f, 0.20f, 0.92f);
        constexpr ColorRGBA BtnDangerPressed(0.50f, 0.12f, 0.12f, 0.95f);
        constexpr ColorRGBA BtnDisabled(0.22f, 0.24f, 0.28f, 0.60f);
    }

    // Drawing helpers
    inline void DrawSolidRect(float x, float y, float w, float h, const ColorRGBA& c)
    {
        glColor4f(c.r, c.g, c.b, c.a);
        RenderColor(x, y, w, h);
        EndRenderColor();
    }

    inline void DrawBorder(float x, float y, float w, float h, float thickness, const ColorRGBA& c)
    {
        glColor4f(c.r, c.g, c.b, c.a);
        RenderColor(x, y, w, thickness);
        RenderColor(x, y + h - thickness, w, thickness);
        RenderColor(x, y, thickness, h);
        RenderColor(x + w - thickness, y, thickness, h);
        EndRenderColor();
    }

    namespace Render
    {
        inline void DrawGlassCard(float x, float y, float w, float h, float alpha = 0.9f)
        {
            glColor4f(0.06f, 0.09f, 0.14f, alpha);
            RenderColor(x, y, w, h);
            EndRenderColor();
        }

        inline void DrawPanelBorder(float x, float y, float w, float h, DWORD color = 0)
        {
            float r = (float)(color & 0xFF) / 255.0f;
            float g = (float)((color >> 8) & 0xFF) / 255.0f;
            float b = (float)((color >> 16) & 0xFF) / 255.0f;
            float a = (float)((color >> 24) & 0xFF) / 255.0f;
            if (a <= 0.0f) a = 1.0f;
            DrawBorder(x, y, w, h, 1.5f, ColorRGBA(r, g, b, a));
        }
    }

    inline void DrawStretched(GLuint tex, float dstX, float dstY, float dstW, float dstH, DWORD color = 0xffffffff)
    {
        BITMAP_t* pImage = &Bitmaps[tex];
        if (!pImage || pImage->Width <= 0.0f || pImage->Height <= 0.0f) return;

        const float srcW = (pImage->OrigWidth > 0.0f) ? pImage->OrigWidth : pImage->Width;
        const float srcH = (pImage->OrigHeight > 0.0f) ? pImage->OrigHeight : pImage->Height;

        const float u = 0.5f / pImage->Width;
        const float v = 0.5f / pImage->Height;
        const float uw = (srcW > 1.0f) ? ((srcW - 1.0f) / pImage->Width) : (1.0f / pImage->Width);
        const float vh = (srcH > 1.0f) ? ((srcH - 1.0f) / pImage->Height) : (1.0f / pImage->Height);

        RenderColorBitmap(tex, dstX, dstY, dstW, dstH, u, v, uw, vh, color);
    }

    inline void DrawSubStretched(GLuint tex, float dstX, float dstY, float dstW, float dstH,
                                 float srcX, float srcY, float srcW, float srcH, DWORD color = 0xffffffff)
    {
        BITMAP_t* pImage = &Bitmaps[tex];
        if (!pImage || pImage->Width <= 0.0f || pImage->Height <= 0.0f) return;

        const float u = (srcX + 0.5f) / pImage->Width;
        const float v = (srcY + 0.5f) / pImage->Height;
        const float uw = (srcW > 1.0f) ? ((srcW - 1.0f) / pImage->Width) : (1.0f / pImage->Width);
        const float vh = (srcH > 1.0f) ? ((srcH - 1.0f) / pImage->Height) : (1.0f / pImage->Height);

        RenderColorBitmap(tex, dstX, dstY, dstW, dstH, u, v, uw, vh, color);
    }

    // Texture-based MU Season 6 Frame Slicing
    inline void DrawSlicedFrame(float x, float y, float w, float h, const char* title = nullptr)
    {
        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // 1. Stony/leather background
        DrawStretched(SEASON3B::CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK, x + 4.0f, y + 4.0f, w - 8.0f, h - 8.0f);

        // 2. Top Header (golden ornate banner, original 190x64)
        const float headerH = (h >= 180.0f) ? 48.0f : 32.0f;
        DrawStretched(BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 1, x, y, w, headerH); // newui_item_back04.tga

        // 3. Side borders (original 21x320)
        const float sideW = 12.0f;
        const float sideY = y + headerH;
        const float sideH = h - headerH - 32.0f;
        if (sideH > 0.0f)
        {
            DrawStretched(BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 2, x, sideY, sideW, sideH); // newui_item_back02-L.tga
            DrawStretched(BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 3, x + w - sideW, sideY, sideW, sideH); // newui_item_back02-R.tga
        }

        // 4. Bottom border (original 190x45)
        const float bottomH = 32.0f;
        DrawStretched(BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 4, x, y + h - bottomH, w, bottomH); // newui_item_back03.tga

        // 5. Title
        if (title != nullptr && std::strlen(title) > 0 && g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetBgColor(0, 0, 0, 0);

            // Shadow
            g_pRenderText->SetTextColor(10, 10, 10, 220);
            g_pRenderText->RenderText(static_cast<int>(x + 19.0f), static_cast<int>(y + 11.0f), title);

            // Gold title
            g_pRenderText->SetTextColor(255, 220, 100, 255);
            g_pRenderText->RenderText(static_cast<int>(x + 18.0f), static_cast<int>(y + 10.0f), title);
        }
        DisableAlphaBlend();
    }

    inline void DrawPanel(float x, float y, float w, float h, const char* title = nullptr, bool goldBorder = false)
    {
        // Use textured sliced frame as standard panel
        DrawSlicedFrame(x, y, w, h, title);
    }

    inline void DrawCloseButton(float cx, float cy, float size, bool isPressed = false)
    {
        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        const GLuint exitBtnId = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 17; // newui_exit_00.tga (36x58, two 36x29 states)
        const float btnH = size * (29.0f / 36.0f);
        const float srcY = isPressed ? 29.0f : 0.0f;
        DrawSubStretched(exitBtnId, cx, cy, size, btnH, 0.0f, srcY, 36.0f, 29.0f);
        DisableAlphaBlend();
    }

    inline void DrawButton(float x, float y, float w, float h, const char* text, bool isPressed, bool enabled, const ColorRGBA& baseColor = Colors::BtnPrimary)
    {
        EnableAlphaTest();
        GLuint btnTex = SEASON3B::CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY;
        float srcW = 108.0f;
        if (w >= 160.0f)
        {
            btnTex = SEASON3B::CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_BIG;
            srcW = 180.0f;
        }
        else if (w < 80.0f)
        {
            btnTex = SEASON3B::CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_SMALL;
            srcW = 64.0f;
        }

        const float srcH = 29.0f;
        const float srcY = isPressed ? 58.0f : 0.0f;

        const DWORD tint = enabled ? 0xffffffff : 0x997f7f7f;
        DrawSubStretched(btnTex, x, y, w, h, 0.0f, srcY, srcW, srcH, tint);

        if (g_pRenderText && text)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetBgColor(0, 0, 0, 0);

            const int ty = static_cast<int>(y + (h - 13.0f) * 0.5f + (isPressed ? 1.0f : 0.0f));

            if (enabled)
            {
                // Shadow
                g_pRenderText->SetTextColor(10, 10, 10, 220);
                g_pRenderText->RenderText(static_cast<int>(x + 1.0f), ty + 1, text, static_cast<int>(w), 0, RT3_SORT_CENTER);

                if (isPressed)
                    g_pRenderText->SetTextColor(255, 200, 80, 255);
                else
                    g_pRenderText->SetTextColor(255, 235, 180, 255);

                g_pRenderText->RenderText(static_cast<int>(x), ty, text, static_cast<int>(w), 0, RT3_SORT_CENTER);
            }
            else
            {
                g_pRenderText->SetTextColor(140, 140, 140, 200);
                g_pRenderText->RenderText(static_cast<int>(x), ty, text, static_cast<int>(w), 0, RT3_SORT_CENTER);
            }
        }
        DisableAlphaBlend();
    }

    inline void DrawTableFrameBorders(float x, float y, float w, float h)
    {
        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        const GLuint tTopLeft     = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN + 1; // newui_item_table01(L) 14x14
        const GLuint tTopRight    = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN + 2; // newui_item_table01(R) 14x14
        const GLuint tBottomLeft  = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN + 3; // newui_item_table02(L) 14x14
        const GLuint tBottomRight = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN + 4; // newui_item_table02(R) 14x14
        const GLuint tTopPixel    = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN + 5; // newui_item_table03(Up) 1x14
        const GLuint tBottomPixel = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN + 6; // newui_item_table03(Dw) 1x14
        const GLuint tLeftPixel   = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN + 7; // newui_item_table03(L) 14x1
        const GLuint tRightPixel  = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN + 8; // newui_item_table03(R) 14x1

        const float WND_TOP_EDGE = 3.0f;
        const float WND_LEFT_EDGE = 4.0f;
        const float WND_BOTTOM_EDGE = 8.0f;
        const float WND_RIGHT_EDGE = 9.0f;

        // 1. Authentic MU Outer Frame Brackets (14x14)
        SEASON3B::RenderImage(tTopLeft, x - WND_LEFT_EDGE, y - WND_TOP_EDGE, 14.0f, 14.0f);
        SEASON3B::RenderImage(tTopRight, x + w - WND_RIGHT_EDGE, y - WND_TOP_EDGE, 14.0f, 14.0f);
        SEASON3B::RenderImage(tBottomLeft, x - WND_LEFT_EDGE, y + h - WND_BOTTOM_EDGE, 14.0f, 14.0f);
        SEASON3B::RenderImage(tBottomRight, x + w - WND_RIGHT_EDGE, y + h - WND_BOTTOM_EDGE, 14.0f, 14.0f);

        // 2. Authentic MU Outer Frame Edges
        const float frameTopLeft = x - WND_LEFT_EDGE + 14.0f;
        const float frameTopWidth = w + WND_LEFT_EDGE - WND_RIGHT_EDGE - 14.0f;
        if (frameTopWidth > 0.0f)
        {
            SEASON3B::RenderImage(tTopPixel, frameTopLeft, y - WND_TOP_EDGE, frameTopWidth, 14.0f);
            SEASON3B::RenderImage(tBottomPixel, frameTopLeft, y + h - WND_BOTTOM_EDGE, frameTopWidth, 14.0f);
        }

        const float frameSideTop = y - WND_TOP_EDGE + 14.0f;
        const float frameSideHeight = h + WND_TOP_EDGE - WND_BOTTOM_EDGE - 14.0f;
        if (frameSideHeight > 0.0f)
        {
            SEASON3B::RenderImage(tLeftPixel, x - WND_LEFT_EDGE, frameSideTop, 14.0f, frameSideHeight);
            SEASON3B::RenderImage(tRightPixel, x + w - WND_RIGHT_EDGE, frameSideTop, 14.0f, frameSideHeight);
        }

        DisableAlphaBlend();
    }

    inline void DrawTableGrid(float x, float y, float w, float h, int cols = 8, int rows = 8)
    {
        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        const GLuint tSquare = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN; // newui_item_box 21x21

        // 1. Grid Cells
        const float cellW = w / static_cast<float>(cols);
        const float cellH = h / static_cast<float>(rows);
        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                DrawStretched(tSquare, x + (c * cellW), y + (r * cellH), cellW, cellH);
            }
        }

        // 2. Outer Frame
        DrawTableFrameBorders(x, y, w, h);
    }

    inline void DrawPaperdollSlot(float x, float y, float w, float h, GLuint slotSilhouetteId = 0, bool isSelected = false, bool isHover = false)
    {
        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // Base slot box
        DrawStretched(BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN, x, y, w, h);

        // Silhouette icon
        if (slotSilhouetteId > 0)
        {
            glColor4f(1.0f, 1.0f, 1.0f, 0.85f);
            DrawStretched(slotSilhouetteId, x, y, w, h);
        }

        // Highlight border
        if (isSelected)
        {
            DrawBorder(x, y, w, h, 2.0f, Colors::PanelBorderGold);
        }
        else if (isHover)
        {
            DrawBorder(x, y, w, h, 2.0f, ColorRGBA(0.2f, 0.9f, 0.2f, 0.9f));
        }
        else
        {
            DrawBorder(x, y, w, h, 1.0f, ColorRGBA(0.25f, 0.35f, 0.45f, 0.4f));
        }
        DisableAlphaBlend();
    }
}

using UIMobile::UIRect;

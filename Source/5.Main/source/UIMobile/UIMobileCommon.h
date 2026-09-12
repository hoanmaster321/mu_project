// =============================================================================
// UIMobileCommon.h
// Common utilities, helpers, and styling for MU Online Mobile UI.
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "ZzzOpenglUtil.h"
#include "ZzzTexture.h"
#include "ZzzInterface.h"
#include "UIControls.h"

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

namespace UIMobile
{
    // Screen coordinate conversion
    inline void TouchToVirtual(float normX, float normY, float& outX, float& outY)
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;
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

    inline void DrawPanel(float x, float y, float w, float h, const char* title = nullptr, bool goldBorder = false)
    {
        // Background
        DrawSolidRect(x, y, w, h, Colors::PanelBg);

        // Header bar if title provided
        if (title != nullptr && std::strlen(title) > 0)
        {
            DrawSolidRect(x, y, w, 32.0f, Colors::HeaderBg);
            DrawSolidRect(x, y + 31.0f, w, 1.5f, goldBorder ? Colors::PanelBorderGold : Colors::PanelBorder);

            if (g_pRenderText)
            {
                g_pRenderText->SetFont(g_hFontBold);
                if (goldBorder)
                    g_pRenderText->SetTextColor(255, 220, 100, 255);
                else
                    g_pRenderText->SetTextColor(220, 235, 255, 255);
                g_pRenderText->SetBgColor(0, 0, 0, 0);
                g_pRenderText->RenderText(static_cast<int>(x + 16.0f), static_cast<int>(y + 9.0f), title);
            }
        }

        // Outer border
        DrawBorder(x, y, w, h, 2.0f, goldBorder ? Colors::PanelBorderGold : Colors::PanelBorder);
    }

    inline void DrawCloseButton(float cx, float cy, float size, bool isPressed = false)
    {
        const ColorRGBA bg = isPressed ? Colors::BtnDangerPressed : Colors::BtnDanger;
        DrawSolidRect(cx, cy, size, size, bg);
        DrawBorder(cx, cy, size, size, 1.5f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.6f));

        if (g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(255, 255, 255, 255);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            const int textX = static_cast<int>(cx + (size * 0.5f) - 4.0f);
            const int textY = static_cast<int>(cy + (size * 0.5f) - 6.0f);
            g_pRenderText->RenderText(textX, textY, "X");
        }
    }

    inline void DrawButton(float x, float y, float w, float h, const char* text, bool isPressed, bool enabled, const ColorRGBA& baseColor)
    {
        ColorRGBA bg = enabled ? (isPressed ? ColorRGBA(baseColor.r * 0.75f, baseColor.g * 0.75f, baseColor.b * 0.75f, baseColor.a) : baseColor) : Colors::BtnDisabled;
        DrawSolidRect(x, y, w, h, bg);

        ColorRGBA border = enabled ? ColorRGBA(1.0f, 1.0f, 1.0f, isPressed ? 0.9f : 0.45f) : ColorRGBA(0.4f, 0.4f, 0.4f, 0.3f);
        DrawBorder(x, y, w, h, 1.5f, border);

        if (g_pRenderText && text)
        {
            g_pRenderText->SetFont(g_hFontBold);
            if (enabled)
                g_pRenderText->SetTextColor(255, 255, 255, 255);
            else
                g_pRenderText->SetTextColor(140, 140, 140, 200);
            g_pRenderText->SetBgColor(0, 0, 0, 0);

            const int approxCharW = 7;
            const int strLen = static_cast<int>(std::strlen(text));
            const int textX = static_cast<int>(x + (w * 0.5f) - (strLen * approxCharW * 0.5f));
            const int textY = static_cast<int>(y + (h * 0.5f) - 6.0f);
            g_pRenderText->RenderText((std::max)(static_cast<int>(x + 4.0f), textX), textY, text);
        }
    }
}

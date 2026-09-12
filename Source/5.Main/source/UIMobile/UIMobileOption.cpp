// =============================================================================
// UIMobileOption.cpp
// Implementation of Mobile Settings Window.
// =============================================================================

#include "stdafx.h"
#include "UIMobileOption.h"
#include "ZzzScene.h"
#include "ZzzOpenglUtil.h"
#include "DSPlaySound.h"

namespace SEASON3B
{
    CUIMobileOption::CUIMobileOption()
        : m_pNewUIMng(nullptr)
        , m_bIsOpen(false)
        , m_winX(0.0f), m_winY(0.0f), m_winW(0.0f), m_winH(0.0f)
        , m_closeBtnX(0.0f), m_closeBtnY(0.0f), m_closeBtnSize(0.0f)
        , m_sfxVolume(8)
        , m_bgmVolume(5)
        , m_bMuteAll(false)
        , m_fpsMode(1) // 60 FPS default
        , m_bEffectGlow(true)
        , m_bFloatingJoystick(true)
        , m_bAutoTarget(true)
    {
    }

    CUIMobileOption::~CUIMobileOption()
    {
        Release();
    }

    bool CUIMobileOption::Create(CNewUIManager* pNewUIMng)
    {
        m_pNewUIMng = pNewUIMng;
        if (m_pNewUIMng)
        {
            m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_OPTION, this);
        }
        ComputeLayout();
        return true;
    }

    void CUIMobileOption::Release()
    {
        if (m_pNewUIMng)
        {
            m_pNewUIMng->RemoveUIObj(this);
            m_pNewUIMng = nullptr;
        }
        m_bIsOpen = false;
    }

    void CUIMobileOption::ComputeLayout()
    {
        m_winW = 460.0f;
        m_winH = 340.0f;
        m_winX = ((float)UIMobile::Screen::GetWidth() - m_winW) * 0.5f;
        m_winY = ((float)UIMobile::Screen::GetHeight() - m_winH) * 0.5f;

        m_closeBtnSize = 34.0f;
        m_closeBtnX = m_winX + m_winW - m_closeBtnSize - 12.0f;
        m_closeBtnY = m_winY + 10.0f;

        float contentX = m_winX + 24.0f;
        float startY = m_winY + 54.0f;
        float rowHeight = 44.0f;

        // Row 1: SFX
        m_rcSfxMinus = { m_winX + m_winW - 170.0f, startY, 34.0f, 30.0f };
        m_rcSfxPlus  = { m_winX + m_winW - 58.0f,  startY, 34.0f, 30.0f };

        // Row 2: BGM
        m_rcBgmMinus = { m_winX + m_winW - 170.0f, startY + rowHeight, 34.0f, 30.0f };
        m_rcBgmPlus  = { m_winX + m_winW - 58.0f,  startY + rowHeight, 34.0f, 30.0f };

        // Row 3: FPS Mode (30 / 60 / 120)
        float fpsBtnW = 46.0f;
        m_rcFps30  = { m_winX + m_winW - 160.0f, startY + rowHeight * 2.0f, fpsBtnW, 30.0f };
        m_rcFps60  = { m_winX + m_winW - 110.0f, startY + rowHeight * 2.0f, fpsBtnW, 30.0f };
        m_rcFps120 = { m_winX + m_winW - 60.0f,  startY + rowHeight * 2.0f, fpsBtnW, 30.0f };

        // Row 4: Joystick Type Toggle
        m_rcJoystickToggle = { m_winX + m_winW - 150.0f, startY + rowHeight * 3.0f, 130.0f, 30.0f };

        // Row 5: Auto-Target Toggle
        m_rcAutoTargetToggle = { m_winX + m_winW - 150.0f, startY + rowHeight * 4.0f, 130.0f, 30.0f };

        // Save Button
        m_rcSaveBtn = { m_winX + (m_winW - 160.0f) * 0.5f, m_winY + m_winH - 46.0f, 160.0f, 34.0f };
    }

    void CUIMobileOption::Open()
    {
        m_bIsOpen = true;
        ComputeLayout();
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileOption::Close()
    {
        m_bIsOpen = false;
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileOption::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    bool CUIMobileOption::Update()
    {
        if (!m_bIsOpen) return true;
        return true;
    }

    bool CUIMobileOption::UpdateKeyEvent()
    {
        if (m_bIsOpen && (SEASON3B::IsPress(VK_ESCAPE)))
        {
            Close();
            return false;
        }
        return true;
    }

    void CUIMobileOption::ApplySettings()
    {
        SetEffectVolumeLevel(m_sfxVolume);
        PlayBuffer(SOUND_CLICK01);
        Close();
    }

    bool CUIMobileOption::OnFingerDown(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        float touchX = ev.x * (float)UIMobile::Screen::GetWidth();
        float touchY = ev.y * (float)UIMobile::Screen::GetHeight();

        // Close button
        if (touchX >= m_closeBtnX && touchX <= m_closeBtnX + m_closeBtnSize &&
            touchY >= m_closeBtnY && touchY <= m_closeBtnY + m_closeBtnSize)
        {
            Close();
            return true;
        }

        // SFX Controls
        if (m_rcSfxMinus.Contains(touchX, touchY))
        {
            if (m_sfxVolume > 0) m_sfxVolume--;
            SetEffectVolumeLevel(m_sfxVolume);
            PlayBuffer(SOUND_CLICK01);
            return true;
        }
        if (m_rcSfxPlus.Contains(touchX, touchY))
        {
            if (m_sfxVolume < 10) m_sfxVolume++;
            SetEffectVolumeLevel(m_sfxVolume);
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        // BGM Controls
        if (m_rcBgmMinus.Contains(touchX, touchY))
        {
            if (m_bgmVolume > 0) m_bgmVolume--;
            PlayBuffer(SOUND_CLICK01);
            return true;
        }
        if (m_rcBgmPlus.Contains(touchX, touchY))
        {
            if (m_bgmVolume < 10) m_bgmVolume++;
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        // FPS Modes
        if (m_rcFps30.Contains(touchX, touchY))  { m_fpsMode = 0; PlayBuffer(SOUND_CLICK01); return true; }
        if (m_rcFps60.Contains(touchX, touchY))  { m_fpsMode = 1; PlayBuffer(SOUND_CLICK01); return true; }
        if (m_rcFps120.Contains(touchX, touchY)) { m_fpsMode = 2; PlayBuffer(SOUND_CLICK01); return true; }

        // Joystick Toggle
        if (m_rcJoystickToggle.Contains(touchX, touchY))
        {
            m_bFloatingJoystick = !m_bFloatingJoystick;
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        // Auto Target Toggle
        if (m_rcAutoTargetToggle.Contains(touchX, touchY))
        {
            m_bAutoTarget = !m_bAutoTarget;
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        // Save Button
        if (m_rcSaveBtn.Contains(touchX, touchY))
        {
            ApplySettings();
            return true;
        }

        // Inside window absorbs touch
        if (touchX >= m_winX && touchX <= m_winX + m_winW &&
            touchY >= m_winY && touchY <= m_winY + m_winH)
        {
            return true;
        }

        return false;
    }

    bool CUIMobileOption::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        float touchX = ev.x * (float)UIMobile::Screen::GetWidth();
        float touchY = ev.y * (float)UIMobile::Screen::GetHeight();
        return (touchX >= m_winX && touchX <= m_winX + m_winW && touchY >= m_winY && touchY <= m_winY + m_winH);
    }

    bool CUIMobileOption::OnFingerUp(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        float touchX = ev.x * (float)UIMobile::Screen::GetWidth();
        float touchY = ev.y * (float)UIMobile::Screen::GetHeight();
        return (touchX >= m_winX && touchX <= m_winX + m_winW && touchY >= m_winY && touchY <= m_winY + m_winH);
    }

    bool CUIMobileOption::Render()
    {
        if (!m_bIsOpen) return true;

        // Background Glass Panel
        UIMobile::Render::DrawGlassCard(m_winX, m_winY, m_winW, m_winH, 0.94f);
        UIMobile::Render::DrawPanelBorder(m_winX, m_winY, m_winW, m_winH, RGBA(90, 160, 240, 220));

        // Header Title
        if (g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(RGBA(255, 220, 80, 255));
            g_pRenderText->SetBgColor(0);
            g_pRenderText->RenderText((int)(m_winX + 24.0f), (int)(m_winY + 16.0f), GlobalText[383] ? GlobalText[383] : "CÀI ĐẶT HỆ THỐNG / SETTINGS");

            // Close Button
            g_pRenderText->SetTextColor(RGBA(255, 100, 100, 255));
            g_pRenderText->RenderText((int)(m_closeBtnX + 10.0f), (int)(m_closeBtnY + 8.0f), "X");
        }

        // Row 1: SFX
        float contentX = m_winX + 24.0f;
        if (g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetTextColor(RGBA(220, 220, 240, 255));
            g_pRenderText->RenderText((int)contentX, (int)(m_rcSfxMinus.y + 6.0f), "Âm lượng hiệu ứng (SFX):");
        }
        UIMobile::Render::DrawGlassCard(m_rcSfxMinus.x, m_rcSfxMinus.y, m_rcSfxMinus.w, m_rcSfxMinus.h, 0.7f);
        UIMobile::Render::DrawGlassCard(m_rcSfxPlus.x, m_rcSfxPlus.y, m_rcSfxPlus.w, m_rcSfxPlus.h, 0.7f);
        if (g_pRenderText)
        {
            g_pRenderText->RenderText((int)(m_rcSfxMinus.x + 12.0f), (int)(m_rcSfxMinus.y + 6.0f), "-");
            g_pRenderText->RenderText((int)(m_rcSfxPlus.x + 11.0f), (int)(m_rcSfxPlus.y + 6.0f), "+");

            char buf[16];
            sprintf_s(buf, "%d / 10", m_sfxVolume);
            g_pRenderText->SetTextColor(RGBA(120, 220, 255, 255));
            g_pRenderText->RenderText((int)(m_rcSfxMinus.x + 42.0f), (int)(m_rcSfxMinus.y + 6.0f), buf);
        }

        // Row 2: BGM
        if (g_pRenderText)
        {
            g_pRenderText->SetTextColor(RGBA(220, 220, 240, 255));
            g_pRenderText->RenderText((int)contentX, (int)(m_rcBgmMinus.y + 6.0f), "Nhạc nền trò chơi (BGM):");
        }
        UIMobile::Render::DrawGlassCard(m_rcBgmMinus.x, m_rcBgmMinus.y, m_rcBgmMinus.w, m_rcBgmMinus.h, 0.7f);
        UIMobile::Render::DrawGlassCard(m_rcBgmPlus.x, m_rcBgmPlus.y, m_rcBgmPlus.w, m_rcBgmPlus.h, 0.7f);
        if (g_pRenderText)
        {
            g_pRenderText->RenderText((int)(m_rcBgmMinus.x + 12.0f), (int)(m_rcBgmMinus.y + 6.0f), "-");
            g_pRenderText->RenderText((int)(m_rcBgmPlus.x + 11.0f), (int)(m_rcBgmPlus.y + 6.0f), "+");

            char buf[16];
            sprintf_s(buf, "%d / 10", m_bgmVolume);
            g_pRenderText->SetTextColor(RGBA(120, 220, 255, 255));
            g_pRenderText->RenderText((int)(m_rcBgmMinus.x + 42.0f), (int)(m_rcBgmMinus.y + 6.0f), buf);
        }

        // Row 3: FPS
        if (g_pRenderText)
        {
            g_pRenderText->SetTextColor(RGBA(220, 220, 240, 255));
            g_pRenderText->RenderText((int)contentX, (int)(m_rcFps30.y + 6.0f), "Giới hạn khung hình (FPS):");
        }
        UIMobile::Render::DrawGlassCard(m_rcFps30.x, m_rcFps30.y, m_rcFps30.w, m_rcFps30.h, m_fpsMode == 0 ? 0.9f : 0.4f);
        UIMobile::Render::DrawGlassCard(m_rcFps60.x, m_rcFps60.y, m_rcFps60.w, m_rcFps60.h, m_fpsMode == 1 ? 0.9f : 0.4f);
        UIMobile::Render::DrawGlassCard(m_rcFps120.x, m_rcFps120.y, m_rcFps120.w, m_rcFps120.h, m_fpsMode == 2 ? 0.9f : 0.4f);
        if (g_pRenderText)
        {
            g_pRenderText->SetTextColor(m_fpsMode == 0 ? RGBA(255, 220, 80, 255) : RGBA(180, 180, 190, 255));
            g_pRenderText->RenderText((int)(m_rcFps30.x + 8.0f), (int)(m_rcFps30.y + 6.0f), "30");

            g_pRenderText->SetTextColor(m_fpsMode == 1 ? RGBA(255, 220, 80, 255) : RGBA(180, 180, 190, 255));
            g_pRenderText->RenderText((int)(m_rcFps60.x + 8.0f), (int)(m_rcFps60.y + 6.0f), "60");

            g_pRenderText->SetTextColor(m_fpsMode == 2 ? RGBA(255, 220, 80, 255) : RGBA(180, 180, 190, 255));
            g_pRenderText->RenderText((int)(m_rcFps120.x + 6.0f), (int)(m_rcFps120.y + 6.0f), "120");
        }

        // Row 4: Joystick
        if (g_pRenderText)
        {
            g_pRenderText->SetTextColor(RGBA(220, 220, 240, 255));
            g_pRenderText->RenderText((int)contentX, (int)(m_rcJoystickToggle.y + 6.0f), "Kiểu cần Joystick:");
        }
        UIMobile::Render::DrawGlassCard(m_rcJoystickToggle.x, m_rcJoystickToggle.y, m_rcJoystickToggle.w, m_rcJoystickToggle.h, 0.7f);
        if (g_pRenderText)
        {
            g_pRenderText->SetTextColor(m_bFloatingJoystick ? RGBA(100, 255, 150, 255) : RGBA(200, 200, 220, 255));
            g_pRenderText->RenderText((int)(m_rcJoystickToggle.x + 14.0f), (int)(m_rcJoystickToggle.y + 6.0f),
                m_bFloatingJoystick ? "Linh hoạt (Float)" : "Cố định (Fixed)");
        }

        // Row 5: Auto Target
        if (g_pRenderText)
        {
            g_pRenderText->SetTextColor(RGBA(220, 220, 240, 255));
            g_pRenderText->RenderText((int)contentX, (int)(m_rcAutoTargetToggle.y + 6.0f), "Tự động khóa mục tiêu:");
        }
        UIMobile::Render::DrawGlassCard(m_rcAutoTargetToggle.x, m_rcAutoTargetToggle.y, m_rcAutoTargetToggle.w, m_rcAutoTargetToggle.h, 0.7f);
        if (g_pRenderText)
        {
            g_pRenderText->SetTextColor(m_bAutoTarget ? RGBA(100, 255, 150, 255) : RGBA(255, 120, 120, 255));
            g_pRenderText->RenderText((int)(m_rcAutoTargetToggle.x + 36.0f), (int)(m_rcAutoTargetToggle.y + 6.0f),
                m_bAutoTarget ? "BẬT" : "TẮT");
        }

        // Save Button
        UIMobile::Render::DrawGlassCard(m_rcSaveBtn.x, m_rcSaveBtn.y, m_rcSaveBtn.w, m_rcSaveBtn.h, 0.85f);
        UIMobile::Render::DrawPanelBorder(m_rcSaveBtn.x, m_rcSaveBtn.y, m_rcSaveBtn.w, m_rcSaveBtn.h, RGBA(100, 220, 100, 255));
        if (g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(RGBA(120, 255, 150, 255));
            g_pRenderText->RenderText((int)(m_rcSaveBtn.x + 28.0f), (int)(m_rcSaveBtn.y + 8.0f), "LƯU CÀI ĐẶT");
        }

        return true;
    }
}

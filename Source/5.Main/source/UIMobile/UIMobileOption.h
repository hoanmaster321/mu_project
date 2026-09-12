// =============================================================================
// UIMobileOption.h
// Mobile Settings Window (Audio, FrameRate, Joystick & Game options).
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "UIMobileCommon.h"

namespace SEASON3B
{
    class CUIMobileOption : public CNewUIObj
    {
    public:
        CUIMobileOption();
        virtual ~CUIMobileOption();

        bool Create(CNewUIManager* pNewUIMng);
        void Release();

        bool Render() override;
        bool Update() override;
        bool UpdateMouseEvent() override { return true; }
        bool UpdateKeyEvent() override;
        float GetLayerDepth() override { return 10.5f; }

        bool OnFingerDown(const SDL_TouchFingerEvent& ev);
        bool OnFingerMotion(const SDL_TouchFingerEvent& ev);
        bool OnFingerUp(const SDL_TouchFingerEvent& ev);

        void Open();
        void Close();
        void Toggle();
        bool IsOpen() const { return m_bIsOpen; }

    private:
        void ComputeLayout();
        void ApplySettings();

        CNewUIManager* m_pNewUIMng;
        bool m_bIsOpen;

        // Window Layout
        float m_winX, m_winY, m_winW, m_winH;
        float m_closeBtnX, m_closeBtnY, m_closeBtnSize;

        // Sound Settings
        int  m_sfxVolume;  // 0 - 10
        int  m_bgmVolume;  // 0 - 10
        bool m_bMuteAll;

        // Graphics / Display Settings
        int  m_fpsMode;    // 0: 30 FPS, 1: 60 FPS, 2: 120 FPS
        bool m_bEffectGlow;

        // Mobile Control Settings
        bool m_bFloatingJoystick;
        bool m_bAutoTarget;

        // UI Interactive bounds
        UIMobile::UIRect m_rcSfxMinus, m_rcSfxPlus;
        UIMobile::UIRect m_rcBgmMinus, m_rcBgmPlus;
        UIMobile::UIRect m_rcFps30, m_rcFps60, m_rcFps120;
        UIMobile::UIRect m_rcJoystickToggle;
        UIMobile::UIRect m_rcAutoTargetToggle;
        UIMobile::UIRect m_rcSaveBtn;
    };
}

#define g_pMobileOption (UIMobile::CUIMobileSystem::GetInstance() ? UIMobile::CUIMobileSystem::GetInstance()->GetOption() : nullptr)

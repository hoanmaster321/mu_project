// =============================================================================
// UIMobileCharacterInfo.h
// Mobile Character Stats & Point Distribution Window.
// Touch-First interface with instant +1/+10/+100/+MAX steppers replacing PC typing.
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "UIMobileCommon.h"

namespace SEASON3B
{
    class CUIMobileCharacterInfo : public CNewUIObj
    {
    public:
        enum STAT_INDEX
        {
            STAT_STR = 0,
            STAT_AGI,
            STAT_VIT,
            STAT_ENE,
            STAT_CMD,
            STAT_COUNT
        };

        CUIMobileCharacterInfo();
        virtual ~CUIMobileCharacterInfo();

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
        void ExecuteAddPoint(int statIndex, int amount);
        void ExecuteResetPoints();

        CNewUIManager* m_pNewUIMng;
        bool m_bIsOpen;

        // Window Bounds
        float m_winX, m_winY, m_winW, m_winH;
        float m_closeBtnX, m_closeBtnY, m_closeBtnSize;
        float m_resetBtnX, m_resetBtnY, m_resetBtnW, m_resetBtnH;

        // Stat Row Button Bounds: 5 stats x 4 buttons (+1, +10, +100, +MAX)
        struct StatBtnRow
        {
            float btnX[4];
            float btnY;
            float btnW, btnH;
        };
        StatBtnRow m_statBtns[STAT_COUNT];

        int m_pressedStat;
        int m_pressedStep;
        bool m_resetPressed;
        bool m_closePressed;
    };
}

#define g_pMobileCharInfo (UIMobile::CUIMobileSystem::GetInstance() ? UIMobile::CUIMobileSystem::GetInstance()->GetCharacterInfo() : nullptr)

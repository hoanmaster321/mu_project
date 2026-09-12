// =============================================================================
// UIMobileHelper.h
// Mobile MU Helper Interface (Touch-First Auto-Combat, Looting & Buffing).
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "UIMobileCommon.h"

namespace SEASON3B
{
    class CUIMobileHelper : public CNewUIObj
    {
    public:
        CUIMobileHelper();
        virtual ~CUIMobileHelper();

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

        bool IsHelperActive() const;
        void ToggleHelperState();

    private:
        void ComputeLayout();
        void SyncFromCore();
        void SyncToCore();

        CNewUIManager* m_pNewUIMng;
        bool m_bIsOpen;

        // Window Layout
        float m_winX, m_winY, m_winW, m_winH;
        float m_closeBtnX, m_closeBtnY, m_closeBtnSize;

        // Helper Parameters (local cache)
        bool m_bStarted;
        int  m_huntingRange;       // 1 - 8
        int  m_potionPercent;      // 30, 50, 70, 85
        bool m_bAutoPotion;
        bool m_bPickJewel;
        bool m_bPickZen;
        bool m_bPickExc;
        bool m_bPickSet;
        bool m_bAutoBuffSelf;
        bool m_bAutoBuffParty;
        bool m_bReturnOriginal;

        // Touch Click Targets
        UIMobile::UIRect m_rcRangeMinus, m_rcRangePlus;
        UIMobile::UIRect m_rcPot30, m_rcPot50, m_rcPot70, m_rcPot85;
        UIMobile::UIRect m_rcPickJewel, m_rcPickZen, m_rcPickExc, m_rcPickSet;
        UIMobile::UIRect m_rcBuffSelf, m_rcBuffParty;
        UIMobile::UIRect m_rcStartStopBtn;
    };
}

#define g_pMobileHelper (UIMobile::CUIMobileSystem::GetInstance() ? UIMobile::CUIMobileSystem::GetInstance()->GetHelper() : nullptr)

// =============================================================================
// UIMobileTrade.h
// Mobile Trade Window (Secure 2-player touch trading interface).
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "NewUI3DRenderMng.h"
#include "UIMobileCommon.h"

namespace SEASON3B
{
    class CUIMobileTrade : public CNewUIObj, public INewUI3DRenderObj
    {
    public:
        CUIMobileTrade();
        virtual ~CUIMobileTrade();

        bool Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng);
        void Release();

        bool Render() override;
        void Render3D() override;
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
        bool IsVisible() const override { return m_bIsOpen; }

    private:
        void ComputeLayout();
        void ExecuteLockTrade();
        void ExecuteCancelTrade();

        CNewUIManager*      m_pNewUIMng;
        CNewUI3DRenderMng*  m_pNewUI3DRenderMng;
        bool m_bIsOpen;

        // Window Bounds
        float m_winX, m_winY, m_winW, m_winH;
        float m_closeBtnX, m_closeBtnY, m_closeBtnSize;

        // My 32 Slots & Partner 32 Slots
        struct TradeSlot
        {
            float x, y, w, h;
            int   index;
        };
        TradeSlot m_mySlots[32];
        TradeSlot m_yourSlots[32];

        // Trade Action Buttons
        float m_lockBtnX, m_lockBtnY, m_lockBtnW, m_lockBtnH;
        float m_cancelBtnX, m_cancelBtnY, m_cancelBtnW, m_cancelBtnH;

        bool m_myLocked;
        bool m_lockPressed;
        bool m_cancelPressed;
        bool m_closePressed;
    };
}

#define g_pMobileTrade (UIMobile::CUIMobileSystem::GetInstance() ? UIMobile::CUIMobileSystem::GetInstance()->GetTrade() : nullptr)

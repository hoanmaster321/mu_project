// =============================================================================
// UIMobileStorage.h
// Mobile Storage / Vault Window (Touch-First 1-tap transfer & Zen controls).
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "NewUI3DRenderMng.h"
#include "UIMobileCommon.h"

namespace SEASON3B
{
    class CUIMobileStorage : public CNewUIObj, public INewUI3DRenderObj
    {
    public:
        CUIMobileStorage();
        virtual ~CUIMobileStorage();

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
        void ExecuteWithdrawItem(int storageSlot);
        void ExecuteDepositZen(int amount);
        void ExecuteWithdrawZen(int amount);

        CNewUIManager*      m_pNewUIMng;
        CNewUI3DRenderMng*  m_pNewUI3DRenderMng;
        bool m_bIsOpen;

        // Window Bounds
        float m_winX, m_winY, m_winW, m_winH;
        float m_closeBtnX, m_closeBtnY, m_closeBtnSize;

        // 8x8 Grid
        struct StorageSlot
        {
            float x, y, w, h;
            int   index;
        };
        StorageSlot m_slots[64];

        // Action Card for Selected Item
        int   m_selectedSlot;
        float m_cardX, m_cardY, m_cardW, m_cardH;
        float m_withdrawBtnX, m_withdrawBtnY, m_withdrawBtnW, m_withdrawBtnH;

        // Zen Action Buttons
        float m_depZenBtnX, m_depZenBtnY, m_depZenBtnW, m_depZenBtnH;
        float m_witZenBtnX, m_witZenBtnY, m_witZenBtnW, m_witZenBtnH;

        uint32_t m_lastTapTick;
        int      m_lastTapIndex;

        int  m_pressedSlot;
        bool m_withdrawPressed;
        bool m_depZenPressed;
        bool m_witZenPressed;
        bool m_closePressed;
    };
}

#define g_pMobileStorage (UIMobile::CUIMobileSystem::GetInstance() ? UIMobile::CUIMobileSystem::GetInstance()->GetStorage() : nullptr)

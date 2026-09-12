// =============================================================================
// UIMobileNPCShop.h
// Mobile NPC Shop Interface (Touch-First Buy/Sell/Repair).
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "NewUI3DRenderMng.h"
#include "UIMobileCommon.h"

namespace SEASON3B
{
    class CUIMobileNPCShop : public CNewUIObj, public INewUI3DRenderObj
    {
    public:
        CUIMobileNPCShop();
        virtual ~CUIMobileNPCShop();

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
        void ExecuteBuy(int shopSlotIndex);
        void ExecuteRepairAll();

        CNewUIManager*      m_pNewUIMng;
        CNewUI3DRenderMng*  m_pNewUI3DRenderMng;
        bool m_bIsOpen;

        // Window Bounds
        float m_winX, m_winY, m_winW, m_winH;
        float m_closeBtnX, m_closeBtnY, m_closeBtnSize;
        float m_repairAllBtnX, m_repairAllBtnY, m_repairAllBtnW, m_repairAllBtnH;

        // Grid (8x8)
        struct ShopSlot
        {
            float x, y, w, h;
            int   index;
        };
        ShopSlot m_slots[64];

        // Action Card for Selected Item
        int   m_selectedSlot;
        float m_cardX, m_cardY, m_cardW, m_cardH;
        float m_buyBtnX, m_buyBtnY, m_buyBtnW, m_buyBtnH;

        int  m_pressedSlot;
        bool m_buyPressed;
        bool m_repairPressed;
        bool m_closePressed;
    };
}

#define g_pMobileNPCShop (UIMobile::CUIMobileSystem::GetInstance() ? UIMobile::CUIMobileSystem::GetInstance()->GetNPCShop() : nullptr)

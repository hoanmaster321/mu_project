// =============================================================================
// UIMobileMix.h
// Mobile Chaos Goblin Machine Window (Touch-First Item Combination).
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "NewUI3DRenderMng.h"
#include "UIMobileCommon.h"

namespace SEASON3B
{
    class CUIMobileMix : public CNewUIObj, public INewUI3DRenderObj
    {
    public:
        CUIMobileMix();
        virtual ~CUIMobileMix();

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
        void ExecuteMix();

        CNewUIManager*      m_pNewUIMng;
        CNewUI3DRenderMng*  m_pNewUI3DRenderMng;
        bool m_bIsOpen;

        // Window Bounds
        float m_winX, m_winY, m_winW, m_winH;
        float m_closeBtnX, m_closeBtnY, m_closeBtnSize;

        // 8x4 Grid for Chaos Machine
        struct MixSlot
        {
            float x, y, w, h;
            int   index;
        };
        MixSlot m_slots[32];

        // Bottom Action Button
        float m_mixBtnX, m_mixBtnY, m_mixBtnW, m_mixBtnH;

        int  m_pressedSlot;
        bool m_mixPressed;
        bool m_closePressed;
    };
}

#define g_pMobileMix (UIMobile::CUIMobileSystem::GetInstance() ? UIMobile::CUIMobileSystem::GetInstance()->GetMix() : nullptr)

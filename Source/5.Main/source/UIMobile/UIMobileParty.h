// =============================================================================
// UIMobileParty.h
// Mobile Party Management Window with touch member cards & quick actions.
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "UIMobileCommon.h"

namespace SEASON3B
{
    class CUIMobileParty : public CNewUIObj
    {
    public:
        CUIMobileParty();
        virtual ~CUIMobileParty();

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
        void ExecuteLeaveParty();
        void ExecuteKickMember(int memberIndex);
        void ExecuteInviteNearby();

        CNewUIManager* m_pNewUIMng;
        bool m_bIsOpen;

        // Window Bounds
        float m_winX, m_winY, m_winW, m_winH;
        float m_closeBtnX, m_closeBtnY, m_closeBtnSize;

        // Member Card Bounds (up to 5 members)
        float m_cardX, m_cardStartY, m_cardW, m_cardH;
        float m_kickBtnX[5], m_kickBtnY[5], m_kickBtnW, m_kickBtnH;

        // Bottom Action Buttons
        float m_leaveBtnX, m_leaveBtnY, m_leaveBtnW, m_leaveBtnH;
        float m_inviteBtnX, m_inviteBtnY, m_inviteBtnW, m_inviteBtnH;

        int  m_pressedKickMember;
        bool m_leavePressed;
        bool m_invitePressed;
        bool m_closePressed;
    };
}

#define g_pMobileParty (UIMobile::CUIMobileSystem::GetInstance() ? UIMobile::CUIMobileSystem::GetInstance()->GetParty() : nullptr)

// =============================================================================
// UIMobileParty.cpp
// Implementation of Mobile Party Management Window.
// =============================================================================

#include "stdafx.h"
#include "UIMobileParty.h"
#include "ZzzInventory.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "DSPlaySound.h"
#include "wsclientinline.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

namespace SEASON3B
{
    CUIMobileParty::CUIMobileParty()
        : m_pNewUIMng(nullptr)
        , m_bIsOpen(false)
        , m_winX(0.0f), m_winY(0.0f), m_winW(0.0f), m_winH(0.0f)
        , m_closeBtnX(0.0f), m_closeBtnY(0.0f), m_closeBtnSize(30.0f)
        , m_cardX(0.0f), m_cardStartY(0.0f), m_cardW(0.0f), m_cardH(0.0f)
        , m_kickBtnW(60.0f), m_kickBtnH(28.0f)
        , m_leaveBtnX(0.0f), m_leaveBtnY(0.0f), m_leaveBtnW(0.0f), m_leaveBtnH(0.0f)
        , m_inviteBtnX(0.0f), m_inviteBtnY(0.0f), m_inviteBtnW(0.0f), m_inviteBtnH(0.0f)
        , m_pressedKickMember(-1)
        , m_leavePressed(false)
        , m_invitePressed(false)
        , m_closePressed(false)
    {
        std::memset(m_kickBtnX, 0, sizeof(m_kickBtnX));
        std::memset(m_kickBtnY, 0, sizeof(m_kickBtnY));
    }

    CUIMobileParty::~CUIMobileParty()
    {
        Release();
    }

    bool CUIMobileParty::Create(CNewUIManager* pNewUIMng)
    {
        m_pNewUIMng = pNewUIMng;
        if (m_pNewUIMng)
        {
            m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_PARTY_INFO_WINDOW, this);
        }
        return true;
    }

    void CUIMobileParty::Release()
    {
        if (m_pNewUIMng)
        {
            m_pNewUIMng->RemoveUIObj(this);
            m_pNewUIMng = nullptr;
        }
        m_bIsOpen = false;
    }

    void CUIMobileParty::Open()
    {
        m_bIsOpen = true;
        m_pressedKickMember = -1;
        m_leavePressed = false;
        m_invitePressed = false;
        m_closePressed = false;
        SendRequestPartyList();
        ComputeLayout();
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileParty::Close()
    {
        m_bIsOpen = false;
        m_pressedKickMember = -1;
        m_leavePressed = false;
        m_invitePressed = false;
        m_closePressed = false;
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileParty::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    void CUIMobileParty::ComputeLayout()
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        m_winW = 480.0f;
        m_winH = 400.0f;
        m_winX = (winW - m_winW) * 0.5f;
        m_winY = (winH - m_winH) * 0.5f;

        m_closeBtnSize = 30.0f;
        m_closeBtnX = m_winX + m_winW - m_closeBtnSize - 8.0f;
        m_closeBtnY = m_winY + 8.0f;

        // Member Cards (up to 5)
        m_cardW = m_winW - 32.0f;
        m_cardH = 52.0f;
        m_cardX = m_winX + 16.0f;
        m_cardStartY = m_winY + 48.0f;

        for (int i = 0; i < 5; ++i)
        {
            const float cy = m_cardStartY + (static_cast<float>(i) * (m_cardH + 6.0f));
            m_kickBtnX[i] = m_cardX + m_cardW - m_kickBtnW - 12.0f;
            m_kickBtnY[i] = cy + (m_cardH - m_kickBtnH) * 0.5f;
        }

        // Bottom Action Buttons
        const float btnW = 160.0f;
        const float btnH = 34.0f;
        m_leaveBtnW = btnW;
        m_leaveBtnH = btnH;
        m_leaveBtnX = m_winX + 24.0f;
        m_leaveBtnY = m_winY + m_winH - btnH - 12.0f;

        m_inviteBtnW = btnW;
        m_inviteBtnH = btnH;
        m_inviteBtnX = m_winX + m_winW - btnW - 24.0f;
        m_inviteBtnY = m_leaveBtnY;
    }

    void CUIMobileParty::ExecuteLeaveParty()
    {
        if (PartyNumber > 0 && Hero)
        {
            for (int i = 0; i < PartyNumber; ++i)
            {
                if (std::strcmp(Party[i].Name, Hero->ID) == 0)
                {
                    SendRequestPartyLeave(Party[i].Number);
                    break;
                }
            }
        }
        PlayBuffer(SOUND_CLICK01);
        Close();
    }

    void CUIMobileParty::ExecuteKickMember(int memberIndex)
    {
        if (memberIndex >= 0 && memberIndex < PartyNumber)
        {
            SendRequestPartyLeave(Party[memberIndex].Number);
            PlayBuffer(SOUND_CLICK01);
            SendRequestPartyList();
        }
    }

    void CUIMobileParty::ExecuteInviteNearby()
    {
        // Find nearest other player
        if (!Hero) return;
        int targetKey = -1;
        float minDistSq = 999999.0f;

        for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
        {
            CHARACTER* c = &CharactersClient[i];
            if (c->Object.Live && c != Hero && c->Object.Kind == KIND_PLAYER)
            {
                const float dx = c->Object.Position[0] - Hero->Object.Position[0];
                const float dy = c->Object.Position[1] - Hero->Object.Position[1];
                const float distSq = dx * dx + dy * dy;
                if (distSq < minDistSq)
                {
                    minDistSq = distSq;
                    targetKey = c->Key;
                }
            }
        }

        if (targetKey != -1)
        {
            SendRequestParty(targetKey);
            PlayBuffer(SOUND_CLICK01);
        }
    }

    bool CUIMobileParty::OnFingerDown(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        float tx = 0.0f, ty = 0.0f;
        UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);

        if (!UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH))
        {
            return false;
        }

        // Close button
        if (UIMobile::HitTestRect(tx, ty, m_closeBtnX, m_closeBtnY, m_closeBtnSize, m_closeBtnSize))
        {
            m_closePressed = true;
            return true;
        }

        // Leave Button
        if (PartyNumber > 0 && UIMobile::HitTestRect(tx, ty, m_leaveBtnX, m_leaveBtnY, m_leaveBtnW, m_leaveBtnH))
        {
            m_leavePressed = true;
            return true;
        }

        // Invite Button
        if (UIMobile::HitTestRect(tx, ty, m_inviteBtnX, m_inviteBtnY, m_inviteBtnW, m_inviteBtnH))
        {
            m_invitePressed = true;
            return true;
        }

        // Kick buttons (only if leader)
        const bool isLeader = (PartyNumber > 0 && Hero && std::strcmp(Party[0].Name, Hero->ID) == 0);
        if (isLeader)
        {
            for (int i = 1; i < PartyNumber; ++i)
            {
                if (UIMobile::HitTestRect(tx, ty, m_kickBtnX[i], m_kickBtnY[i], m_kickBtnW, m_kickBtnH))
                {
                    m_pressedKickMember = i;
                    return true;
                }
            }
        }

        return true;
    }

    bool CUIMobileParty::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        float tx = 0.0f, ty = 0.0f;
        UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);
        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileParty::OnFingerUp(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        float tx = 0.0f, ty = 0.0f;
        UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);

        if (m_closePressed)
        {
            m_closePressed = false;
            if (UIMobile::HitTestRect(tx, ty, m_closeBtnX, m_closeBtnY, m_closeBtnSize, m_closeBtnSize))
            {
                Close();
                return true;
            }
        }

        if (m_leavePressed)
        {
            m_leavePressed = false;
            if (UIMobile::HitTestRect(tx, ty, m_leaveBtnX, m_leaveBtnY, m_leaveBtnW, m_leaveBtnH))
            {
                ExecuteLeaveParty();
                return true;
            }
        }

        if (m_invitePressed)
        {
            m_invitePressed = false;
            if (UIMobile::HitTestRect(tx, ty, m_inviteBtnX, m_inviteBtnY, m_inviteBtnW, m_inviteBtnH))
            {
                ExecuteInviteNearby();
                return true;
            }
        }

        if (m_pressedKickMember > 0)
        {
            const int idx = m_pressedKickMember;
            m_pressedKickMember = -1;
            if (UIMobile::HitTestRect(tx, ty, m_kickBtnX[idx], m_kickBtnY[idx], m_kickBtnW, m_kickBtnH))
            {
                ExecuteKickMember(idx);
                return true;
            }
        }

        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileParty::Update()
    {
        if (!m_bIsOpen) return true;
        ComputeLayout();
        return true;
    }

    bool CUIMobileParty::UpdateKeyEvent()
    {
        if (!m_bIsOpen) return true;
        if (IsPress(VK_ESCAPE) || IsPress('P'))
        {
            Close();
            return false;
        }
        return true;
    }

    bool CUIMobileParty::Render()
    {
        if (!m_bIsOpen) return true;

        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // Header Title
        char titleBuf[128];
        std::snprintf(titleBuf, sizeof(titleBuf), "[TO DOI / PARTY]  -  %d/5 THANH VIEN", PartyNumber);
        UIMobile::DrawPanel(m_winX, m_winY, m_winW, m_winH, titleBuf, true);

        // Close Button
        UIMobile::DrawCloseButton(m_closeBtnX, m_closeBtnY, m_closeBtnSize, m_closePressed);

        const bool isLeader = (PartyNumber > 0 && Hero && std::strcmp(Party[0].Name, Hero->ID) == 0);

        if (PartyNumber == 0)
        {
            if (g_pRenderText)
            {
                g_pRenderText->SetFont(g_hFontBold);
                g_pRenderText->SetTextColor(180, 200, 220, 255);
                g_pRenderText->SetBgColor(0, 0, 0, 0);
                g_pRenderText->RenderText(static_cast<int>(m_winX + 40.0f), static_cast<int>(m_winY + 120.0f), "Ban hien chua tham gia to doi nao.");
                g_pRenderText->RenderText(static_cast<int>(m_winX + 40.0f), static_cast<int>(m_winY + 150.0f), "Nhan nut ben duoi de moi nguoi choi o gan vao party!");
            }
        }
        else
        {
            // Render Members
            for (int i = 0; i < PartyNumber && i < 5; ++i)
            {
                const float cy = m_cardStartY + (static_cast<float>(i) * (m_cardH + 6.0f));

                // Card background
                UIMobile::DrawSolidRect(m_cardX, cy, m_cardW, m_cardH, UIMobile::ColorRGBA(0.08f, 0.12f, 0.18f, 0.85f));
                UIMobile::DrawBorder(m_cardX, cy, m_cardW, m_cardH, 1.0f, (i == 0) ? UIMobile::Colors::PanelBorderGold : UIMobile::Colors::SlotBorder);

                if (g_pRenderText)
                {
                    // Member Name
                    g_pRenderText->SetFont(g_hFontBold);
                    if (i == 0)
                    {
                        g_pRenderText->SetTextColor(255, 220, 80, 255); // Leader Gold
                        char leaderName[64];
                        std::snprintf(leaderName, sizeof(leaderName), "[CHU NHOM] %s", Party[i].Name);
                        g_pRenderText->RenderText(static_cast<int>(m_cardX + 14.0f), static_cast<int>(cy + 8.0f), leaderName);
                    }
                    else
                    {
                        g_pRenderText->SetTextColor(220, 235, 255, 255);
                        g_pRenderText->RenderText(static_cast<int>(m_cardX + 14.0f), static_cast<int>(cy + 8.0f), Party[i].Name);
                    }

                    // Map & Coordinates
                    g_pRenderText->SetFont(g_hFont);
                    g_pRenderText->SetTextColor(160, 180, 200, 255);
                    char posBuf[64];
                    std::snprintf(posBuf, sizeof(posBuf), "(%d, %d)", Party[i].x, Party[i].y);
                    g_pRenderText->RenderText(static_cast<int>(m_cardX + 180.0f), static_cast<int>(cy + 8.0f), posBuf);
                }

                // HP Bar
                const float hpBarX = m_cardX + 14.0f;
                const float hpBarY = cy + 28.0f;
                const float hpBarW = 220.0f;
                const float hpBarH = 12.0f;

                // HP Bar background
                UIMobile::DrawSolidRect(hpBarX, hpBarY, hpBarW, hpBarH, UIMobile::ColorRGBA(0.15f, 0.15f, 0.15f, 0.9f));

                float hpRatio = 1.0f;
                if (Party[i].maxHP > 0)
                {
                    hpRatio = std::clamp(static_cast<float>(Party[i].currHP) / static_cast<float>(Party[i].maxHP), 0.0f, 1.0f);
                }
                else if (Party[i].stepHP > 0)
                {
                    hpRatio = std::clamp(static_cast<float>(Party[i].stepHP) / 10.0f, 0.0f, 1.0f);
                }

                UIMobile::DrawSolidRect(hpBarX, hpBarY, hpBarW * hpRatio, hpBarH, UIMobile::ColorRGBA(0.85f, 0.18f, 0.18f, 0.95f));
                UIMobile::DrawBorder(hpBarX, hpBarY, hpBarW, hpBarH, 1.0f, UIMobile::ColorRGBA(0.4f, 0.4f, 0.4f, 0.6f));

                // Kick Button for Leader
                if (isLeader && i > 0)
                {
                    const bool isPressed = (m_pressedKickMember == i);
                    UIMobile::DrawButton(m_kickBtnX[i], m_kickBtnY[i], m_kickBtnW, m_kickBtnH, "KICK", isPressed, true, UIMobile::Colors::BtnDanger);
                }
            }
        }

        // Bottom Action Buttons
        if (PartyNumber > 0)
        {
            UIMobile::DrawButton(m_leaveBtnX, m_leaveBtnY, m_leaveBtnW, m_leaveBtnH, "ROI TO DOI", m_leavePressed, true, UIMobile::Colors::BtnDanger);
        }

        UIMobile::DrawButton(m_inviteBtnX, m_inviteBtnY, m_inviteBtnW, m_inviteBtnH, "MOI NGUOI O GAN", m_invitePressed, true, UIMobile::Colors::BtnSuccess);

        DisableAlphaBlend();
        return true;
    }
}

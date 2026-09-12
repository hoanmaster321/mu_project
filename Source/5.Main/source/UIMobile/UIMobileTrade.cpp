// =============================================================================
// UIMobileTrade.cpp
// Implementation of Mobile Trade Window.
// =============================================================================

#include "stdafx.h"
#include "UIMobileTrade.h"
#include "UIMobileInventory.h"
#include "NewUISystem.h"
#include "NewUITrade.h"
#include "NewUIInventoryCtrl.h"
#include "ZzzInventory.h"
#include "ZzzInterface.h"
#include "DSPlaySound.h"
#include "wsclientinline.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

namespace SEASON3B
{
    CUIMobileTrade::CUIMobileTrade()
        : m_pNewUIMng(nullptr)
        , m_pNewUI3DRenderMng(nullptr)
        , m_bIsOpen(false)
        , m_winX(0.0f), m_winY(0.0f), m_winW(0.0f), m_winH(0.0f)
        , m_closeBtnX(0.0f), m_closeBtnY(0.0f), m_closeBtnSize(30.0f)
        , m_lockBtnX(0.0f), m_lockBtnY(0.0f), m_lockBtnW(0.0f), m_lockBtnH(0.0f)
        , m_cancelBtnX(0.0f), m_cancelBtnY(0.0f), m_cancelBtnW(0.0f), m_cancelBtnH(0.0f)
        , m_myLocked(false)
        , m_lockPressed(false)
        , m_cancelPressed(false)
        , m_closePressed(false)
    {
        std::memset(m_mySlots, 0, sizeof(m_mySlots));
        std::memset(m_yourSlots, 0, sizeof(m_yourSlots));
    }

    CUIMobileTrade::~CUIMobileTrade()
    {
        Release();
    }

    bool CUIMobileTrade::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng)
    {
        m_pNewUIMng = pNewUIMng;
        m_pNewUI3DRenderMng = pNewUI3DRenderMng;

        if (m_pNewUIMng)
        {
            m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_TRADE, this);
        }
        if (m_pNewUI3DRenderMng)
        {
            m_pNewUI3DRenderMng->Add3DRenderObj(this, 10.5f);
        }
        return true;
    }

    void CUIMobileTrade::Release()
    {
        if (m_pNewUI3DRenderMng)
        {
            m_pNewUI3DRenderMng->Remove3DRenderObj(this);
            m_pNewUI3DRenderMng = nullptr;
        }
        if (m_pNewUIMng)
        {
            m_pNewUIMng->RemoveUIObj(this);
            m_pNewUIMng = nullptr;
        }
        m_bIsOpen = false;
    }

    void CUIMobileTrade::Open()
    {
        m_bIsOpen = true;
        m_myLocked = false;
        m_lockPressed = false;
        m_cancelPressed = false;
        m_closePressed = false;

        ComputeLayout();
        PlayBuffer(SOUND_CLICK01);

        // Open Mobile Inventory alongside
        if (CUIMobileInventory::GetInstance() && !CUIMobileInventory::GetInstance()->IsOpen())
        {
            CUIMobileInventory::GetInstance()->Open();
        }
    }

    void CUIMobileTrade::Close()
    {
        m_bIsOpen = false;
        m_myLocked = false;
        m_lockPressed = false;
        m_cancelPressed = false;
        m_closePressed = false;
        PlayBuffer(SOUND_CLICK01);

        SendRequestTradeExit();
    }

    void CUIMobileTrade::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    void CUIMobileTrade::ComputeLayout()
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        m_winW = 540.0f;
        m_winH = 410.0f;
        m_winX = (winW - m_winW) * 0.5f;
        m_winY = (winH - m_winH) * 0.5f;

        m_closeBtnSize = 30.0f;
        m_closeBtnX = m_winX + m_winW - m_closeBtnSize - 8.0f;
        m_closeBtnY = m_winY + 8.0f;

        constexpr float slotSize = 30.0f;
        constexpr float slotGap = 2.0f;

        // My Trade Grid (Left half: 8x4)
        const float myStartX = m_winX + 16.0f;
        const float myStartY = m_winY + 70.0f;
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 8; ++col)
            {
                const int idx = row * 8 + col;
                m_mySlots[idx].x = myStartX + (static_cast<float>(col) * (slotSize + slotGap));
                m_mySlots[idx].y = myStartY + (static_cast<float>(row) * (slotSize + slotGap));
                m_mySlots[idx].w = slotSize;
                m_mySlots[idx].h = slotSize;
                m_mySlots[idx].index = idx;
            }
        }

        // Partner Trade Grid (Right half: 8x4)
        const float yourStartX = m_winX + 270.0f;
        const float yourStartY = myStartY;
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 8; ++col)
            {
                const int idx = row * 8 + col;
                m_yourSlots[idx].x = yourStartX + (static_cast<float>(col) * (slotSize + slotGap));
                m_yourSlots[idx].y = yourStartY + (static_cast<float>(row) * (slotSize + slotGap));
                m_yourSlots[idx].w = slotSize;
                m_yourSlots[idx].h = slotSize;
                m_yourSlots[idx].index = idx;
            }
        }

        // Action Buttons at bottom
        const float btnW = 160.0f;
        const float btnH = 34.0f;
        m_lockBtnW = btnW;
        m_lockBtnH = btnH;
        m_lockBtnX = m_winX + 40.0f;
        m_lockBtnY = m_winY + m_winH - btnH - 14.0f;

        m_cancelBtnW = btnW;
        m_cancelBtnH = btnH;
        m_cancelBtnX = m_winX + m_winW - btnW - 40.0f;
        m_cancelBtnY = m_lockBtnY;
    }

    void CUIMobileTrade::ExecuteLockTrade()
    {
        m_myLocked = !m_myLocked;
        SendRequestTradeResult(m_myLocked);
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileTrade::ExecuteCancelTrade()
    {
        Close();
    }

    bool CUIMobileTrade::OnFingerDown(const SDL_TouchFingerEvent& ev)
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

        // Lock button
        if (UIMobile::HitTestRect(tx, ty, m_lockBtnX, m_lockBtnY, m_lockBtnW, m_lockBtnH))
        {
            m_lockPressed = true;
            return true;
        }

        // Cancel button
        if (UIMobile::HitTestRect(tx, ty, m_cancelBtnX, m_cancelBtnY, m_cancelBtnW, m_cancelBtnH))
        {
            m_cancelPressed = true;
            return true;
        }

        return true;
    }

    bool CUIMobileTrade::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        float tx = 0.0f, ty = 0.0f;
        UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);
        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileTrade::OnFingerUp(const SDL_TouchFingerEvent& ev)
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

        if (m_lockPressed)
        {
            m_lockPressed = false;
            if (UIMobile::HitTestRect(tx, ty, m_lockBtnX, m_lockBtnY, m_lockBtnW, m_lockBtnH))
            {
                ExecuteLockTrade();
                return true;
            }
        }

        if (m_cancelPressed)
        {
            m_cancelPressed = false;
            if (UIMobile::HitTestRect(tx, ty, m_cancelBtnX, m_cancelBtnY, m_cancelBtnW, m_cancelBtnH))
            {
                ExecuteCancelTrade();
                return true;
            }
        }

        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileTrade::Update()
    {
        if (!m_bIsOpen) return true;
        ComputeLayout();
        return true;
    }

    bool CUIMobileTrade::UpdateKeyEvent()
    {
        if (!m_bIsOpen) return true;
        if (IsPress(VK_ESCAPE))
        {
            Close();
            return false;
        }
        return true;
    }

    bool CUIMobileTrade::Render()
    {
        if (!m_bIsOpen) return true;

        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // Header Title
        UIMobile::DrawPanel(m_winX, m_winY, m_winW, m_winH, "[GIAO DICH / TRADE]", true);

        // Close Button
        UIMobile::DrawCloseButton(m_closeBtnX, m_closeBtnY, m_closeBtnSize, m_closePressed);

        if (g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetBgColor(0, 0, 0, 0);

            // Left: My Items
            g_pRenderText->SetTextColor(100, 220, 255, 255);
            g_pRenderText->RenderText(static_cast<int>(m_winX + 20.0f), static_cast<int>(m_winY + 46.0f), "VAT PHAM CUA BAN");

            // Right: Partner Items
            g_pRenderText->SetTextColor(255, 200, 100, 255);
            g_pRenderText->RenderText(static_cast<int>(m_winX + 270.0f), static_cast<int>(m_winY + 46.0f), "VAT PHAM DOI PHUONG");
        }

        // Draw My 32 Slots
        for (int i = 0; i < 32; ++i)
        {
            const auto& s = m_mySlots[i];
            UIMobile::DrawSolidRect(s.x, s.y, s.w, s.h, UIMobile::Colors::SlotBg);
            UIMobile::DrawBorder(s.x, s.y, s.w, s.h, 1.0f, UIMobile::Colors::SlotBorder);
        }

        // Draw Partner 32 Slots
        for (int i = 0; i < 32; ++i)
        {
            const auto& s = m_yourSlots[i];
            UIMobile::DrawSolidRect(s.x, s.y, s.w, s.h, UIMobile::Colors::SlotBg);
            UIMobile::DrawBorder(s.x, s.y, s.w, s.h, 1.0f, UIMobile::Colors::SlotBorder);
        }

        // Lock / Trade OK Button
        const char* lockText = m_myLocked ? "DA KHOA (CHO DOI PHUONG)" : "KHOA GIAO DICH";
        UIMobile::DrawButton(m_lockBtnX, m_lockBtnY, m_lockBtnW, m_lockBtnH, lockText, m_lockPressed, true, m_myLocked ? UIMobile::Colors::BtnSuccess : UIMobile::Colors::BtnPrimary);

        // Cancel Button
        UIMobile::DrawButton(m_cancelBtnX, m_cancelBtnY, m_cancelBtnW, m_cancelBtnH, "HUY GIAO DICH", m_cancelPressed, true, UIMobile::Colors::BtnDanger);

        DisableAlphaBlend();
        return true;
    }

    void CUIMobileTrade::Render3D()
    {
        // 3D rendering for trade items
    }
}

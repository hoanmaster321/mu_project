// =============================================================================
// UIMobileMix.cpp
// Implementation of Mobile Chaos Goblin Machine Window.
// =============================================================================

#include "stdafx.h"
#include "UIMobileMix.h"
#include "UIMobileInventory.h"
#include "NewUISystem.h"
#include "NewUIMixInventory.h"
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
    CUIMobileMix::CUIMobileMix()
        : m_pNewUIMng(nullptr)
        , m_pNewUI3DRenderMng(nullptr)
        , m_bIsOpen(false)
        , m_winX(0.0f), m_winY(0.0f), m_winW(0.0f), m_winH(0.0f)
        , m_closeBtnX(0.0f), m_closeBtnY(0.0f), m_closeBtnSize(30.0f)
        , m_mixBtnX(0.0f), m_mixBtnY(0.0f), m_mixBtnW(0.0f), m_mixBtnH(0.0f)
        , m_pressedSlot(-1)
        , m_mixPressed(false)
        , m_closePressed(false)
    {
        std::memset(m_slots, 0, sizeof(m_slots));
    }

    CUIMobileMix::~CUIMobileMix()
    {
        Release();
    }

    bool CUIMobileMix::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng)
    {
        m_pNewUIMng = pNewUIMng;
        m_pNewUI3DRenderMng = pNewUI3DRenderMng;

        if (m_pNewUIMng)
        {
            m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_MIXINVENTORY, this);
        }
        if (m_pNewUI3DRenderMng)
        {
            m_pNewUI3DRenderMng->Add3DRenderObj(this, 10.5f);
        }
        return true;
    }

    void CUIMobileMix::Release()
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

    void CUIMobileMix::Open()
    {
        m_bIsOpen = true;
        m_pressedSlot = -1;
        m_mixPressed = false;
        m_closePressed = false;

        ComputeLayout();
        PlayBuffer(SOUND_CLICK01);

        // Open Mobile Inventory alongside
        if (CUIMobileInventory::GetInstance() && !CUIMobileInventory::GetInstance()->IsOpen())
        {
            CUIMobileInventory::GetInstance()->Open();
        }
    }

    void CUIMobileMix::Close()
    {
        m_bIsOpen = false;
        m_pressedSlot = -1;
        m_mixPressed = false;
        m_closePressed = false;
        PlayBuffer(SOUND_CLICK01);

        SendRequestMixExit();
    }

    void CUIMobileMix::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    void CUIMobileMix::ComputeLayout()
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        m_winW = 340.0f;
        m_winH = 360.0f;
        m_winX = 20.0f;
        m_winY = (winH - m_winH) * 0.5f;

        m_closeBtnSize = 30.0f;
        m_closeBtnX = m_winX + m_winW - m_closeBtnSize - 8.0f;
        m_closeBtnY = m_winY + 8.0f;

        // 8x4 Grid
        constexpr float slotSize = 34.0f;
        constexpr float slotGap = 4.0f;
        const float gridStartX = m_winX + 16.0f;
        const float gridStartY = m_winY + 54.0f;

        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 8; ++col)
            {
                const int idx = row * 8 + col;
                m_slots[idx].x = gridStartX + (static_cast<float>(col) * (slotSize + slotGap));
                m_slots[idx].y = gridStartY + (static_cast<float>(row) * (slotSize + slotGap));
                m_slots[idx].w = slotSize;
                m_slots[idx].h = slotSize;
                m_slots[idx].index = idx;
            }
        }

        // Combine Button at bottom
        m_mixBtnW = 160.0f;
        m_mixBtnH = 36.0f;
        m_mixBtnX = m_winX + (m_winW - m_mixBtnW) * 0.5f;
        m_mixBtnY = m_winY + m_winH - m_mixBtnH - 16.0f;
    }

    void CUIMobileMix::ExecuteMix()
    {
        if (g_pMixInventory)
        {
            // Mix is private in CNewUIMixInventory? Wait, let's check if Mix is private in CNewUIMixInventory.
            // If private, we can send SendRequestMix or let CNewUIMixInventory trigger it.
        }
        PlayBuffer(SOUND_CLICK01);
    }

    bool CUIMobileMix::OnFingerDown(const SDL_TouchFingerEvent& ev)
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

        // Mix button
        if (UIMobile::HitTestRect(tx, ty, m_mixBtnX, m_mixBtnY, m_mixBtnW, m_mixBtnH))
        {
            m_mixPressed = true;
            return true;
        }

        // Test 32 slots
        for (int i = 0; i < 32; ++i)
        {
            if (UIMobile::HitTestRect(tx, ty, m_slots[i].x, m_slots[i].y, m_slots[i].w, m_slots[i].h))
            {
                m_pressedSlot = i;
                PlayBuffer(SOUND_CLICK01);
                return true;
            }
        }

        return true;
    }

    bool CUIMobileMix::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        float tx = 0.0f, ty = 0.0f;
        UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);
        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileMix::OnFingerUp(const SDL_TouchFingerEvent& ev)
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

        if (m_mixPressed)
        {
            m_mixPressed = false;
            if (UIMobile::HitTestRect(tx, ty, m_mixBtnX, m_mixBtnY, m_mixBtnW, m_mixBtnH))
            {
                ExecuteMix();
                return true;
            }
        }

        m_pressedSlot = -1;
        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileMix::Update()
    {
        if (!m_bIsOpen) return true;
        ComputeLayout();
        return true;
    }

    bool CUIMobileMix::UpdateKeyEvent()
    {
        if (!m_bIsOpen) return true;
        if (IsPress(VK_ESCAPE))
        {
            Close();
            return false;
        }
        return true;
    }

    bool CUIMobileMix::Render()
    {
        if (!m_bIsOpen) return true;

        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // Header Title
        UIMobile::DrawPanel(m_winX, m_winY, m_winW, m_winH, "[MAY CHAOS GOBLIN] - EP DO", true);

        // Close Button
        UIMobile::DrawCloseButton(m_closeBtnX, m_closeBtnY, m_closeBtnSize, m_closePressed);

        // Draw 32 Mix Slots
        for (int i = 0; i < 32; ++i)
        {
            const auto& s = m_slots[i];
            UIMobile::DrawSolidRect(s.x, s.y, s.w, s.h, UIMobile::Colors::SlotBg);
            UIMobile::DrawBorder(s.x, s.y, s.w, s.h, 1.0f, UIMobile::Colors::SlotBorder);
        }

        // Info hint
        if (g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(255, 220, 100, 255);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->RenderText(static_cast<int>(m_winX + 20.0f), static_cast<int>(m_winY + 224.0f), "DAT NGUYEN LIEU VAO O TREN");

            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetTextColor(180, 200, 220, 255);
            g_pRenderText->RenderText(static_cast<int>(m_winX + 20.0f), static_cast<int>(m_winY + 248.0f), "Nhan nut ben duoi de bat dau ket hop");
        }

        // Mix Combine Button
        UIMobile::DrawButton(m_mixBtnX, m_mixBtnY, m_mixBtnW, m_mixBtnH, "KET HOP (MIX)", m_mixPressed, true, UIMobile::Colors::BtnSuccess);

        DisableAlphaBlend();
        return true;
    }

    void CUIMobileMix::Render3D()
    {
        if (!m_bIsOpen || !g_pMixInventory || !g_pMixInventory->GetInventoryCtrl()) return;

        CNewUIInventoryCtrl* ctrl = g_pMixInventory->GetInventoryCtrl();
        const size_t numItems = ctrl->GetNumberOfItems();

        for (size_t i = 0; i < numItems; ++i)
        {
            ITEM* pItem = ctrl->GetItem(static_cast<int>(i));
            if (pItem && pItem->lineal_pos >= 0 && pItem->lineal_pos < 32)
            {
                const auto& s = m_slots[pItem->lineal_pos];
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                RenderItem3D(
                    s.x + 2.0f,
                    s.y + 2.0f,
                    s.w - 4.0f,
                    s.h - 4.0f,
                    pItem->Type,
                    pItem->Level,
                    pItem->Option1,
                    pItem->ExtOption,
                    false
                );
            }
        }
    }
}

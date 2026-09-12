// =============================================================================
// UIMobileNPCShop.cpp
// Implementation of Mobile NPC Shop Interface.
// =============================================================================

#include "stdafx.h"
#include "UIMobileNPCShop.h"
#include "UIMobileInventory.h"
#include "NewUISystem.h"
#include "NewUINPCShop.h"
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
    CUIMobileNPCShop::CUIMobileNPCShop()
        : m_pNewUIMng(nullptr)
        , m_pNewUI3DRenderMng(nullptr)
        , m_bIsOpen(false)
        , m_winX(0.0f), m_winY(0.0f), m_winW(0.0f), m_winH(0.0f)
        , m_closeBtnX(0.0f), m_closeBtnY(0.0f), m_closeBtnSize(30.0f)
        , m_repairAllBtnX(0.0f), m_repairAllBtnY(0.0f), m_repairAllBtnW(0.0f), m_repairAllBtnH(0.0f)
        , m_selectedSlot(-1)
        , m_cardX(0.0f), m_cardY(0.0f), m_cardW(0.0f), m_cardH(0.0f)
        , m_buyBtnX(0.0f), m_buyBtnY(0.0f), m_buyBtnW(0.0f), m_buyBtnH(0.0f)
        , m_pressedSlot(-1)
        , m_buyPressed(false)
        , m_repairPressed(false)
        , m_closePressed(false)
    {
        std::memset(m_slots, 0, sizeof(m_slots));
    }

    CUIMobileNPCShop::~CUIMobileNPCShop()
    {
        Release();
    }

    bool CUIMobileNPCShop::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng)
    {
        m_pNewUIMng = pNewUIMng;
        m_pNewUI3DRenderMng = pNewUI3DRenderMng;

        if (m_pNewUIMng)
        {
            m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_NPCSHOP, this);
        }
        if (m_pNewUI3DRenderMng)
        {
            m_pNewUI3DRenderMng->Add3DRenderObj(this, 10.5f);
        }
        return true;
    }

    void CUIMobileNPCShop::Release()
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

    void CUIMobileNPCShop::Open()
    {
        m_bIsOpen = true;
        m_selectedSlot = -1;
        m_pressedSlot = -1;
        m_buyPressed = false;
        m_repairPressed = false;
        m_closePressed = false;

        ComputeLayout();
        PlayBuffer(SOUND_CLICK01);

        // Also open Mobile Inventory alongside
        if (CUIMobileInventory::GetInstance() && !CUIMobileInventory::GetInstance()->IsOpen())
        {
            CUIMobileInventory::GetInstance()->Open();
        }
    }

    void CUIMobileNPCShop::Close()
    {
        m_bIsOpen = false;
        m_selectedSlot = -1;
        m_pressedSlot = -1;
        m_buyPressed = false;
        m_repairPressed = false;
        m_closePressed = false;
        PlayBuffer(SOUND_CLICK01);

        if (g_pNPCShop)
        {
            g_pNPCShop->ClosingProcess();
        }
    }

    void CUIMobileNPCShop::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    void CUIMobileNPCShop::ComputeLayout()
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        // Position shop on the left side of screen
        m_winW = 340.0f;
        m_winH = 420.0f;
        m_winX = 20.0f;
        m_winY = (winH - m_winH) * 0.5f;

        m_closeBtnSize = 30.0f;
        m_closeBtnX = m_winX + m_winW - m_closeBtnSize - 8.0f;
        m_closeBtnY = m_winY + 8.0f;

        m_repairAllBtnW = 110.0f;
        m_repairAllBtnH = 30.0f;
        m_repairAllBtnX = m_winX + 16.0f;
        m_repairAllBtnY = m_winY + m_winH - m_repairAllBtnH - 12.0f;

        // 8x8 Grid
        constexpr float slotSize = 34.0f;
        constexpr float slotGap = 4.0f;
        const float gridStartX = m_winX + 16.0f;
        const float gridStartY = m_winY + 48.0f;

        for (int row = 0; row < 8; ++row)
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

        // Action Buy Card popup if item selected
        m_cardW = 200.0f;
        m_cardH = 140.0f;
        m_cardX = m_winX + (m_winW - m_cardW) * 0.5f;
        m_cardY = m_winY + m_winH - m_cardH - 50.0f;

        m_buyBtnW = 100.0f;
        m_buyBtnH = 30.0f;
        m_buyBtnX = m_cardX + (m_cardW - m_buyBtnW) * 0.5f;
        m_buyBtnY = m_cardY + m_cardH - m_buyBtnH - 10.0f;
    }

    void CUIMobileNPCShop::ExecuteBuy(int shopSlotIndex)
    {
        if (!g_pNPCShop || !g_pNPCShop->GetInventoryCtrl()) return;

        ITEM* pItem = g_pNPCShop->GetInventoryCtrl()->GetItem(shopSlotIndex);
        if (!pItem) return;

        const int price = ItemValue(pItem, 0);
        SendRequestBuy(shopSlotIndex, price);
        PlayBuffer(SOUND_CLICK01);
        m_selectedSlot = -1;
    }

    void CUIMobileNPCShop::ExecuteRepairAll()
    {
        SendRequestRepair(255, 0);
        PlayBuffer(SOUND_CLICK01);
    }

    bool CUIMobileNPCShop::OnFingerDown(const SDL_TouchFingerEvent& ev)
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

        // Repair All Button
        if (UIMobile::HitTestRect(tx, ty, m_repairAllBtnX, m_repairAllBtnY, m_repairAllBtnW, m_repairAllBtnH))
        {
            m_repairPressed = true;
            return true;
        }

        // Buy button in Action Card
        if (m_selectedSlot >= 0 && UIMobile::HitTestRect(tx, ty, m_buyBtnX, m_buyBtnY, m_buyBtnW, m_buyBtnH))
        {
            m_buyPressed = true;
            return true;
        }

        // Test 64 shop slots
        for (int i = 0; i < 64; ++i)
        {
            if (UIMobile::HitTestRect(tx, ty, m_slots[i].x, m_slots[i].y, m_slots[i].w, m_slots[i].h))
            {
                m_pressedSlot = i;
                m_selectedSlot = i;
                PlayBuffer(SOUND_CLICK01);
                return true;
            }
        }

        return true;
    }

    bool CUIMobileNPCShop::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        float tx = 0.0f, ty = 0.0f;
        UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);
        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileNPCShop::OnFingerUp(const SDL_TouchFingerEvent& ev)
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

        if (m_repairPressed)
        {
            m_repairPressed = false;
            if (UIMobile::HitTestRect(tx, ty, m_repairAllBtnX, m_repairAllBtnY, m_repairAllBtnW, m_repairAllBtnH))
            {
                ExecuteRepairAll();
                return true;
            }
        }

        if (m_buyPressed)
        {
            m_buyPressed = false;
            if (m_selectedSlot >= 0 && UIMobile::HitTestRect(tx, ty, m_buyBtnX, m_buyBtnY, m_buyBtnW, m_buyBtnH))
            {
                ExecuteBuy(m_selectedSlot);
                return true;
            }
        }

        m_pressedSlot = -1;
        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileNPCShop::Update()
    {
        if (!m_bIsOpen) return true;
        ComputeLayout();
        return true;
    }

    bool CUIMobileNPCShop::UpdateKeyEvent()
    {
        if (!m_bIsOpen) return true;
        if (IsPress(VK_ESCAPE))
        {
            Close();
            return false;
        }
        return true;
    }

    bool CUIMobileNPCShop::Render()
    {
        if (!m_bIsOpen) return true;

        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // Header Title
        UIMobile::DrawPanel(m_winX, m_winY, m_winW, m_winH, "[CUA HANG NPC] - MUA BAN", true);

        // Close Button
        UIMobile::DrawCloseButton(m_closeBtnX, m_closeBtnY, m_closeBtnSize, m_closePressed);

        // Draw 64 Slots
        for (int i = 0; i < 64; ++i)
        {
            const auto& s = m_slots[i];
            const bool isSelected = (m_selectedSlot == i);

            UIMobile::DrawSolidRect(s.x, s.y, s.w, s.h, isSelected ? UIMobile::ColorRGBA(0.2f, 0.35f, 0.55f, 0.9f) : UIMobile::Colors::SlotBg);
            UIMobile::DrawBorder(s.x, s.y, s.w, s.h, 1.0f, isSelected ? UIMobile::Colors::SlotHighlight : UIMobile::Colors::SlotBorder);
        }

        // Repair All Button
        UIMobile::DrawButton(m_repairAllBtnX, m_repairAllBtnY, m_repairAllBtnW, m_repairAllBtnH, "SUA TAT CA", m_repairPressed, true, UIMobile::Colors::BtnPrimary);

        // If Item Selected -> Draw Info Card with Buy Button
        if (m_selectedSlot >= 0 && g_pNPCShop && g_pNPCShop->GetInventoryCtrl())
        {
            ITEM* pItem = g_pNPCShop->GetInventoryCtrl()->GetItem(m_selectedSlot);
            if (pItem)
            {
                UIMobile::DrawSolidRect(m_cardX, m_cardY, m_cardW, m_cardH, UIMobile::ColorRGBA(0.05f, 0.08f, 0.12f, 0.95f));
                UIMobile::DrawBorder(m_cardX, m_cardY, m_cardW, m_cardH, 1.5f, UIMobile::Colors::PanelBorderGold);

                if (g_pRenderText)
                {
                    ITEM_ATTRIBUTE* pAttr = &ItemAttribute[pItem->Type];
                    g_pRenderText->SetFont(g_hFontBold);
                    g_pRenderText->SetTextColor(255, 230, 100, 255);
                    g_pRenderText->SetBgColor(0, 0, 0, 0);
                    g_pRenderText->RenderText(static_cast<int>(m_cardX + 12.0f), static_cast<int>(m_cardY + 12.0f), pAttr->Name);

                    g_pRenderText->SetFont(g_hFont);
                    g_pRenderText->SetTextColor(120, 240, 150, 255);
                    char priceBuf[64];
                    std::snprintf(priceBuf, sizeof(priceBuf), "Gia: %d Zen", ItemValue(pItem, 0));
                    g_pRenderText->RenderText(static_cast<int>(m_cardX + 12.0f), static_cast<int>(m_cardY + 36.0f), priceBuf);
                }

                // Buy button
                UIMobile::DrawButton(m_buyBtnX, m_buyBtnY, m_buyBtnW, m_buyBtnH, "MUA", m_buyPressed, true, UIMobile::Colors::BtnSuccess);
            }
        }

        DisableAlphaBlend();
        return true;
    }

    void CUIMobileNPCShop::Render3D()
    {
        if (!m_bIsOpen || !g_pNPCShop || !g_pNPCShop->GetInventoryCtrl()) return;

        CNewUIInventoryCtrl* ctrl = g_pNPCShop->GetInventoryCtrl();
        const size_t numItems = ctrl->GetNumberOfItems();

        for (size_t i = 0; i < numItems; ++i)
        {
            ITEM* pItem = ctrl->GetItem(static_cast<int>(i));
            if (pItem && pItem->lineal_pos >= 0 && pItem->lineal_pos < 64)
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

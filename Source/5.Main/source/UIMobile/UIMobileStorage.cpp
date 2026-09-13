// =============================================================================
// UIMobileStorage.cpp
// Implementation of Mobile Storage / Vault Window.
// =============================================================================

#include "stdafx.h"
#include "UIMobileStorage.h"
#include "UIMobileInventory.h"
#include "NewUISystem.h"
#include "NewUIStorageInventory.h"
#include "NewUIMyInventory.h"
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
    CUIMobileStorage::CUIMobileStorage()
        : m_pNewUIMng(nullptr)
        , m_pNewUI3DRenderMng(nullptr)
        , m_bIsOpen(false)
        , m_winX(0.0f), m_winY(0.0f), m_winW(0.0f), m_winH(0.0f)
        , m_closeBtnX(0.0f), m_closeBtnY(0.0f), m_closeBtnSize(30.0f)
        , m_selectedSlot(-1)
        , m_cardX(0.0f), m_cardY(0.0f), m_cardW(0.0f), m_cardH(0.0f)
        , m_withdrawBtnX(0.0f), m_withdrawBtnY(0.0f), m_withdrawBtnW(0.0f), m_withdrawBtnH(0.0f)
        , m_depZenBtnX(0.0f), m_depZenBtnY(0.0f), m_depZenBtnW(0.0f), m_depZenBtnH(0.0f)
        , m_witZenBtnX(0.0f), m_witZenBtnY(0.0f), m_witZenBtnW(0.0f), m_witZenBtnH(0.0f)
        , m_lastTapTick(0), m_lastTapIndex(-1)
        , m_pressedSlot(-1)
        , m_withdrawPressed(false)
        , m_depZenPressed(false)
        , m_witZenPressed(false)
        , m_closePressed(false)
    {
        std::memset(m_slots, 0, sizeof(m_slots));
    }

    CUIMobileStorage::~CUIMobileStorage()
    {
        Release();
    }

    bool CUIMobileStorage::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng)
    {
        m_pNewUIMng = pNewUIMng;
        m_pNewUI3DRenderMng = pNewUI3DRenderMng;

        if (m_pNewUIMng)
        {
            m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_STORAGE, this);
        }
        if (m_pNewUI3DRenderMng)
        {
            m_pNewUI3DRenderMng->Add3DRenderObj(this, 10.5f);
        }
        return true;
    }

    void CUIMobileStorage::Release()
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

    void CUIMobileStorage::Open()
    {
        m_bIsOpen = true;
        m_selectedSlot = -1;
        m_pressedSlot = -1;
        m_withdrawPressed = false;
        m_depZenPressed = false;
        m_witZenPressed = false;
        m_closePressed = false;

        ComputeLayout();
        PlayBuffer(SOUND_CLICK01);

        // Also open Mobile Inventory alongside
        if (CUIMobileInventory::GetInstance() && !CUIMobileInventory::GetInstance()->IsOpen())
        {
            CUIMobileInventory::GetInstance()->Open();
        }
    }

    void CUIMobileStorage::Close()
    {
        m_bIsOpen = false;
        m_selectedSlot = -1;
        m_pressedSlot = -1;
        PlayBuffer(SOUND_CLICK01);
        SendRequestStorageExit();
    }

    void CUIMobileStorage::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    void CUIMobileStorage::ComputeLayout()
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        m_winW = 340.0f;
        m_winH = 420.0f;
        m_winX = 20.0f;
        m_winY = (winH - m_winH) * 0.5f;

        m_closeBtnSize = 30.0f;
        m_closeBtnX = m_winX + m_winW - m_closeBtnSize - 8.0f;
        m_closeBtnY = m_winY + 8.0f;

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

        // Action Card popup if item selected
        m_cardW = 200.0f;
        m_cardH = 120.0f;
        m_cardX = m_winX + (m_winW - m_cardW) * 0.5f;
        m_cardY = m_winY + m_winH - m_cardH - 50.0f;

        m_withdrawBtnW = 120.0f;
        m_withdrawBtnH = 30.0f;
        m_withdrawBtnX = m_cardX + (m_cardW - m_withdrawBtnW) * 0.5f;
        m_withdrawBtnY = m_cardY + m_cardH - m_withdrawBtnH - 10.0f;

        // Zen Deposit / Withdraw Buttons at bottom
        const float zenBtnW = 140.0f;
        const float zenBtnH = 28.0f;
        m_depZenBtnW = zenBtnW;
        m_depZenBtnH = zenBtnH;
        m_depZenBtnX = m_winX + 16.0f;
        m_depZenBtnY = m_winY + m_winH - zenBtnH - 10.0f;

        m_witZenBtnW = zenBtnW;
        m_witZenBtnH = zenBtnH;
        m_witZenBtnX = m_winX + m_winW - zenBtnW - 16.0f;
        m_witZenBtnY = m_depZenBtnY;
    }

    void CUIMobileStorage::ExecuteWithdrawItem(int storageSlot)
    {
        if (!g_pStorageInventory || !g_pStorageInventory->GetInventoryCtrl() || !g_pMyInventory) return;

        ITEM* pItem = g_pStorageInventory->GetInventoryCtrl()->GetItem(storageSlot);
        if (!pItem) return;

        const int emptySlot = g_pMyInventory->FindEmptySlot(pItem);
        if (emptySlot != -1)
        {
            SendRequestEquipmentItem(REQUEST_EQUIPMENT_STORAGE, storageSlot, pItem, REQUEST_EQUIPMENT_INVENTORY, emptySlot);
            PlayBuffer(SOUND_CLICK01);
            m_selectedSlot = -1;
        }
    }

    void CUIMobileStorage::ExecuteDepositZen(int amount)
    {
        if (!CharacterMachine) return;
        const DWORD myZen = CharacterMachine->Gold;
        const int depAmount = (amount <= 0 || static_cast<DWORD>(amount) > myZen) ? static_cast<int>(myZen) : amount;
        if (depAmount > 0)
        {
            SendRequestStorageGold(0, depAmount);
            PlayBuffer(SOUND_CLICK01);
        }
    }

    void CUIMobileStorage::ExecuteWithdrawZen(int amount)
    {
        if (!CharacterMachine) return;
        const int storeZen = CharacterMachine->StorageGold;
        const int witAmount = (amount <= 0 || amount > storeZen) ? storeZen : amount;
        if (witAmount > 0)
        {
            SendRequestStorageGold(1, witAmount);
            PlayBuffer(SOUND_CLICK01);
        }
    }

    bool CUIMobileStorage::OnFingerDown(const SDL_TouchFingerEvent& ev)
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

        // Deposit Zen
        if (UIMobile::HitTestRect(tx, ty, m_depZenBtnX, m_depZenBtnY, m_depZenBtnW, m_depZenBtnH))
        {
            m_depZenPressed = true;
            return true;
        }

        // Withdraw Zen
        if (UIMobile::HitTestRect(tx, ty, m_witZenBtnX, m_witZenBtnY, m_witZenBtnW, m_witZenBtnH))
        {
            m_witZenPressed = true;
            return true;
        }

        // Withdraw item button in Card
        if (m_selectedSlot >= 0 && UIMobile::HitTestRect(tx, ty, m_withdrawBtnX, m_withdrawBtnY, m_withdrawBtnW, m_withdrawBtnH))
        {
            m_withdrawPressed = true;
            return true;
        }

        // Test 64 slots (with double tap quick withdraw)
        for (int i = 0; i < 64; ++i)
        {
            if (UIMobile::HitTestRect(tx, ty, m_slots[i].x, m_slots[i].y, m_slots[i].w, m_slots[i].h))
            {
                const uint32_t now = SDL_GetTicks();
                if ((now - m_lastTapTick <= 300) && (m_lastTapIndex == i))
                {
                    // Double tap -> instant withdraw!
                    ExecuteWithdrawItem(i);
                    m_lastTapTick = 0;
                    m_lastTapIndex = -1;
                    return true;
                }

                m_lastTapTick = now;
                m_lastTapIndex = i;
                m_pressedSlot = i;
                m_selectedSlot = i;
                PlayBuffer(SOUND_CLICK01);
                return true;
            }
        }

        return true;
    }

    bool CUIMobileStorage::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        float tx = 0.0f, ty = 0.0f;
        UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);
        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileStorage::OnFingerUp(const SDL_TouchFingerEvent& ev)
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

        if (m_depZenPressed)
        {
            m_depZenPressed = false;
            if (UIMobile::HitTestRect(tx, ty, m_depZenBtnX, m_depZenBtnY, m_depZenBtnW, m_depZenBtnH))
            {
                ExecuteDepositZen(1000000); // 1M Zen quick deposit
                return true;
            }
        }

        if (m_witZenPressed)
        {
            m_witZenPressed = false;
            if (UIMobile::HitTestRect(tx, ty, m_witZenBtnX, m_witZenBtnY, m_witZenBtnW, m_witZenBtnH))
            {
                ExecuteWithdrawZen(1000000); // 1M Zen quick withdraw
                return true;
            }
        }

        if (m_withdrawPressed)
        {
            m_withdrawPressed = false;
            if (m_selectedSlot >= 0 && UIMobile::HitTestRect(tx, ty, m_withdrawBtnX, m_withdrawBtnY, m_withdrawBtnW, m_withdrawBtnH))
            {
                ExecuteWithdrawItem(m_selectedSlot);
                return true;
            }
        }

        m_pressedSlot = -1;
        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileStorage::Update()
    {
        if (!m_bIsOpen) return true;
        ComputeLayout();
        return true;
    }

    bool CUIMobileStorage::UpdateKeyEvent()
    {
        if (!m_bIsOpen) return true;
        if (IsPress(VK_ESCAPE))
        {
            Close();
            return false;
        }
        return true;
    }

    bool CUIMobileStorage::Render()
    {
        if (!m_bIsOpen) return true;

        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // Header Title with storage zen
        const int sZen = CharacterMachine ? CharacterMachine->StorageGold : 0;
        char titleBuf[128];
        std::snprintf(titleBuf, sizeof(titleBuf), "[RUONG KHO]  ZEN KHO: %d", sZen);
        UIMobile::DrawPanel(m_winX, m_winY, m_winW, m_winH, titleBuf, true);

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

        // Zen Buttons
        UIMobile::DrawButton(m_depZenBtnX, m_depZenBtnY, m_depZenBtnW, m_depZenBtnH, "GUI 1M ZEN", m_depZenPressed, true, UIMobile::Colors::BtnPrimary);
        UIMobile::DrawButton(m_witZenBtnX, m_witZenBtnY, m_witZenBtnW, m_witZenBtnH, "RUT 1M ZEN", m_witZenPressed, true, UIMobile::Colors::BtnSuccess);

        // Action Card if item selected
        if (m_selectedSlot >= 0 && g_pStorageInventory && g_pStorageInventory->GetInventoryCtrl())
        {
            ITEM* pItem = g_pStorageInventory->GetInventoryCtrl()->GetItem(m_selectedSlot);
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
                    g_pRenderText->SetTextColor(200, 220, 240, 255);
                    g_pRenderText->RenderText(static_cast<int>(m_cardX + 12.0f), static_cast<int>(m_cardY + 36.0f), "Cham dup de rut nhanh");
                }

                // Withdraw Button
                UIMobile::DrawButton(m_withdrawBtnX, m_withdrawBtnY, m_withdrawBtnW, m_withdrawBtnH, "RUT VE TUI", m_withdrawPressed, true, UIMobile::Colors::BtnSuccess);
            }
        }

        DisableAlphaBlend();
        return true;
    }

    void CUIMobileStorage::Render3D()
    {
        if (!m_bIsOpen || !g_pStorageInventory || !g_pStorageInventory->GetInventoryCtrl()) return;

        CNewUIInventoryCtrl* ctrl = g_pStorageInventory->GetInventoryCtrl();
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

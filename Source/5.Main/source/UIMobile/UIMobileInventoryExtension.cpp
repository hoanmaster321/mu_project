// =============================================================================
// UIMobileInventoryExtension.cpp
// Implementation of Mobile Inventory Extension UI (PC visual style, Mobile scale)
// =============================================================================

#include "stdafx.h"
#include "UIMobileInventoryExtension.h"
#include "UIMobileInventory.h"
#include "UIMobileCommon.h"
#include "NewUISystem.h"
#include "NewUIInventoryCtrl.h"
#include "NewUIInventoryExtension.h"
#include "NewUIMyInventory.h"
#include "NewUIMessageBox.h"
#include "wsclientinline.h"
#include "DSPlaySound.h"
#include "ZzzInventory.h"
#include "ZzzTexture.h"
#include "ZzzCharacter.h"
#include "_define.h"

#if defined(__ANDROID__)
#include <android/log.h>
#endif

extern int DisplayWin;
extern int DisplayHeight;

static SEASON3B::CUIMobileInventoryExtension* s_pExtensionInstance = nullptr;

namespace SEASON3B
{
    CUIMobileInventoryExtension::CUIMobileInventoryExtension()
        : m_pNewUIMng(nullptr)
        , m_pNewUI3DRenderMng(nullptr)
        , m_bIsOpen(false)
        , m_winX(0.0f), m_winY(0.0f), m_winW(280.0f), m_winH(464.0f)
        , m_closeBtnX(0.0f), m_closeBtnY(0.0f), m_closeBtnSize(24.0f)
        , m_exitBtnX(0.0f), m_exitBtnY(0.0f), m_exitBtnW(36.0f), m_exitBtnH(29.0f)
        , m_slotSize(30.0f)
        , m_boxStartX(0.0f)
        , m_selectedExt(-1), m_selectedSlot(-1)
        , m_bShowActionCard(false)
        , m_cardX(0.0f), m_cardY(0.0f), m_cardW(0.0f), m_cardH(0.0f)
        , m_btnToBagX(0.0f), m_btnToBagY(0.0f), m_btnToBagW(0.0f), m_btnToBagH(0.0f)
        , m_btnCloseCardX(0.0f), m_btnCloseCardY(0.0f), m_btnCloseCardW(0.0f), m_btnCloseCardH(0.0f)
        , m_bDragging(false), m_dragFingerId(0), m_dragExt(-1), m_dragSlot(-1)
        , m_dragStartX(0.0f), m_dragStartY(0.0f), m_dragCurX(0.0f), m_dragCurY(0.0f)
        , m_dragStartTick(0)
        , m_lastTapTick(0), m_lastTapExt(-1), m_lastTapSlot(-1)
    {
        m_boxStartY[0] = 0.0f;
        m_boxStartY[1] = 0.0f;
        s_pExtensionInstance = this;
    }

    CUIMobileInventoryExtension::~CUIMobileInventoryExtension()
    {
        Release();
        if (s_pExtensionInstance == this) s_pExtensionInstance = nullptr;
    }

    CUIMobileInventoryExtension* CUIMobileInventoryExtension::GetInstance()
    {
        return s_pExtensionInstance;
    }

    bool CUIMobileInventoryExtension::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng)
    {
        m_pNewUIMng = pNewUIMng;
        m_pNewUI3DRenderMng = pNewUI3DRenderMng;

        EnsureTextures();
        ComputeLayout();
        return true;
    }

    void CUIMobileInventoryExtension::Release()
    {
        m_pNewUI3DRenderMng = nullptr;
        m_pNewUIMng = nullptr;
        m_bIsOpen = false;
    }

    void CUIMobileInventoryExtension::EnsureTextures()
    {
        LoadBitmap("Interface\\newui_item_add_marking_non.jpg", IMAGE_EXTENSION_EMPTY, GL_LINEAR);
        LoadBitmap("Interface\\newui_item_add_table.tga", IMAGE_EXTENSION_TABLE, GL_LINEAR);
        LoadBitmap("Interface\\newui_item_add_marking_no01.tga", IMAGE_EXTENSION_NO1, GL_LINEAR);
        LoadBitmap("Interface\\newui_item_add_marking_no02.tga", IMAGE_EXTENSION_NO2, GL_LINEAR);
        LoadBitmap("Interface\\newui_item_add_marking_no03.tga", IMAGE_EXTENSION_NO3, GL_LINEAR);
        LoadBitmap("Interface\\newui_item_add_marking_no04.tga", IMAGE_EXTENSION_NO4, GL_LINEAR);
    }

    void CUIMobileInventoryExtension::ComputeLayout()
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        m_winW = 280.0f;
        m_winH = 464.0f;

        float invenX = winW - 280.0f - 26.0f;
        float invenY = (winH - m_winH) * 0.5f;

        if (CUIMobileInventory::GetInstance())
        {
            invenX = CUIMobileInventory::GetInstance()->GetPanelX();
            invenY = CUIMobileInventory::GetInstance()->GetPanelY();
        }

        m_winX = invenX - m_winW - 8.0f;
        if (m_winX < 8.0f)
        {
            m_winX = 8.0f;
        }
        m_winY = invenY;

        // Close button (top right)
        m_closeBtnSize = 24.0f;
        m_closeBtnX = m_winX + m_winW - m_closeBtnSize - 10.0f;
        m_closeBtnY = m_winY + 7.0f;

        // Exit button (bottom left)
        m_exitBtnX = m_winX + 16.0f;
        m_exitBtnY = m_winY + m_winH - 38.0f;
        m_exitBtnW = 36.0f;
        m_exitBtnH = 29.0f;

        // 8x4 Grid boxes (30px slots, 240px wide)
        m_slotSize = 30.0f;
        m_boxStartX = m_winX + ((m_winW - (8.0f * m_slotSize)) * 0.5f);
        m_boxStartY[0] = m_winY + 46.0f;
        m_boxStartY[1] = m_winY + 176.0f;

        // Action card (below Box 1)
        m_cardX = m_winX + 16.0f;
        m_cardY = m_winY + 296.0f;
        m_cardW = m_winW - 32.0f;
        m_cardH = 118.0f;

        m_btnToBagX = m_cardX + 10.0f;
        m_btnToBagY = m_cardY + 74.0f;
        m_btnToBagW = 132.0f;
        m_btnToBagH = 34.0f;

        m_btnCloseCardX = m_cardX + 152.0f;
        m_btnCloseCardY = m_cardY + 74.0f;
        m_btnCloseCardW = 76.0f;
        m_btnCloseCardH = 34.0f;
    }

    void CUIMobileInventoryExtension::Open()
    {
        m_bIsOpen = true;
        m_selectedExt = -1;
        m_selectedSlot = -1;
        m_bShowActionCard = false;
        m_bDragging = false;

        ComputeLayout();
        EnsureTextures();
        PlayBuffer(SOUND_CLICK01);

        // Ensure PC interface is hidden so it doesn't double-render
        if (g_pNewUISystem && g_pNewUISystem->IsVisible(INTERFACE_ExpandInventory))
        {
            g_pNewUISystem->Hide(INTERFACE_ExpandInventory);
        }

        // Open mobile inventory if closed
        if (CUIMobileInventory::GetInstance() && !CUIMobileInventory::GetInstance()->IsOpen())
        {
            CUIMobileInventory::GetInstance()->Open();
        }
    }

    void CUIMobileInventoryExtension::Close()
    {
        m_bIsOpen = false;
        m_selectedExt = -1;
        m_selectedSlot = -1;
        m_bShowActionCard = false;
        m_bDragging = false;
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileInventoryExtension::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    int CUIMobileInventoryExtension::HitTestSlot(float tx, float ty, int& outExtIndex) const
    {
        outExtIndex = -1;
        for (int ext = 0; ext < 2; ++ext)
        {
            const float bX = m_boxStartX;
            const float bY = m_boxStartY[ext];
            const float bW = 8.0f * m_slotSize;
            const float bH = 4.0f * m_slotSize;

            if (tx >= bX && tx < bX + bW && ty >= bY && ty < bY + bH)
            {
                const int col = static_cast<int>((tx - bX) / m_slotSize);
                const int row = static_cast<int>((ty - bY) / m_slotSize);
                if (col >= 0 && col < 8 && row >= 0 && row < 4)
                {
                    outExtIndex = ext;
                    return row * 8 + col;
                }
            }
        }
        return -1;
    }

    ITEM* CUIMobileInventoryExtension::GetExtensionItem(int extIndex, int slotIndex, int& outItemLinealIndex) const
    {
        outItemLinealIndex = -1;
        if (!g_pMyInventoryExt || !CharacterAttribute) return nullptr;
        if (extIndex < 0 || extIndex >= CharacterAttribute->InventoryExtensions) return nullptr;

        CNewUIInventoryCtrl* pCtrl = g_pMyInventoryExt->GetInventoryCtrl(extIndex);
        if (!pCtrl) return nullptr;

        const int col = slotIndex % 8;
        const int row = slotIndex / 8;

        const size_t numItems = pCtrl->GetNumberOfItems();
        for (size_t i = 0; i < numItems; ++i)
        {
            ITEM* pItem = pCtrl->GetItem(static_cast<int>(i));
            if (!pItem || pItem->Type < 0) continue;
            const ITEM_ATTRIBUTE* pAttr = (ItemAttribute != nullptr) ? &ItemAttribute[pItem->Type] : nullptr;
            const int w = (pAttr && pAttr->Width > 0) ? pAttr->Width : 1;
            const int h = (pAttr && pAttr->Height > 0) ? pAttr->Height : 1;
            if (col >= pItem->x && col < pItem->x + w &&
                row >= pItem->y && row < pItem->y + h)
            {
                outItemLinealIndex = pCtrl->GetIndexByItem(pItem);
                return pItem;
            }
        }
        return nullptr;
    }

    void CUIMobileInventoryExtension::ExecuteTransferToBag(int extIndex, int slotIndex)
    {
        int srcLinealIndex = -1;
        ITEM* pItem = GetExtensionItem(extIndex, slotIndex, srcLinealIndex);
        if (!pItem || !g_pMyInventory) return;

        const int emptySlot = g_pMyInventory->FindEmptySlot(pItem);
        if (emptySlot == -1)
        {
            PlayBuffer(SOUND_CLICK01);
            return;
        }

        ITEM itemCopy = *pItem;
        SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, srcLinealIndex, &itemCopy, REQUEST_EQUIPMENT_INVENTORY, emptySlot);
        PlayBuffer(SOUND_CLICK01);

        m_selectedExt = -1;
        m_selectedSlot = -1;
        m_bShowActionCard = false;
    }

    void CUIMobileInventoryExtension::ExecuteTransferFromBag(int bagSlotIndex)
    {
        if (!g_pMobileInventory || !g_pMyInventoryExt) return;
        ITEM* pItem = g_pMobileInventory->GetBagItem(bagSlotIndex);
        if (!pItem) return;

        const int emptyExtSlot = g_pMyInventoryExt->FindEmptySlot(pItem);
        if (emptyExtSlot == -1)
        {
            PlayBuffer(SOUND_CLICK01);
            return;
        }

        const int srcBagIndex = 12 + bagSlotIndex;
        ITEM itemCopy = *pItem;
        SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, srcBagIndex, &itemCopy, REQUEST_EQUIPMENT_INVENTORY, emptyExtSlot);
        PlayBuffer(SOUND_CLICK01);
    }

    bool CUIMobileInventoryExtension::OnFingerDown(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        float tx = 0.0f, ty = 0.0f;
        UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);

        // Check bounds
        if (!UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH))
        {
            return false;
        }

        // Close button [X]
        if (UIMobile::HitTestRect(tx, ty, m_closeBtnX - 4.0f, m_closeBtnY - 4.0f, m_closeBtnSize + 8.0f, m_closeBtnSize + 8.0f))
        {
            Close();
            return true;
        }

        // Action card buttons
        if (m_bShowActionCard)
        {
            if (UIMobile::HitTestRect(tx, ty, m_btnToBagX, m_btnToBagY, m_btnToBagW, m_btnToBagH))
            {
                ExecuteTransferToBag(m_selectedExt, m_selectedSlot);
                return true;
            }
            if (UIMobile::HitTestRect(tx, ty, m_btnCloseCardX, m_btnCloseCardY, m_btnCloseCardW, m_btnCloseCardH))
            {
                m_bShowActionCard = false;
                m_selectedExt = -1;
                m_selectedSlot = -1;
                PlayBuffer(SOUND_CLICK01);
                return true;
            }
        }

        // Slot touch
        int extIndex = -1;
        const int slotIndex = HitTestSlot(tx, ty, extIndex);
        if (slotIndex != -1 && extIndex != -1)
        {
            if (CharacterAttribute && extIndex < CharacterAttribute->InventoryExtensions)
            {
                int itemLineal = -1;
                ITEM* pItem = GetExtensionItem(extIndex, slotIndex, itemLineal);
                const uint32_t now = SDL_GetTicks();

                if (pItem)
                {
                    // Double tap detection (< 350ms on same slot)
                    if (m_lastTapExt == extIndex && m_lastTapSlot == slotIndex && (now - m_lastTapTick) < 350)
                    {
                        ExecuteTransferToBag(extIndex, slotIndex);
                        m_lastTapTick = 0;
                        m_lastTapExt = -1;
                        m_lastTapSlot = -1;
                        return true;
                    }

                    m_lastTapTick = now;
                    m_lastTapExt = extIndex;
                    m_lastTapSlot = slotIndex;

                    m_selectedExt = extIndex;
                    m_selectedSlot = slotIndex;
                    m_bShowActionCard = true;

                    // Start drag
                    m_bDragging = true;
                    m_dragFingerId = ev.fingerID;
                    m_dragExt = extIndex;
                    m_dragSlot = slotIndex;
                    m_dragStartX = tx;
                    m_dragStartY = ty;
                    m_dragCurX = tx;
                    m_dragCurY = ty;
                    m_dragStartTick = now;

                    PlayBuffer(SOUND_CLICK01);
                    return true;
                }
                else
                {
                    m_selectedExt = -1;
                    m_selectedSlot = -1;
                    m_bShowActionCard = false;
                }
            }
        }

        // Touched background inside window
        return true;
    }

    bool CUIMobileInventoryExtension::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        if (m_bDragging && m_dragFingerId == ev.fingerID)
        {
            float tx = 0.0f, ty = 0.0f;
            UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);
            m_dragCurX = tx;
            m_dragCurY = ty;
            return true;
        }
        return false;
    }

    bool CUIMobileInventoryExtension::OnFingerUp(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        if (m_bDragging && m_dragFingerId == ev.fingerID)
        {
            float tx = 0.0f, ty = 0.0f;
            UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);

            // Check if dropped onto Mobile Inventory
            if (CUIMobileInventory::GetInstance() && CUIMobileInventory::GetInstance()->IsOpen())
            {
                const float invenX = CUIMobileInventory::GetInstance()->GetPanelX();
                const float invenY = CUIMobileInventory::GetInstance()->GetPanelY();
                const float invenW = CUIMobileInventory::GetInstance()->GetPanelW();
                const float invenH = CUIMobileInventory::GetInstance()->GetPanelH();

                if (tx >= invenX && tx <= invenX + invenW && ty >= invenY && ty <= invenY + invenH)
                {
                    ExecuteTransferToBag(m_dragExt, m_dragSlot);
                }
            }

            m_bDragging = false;
            return true;
        }
        return false;
    }

    bool CUIMobileInventoryExtension::Update()
    {
        if (!m_bIsOpen) return true;
        ComputeLayout();
        return true;
    }

    bool CUIMobileInventoryExtension::UpdateKeyEvent()
    {
        if (!m_bIsOpen) return true;
        return true;
    }

    void CUIMobileInventoryExtension::RenderFrame()
    {
        UIMobile::DrawSlicedFrame(m_winX, m_winY, m_winW, m_winH, GlobalText[3323]);
        UIMobile::DrawCloseButton(m_closeBtnX, m_closeBtnY, m_closeBtnSize, false);
    }

    void CUIMobileInventoryExtension::RenderBoxes()
    {
        const int unlockedCount = (CharacterAttribute != nullptr) ? CharacterAttribute->InventoryExtensions : 0;

        for (int ext = 0; ext < 2; ++ext)
        {
            const float bX = m_boxStartX;
            const float bY = m_boxStartY[ext];
            const float bW = 8.0f * m_slotSize;
            const float bH = 4.0f * m_slotSize;

            if (ext >= unlockedCount)
            {
                // Locked Box (Clean GlassCard background with authentic MU table borders)
                UIMobile::Render::DrawGlassCard(bX, bY, bW, bH, 0.6f);
                UIMobile::DrawTableFrameBorders(bX, bY, bW, bH);

                // Roman numeral icon (NO1 / NO2)
                EnableAlphaTest();
                glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
                RenderImage(static_cast<GLuint>(IMAGE_EXTENSION_NO1 + ext), bX + (bW * 0.5f) - 15.0f, bY + (bH * 0.5f) - 22.0f, 30.0f, 34.0f);
                DisableAlphaBlend();

                g_pRenderText->SetFont(g_hFont);
                g_pRenderText->SetBgColor(0, 0, 0, 0);
                g_pRenderText->SetTextColor(180, 180, 180, 255);
                g_pRenderText->RenderText(static_cast<int>(bX), static_cast<int>(bY + bH - 26.0f), "Chưa mở khóa", static_cast<int>(bW), 0, RT3_SORT_CENTER);
            }
            else
            {
                // Unlocked Box: Grid with authentic MU table borders
                UIMobile::DrawTableGrid(bX, bY, bW, bH, 8, 4);

                // Highlight selected slot
                if (m_selectedExt == ext && m_selectedSlot != -1)
                {
                    const int c = m_selectedSlot % 8;
                    const int r = m_selectedSlot / 8;
                    const float sX = bX + (static_cast<float>(c) * m_slotSize);
                    const float sY = bY + (static_cast<float>(r) * m_slotSize);
                    UIMobile::DrawBorder(sX, sY, m_slotSize, m_slotSize, 2.0f, UIMobile::Colors::SlotHighlight);
                }
            }
        }
    }

    void CUIMobileInventoryExtension::RenderActionCard()
    {
        if (m_selectedExt == -1 || m_selectedSlot == -1) return;
        int dummyLineal = -1;
        ITEM* pItem = GetExtensionItem(m_selectedExt, m_selectedSlot, dummyLineal);
        if (!pItem) return;

        // Card backdrop
        UIMobile::Render::DrawGlassCard(m_cardX, m_cardY, m_cardW, m_cardH);
        UIMobile::DrawBorder(m_cardX, m_cardY, m_cardW, m_cardH, 1.5f, UIMobile::Colors::PanelBorder);

        // Item Name & info
        const ITEM_ATTRIBUTE* pAttr = (ItemAttribute != nullptr) ? &ItemAttribute[pItem->Type] : nullptr;
        char nameBuf[128] = { 0 };
        const int itemLevel = (pItem->Level >> 3) & 15;
        if (pAttr)
        {
            if (itemLevel > 0)
                std::snprintf(nameBuf, sizeof(nameBuf), "%s +%d", pAttr->Name, itemLevel);
            else
                std::snprintf(nameBuf, sizeof(nameBuf), "%s", pAttr->Name);
        }
        else
        {
            std::snprintf(nameBuf, sizeof(nameBuf), "Vật phẩm");
        }

#if defined(ANDROID) || defined(__ANDROID__)
        g_pRenderText->SetFont((g_hFontItemInfoBold != nullptr) ? g_hFontItemInfoBold : g_hFontBold);
#else
        g_pRenderText->SetFont(g_hFontBold);
#endif
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->SetTextColor(255, 230, 90, 255);
        g_pRenderText->RenderText(static_cast<int>(m_cardX + 12.0f), static_cast<int>(m_cardY + 8.0f), nameBuf);

        // Durability / stats
        char durBuf[64] = { 0 };
        std::snprintf(durBuf, sizeof(durBuf), "Độ bền: %d", pItem->Durability);
#if defined(ANDROID) || defined(__ANDROID__)
        g_pRenderText->SetFont((g_hFontItemInfo != nullptr) ? g_hFontItemInfo : g_hFont);
#else
        g_pRenderText->SetFont(g_hFont);
#endif
        g_pRenderText->SetTextColor(180, 180, 180, 255);
        g_pRenderText->RenderText(static_cast<int>(m_cardX + 12.0f), static_cast<int>(m_cardY + 38.0f), durBuf);
        g_pRenderText->SetFont(g_hFont);

        // Buttons: [Chuyển Về Túi] and [Đóng]
        UIMobile::DrawButton(m_btnToBagX, m_btnToBagY, m_btnToBagW, m_btnToBagH, "Chuyển Về Túi", false, true, UIMobile::Colors::BtnPrimary);
        UIMobile::DrawButton(m_btnCloseCardX, m_btnCloseCardY, m_btnCloseCardW, m_btnCloseCardH, "Đóng", false, true, UIMobile::Colors::BtnDanger);
    }

    void CUIMobileInventoryExtension::RenderDraggedItem()
    {
        if (!m_bDragging || m_dragExt == -1 || m_dragSlot == -1) return;
        int dummyLineal = -1;
        ITEM* pItem = GetExtensionItem(m_dragExt, m_dragSlot, dummyLineal);
        if (!pItem || pItem->Type < 0) return;

        const ITEM_ATTRIBUTE* pAttr = (ItemAttribute != nullptr) ? &ItemAttribute[pItem->Type] : nullptr;
        const int itemW = (pAttr && pAttr->Width > 0) ? pAttr->Width : 1;
        const int itemH = (pAttr && pAttr->Height > 0) ? pAttr->Height : 1;
        const float drawW = static_cast<float>(itemW) * m_slotSize;
        const float drawH = static_cast<float>(itemH) * m_slotSize;

        RenderItem3D(
            m_dragCurX - (drawW * 0.5f),
            m_dragCurY - (drawH * 0.5f),
            drawW - 2.0f,
            drawH - 2.0f,
            pItem->Type,
            pItem->Level,
            pItem->Option1,
            pItem->ExtOption,
            true
        );
    }

    bool CUIMobileInventoryExtension::Render()
    {
        if (!m_bIsOpen) return true;

        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        RenderFrame();
        RenderBoxes();

        if (m_bShowActionCard)
        {
            RenderActionCard();
        }
        else
        {
            // Subtle instruction hint
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->SetTextColor(160, 160, 160, 200);
            g_pRenderText->RenderText(static_cast<int>(m_winX + 16.0f), static_cast<int>(m_winY + 314.0f), "Chạm vào vật phẩm để xem và chuyển đồ.");
            g_pRenderText->RenderText(static_cast<int>(m_winX + 16.0f), static_cast<int>(m_winY + 332.0f), "Chạm đúp để chuyển nhanh về túi.");
        }

        DisableAlphaBlend();
        return true;
    }

    void CUIMobileInventoryExtension::Render3D()
    {
        if (!m_bIsOpen || !g_pMyInventoryExt || !CharacterAttribute) return;

        const int unlockedCount = CharacterAttribute->InventoryExtensions;

        for (int ext = 0; ext < 2 && ext < unlockedCount; ++ext)
        {
            CNewUIInventoryCtrl* pCtrl = g_pMyInventoryExt->GetInventoryCtrl(ext);
            if (!pCtrl) continue;

            const float bX = m_boxStartX;
            const float bY = m_boxStartY[ext];
            const size_t numItems = pCtrl->GetNumberOfItems();

            for (size_t i = 0; i < numItems; ++i)
            {
                ITEM* pItem = pCtrl->GetItem(static_cast<int>(i));
                if (!pItem || pItem->Type < 0) continue;

                // If currently dragging this item, don't draw in slot
                if (m_bDragging && m_dragExt == ext && m_dragSlot == (pItem->y * 8 + pItem->x))
                {
                    continue;
                }

                const ITEM_ATTRIBUTE* pAttr = (ItemAttribute != nullptr) ? &ItemAttribute[pItem->Type] : nullptr;
                const int itemW = (pAttr && pAttr->Width > 0) ? pAttr->Width : 1;
                const int itemH = (pAttr && pAttr->Height > 0) ? pAttr->Height : 1;

                const float itemX = bX + (static_cast<float>(pItem->x) * m_slotSize);
                const float itemY = bY + (static_cast<float>(pItem->y) * m_slotSize);

                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                RenderItem3D(
                    itemX + 1.0f,
                    itemY + 1.0f,
                    (static_cast<float>(itemW) * m_slotSize) - 2.0f,
                    (static_cast<float>(itemH) * m_slotSize) - 2.0f,
                    pItem->Type,
                    pItem->Level,
                    pItem->Option1,
                    pItem->ExtOption,
                    true
                );
            }
        }

        if (m_bDragging)
        {
            RenderDraggedItem();
        }
    }
}

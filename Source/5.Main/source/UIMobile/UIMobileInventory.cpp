// =============================================================================
// UIMobileInventory.cpp
// Implementation of Dedicated Mobile Inventory UI for MU Online.
// =============================================================================

#include "stdafx.h"
#include "UIMobileInventory.h"
#include "ZzzOpenglUtil.h"
#include "ZzzTexture.h"
#include "ZzzCharacter.h"
#include "ZzzScene.h"
#include "ZzzInterface.h"
#include "NewUISystem.h"
#include "NewUIMyInventory.h"
#include "NewUIInventoryCtrl.h"
#include "CharacterManager.h"
#include "ZzzInventory.h"
#include "DSPlaySound.h"
#include "ProtocolSend.h"
#include "wsclientinline.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

extern int DisplayWin;
extern int DisplayHeight;
extern float g_fScreenRate_x;
extern float g_fScreenRate_y;
extern int MouseX;
extern int MouseY;
extern CHARACTER* Hero;

static SEASON3B::CUIMobileInventory* s_pInstance = nullptr;

namespace SEASON3B
{
    static void TouchToVirtual(float normX, float normY, float& outX, float& outY)
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;
        outX = normX * winW;
        outY = normY * winH;
    }

    CUIMobileInventory::CUIMobileInventory()
        : m_pNewUIMng(nullptr)
        , m_pNewUI3DRenderMng(nullptr)
        , m_bIsOpen(false)
        , m_selectedType(SLOT_TYPE_NONE)
        , m_selectedIndex(-1)
        , m_bShowActionCard(false)
        , m_lastTapTick(0)
        , m_lastTapType(SLOT_TYPE_NONE)
        , m_lastTapIndex(-1)
        , m_bDragging(false)
        , m_dragFingerId(0)
        , m_dragSrcType(SLOT_TYPE_NONE)
        , m_dragSrcIndex(-1)
        , m_dragStartX(0.0f)
        , m_dragStartY(0.0f)
        , m_dragCurX(0.0f)
        , m_dragCurY(0.0f)
        , m_dragStartTick(0)
        , m_hoverDstIndex(-1)
        , m_hoverDstType(SLOT_TYPE_NONE)
        , m_panelX(0.0f), m_panelY(0.0f), m_panelW(0.0f), m_panelH(0.0f)
        , m_cardX(0.0f), m_cardY(0.0f), m_cardW(0.0f), m_cardH(0.0f)
        , m_closeBtnX(0.0f), m_closeBtnY(0.0f), m_closeBtnSize(32.0f)
        , m_btnEquipX(0.0f), m_btnEquipY(0.0f), m_btnEquipW(0.0f), m_btnEquipH(0.0f)
        , m_btnUseX(0.0f), m_btnUseY(0.0f), m_btnUseW(0.0f), m_btnUseH(0.0f)
        , m_btnDropX(0.0f), m_btnDropY(0.0f), m_btnDropW(0.0f), m_btnDropH(0.0f)
        , m_btnCloseCardX(0.0f), m_btnCloseCardY(0.0f), m_btnCloseCardW(0.0f), m_btnCloseCardH(0.0f)
        , m_texBackdrop(0)
        , m_texSlotBg(0)
        , m_texSlotActive(0)
        , m_texActionCardBg(0)
        , m_texBtnNormal(0)
        , m_texBtnPressed(0)
    {
        s_pInstance = this;
        std::memset(m_equipSlots, 0, sizeof(m_equipSlots));
        std::memset(m_bagSlots, 0, sizeof(m_bagSlots));
    }

    CUIMobileInventory::~CUIMobileInventory()
    {
        Release();
        if (s_pInstance == this) s_pInstance = nullptr;
    }

    CUIMobileInventory* CUIMobileInventory::GetInstance()
    {
        return s_pInstance;
    }

    bool CUIMobileInventory::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng)
    {
        m_pNewUIMng = pNewUIMng;
        m_pNewUI3DRenderMng = pNewUI3DRenderMng;

        if (m_pNewUIMng)
        {
            m_pNewUIMng->AddUIObj(INTERFACE_INVENTORY, this);
        }
        if (m_pNewUI3DRenderMng)
        {
            m_pNewUI3DRenderMng->Add3DRenderObj(this, INVENTORY_CAMERA_Z_ORDER);
        }

        EnsureTextures();
        ComputeLayout();
        m_bIsOpen = false;
        return true;
    }

    void CUIMobileInventory::Release()
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

    void CUIMobileInventory::EnsureTextures()
    {
        // Use existing base textures from the game client
        m_texBackdrop     = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN;     // newui_item_back01.tga
        m_texSlotBg       = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN;        // newui_item_box.tga
        m_texSlotActive   = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN + 1;    // table highlight
        m_texActionCardBg = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK;
    }

    void CUIMobileInventory::ComputeLayout()
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        // 1. Right-side panel
        m_panelW = 310.0f;
        m_panelH = winH - 16.0f;
        m_panelX = winW - m_panelW - 8.0f;
        m_panelY = 8.0f;

        // Close button (top right of panel)
        m_closeBtnSize = 28.0f;
        m_closeBtnX = m_panelX + m_panelW - m_closeBtnSize - 8.0f;
        m_closeBtnY = m_panelY + 8.0f;

        // 2. Equipment slots layout (Paperdoll layout inside top half of panel)
        // Center of paperdoll:
        const float paperCenterX = m_panelX + (m_panelW * 0.5f);
        const float paperTopY    = m_panelY + 36.0f;
        const float eqSize = 34.0f;

        // Helper / Pet (Top-Left)
        m_equipSlots[EQUIPMENT_HELPER] = { paperCenterX - 100.0f, paperTopY, eqSize, eqSize, EQUIPMENT_HELPER, SLOT_TYPE_EQUIPMENT };
        // Helm (Top-Center)
        m_equipSlots[EQUIPMENT_HELM]   = { paperCenterX - (eqSize * 0.5f), paperTopY, eqSize, eqSize, EQUIPMENT_HELM, SLOT_TYPE_EQUIPMENT };
        // Wing (Top-Right)
        m_equipSlots[EQUIPMENT_WING]   = { paperCenterX + 100.0f - eqSize, paperTopY, eqSize, eqSize, EQUIPMENT_WING, SLOT_TYPE_EQUIPMENT };

        // Weapon Right (Mid-Left)
        m_equipSlots[EQUIPMENT_WEAPON_RIGHT] = { paperCenterX - 110.0f, paperTopY + 40.0f, eqSize, eqSize * 1.5f, EQUIPMENT_WEAPON_RIGHT, SLOT_TYPE_EQUIPMENT };
        // Armor (Mid-Center)
        m_equipSlots[EQUIPMENT_ARMOR]        = { paperCenterX - (eqSize * 0.5f), paperTopY + 40.0f, eqSize, eqSize * 1.3f, EQUIPMENT_ARMOR, SLOT_TYPE_EQUIPMENT };
        // Weapon Left / Shield (Mid-Right)
        m_equipSlots[EQUIPMENT_WEAPON_LEFT]  = { paperCenterX + 110.0f - eqSize, paperTopY + 40.0f, eqSize, eqSize * 1.5f, EQUIPMENT_WEAPON_LEFT, SLOT_TYPE_EQUIPMENT };

        // Gloves
        m_equipSlots[EQUIPMENT_GLOVES] = { paperCenterX - 110.0f, paperTopY + 96.0f, eqSize, eqSize, EQUIPMENT_GLOVES, SLOT_TYPE_EQUIPMENT };
        // Pants
        m_equipSlots[EQUIPMENT_PANTS]  = { paperCenterX - (eqSize * 0.5f), paperTopY + 88.0f, eqSize, eqSize * 1.2f, EQUIPMENT_PANTS, SLOT_TYPE_EQUIPMENT };
        // Boots
        m_equipSlots[EQUIPMENT_BOOTS]  = { paperCenterX + 110.0f - eqSize, paperTopY + 96.0f, eqSize, eqSize, EQUIPMENT_BOOTS, SLOT_TYPE_EQUIPMENT };

        // Amulet & Rings (Jewelry row)
        const float jewY = paperTopY + 134.0f;
        m_equipSlots[EQUIPMENT_RING_RIGHT] = { paperCenterX - 70.0f, jewY, 26.0f, 26.0f, EQUIPMENT_RING_RIGHT, SLOT_TYPE_EQUIPMENT };
        m_equipSlots[EQUIPMENT_AMULET]     = { paperCenterX - 13.0f, jewY, 26.0f, 26.0f, EQUIPMENT_AMULET, SLOT_TYPE_EQUIPMENT };
        m_equipSlots[EQUIPMENT_RING_LEFT]  = { paperCenterX + 44.0f, jewY, 26.0f, 26.0f, EQUIPMENT_RING_LEFT, SLOT_TYPE_EQUIPMENT };

        // 3. Bag Grid layout (8 cols x 8 rows = 64 slots)
        const float bagSlotSize = 33.0f;
        const float bagGap      = 2.0f;
        const float bagStartX   = m_panelX + 15.0f;
        const float bagStartY   = m_panelY + 172.0f;

        for (int row = 0; row < 8; ++row)
        {
            for (int col = 0; col < 8; ++col)
            {
                const int idx = row * 8 + col;
                m_bagSlots[idx].x = bagStartX + col * (bagSlotSize + bagGap);
                m_bagSlots[idx].y = bagStartY + row * (bagSlotSize + bagGap);
                m_bagSlots[idx].w = bagSlotSize;
                m_bagSlots[idx].h = bagSlotSize;
                m_bagSlots[idx].slotIndex = idx;
                m_bagSlots[idx].slotType = SLOT_TYPE_BAG;
            }
        }

        // 4. Action Card layout (Left of inventory panel)
        m_cardW = 240.0f;
        m_cardH = 360.0f;
        m_cardX = (std::max)(10.0f, m_panelX - m_cardW - 12.0f);
        m_cardY = m_panelY + 20.0f;

        // Action Buttons at bottom of card
        const float btnW = (m_cardW - 32.0f) * 0.5f;
        const float btnH = 34.0f;
        m_btnEquipX = m_cardX + 12.0f;
        m_btnEquipY = m_cardY + m_cardH - btnH - 12.0f;
        m_btnEquipW = btnW;
        m_btnEquipH = btnH;

        m_btnUseX = m_cardX + 12.0f + btnW + 8.0f;
        m_btnUseY = m_btnEquipY;
        m_btnUseW = btnW;
        m_btnUseH = btnH;

        m_btnDropX = m_cardX + 12.0f;
        m_btnDropY = m_btnEquipY - btnH - 8.0f;
        m_btnDropW = btnW;
        m_btnDropH = btnH;

        m_btnCloseCardX = m_cardX + 12.0f + btnW + 8.0f;
        m_btnCloseCardY = m_btnDropY;
        m_btnCloseCardW = btnW;
        m_btnCloseCardH = btnH;
    }

    void CUIMobileInventory::Open()
    {
        m_bIsOpen = true;
        m_bShowActionCard = false;
        m_selectedType = SLOT_TYPE_NONE;
        m_selectedIndex = -1;
        m_bDragging = false;
        ComputeLayout();
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileInventory::Close()
    {
        m_bIsOpen = false;
        m_bShowActionCard = false;
        m_selectedType = SLOT_TYPE_NONE;
        m_selectedIndex = -1;
        m_bDragging = false;
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileInventory::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    int CUIMobileInventory::HitTestSlot(float x, float y, SLOT_TYPE& outType) const
    {
        // 1. Test equipment slots
        for (int i = 0; i < 12; ++i)
        {
            const auto& s = m_equipSlots[i];
            if (x >= s.x && x <= s.x + s.w && y >= s.y && y <= s.y + s.h)
            {
                outType = SLOT_TYPE_EQUIPMENT;
                return s.slotIndex;
            }
        }
        // 2. Test bag slots
        for (int i = 0; i < 64; ++i)
        {
            const auto& s = m_bagSlots[i];
            if (x >= s.x && x <= s.x + s.w && y >= s.y && y <= s.y + s.h)
            {
                outType = SLOT_TYPE_BAG;
                return s.slotIndex;
            }
        }
        outType = SLOT_TYPE_NONE;
        return -1;
    }

    bool CUIMobileInventory::HitTestButton(float x, float y, float bx, float by, float bw, float bh) const
    {
        return (x >= bx && x <= bx + bw && y >= by && y <= by + bh);
    }

    ITEM* CUIMobileInventory::GetEquipItem(int equipIndex) const
    {
        if (!CharacterMachine || equipIndex < 0 || equipIndex >= MAX_EQUIPMENT_INDEX) return nullptr;
        ITEM* item = &CharacterMachine->Equipment[equipIndex];
        return (item && item->Type >= 0) ? item : nullptr;
    }

    ITEM* CUIMobileInventory::GetBagItem(int bagIndex) const
    {
        if (!g_pMyInventory || !g_pMyInventory->GetInventoryCtrl()) return nullptr;
        CNewUIInventoryCtrl* ctrl = g_pMyInventory->GetInventoryCtrl();
        return ctrl->FindItem(bagIndex);
    }

    bool CUIMobileInventory::IsConsumable(const ITEM* pItem) const
    {
        if (!pItem || pItem->Type < 0) return false;
        const int type = pItem->Type;
        // Potions: Apple, Small/Medium/Large Healing, Mana, Antidote, Alcohol
        if (type >= ITEM_POTION && type <= ITEM_POTION + 9) return true;
        // Complex / SD potions
        if (type >= ITEM_POTION + 35 && type <= ITEM_POTION + 40) return true;
        // Jewels: Bless, Soul, Chaos, Life, Creation, Harmony, Guardian
        if (type == ITEM_POTION + 13 || type == ITEM_POTION + 14 || type == ITEM_POTION + 15
            || type == ITEM_POTION + 16 || type == ITEM_POTION + 22 || type == ITEM_POTION + 31) return true;
        // Town portal, scrolls, boxes, ticket items
        if (type == ITEM_POTION + 10 || type == ITEM_POTION + 11 || (type >= ITEM_POTION + 17 && type <= ITEM_POTION + 21)) return true;
        return false;
    }

    bool CUIMobileInventory::IsEquipableItem(const ITEM* pItem) const
    {
        if (!pItem || pItem->Type < 0) return false;
        const int cat = pItem->Type / MAX_ITEM_INDEX;
        // Weapons (0..5), Shields (6), Helms (7), Armor (8), Pants (9), Gloves (10), Boots (11), Wings (12), Pets/Jewelry (13)
        return (cat >= 0 && cat <= 13 && !IsConsumable(pItem));
    }

    bool CUIMobileInventory::OnFingerDown(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        float tx = 0.0f, ty = 0.0f;
        TouchToVirtual(ev.x, ev.y, tx, ty);

        // 1. Check Close [X] button
        if (HitTestButton(tx, ty, m_closeBtnX - 4.0f, m_closeBtnY - 4.0f, m_closeBtnSize + 8.0f, m_closeBtnSize + 8.0f))
        {
            Close();
            return true;
        }

        // 2. Check Action Card Buttons if open
        if (m_bShowActionCard)
        {
            // Close Card button
            if (HitTestButton(tx, ty, m_btnCloseCardX, m_btnCloseCardY, m_btnCloseCardW, m_btnCloseCardH))
            {
                m_bShowActionCard = false;
                PlayBuffer(SOUND_CLICK01);
                return true;
            }

            // Equip / Unequip button
            if (HitTestButton(tx, ty, m_btnEquipX, m_btnEquipY, m_btnEquipW, m_btnEquipH))
            {
                if (m_selectedType == SLOT_TYPE_BAG)
                {
                    ExecuteEquip(m_selectedIndex);
                }
                else if (m_selectedType == SLOT_TYPE_EQUIPMENT)
                {
                    ExecuteUnequip(m_selectedIndex);
                }
                m_bShowActionCard = false;
                return true;
            }

            // Use button (if consumable)
            if (HitTestButton(tx, ty, m_btnUseX, m_btnUseY, m_btnUseW, m_btnUseH))
            {
                if (m_selectedType == SLOT_TYPE_BAG)
                {
                    ExecuteUse(m_selectedIndex);
                }
                m_bShowActionCard = false;
                return true;
            }

            // Drop button
            if (HitTestButton(tx, ty, m_btnDropX, m_btnDropY, m_btnDropW, m_btnDropH))
            {
                ExecuteDrop(m_selectedIndex, m_selectedType);
                m_bShowActionCard = false;
                return true;
            }

            // If tapped inside card area, consume touch so background doesn't activate
            if (HitTestButton(tx, ty, m_cardX, m_cardY, m_cardW, m_cardH))
            {
                return true;
            }
        }

        // 3. Test item slots in Equipment or Bag
        SLOT_TYPE hitType = SLOT_TYPE_NONE;
        const int hitIndex = HitTestSlot(tx, ty, hitType);

        if (hitIndex != -1)
        {
            const uint32_t now = SDL_GetTicks();
            ITEM* pItem = (hitType == SLOT_TYPE_BAG) ? GetBagItem(hitIndex) : GetEquipItem(hitIndex);

            // Double-Tap Quick Action Detector (< 300ms on same slot)
            if (pItem != nullptr && (now - m_lastTapTick <= 300)
                && (m_lastTapType == hitType) && (m_lastTapIndex == hitIndex))
            {
                // Quick execute!
                if (hitType == SLOT_TYPE_BAG)
                {
                    if (IsConsumable(pItem))
                    {
                        ExecuteUse(hitIndex);
                    }
                    else if (IsEquipableItem(pItem))
                    {
                        ExecuteEquip(hitIndex);
                    }
                }
                else if (hitType == SLOT_TYPE_EQUIPMENT)
                {
                    ExecuteUnequip(hitIndex);
                }

                m_lastTapTick = 0;
                m_bShowActionCard = false;
                m_bDragging = false;
                return true;
            }

            // Single Tap: Select and open Action Card if item exists
            m_lastTapTick  = now;
            m_lastTapType  = hitType;
            m_lastTapIndex = hitIndex;

            m_selectedType  = hitType;
            m_selectedIndex = hitIndex;
            m_bShowActionCard = (pItem != nullptr);

            // Prepare Drag and drop
            if (pItem != nullptr)
            {
                m_bDragging      = false;
                m_dragFingerId   = ev.fingerID;
                m_dragSrcType    = hitType;
                m_dragSrcIndex   = hitIndex;
                m_dragStartX     = tx;
                m_dragStartY     = ty;
                m_dragCurX       = tx;
                m_dragCurY       = ty;
                m_dragStartTick  = now;
                m_hoverDstIndex  = -1;
                m_hoverDstType   = SLOT_TYPE_NONE;
            }

            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        // If touched inside panel but outside slots, consume touch
        if (HitTestButton(tx, ty, m_panelX, m_panelY, m_panelW, m_panelH))
        {
            m_bShowActionCard = false;
            return true;
        }

        // Tap outside both panel and card -> close inventory
        Close();
        return true;
    }

    bool CUIMobileInventory::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        float tx = 0.0f, ty = 0.0f;
        TouchToVirtual(ev.x, ev.y, tx, ty);

        if (m_dragFingerId == ev.fingerID && m_dragSrcIndex != -1)
        {
            const float dx = tx - m_dragStartX;
            const float dy = ty - m_dragStartY;
            if (!m_bDragging && (dx * dx + dy * dy >= 100.0f)) // > 10px drag threshold
            {
                m_bDragging = true;
                m_bShowActionCard = false; // hide card during active drag
            }

            if (m_bDragging)
            {
                m_dragCurX = tx;
                m_dragCurY = ty;

                // Hit test potential drop destination
                m_hoverDstIndex = HitTestSlot(tx, ty - 40.0f, m_hoverDstType);
                return true;
            }
        }

        return false;
    }

    bool CUIMobileInventory::OnFingerUp(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        if (m_dragFingerId == ev.fingerID)
        {
            if (m_bDragging)
            {
                float tx = 0.0f, ty = 0.0f;
                TouchToVirtual(ev.x, ev.y, tx, ty);

                SLOT_TYPE dstType = SLOT_TYPE_NONE;
                const int dstIndex = HitTestSlot(tx, ty - 40.0f, dstType);

                if (dstIndex != -1)
                {
                    ExecuteMoveOrSwap(m_dragSrcType, m_dragSrcIndex, dstType, dstIndex);
                }

                m_bDragging = false;
                m_dragFingerId = 0;
                m_dragSrcIndex = -1;
                m_hoverDstIndex = -1;
                return true;
            }

            m_dragFingerId = 0;
            m_dragSrcIndex = -1;
        }

        return false;
    }

    void CUIMobileInventory::ExecuteEquip(int bagSlotIndex)
    {
        ITEM* pItem = GetBagItem(bagSlotIndex);
        if (!pItem) return;

        // Automatically determine target equipment slot for item category
        int targetSlot = -1;
        const int cat = pItem->Type / MAX_ITEM_INDEX;

        if (cat >= 0 && cat <= 5)
        {
            // Weapon -> check right hand, then left hand
            if (CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT].Type == -1) targetSlot = EQUIPMENT_WEAPON_RIGHT;
            else if (CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT].Type == -1) targetSlot = EQUIPMENT_WEAPON_LEFT;
            else targetSlot = EQUIPMENT_WEAPON_RIGHT; // replace main hand
        }
        else if (cat == 6) targetSlot = EQUIPMENT_WEAPON_LEFT; // Shield
        else if (cat == 7) targetSlot = EQUIPMENT_HELM;
        else if (cat == 8) targetSlot = EQUIPMENT_ARMOR;
        else if (cat == 9) targetSlot = EQUIPMENT_PANTS;
        else if (cat == 10) targetSlot = EQUIPMENT_GLOVES;
        else if (cat == 11) targetSlot = EQUIPMENT_BOOTS;
        else if (cat == 12) targetSlot = EQUIPMENT_WING;
        else if (cat == 13)
        {
            if (pItem->Type == ITEM_HELPER + 20 || pItem->Type == ITEM_HELPER + 21) // Rings
            {
                if (CharacterMachine->Equipment[EQUIPMENT_RING_RIGHT].Type == -1) targetSlot = EQUIPMENT_RING_RIGHT;
                else targetSlot = EQUIPMENT_RING_LEFT;
            }
            else if (pItem->Type >= ITEM_HELPER + 22 && pItem->Type <= ITEM_HELPER + 28) // Amulets
            {
                targetSlot = EQUIPMENT_AMULET;
            }
            else
            {
                targetSlot = EQUIPMENT_HELPER; // Pet / Demon / Angel
            }
        }

        if (targetSlot != -1)
        {
            SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, 12 + bagSlotIndex, pItem, REQUEST_EQUIPMENT_INVENTORY, targetSlot);
            PlayBuffer(SOUND_CLICK01);
        }
    }

    void CUIMobileInventory::ExecuteUnequip(int equipSlotIndex)
    {
        ITEM* pEquipped = GetEquipItem(equipSlotIndex);
        if (!pEquipped || !g_pMyInventory) return;

        const int emptySlot = g_pMyInventory->FindEmptySlot(pEquipped);
        if (emptySlot != -1)
        {
            SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, equipSlotIndex, pEquipped, REQUEST_EQUIPMENT_INVENTORY, 12 + emptySlot);
            PlayBuffer(SOUND_CLICK01);
        }
    }

    void CUIMobileInventory::ExecuteUse(int bagSlotIndex)
    {
        SendRequestUse(12 + bagSlotIndex, 0);
        PlayBuffer(SOUND_DRINK01);
    }

    void CUIMobileInventory::ExecuteDrop(int slotIndex, SLOT_TYPE slotType)
    {
        if (!Hero) return;
        const int packetSlot = (slotType == SLOT_TYPE_BAG) ? (12 + slotIndex) : slotIndex;
        const int dropX = static_cast<int>(Hero->PositionX);
        const int dropY = static_cast<int>(Hero->PositionY);
        SendRequestDropItem(packetSlot, dropX, dropY);
        PlayBuffer(SOUND_DROP_ITEM01);
    }

    void CUIMobileInventory::ExecuteMoveOrSwap(SLOT_TYPE srcType, int srcIndex, SLOT_TYPE dstType, int dstIndex)
    {
        if (srcType == dstType && srcIndex == dstIndex) return;

        ITEM* pSrcItem = (srcType == SLOT_TYPE_BAG) ? GetBagItem(srcIndex) : GetEquipItem(srcIndex);
        if (!pSrcItem) return;

        const int sSlot = (srcType == SLOT_TYPE_BAG) ? (12 + srcIndex) : srcIndex;
        const int dSlot = (dstType == SLOT_TYPE_BAG) ? (12 + dstIndex) : dstIndex;

        SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, sSlot, pSrcItem, REQUEST_EQUIPMENT_INVENTORY, dSlot);
        PlayBuffer(SOUND_CLICK01);
    }

    bool CUIMobileInventory::Update()
    {
        if (!m_bIsOpen) return true;
        ComputeLayout();
        return true;
    }

    bool CUIMobileInventory::UpdateKeyEvent()
    {
        if (!m_bIsOpen) return true;
        if (IsPress(VK_ESCAPE) || IsPress('I'))
        {
            Close();
            return false;
        }
        return true;
    }

    bool CUIMobileInventory::Render()
    {
        if (!m_bIsOpen) return true;

        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        RenderBackdrop();
        RenderHeaderAndZen();
        RenderEquipmentSlots();
        RenderBagGrid();

        if (m_bShowActionCard)
        {
            RenderActionCard();
        }

        if (m_bDragging)
        {
            RenderDraggedItem();
        }

        DisableAlphaBlend();
        return true;
    }

    void CUIMobileInventory::Render3D()
    {
        if (!m_bIsOpen) return;

        // Render 3D items inside equipment slots
        for (int i = 0; i < 12; ++i)
        {
            ITEM* pItem = GetEquipItem(i);
            if (pItem)
            {
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                RenderItem3D(
                    m_equipSlots[i].x + 2.0f,
                    m_equipSlots[i].y + 2.0f,
                    m_equipSlots[i].w - 4.0f,
                    m_equipSlots[i].h - 4.0f,
                    pItem->Type,
                    pItem->Level,
                    pItem->Option1,
                    pItem->ExtOption,
                    false
                );
            }
        }

        // Render 3D items inside bag grid
        if (g_pMyInventory && g_pMyInventory->GetInventoryCtrl())
        {
            CNewUIInventoryCtrl* ctrl = g_pMyInventory->GetInventoryCtrl();
            const size_t numItems = ctrl->GetNumberOfItems();
            for (size_t i = 0; i < numItems; ++i)
            {
                ITEM* pItem = ctrl->GetItem(static_cast<int>(i));
                if (pItem && pItem->lineal_pos >= 0 && pItem->lineal_pos < 64)
                {
                    const auto& s = m_bagSlots[pItem->lineal_pos];
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

        // 3D Item preview in Action Card if open
        if (m_bShowActionCard)
        {
            ITEM* pSel = (m_selectedType == SLOT_TYPE_BAG) ? GetBagItem(m_selectedIndex) : GetEquipItem(m_selectedIndex);
            if (pSel)
            {
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                RenderItem3D(
                    m_cardX + (m_cardW * 0.5f) - 30.0f,
                    m_cardY + 36.0f,
                    60.0f,
                    60.0f,
                    pSel->Type,
                    pSel->Level,
                    pSel->Option1,
                    pSel->ExtOption,
                    false
                );
            }
        }
    }

    void CUIMobileInventory::RenderBackdrop()
    {
        // Dark translucent panel background
        glColor4f(0.05f, 0.07f, 0.12f, 0.90f);
        RenderColor(m_panelX, m_panelY, m_panelW, m_panelH);
        EndRenderColor();

        // Elegant gold/blue border around panel
        glColor4f(0.85f, 0.65f, 0.20f, 0.85f);
        RenderColor(m_panelX, m_panelY, m_panelW, 2.0f);
        RenderColor(m_panelX, m_panelY + m_panelH - 2.0f, m_panelW, 2.0f);
        RenderColor(m_panelX, m_panelY, 2.0f, m_panelH);
        RenderColor(m_panelX + m_panelW - 2.0f, m_panelY, 2.0f, m_panelH);
        EndRenderColor();

        // Close button [X]
        glColor4f(0.8f, 0.2f, 0.2f, 0.85f);
        RenderColor(m_closeBtnX, m_closeBtnY, m_closeBtnSize, m_closeBtnSize);
        EndRenderColor();

        if (g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(255, 255, 255, 255);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->RenderText(m_closeBtnX + 8.0f, m_closeBtnY + 6.0f, "X");
        }
    }

    void CUIMobileInventory::RenderHeaderAndZen()
    {
        if (!g_pRenderText) return;

        // Title
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(255, 204, 25, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->RenderText(m_panelX + 16.0f, m_panelY + 12.0f, "HÒM ĐỒ & TRANG BỊ");

        // Zen display
        if (CharacterMachine)
        {
            char zenStr[64];
            std::snprintf(zenStr, sizeof(zenStr), "Zen: %u", CharacterMachine->Gold);
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetTextColor(255, 230, 100, 255);
            g_pRenderText->RenderText(m_panelX + 16.0f, m_panelY + m_panelH - 20.0f, zenStr);
        }
    }

    void CUIMobileInventory::RenderEquipmentSlots()
    {
        for (int i = 0; i < 12; ++i)
        {
            const auto& s = m_equipSlots[i];

            // Slot background
            glColor4f(0.12f, 0.16f, 0.24f, 0.85f);
            RenderColor(s.x, s.y, s.w, s.h);
            EndRenderColor();

            // Border (highlight if selected or hover destination)
            if (m_selectedType == SLOT_TYPE_EQUIPMENT && m_selectedIndex == i)
            {
                glColor4f(1.0f, 0.85f, 0.10f, 1.0f); // Bright Gold selection
            }
            else if (m_bDragging && m_hoverDstType == SLOT_TYPE_EQUIPMENT && m_hoverDstIndex == i)
            {
                glColor4f(0.2f, 0.9f, 0.2f, 1.0f); // Green drop hover
            }
            else
            {
                glColor4f(0.35f, 0.45f, 0.60f, 0.6f); // Normal slot border
            }

            RenderColor(s.x, s.y, s.w, 1.5f);
            RenderColor(s.x, s.y + s.h - 1.5f, s.w, 1.5f);
            RenderColor(s.x, s.y, 1.5f, s.h);
            RenderColor(s.x + s.w - 1.5f, s.y, 1.5f, s.h);
            EndRenderColor();
        }
    }

    void CUIMobileInventory::RenderBagGrid()
    {
        for (int i = 0; i < 64; ++i)
        {
            const auto& s = m_bagSlots[i];

            // Slot background
            glColor4f(0.08f, 0.12f, 0.18f, 0.80f);
            RenderColor(s.x, s.y, s.w, s.h);
            EndRenderColor();

            // Border
            if (m_selectedType == SLOT_TYPE_BAG && m_selectedIndex == i)
            {
                glColor4f(1.0f, 0.85f, 0.10f, 1.0f); // Gold selection
            }
            else if (m_bDragging && m_hoverDstType == SLOT_TYPE_BAG && m_hoverDstIndex == i)
            {
                glColor4f(0.2f, 0.9f, 0.2f, 1.0f); // Green drop hover
            }
            else
            {
                glColor4f(0.25f, 0.35f, 0.48f, 0.5f);
            }

            RenderColor(s.x, s.y, s.w, 1.0f);
            RenderColor(s.x, s.y + s.h - 1.0f, s.w, 1.0f);
            RenderColor(s.x, s.y, 1.0f, s.h);
            RenderColor(s.x + s.w - 1.0f, s.y, 1.0f, s.h);
            EndRenderColor();
        }
    }

    void CUIMobileInventory::RenderActionCard()
    {
        ITEM* pItem = (m_selectedType == SLOT_TYPE_BAG) ? GetBagItem(m_selectedIndex) : GetEquipItem(m_selectedIndex);
        if (!pItem) return;

        // Card backdrop
        glColor4f(0.06f, 0.08f, 0.14f, 0.95f);
        RenderColor(m_cardX, m_cardY, m_cardW, m_cardH);
        EndRenderColor();

        // Card glowing border
        glColor4f(0.3f, 0.7f, 1.0f, 0.85f);
        RenderColor(m_cardX, m_cardY, m_cardW, 2.0f);
        RenderColor(m_cardX, m_cardY + m_cardH - 2.0f, m_cardW, 2.0f);
        RenderColor(m_cardX, m_cardY, 2.0f, m_cardH);
        RenderColor(m_cardX + m_cardW - 2.0f, m_cardY, 2.0f, m_cardH);
        EndRenderColor();

        if (g_pRenderText)
        {
            ITEM_ATTRIBUTE* pAttr = &ItemAttribute[pItem->Type];
            const int itemLevel = (pItem->Level >> 3) & 15;

            // Item Name
            char nameBuf[128];
            if (itemLevel > 0)
                std::snprintf(nameBuf, sizeof(nameBuf), "%s +%d", pAttr->Name, itemLevel);
            else
                std::snprintf(nameBuf, sizeof(nameBuf), "%s", pAttr->Name);

            g_pRenderText->SetFont(g_hFontBold);
            if (pItem->ExtOption > 0)
                g_pRenderText->SetTextColor(100, 255, 100, 255); // Excellent Green
            else
                g_pRenderText->SetTextColor(255, 255, 255, 255);

            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->RenderText(m_cardX + 16.0f, m_cardY + 12.0f, nameBuf);

            // Durability
            char durBuf[64];
            std::snprintf(durBuf, sizeof(durBuf), "Độ bền: %d/%d", pItem->Durability, calcMaxDurability(pItem, pAttr, itemLevel));
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetTextColor(200, 200, 200, 255);
            g_pRenderText->RenderText(m_cardX + 16.0f, m_cardY + 105.0f, durBuf);

            // Requirements & Stats hints
            if (pAttr->RequireStrength > 0)
            {
                char reqBuf[64];
                std::snprintf(reqBuf, sizeof(reqBuf), "Sức mạnh yêu cầu: %d", pAttr->RequireStrength);
                g_pRenderText->RenderText(m_cardX + 16.0f, m_cardY + 125.0f, reqBuf);
            }
            if (pAttr->RequireDexterity > 0)
            {
                char reqBuf[64];
                std::snprintf(reqBuf, sizeof(reqBuf), "Nhanh nhẹn yêu cầu: %d", pAttr->RequireDexterity);
                g_pRenderText->RenderText(m_cardX + 16.0f, m_cardY + 145.0f, reqBuf);
            }
        }

        // Action Buttons
        // 1. Equip / Unequip button
        glColor4f(0.15f, 0.45f, 0.85f, 0.90f);
        RenderColor(m_btnEquipX, m_btnEquipY, m_btnEquipW, m_btnEquipH);
        EndRenderColor();

        // 2. Use button (if consumable)
        if (IsConsumable(pItem))
        {
            glColor4f(0.15f, 0.75f, 0.35f, 0.90f);
            RenderColor(m_btnUseX, m_btnUseY, m_btnUseW, m_btnUseH);
            EndRenderColor();
        }

        // 3. Drop button
        glColor4f(0.75f, 0.25f, 0.25f, 0.90f);
        RenderColor(m_btnDropX, m_btnDropY, m_btnDropW, m_btnDropH);
        EndRenderColor();

        // 4. Close card button
        glColor4f(0.35f, 0.35f, 0.40f, 0.90f);
        RenderColor(m_btnCloseCardX, m_btnCloseCardY, m_btnCloseCardW, m_btnCloseCardH);
        EndRenderColor();

        // Render button labels
        if (g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(255, 255, 255, 255);
            g_pRenderText->SetBgColor(0, 0, 0, 0);

            const char* eqLabel = (m_selectedType == SLOT_TYPE_BAG) ? "Trang Bị" : "Tháo Đồ";
            g_pRenderText->RenderText(m_btnEquipX + 18.0f, m_btnEquipY + 9.0f, eqLabel);

            if (IsConsumable(pItem))
            {
                g_pRenderText->RenderText(m_btnUseX + 22.0f, m_btnUseY + 9.0f, "Dùng");
            }

            g_pRenderText->RenderText(m_btnDropX + 24.0f, m_btnDropY + 9.0f, "Vứt");
            g_pRenderText->RenderText(m_btnCloseCardX + 20.0f, m_btnCloseCardY + 9.0f, "Đóng");
        }
    }

    void CUIMobileInventory::RenderDraggedItem()
    {
        ITEM* pItem = (m_dragSrcType == SLOT_TYPE_BAG) ? GetBagItem(m_dragSrcIndex) : GetEquipItem(m_dragSrcIndex);
        if (!pItem) return;

        // Render dragging halo with vertical offset of -40px (above finger tip)
        const float renderX = m_dragCurX - 24.0f;
        const float renderY = m_dragCurY - 40.0f - 24.0f;

        glColor4f(0.2f, 0.8f, 1.0f, 0.5f);
        RenderColor(renderX, renderY, 48.0f, 48.0f);
        EndRenderColor();
    }
}

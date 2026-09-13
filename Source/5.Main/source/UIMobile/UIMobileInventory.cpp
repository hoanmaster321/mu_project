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
#include "NewUIInventoryExtension.h"
#include "UIMobileInventoryExtension.h"
#include "NewUIMyShopInventory.h"
#include "CharacterManager.h"
#include "ZzzInventory.h"
#include "DSPlaySound.h"
#include "ProtocolSend.h"
#include "wsclientinline.h"
#include "ZzzInfomation.h"

#include <algorithm>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstring>
#if defined(__ANDROID__)
#include <android/log.h>
#endif

extern int DisplayWin;
extern int DisplayHeight;
extern float g_fScreenRate_x;
extern float g_fScreenRate_y;
extern int MouseX;
extern int MouseY;
extern CHARACTER* Hero;
extern float CacheY;
extern DWORD CacheTimeRenterTip1;
extern float g_fTooltipMaxRight;
extern float g_fTooltipActualLeft;
extern float g_fTooltipActualTop;
extern float g_fTooltipActualWidth;
extern float g_fTooltipActualHeight;

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
        , m_bagSlotSize(24.0f), m_bagStartX(0.0f), m_bagStartY(0.0f)
        , m_btnEquipX(0.0f), m_btnEquipY(0.0f), m_btnEquipW(0.0f), m_btnEquipH(0.0f)
        , m_btnUseX(0.0f), m_btnUseY(0.0f), m_btnUseW(0.0f), m_btnUseH(0.0f)
        , m_btnDropX(0.0f), m_btnDropY(0.0f), m_btnDropW(0.0f), m_btnDropH(0.0f)
        , m_btnCloseCardX(0.0f), m_btnCloseCardY(0.0f), m_btnCloseCardW(0.0f), m_btnCloseCardH(0.0f)
        , m_btnRepairX(0.0f), m_btnRepairY(0.0f), m_btnRepairW(0.0f), m_btnRepairH(0.0f)
        , m_btnExpandX(0.0f), m_btnExpandY(0.0f), m_btnExpandW(0.0f), m_btnExpandH(0.0f)
        , m_btnMyShopX(0.0f), m_btnMyShopY(0.0f), m_btnMyShopW(0.0f), m_btnMyShopH(0.0f)
        , m_texBackdrop(0)
        , m_texSlotBg(0)
        , m_texSlotActive(0)
        , m_texActionCardBg(0)
        , m_texBtnNormal(0)
        , m_texBtnPressed(0)
        , m_texBtnRepair(0)
        , m_texBtnExpand(0)
        , m_texBtnMyShop(0)
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
        if (!pNewUIMng || !pNewUI3DRenderMng) return false;

        m_pNewUIMng = pNewUIMng;
        m_pNewUI3DRenderMng = pNewUI3DRenderMng;

        m_pNewUIMng->AddUIObj(INTERFACE_INVENTORY, this);
        m_pNewUI3DRenderMng->Add3DRenderObj(this, INVENTORY_CAMERA_Z_ORDER);

        EnsureTextures();
        ComputeLayout();

        Show(false);
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
        // Silhouette icons for paperdoll equipment slots
        m_equipSilhouettes[EQUIPMENT_WEAPON_RIGHT] = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 9;  // newui_item_weapon(L).tga
        m_equipSilhouettes[EQUIPMENT_WEAPON_LEFT]  = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 10; // newui_item_weapon(R).tga
        m_equipSilhouettes[EQUIPMENT_HELM]         = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 6;  // newui_item_cap.tga
        m_equipSilhouettes[EQUIPMENT_ARMOR]        = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 11; // newui_item_upper.tga
        m_equipSilhouettes[EQUIPMENT_PANTS]        = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 13; // newui_item_lower.tga
        m_equipSilhouettes[EQUIPMENT_GLOVES]       = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 12; // newui_item_gloves.tga
        m_equipSilhouettes[EQUIPMENT_BOOTS]        = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 5;  // newui_item_boots.tga
        m_equipSilhouettes[EQUIPMENT_WING]         = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 8;  // newui_item_wing.tga
        m_equipSilhouettes[EQUIPMENT_HELPER]       = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 7;  // newui_item_fairy.tga
        m_equipSilhouettes[EQUIPMENT_AMULET]       = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 15; // newui_item_necklace.tga
        m_equipSilhouettes[EQUIPMENT_RING_RIGHT]   = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 14; // newui_item_ring.tga
        m_equipSilhouettes[EQUIPMENT_RING_LEFT]    = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 14; // newui_item_ring.tga

        m_texBackdrop     = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN;     // newui_item_back01.tga
        m_texSlotBg       = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN;        // newui_item_box.tga
        m_texSlotActive   = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN + 1;    // table highlight
        m_texActionCardBg = BITMAP_INTERFACE_NEW_MESSAGEBOX_BEGIN;            // newui_msgbox_back.jpg

        // Utility Buttons (PC MU Season 6 personal inventory buttons)
        m_texBtnRepair    = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 18; // newui_repair_00.tga
        m_texBtnExpand    = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 19; // newui_expansion_btn.tga
        m_texBtnMyShop    = BITMAP_MYSHOPINTERFACE_NEW_PERSONALINVENTORY_BEGIN + 1; // newui_Bt_openshop.tga
    }

    void CUIMobileInventory::ComputeLayout()
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        // 1. Right-side panel (full height mobile panel docked to right edge with 26px margin to avoid curved glass edges)
        m_panelW = 280.0f;
        m_panelH = 464.0f;
        m_panelX = winW - m_panelW - 26.0f;
        m_panelY = (winH - m_panelH) * 0.5f;

        // Close button (top right of panel, inset so curved screen/notch doesn't clip)
        m_closeBtnSize = 24.0f;
        m_closeBtnX = m_panelX + m_panelW - m_closeBtnSize - 10.0f;
        m_closeBtnY = m_panelY + 7.0f;

        // 2. Equipment slots layout (Paperdoll layout shifted to the left, enlarged slots)
        const float paperCenterX = m_panelX + 104.0f;
        const float paperTopY    = m_panelY + 34.0f;
        const float eqSize       = 28.0f;

        // Helper / Pet (Top-Left)
        m_equipSlots[EQUIPMENT_HELPER] = { paperCenterX - 86.0f, paperTopY, eqSize, eqSize, EQUIPMENT_HELPER, SLOT_TYPE_EQUIPMENT };
        // Helm (Top-Center)
        m_equipSlots[EQUIPMENT_HELM]   = { paperCenterX - (eqSize * 0.5f), paperTopY, eqSize, eqSize, EQUIPMENT_HELM, SLOT_TYPE_EQUIPMENT };
        // Wing (Top-Right)
        m_equipSlots[EQUIPMENT_WING]   = { paperCenterX + 50.0f, paperTopY, 44.0f, eqSize, EQUIPMENT_WING, SLOT_TYPE_EQUIPMENT };

        // Weapon Right (Mid-Left)
        m_equipSlots[EQUIPMENT_WEAPON_RIGHT] = { paperCenterX - 86.0f, paperTopY + 32.0f, eqSize, 42.0f, EQUIPMENT_WEAPON_RIGHT, SLOT_TYPE_EQUIPMENT };
        // Armor (Mid-Center)
        m_equipSlots[EQUIPMENT_ARMOR]        = { paperCenterX - (eqSize * 0.5f), paperTopY + 32.0f, eqSize, 38.0f, EQUIPMENT_ARMOR, SLOT_TYPE_EQUIPMENT };
        // Weapon Left / Shield (Mid-Right)
        m_equipSlots[EQUIPMENT_WEAPON_LEFT]  = { paperCenterX + 58.0f, paperTopY + 32.0f, eqSize, 42.0f, EQUIPMENT_WEAPON_LEFT, SLOT_TYPE_EQUIPMENT };

        // Gloves (Low-Left)
        m_equipSlots[EQUIPMENT_GLOVES] = { paperCenterX - 86.0f, paperTopY + 78.0f, eqSize, eqSize, EQUIPMENT_GLOVES, SLOT_TYPE_EQUIPMENT };
        // Pants (Low-Center)
        m_equipSlots[EQUIPMENT_PANTS]  = { paperCenterX - (eqSize * 0.5f), paperTopY + 74.0f, eqSize, 36.0f, EQUIPMENT_PANTS, SLOT_TYPE_EQUIPMENT };
        // Boots (Low-Right)
        m_equipSlots[EQUIPMENT_BOOTS]  = { paperCenterX + 58.0f, paperTopY + 78.0f, eqSize, eqSize, EQUIPMENT_BOOTS, SLOT_TYPE_EQUIPMENT };

        // Jewelry row
        const float jewY = paperTopY + 118.0f;
        const float jewSize = 22.0f;
        m_equipSlots[EQUIPMENT_RING_RIGHT] = { paperCenterX - 56.0f, jewY, jewSize, jewSize, EQUIPMENT_RING_RIGHT, SLOT_TYPE_EQUIPMENT };
        m_equipSlots[EQUIPMENT_AMULET]     = { paperCenterX - (jewSize * 0.5f), jewY, jewSize, jewSize, EQUIPMENT_AMULET, SLOT_TYPE_EQUIPMENT };
        m_equipSlots[EQUIPMENT_RING_LEFT]  = { paperCenterX + 34.0f, jewY, jewSize, jewSize, EQUIPMENT_RING_LEFT, SLOT_TYPE_EQUIPMENT };

        // Utility Buttons (Repair All, Expand Inventory, Personal Shop) beside paperdoll
        const float utilBtnX = m_panelX + 204.0f;
        const float utilBtnW = 64.0f;
        const float utilBtnH = 34.0f;

        m_btnRepairX = utilBtnX;
        m_btnRepairY = paperTopY + 4.0f;
        m_btnRepairW = utilBtnW;
        m_btnRepairH = utilBtnH;

        m_btnExpandX = utilBtnX;
        m_btnExpandY = m_btnRepairY + utilBtnH + 6.0f;
        m_btnExpandW = utilBtnW;
        m_btnExpandH = utilBtnH;

        m_btnMyShopX = utilBtnX;
        m_btnMyShopY = m_btnExpandY + utilBtnH + 6.0f;
        m_btnMyShopW = utilBtnW;
        m_btnMyShopH = utilBtnH;

        // 3. Bag Grid layout (8 cols x 8 rows = 64 slots, 33px slot size, 264x264 total)
        m_bagSlotSize = 33.0f;
        m_bagStartX   = m_panelX + ((m_panelW - (8.0f * m_bagSlotSize)) * 0.5f);
        m_bagStartY   = m_panelY + 172.0f;

        for (int row = 0; row < 8; ++row)
        {
            for (int col = 0; col < 8; ++col)
            {
                const int idx = row * 8 + col;
                m_bagSlots[idx].x = m_bagStartX + (col * m_bagSlotSize);
                m_bagSlots[idx].y = m_bagStartY + (row * m_bagSlotSize);
                m_bagSlots[idx].w = m_bagSlotSize;
                m_bagSlots[idx].h = m_bagSlotSize;
                m_bagSlots[idx].slotIndex = idx;
                m_bagSlots[idx].slotType = SLOT_TYPE_BAG;
            }
        }

        // Action Card default bounds
        m_cardW = 160.0f;
        m_cardH = 260.0f;
        m_cardX = (std::max)(8.0f, m_panelX - m_cardW - 8.0f);
        m_cardY = m_panelY + 10.0f;

        const float btnW = (m_cardW - 28.0f) * 0.5f;
        const float btnH = 22.0f;
        const float row2Y = m_cardY + m_cardH - btnH - 10.0f;
        const float row1Y = row2Y - btnH - 6.0f;

        m_btnEquipX = m_cardX + 10.0f;
        m_btnEquipY = row1Y;
        m_btnEquipW = btnW;
        m_btnEquipH = btnH;

        m_btnUseX = m_btnEquipX;
        m_btnUseY = row1Y;
        m_btnUseW = btnW;
        m_btnUseH = btnH;

        m_btnDropX = m_cardX + 10.0f + btnW + 8.0f;
        m_btnDropY = row1Y;
        m_btnDropW = btnW;
        m_btnDropH = btnH;

        m_btnCloseCardX = m_cardX + 10.0f;
        m_btnCloseCardY = row2Y;
        m_btnCloseCardW = m_cardW - 20.0f;
        m_btnCloseCardH = btnH;

        if (g_pMyInventoryExt)
        {
            const int extX = static_cast<int>(m_panelX - 280.0f - 8.0f);
            g_pMyInventoryExt->SetPos((extX < 8) ? 8 : extX, static_cast<int>(m_panelY));
        }
    }

    void CUIMobileInventory::Open()
    {
        m_bIsOpen = true;
        m_bShowActionCard = false;
        m_selectedType = SLOT_TYPE_NONE;
        m_selectedIndex = -1;
        m_bDragging = false;
        CNewUIInventoryCtrl::DeletePickedItem();
        ComputeLayout();
        PlayBuffer(SOUND_CLICK01);

#if defined(__ANDROID__)
        if (CharacterMachine)
        {
            for (int i = 0; i < 12; ++i)
            {
                if (CharacterMachine->Equipment[i].Type >= 0)
                {
                    __android_log_print(ANDROID_LOG_INFO, "MuMain", "[MobileInven] Equip[%d]: Type=%d Level=%d", i,
                        CharacterMachine->Equipment[i].Type,
                        CharacterMachine->Equipment[i].Level);
                }
            }
        }
        if (g_pMyInventory && g_pMyInventory->GetInventoryCtrl())
        {
            CNewUIInventoryCtrl* ctrl = g_pMyInventory->GetInventoryCtrl();
            __android_log_print(ANDROID_LOG_INFO, "MuMain", "[MobileInven] Bag count=%zu", ctrl->GetNumberOfItems());
            for (size_t i = 0; i < ctrl->GetNumberOfItems(); ++i)
            {
                ITEM* p = ctrl->GetItem(static_cast<int>(i));
                if (p)
                {
                    __android_log_print(ANDROID_LOG_INFO, "MuMain", "[MobileInven] BagItem[%zu]: Type=%d (col=%d, row=%d)", i, p->Type, p->x, p->y);
                }
            }
        }
#endif
    }

    void CUIMobileInventory::Close()
    {
        m_bIsOpen = false;
        m_bShowActionCard = false;
        m_selectedType = SLOT_TYPE_NONE;
        m_selectedIndex = -1;
        m_bDragging = false;
        if (CUIMobileInventoryExtension::GetInstance() && CUIMobileInventoryExtension::GetInstance()->IsOpen())
        {
            CUIMobileInventoryExtension::GetInstance()->Close();
        }
        if (g_pNewUISystem)
        {
            if (g_pNewUISystem->IsVisible(INTERFACE_MYSHOP_INVENTORY))
            {
                g_pNewUISystem->Hide(INTERFACE_MYSHOP_INVENTORY);
                g_pNewUISystem->Hide(INTERFACE_PURCHASESHOP_INVENTORY);
            }
            if (g_pNewUISystem->IsVisible(INTERFACE_ExpandInventory))
            {
                g_pNewUISystem->Hide(INTERFACE_ExpandInventory);
            }
        }
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileInventory::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    int CUIMobileInventory::HitTestSlot(float x, float y, SLOT_TYPE& outType, bool bForDrop) const
    {
        // 1. Test equipment slots with 2px touch tolerance
        for (int i = 0; i < 12; ++i)
        {
            const auto& s = m_equipSlots[i];
            if (x >= s.x - 2.0f && x <= s.x + s.w + 2.0f && y >= s.y - 2.0f && y <= s.y + s.h + 2.0f)
            {
                outType = SLOT_TYPE_EQUIPMENT;
                return s.slotIndex;
            }
        }
        // 2. Test bag slots with 2px touch tolerance and coordinate clamping
        const float bagGridW = 8.0f * m_bagSlotSize;
        const float bagGridH = 8.0f * m_bagSlotSize;
        if (x >= m_bagStartX - 2.0f && x < m_bagStartX + bagGridW + 2.0f &&
            y >= m_bagStartY - 2.0f && y < m_bagStartY + bagGridH + 2.0f)
        {
            int col = static_cast<int>((x - m_bagStartX) / m_bagSlotSize);
            int row = static_cast<int>((y - m_bagStartY) / m_bagSlotSize);
            if (col < 0) col = 0;
            if (col > 7) col = 7;
            if (row < 0) row = 0;
            if (row > 7) row = 7;

            const int cellIndex = row * 8 + col;
            outType = SLOT_TYPE_BAG;

            if (bForDrop)
            {
                // When dropping, always target the exact grid cell (0..63)
                return cellIndex;
            }

            ITEM* pItem = GetBagItem(cellIndex);
            if (pItem)
            {
                return (pItem->y * 8 + pItem->x);
            }
            return cellIndex;
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
        if (bagIndex < 0 || bagIndex >= 64) return nullptr;

        const int col = bagIndex % 8;
        const int row = bagIndex / 8;

        const size_t numItems = ctrl->GetNumberOfItems();
        for (size_t i = 0; i < numItems; ++i)
        {
            ITEM* pItem = ctrl->GetItem(static_cast<int>(i));
            if (!pItem || pItem->Type < 0) continue;
            const ITEM_ATTRIBUTE* pAttr = (ItemAttribute != nullptr) ? &ItemAttribute[pItem->Type] : nullptr;
            const int w = (pAttr && pAttr->Width > 0) ? pAttr->Width : 1;
            const int h = (pAttr && pAttr->Height > 0) ? pAttr->Height : 1;
            if (col >= pItem->x && col < pItem->x + w &&
                row >= pItem->y && row < pItem->y + h)
            {
                return pItem;
            }
        }

        return ctrl->FindItem(bagIndex + 12);
    }


    bool CUIMobileInventory::IsConsumable(const ITEM* pItem) const
    {
        if (!pItem || pItem->Type < 0 || pItem->Type >= MAX_ITEM) return false;
        const int type = pItem->Type;
        // Potions: Apple, Small/Medium/Large Healing, Mana, Antidote, Alcohol
        if (type >= ITEM_POTION && type <= ITEM_POTION + 9) return true;
        // Complex / SD potions
        if (type >= ITEM_POTION + 35 && type <= ITEM_POTION + 40) return true;
        // Jewels: Bless (14,13), Soul (14,14), Chaos (12,15), Life (14,16), Creation (14,22), Guardian (14,31), Harmony (14,42)
        if (type == ITEM_WING + 15) return true;
        if (type == ITEM_POTION + 13 || type == ITEM_POTION + 14 || type == ITEM_POTION + 15
            || type == ITEM_POTION + 16 || type == ITEM_POTION + 22 || type == ITEM_POTION + 31 || type == ITEM_POTION + 42) return true;
        // Town portal, scrolls, boxes, ticket items
        if (type == ITEM_POTION + 10 || type == ITEM_POTION + 11 || (type >= ITEM_POTION + 17 && type <= ITEM_POTION + 21)) return true;
        return false;
    }

    bool CUIMobileInventory::IsEquipableItem(const ITEM* pItem) const
    {
        if (!pItem || pItem->Type < 0 || pItem->Type >= MAX_ITEM) return false;
        if (IsConsumable(pItem)) return false;
        const ITEM_ATTRIBUTE* pAttr = &ItemAttribute[pItem->Type];
        return (pAttr->m_byItemSlot < MAX_EQUIPMENT_INDEX);
    }

    bool CUIMobileInventory::OnFingerDown(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        if (CUIMobileInventoryExtension::GetInstance() && CUIMobileInventoryExtension::GetInstance()->IsOpen())
        {
            if (CUIMobileInventoryExtension::GetInstance()->OnFingerDown(ev))
                return true;
        }

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

            // Equip / Unequip / Use button
            if (HitTestButton(tx, ty, m_btnEquipX, m_btnEquipY, m_btnEquipW, m_btnEquipH))
            {
#if defined(__ANDROID__)
                __android_log_print(ANDROID_LOG_INFO, "MuMain", "[MobileInven] Tap Equip/Unequip/Use button: selType=%d selIdx=%d",
                    (int)m_selectedType, m_selectedIndex);
#endif
                if (m_selectedType == SLOT_TYPE_BAG)
                {
                    ITEM* pItem = GetBagItem(m_selectedIndex);
                    if (pItem && IsEquipableItem(pItem))
                    {
                        if (IsRequireEquipItem(pItem))
                        {
                            ExecuteEquip(m_selectedIndex);
                            m_bShowActionCard = false;
                            m_selectedType = SLOT_TYPE_NONE;
                            m_selectedIndex = -1;
                        }
                        else
                        {
                            PlayBuffer(SOUND_CLICK01);
                        }
                    }
                    else if (pItem && IsConsumable(pItem))
                    {
                        ExecuteUse(m_selectedIndex);
                        m_bShowActionCard = false;
                        m_selectedType = SLOT_TYPE_NONE;
                        m_selectedIndex = -1;
                    }
                }
                else if (m_selectedType == SLOT_TYPE_EQUIPMENT)
                {
                    ExecuteUnequip(m_selectedIndex);
                    m_bShowActionCard = false;
                    m_selectedType = SLOT_TYPE_NONE;
                    m_selectedIndex = -1;
                }
                return true;
            }

            // Drop button
            if (HitTestButton(tx, ty, m_btnDropX, m_btnDropY, m_btnDropW, m_btnDropH))
            {
                ExecuteDrop(m_selectedIndex, m_selectedType);
                m_bShowActionCard = false;
                m_selectedType = SLOT_TYPE_NONE;
                m_selectedIndex = -1;
                return true;
            }

            // If tapped inside card area, consume touch so background doesn't activate
            if (HitTestButton(tx, ty, m_cardX, m_cardY, m_cardW, m_cardH))
            {
                return true;
            }
        }

        // 3. Check Utility Buttons (Repair All, Expand, Personal Shop)
        if (HitTestButton(tx, ty, m_btnRepairX, m_btnRepairY, m_btnRepairW, m_btnRepairH))
        {
            ExecuteRepairAll();
            return true;
        }

        if (HitTestButton(tx, ty, m_btnExpandX, m_btnExpandY, m_btnExpandW, m_btnExpandH))
        {
            if (CUIMobileInventoryExtension::GetInstance())
            {
                CUIMobileInventoryExtension::GetInstance()->Toggle();
            }
            return true;
        }

        if (HitTestButton(tx, ty, m_btnMyShopX, m_btnMyShopY, m_btnMyShopW, m_btnMyShopH))
        {
            if (g_pNewUISystem)
            {
                if (g_pNewUISystem->IsVisible(INTERFACE_MYSHOP_INVENTORY))
                {
                    g_pNewUISystem->Hide(INTERFACE_MYSHOP_INVENTORY);
                    g_pNewUISystem->Hide(INTERFACE_PURCHASESHOP_INVENTORY);
                }
                else
                {
                    g_pNewUISystem->Show(INTERFACE_MYSHOP_INVENTORY);
                }
                PlayBuffer(SOUND_CLICK01);
            }
            return true;
        }

        // 4. Test item slots in Equipment or Bag
        SLOT_TYPE hitType = SLOT_TYPE_NONE;
        const int hitIndex = HitTestSlot(tx, ty, hitType);

#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_INFO, "MuMain", "[MobileInven] Touch: (%.1f, %.1f) -> hitType=%d, hitIdx=%d", tx, ty, (int)hitType, hitIndex);
#endif

        if (hitIndex != -1)
        {
            const uint32_t now = SDL_GetTicks();
            ITEM* pItem = (hitType == SLOT_TYPE_BAG) ? GetBagItem(hitIndex) : GetEquipItem(hitIndex);

#if defined(__ANDROID__)
            if (pItem)
                __android_log_print(ANDROID_LOG_INFO, "MuMain", "[MobileInven] Hit item: Type=%d Level=%d", pItem->Type, (pItem->Level >> 3) & 15);
            else
                __android_log_print(ANDROID_LOG_INFO, "MuMain", "[MobileInven] Slot %d is empty", hitIndex);
#endif

            // Single Tap: Select and open Action Card if item exists
            m_lastTapTick  = now;
            m_lastTapType  = hitType;
            m_lastTapIndex = hitIndex;

            m_selectedType  = hitType;
            m_selectedIndex = hitIndex;
            m_bShowActionCard = (pItem != nullptr);
            if (pItem != nullptr)
            {
                if (IsConsumable(pItem))
                {
                    m_cardW = 100.0f;
                    m_cardH = 120.0f;
                }
                else
                {
                    m_cardW = 148.0f;
                    m_cardH = 220.0f;
                }
            }

            CacheTimeRenterTip1 = 0;

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
            m_selectedType = SLOT_TYPE_NONE;
            m_selectedIndex = -1;
            return true;
        }

        // If touched inside Mobile Inventory Extension window, don't close inventory and pass through
        if (CUIMobileInventoryExtension::GetInstance() && CUIMobileInventoryExtension::GetInstance()->IsOpen())
        {
            const float extX = CUIMobileInventoryExtension::GetInstance()->GetWinX();
            const float extY = CUIMobileInventoryExtension::GetInstance()->GetWinY();
            const float extW = CUIMobileInventoryExtension::GetInstance()->GetWinW();
            const float extH = CUIMobileInventoryExtension::GetInstance()->GetWinH();
            if (tx >= extX && tx <= extX + extW && ty >= extY && ty <= extY + extH)
            {
                m_bShowActionCard = false;
                m_selectedType = SLOT_TYPE_NONE;
                m_selectedIndex = -1;
                return true;
            }
        }

        // If touched inside Personal Shop window, don't close inventory and pass through touch
        if (g_pNewUISystem && g_pNewUISystem->IsVisible(INTERFACE_MYSHOP_INVENTORY))
        {
            if (tx < m_panelX)
            {
                m_bShowActionCard = false;
                m_selectedType = SLOT_TYPE_NONE;
                m_selectedIndex = -1;
                return false; // Pass through to PC mouse click handling
            }
        }

        if (m_bShowActionCard)
        {
            m_bShowActionCard = false;
            m_selectedType = SLOT_TYPE_NONE;
            m_selectedIndex = -1;
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        // Tap outside both panel and card -> close inventory
        Close();
        return true;
    }

    bool CUIMobileInventory::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        if (CUIMobileInventoryExtension::GetInstance() && CUIMobileInventoryExtension::GetInstance()->IsOpen())
        {
            if (CUIMobileInventoryExtension::GetInstance()->OnFingerMotion(ev))
                return true;
        }

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
                m_hoverDstIndex = HitTestSlot(tx, ty, m_hoverDstType, true);
                return true;
            }
        }

        return true; // Consume motion while inventory is open so it doesn't move PC mouse
    }

    bool CUIMobileInventory::OnFingerUp(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        if (CUIMobileInventoryExtension::GetInstance() && CUIMobileInventoryExtension::GetInstance()->IsOpen())
        {
            if (CUIMobileInventoryExtension::GetInstance()->OnFingerUp(ev))
                return true;
        }

        if (m_dragFingerId == ev.fingerID)
        {
            if (m_bDragging)
            {
                float tx = 0.0f, ty = 0.0f;
                TouchToVirtual(ev.x, ev.y, tx, ty);

                SLOT_TYPE dstType = SLOT_TYPE_NONE;
                const int dstIndex = HitTestSlot(tx, ty, dstType, true);

                if (dstIndex != -1)
                {
                    ExecuteMoveOrSwap(m_dragSrcType, m_dragSrcIndex, dstType, dstIndex);
                }
                else if (CUIMobileInventoryExtension::GetInstance() && CUIMobileInventoryExtension::GetInstance()->IsOpen())
                {
                    const float extX = CUIMobileInventoryExtension::GetInstance()->GetWinX();
                    const float extY = CUIMobileInventoryExtension::GetInstance()->GetWinY();
                    const float extW = CUIMobileInventoryExtension::GetInstance()->GetWinW();
                    const float extH = CUIMobileInventoryExtension::GetInstance()->GetWinH();
                    if (tx >= extX && tx <= extX + extW && ty >= extY && ty <= extY + extH && m_dragSrcType == SLOT_TYPE_BAG)
                    {
                        CUIMobileInventoryExtension::GetInstance()->ExecuteTransferFromBag(m_dragSrcIndex);
                    }
                    else
                    {
                        PlayBuffer(SOUND_CLICK01);
                    }
                }
                else
                {
                    // Drag cancelled safely outside valid slots
                    PlayBuffer(SOUND_CLICK01);
                }

                m_bDragging = false;
                m_dragFingerId = 0;
                m_dragSrcIndex = -1;
                m_hoverDstIndex = -1;
                m_selectedType = SLOT_TYPE_NONE;
                m_selectedIndex = -1;
                return true;
            }

            m_dragFingerId = 0;
            m_dragSrcIndex = -1;
        }

        return true; // Consume finger release while inventory is open to prevent fake mouse click leak
    }

    static int GetItemEquipmentSlot(const ITEM* pItem)
    {
        if (!pItem || pItem->Type < 0 || pItem->Type >= MAX_ITEM) return -1;
        const ITEM_ATTRIBUTE* pItemAttr = &ItemAttribute[pItem->Type];
        if (pItemAttr->m_byItemSlot >= MAX_EQUIPMENT_INDEX) return -1;

        int targetSlot = pItemAttr->m_byItemSlot;

        // Weapon slot routing: check 2-hand, right hand vs left hand
        if (targetSlot == EQUIPMENT_WEAPON_RIGHT)
        {
            const ITEM* pRight = &CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT];
            const ITEM* pLeft  = &CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT];

            if (pRight && pRight->Type != -1)
            {
                const BYTE heroClass = gCharacterManager.GetBaseClass(Hero->Class);
                if (!pItemAttr->TwoHand && (heroClass == CLASS_KNIGHT || heroClass == CLASS_DARK || heroClass == CLASS_RAGEFIGHTER || heroClass == CLASS_SUMMONER))
                {
                    if (!pLeft || pLeft->Type == -1)
                    {
                        targetSlot = EQUIPMENT_WEAPON_LEFT;
                    }
                }
            }
        }
        else if (targetSlot == EQUIPMENT_RING_RIGHT)
        {
            const ITEM* pRingR = &CharacterMachine->Equipment[EQUIPMENT_RING_RIGHT];
            const ITEM* pRingL = &CharacterMachine->Equipment[EQUIPMENT_RING_LEFT];
            if (pRingR && pRingR->Type != -1 && (!pRingL || pRingL->Type == -1))
            {
                targetSlot = EQUIPMENT_RING_LEFT;
            }
        }

        return targetSlot;
    }

    void CUIMobileInventory::ExecuteEquip(int bagSlotIndex)
    {
        if (!g_pMyInventory) return;
        CNewUIInventoryCtrl* ctrl = g_pMyInventory->GetInventoryCtrl();

        ITEM* pItem = GetBagItem(bagSlotIndex);
        if (!pItem) return;

        const int targetSlot = GetItemEquipmentSlot(pItem);
        if (targetSlot < 0 || targetSlot >= MAX_EQUIPMENT_INDEX) return;

        // Full equipment requirement check (Class, Step, Level, Stats, Harmony)
        if (!IsRequireEquipItem(pItem))
        {
#if defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_WARN, "MuMain", "[MobileInven] ExecuteEquip blocked: Character cannot equip item (type=%d)", pItem->Type);
#endif
            PlayBuffer(SOUND_CLICK01);
            return;
        }

        int iSrcIndex = ctrl ? ctrl->GetIndexByItem(pItem) : -1;
        if (iSrcIndex < 12)
        {
            iSrcIndex = 12 + (pItem->y * 8 + pItem->x);
        }

#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_INFO, "MuMain", "[MobileInven] ExecuteEquip via SendRequestEquipmentItem srcIndex=%d targetSlot=%d type=%d level=%d",
            iSrcIndex, targetSlot, pItem->Type, (pItem->Level >> 3) & 15);
#endif

        ITEM itemCopy = *pItem;
        if (CNewUIInventoryCtrl::CreatePickedItem(ctrl, pItem))
        {
            CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();
            if (pPickedItem) pPickedItem->HidePickedItem();
            if (ctrl) ctrl->RemoveItem(pItem);
        }
        else if (ctrl)
        {
            ctrl->RemoveItem(pItem);
        }

        SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, iSrcIndex, &itemCopy, REQUEST_EQUIPMENT_INVENTORY, targetSlot);
        PlayBuffer(SOUND_GET_ITEM01);
    }

    void CUIMobileInventory::ExecuteUnequip(int equipSlotIndex)
    {
        if (!g_pMyInventory || !CharacterMachine) return;
        if (equipSlotIndex < 0 || equipSlotIndex >= MAX_EQUIPMENT_INDEX) return;

        ITEM* pEquipped = GetEquipItem(equipSlotIndex);
        if (!pEquipped || pEquipped->Type < 0) return;

        const int emptySlot = g_pMyInventory->FindEmptySlot(pEquipped);
        if (emptySlot == -1)
        {
#if defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_WARN, "MuMain", "[MobileInven] ExecuteUnequip failed: Bag is full");
#endif
            PlayBuffer(SOUND_CLICK01);
            return;
        }

        const int iDstIndex = emptySlot;

        ITEM itemCopy = *pEquipped;

        if (CNewUIInventoryCtrl::CreatePickedItem(nullptr, pEquipped))
        {
            CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();
            g_pMyInventory->UnequipItem(equipSlotIndex);
            if (pPickedItem)
                pPickedItem->HidePickedItem();
        }

#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_INFO, "MuMain", "[MobileInven] ExecuteUnequip via SendRequestEquipmentItem equipSlot=%d dstIndex=%d type=%d",
            equipSlotIndex, iDstIndex, itemCopy.Type);
#endif

        SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, equipSlotIndex, &itemCopy, REQUEST_EQUIPMENT_INVENTORY, iDstIndex);
        PlayBuffer(SOUND_GET_ITEM01);
    }

    void CUIMobileInventory::ExecuteUse(int bagSlotIndex)
    {
        ITEM* pItem = GetBagItem(bagSlotIndex);
        if (!pItem) return;

        const int iSrcIndex = 12 + (pItem->y * 8 + pItem->x);
        SendRequestUse(iSrcIndex, 0);
        PlayBuffer(SOUND_DRINK01);
    }

    void CUIMobileInventory::ExecuteDrop(int slotIndex, SLOT_TYPE slotType)
    {
        if (!Hero || Hero->Dead) return;

        ITEM* pItem = (slotType == SLOT_TYPE_BAG) ? GetBagItem(slotIndex) : GetEquipItem(slotIndex);
        if (!pItem) return;

        if (pItem->Jewel_Of_Harmony_Option != 0 || IsHighValueItem(pItem) || IsDropBan(pItem))
        {
            PlayBuffer(SOUND_CLICK01);
            return;
        }

        const int packetSlot = (slotType == SLOT_TYPE_BAG)
            ? (12 + (pItem->y * 8 + pItem->x))
            : slotIndex;

        const int dropX = static_cast<int>(Hero->PositionX);
        const int dropY = static_cast<int>(Hero->PositionY);

        CNewUIInventoryCtrl* ctrl = (slotType == SLOT_TYPE_BAG && g_pMyInventory) ? g_pMyInventory->GetInventoryCtrl() : nullptr;
        if (slotType == SLOT_TYPE_BAG)
        {
            if (CNewUIInventoryCtrl::CreatePickedItem(ctrl, pItem))
            {
                CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();
                if (pPickedItem) pPickedItem->HidePickedItem();
                if (ctrl) ctrl->RemoveItem(pItem);
            }
            else if (ctrl)
            {
                ctrl->RemoveItem(pItem);
            }
        }
        else if (slotType == SLOT_TYPE_EQUIPMENT)
        {
            if (CNewUIInventoryCtrl::CreatePickedItem(nullptr, pItem))
            {
                CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();
                if (pPickedItem) pPickedItem->HidePickedItem();
                if (g_pMyInventory) g_pMyInventory->UnequipItem(slotIndex);
            }
        }

        SendRequestDropItem(packetSlot, dropX, dropY);
        PlayBuffer(SOUND_DROP_ITEM01);
    }

    void CUIMobileInventory::ExecuteRepairAll()
    {
        const BYTE byGoldType = (g_pNewUISystem && g_pNewUISystem->IsVisible(INTERFACE_NPCSHOP)) ? 0 : 1;
        SendRequestRepair(255, byGoldType);
        PlayBuffer(SOUND_REPAIR);
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_INFO, "MuMain", "[MobileInven] ExecuteRepairAll sent (255, %d)", byGoldType);
#endif
    }

    void CUIMobileInventory::ExecuteMoveOrSwap(SLOT_TYPE srcType, int srcIndex, SLOT_TYPE dstType, int dstIndex)
    {
        if (srcType == dstType && srcIndex == dstIndex) return;

        ITEM* pSrcItem = (srcType == SLOT_TYPE_BAG) ? GetBagItem(srcIndex) : GetEquipItem(srcIndex);
        if (!pSrcItem) return;

        // 1. If moving bag to equipment
        if (srcType == SLOT_TYPE_BAG && dstType == SLOT_TYPE_EQUIPMENT)
        {
            if (!IsRequireEquipItem(pSrcItem))
            {
                PlayBuffer(SOUND_CLICK01);
                return;
            }

            const ITEM_ATTRIBUTE* pAttr = (ItemAttribute != nullptr) ? &ItemAttribute[pSrcItem->Type] : nullptr;
            if (!pAttr) return;

            int targetEquipSlot = dstIndex;
            bool bCompatible = false;
            if (pAttr->m_byItemSlot == targetEquipSlot)
            {
                bCompatible = true;
            }
            else if (targetEquipSlot == EQUIPMENT_WEAPON_LEFT && pAttr->m_byItemSlot == EQUIPMENT_WEAPON_RIGHT)
            {
                // Shield or second 1-hand weapon in left hand
                bCompatible = true;
            }
            else if (targetEquipSlot == EQUIPMENT_RING_LEFT && pAttr->m_byItemSlot == EQUIPMENT_RING_RIGHT)
            {
                bCompatible = true;
            }

            if (bCompatible)
            {
                CNewUIInventoryCtrl* ctrl = g_pMyInventory ? g_pMyInventory->GetInventoryCtrl() : nullptr;
                int sSlot = ctrl ? ctrl->GetIndexByItem(pSrcItem) : -1;
                if (sSlot < 12) sSlot = 12 + (pSrcItem->y * 8 + pSrcItem->x);

                ITEM itemCopy = *pSrcItem;
                if (CNewUIInventoryCtrl::CreatePickedItem(ctrl, pSrcItem))
                {
                    CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();
                    if (pPickedItem) pPickedItem->HidePickedItem();
                    if (ctrl) ctrl->RemoveItem(pSrcItem);
                }

#if defined(__ANDROID__)
                __android_log_print(ANDROID_LOG_INFO, "MuMain", "[MobileInven] Move Bag->Equip sSlot=%d targetEquipSlot=%d", sSlot, targetEquipSlot);
#endif
                SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, sSlot, &itemCopy, REQUEST_EQUIPMENT_INVENTORY, targetEquipSlot);
                PlayBuffer(SOUND_GET_ITEM01);
            }
            else
            {
                // Fall back to auto-equip slot routing
                ExecuteEquip(srcIndex);
            }
            return;
        }

        // 2. If moving equipment to bag
        if (srcType == SLOT_TYPE_EQUIPMENT && dstType == SLOT_TYPE_BAG)
        {
            CNewUIInventoryCtrl* ctrl = g_pMyInventory ? g_pMyInventory->GetInventoryCtrl() : nullptr;
            if (!ctrl) return;

            const ITEM_ATTRIBUTE* pAttr = (ItemAttribute != nullptr) ? &ItemAttribute[pSrcItem->Type] : nullptr;
            const int w = (pAttr && pAttr->Width > 0) ? pAttr->Width : 1;
            const int h = (pAttr && pAttr->Height > 0) ? pAttr->Height : 1;

            const int dstCol = dstIndex % 8;
            const int dstRow = dstIndex / 8;

            int targetSlot = -1;
            if (dstCol + w <= 8 && dstRow + h <= 8)
            {
                bool bOccupied = false;
                const size_t numItems = ctrl->GetNumberOfItems();
                for (size_t i = 0; i < numItems && !bOccupied; ++i)
                {
                    ITEM* pOther = ctrl->GetItem(static_cast<int>(i));
                    if (!pOther) continue;
                    const ITEM_ATTRIBUTE* pOAttr = (ItemAttribute != nullptr) ? &ItemAttribute[pOther->Type] : nullptr;
                    const int ow = (pOAttr && pOAttr->Width > 0) ? pOAttr->Width : 1;
                    const int oh = (pOAttr && pOAttr->Height > 0) ? pOAttr->Height : 1;
                    if (dstCol < pOther->x + ow && dstCol + w > pOther->x &&
                        dstRow < pOther->y + oh && dstRow + h > pOther->y)
                    {
                        bOccupied = true;
                    }
                }

                if (!bOccupied)
                {
                    targetSlot = 12 + dstIndex;
                }
            }

            if (targetSlot == -1)
            {
                targetSlot = g_pMyInventory->FindEmptySlot(pSrcItem);
            }

            if (targetSlot == -1)
            {
                PlayBuffer(SOUND_CLICK01);
                return;
            }

            ITEM itemCopy = *pSrcItem;
            if (CNewUIInventoryCtrl::CreatePickedItem(nullptr, pSrcItem))
            {
                CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();
                g_pMyInventory->UnequipItem(srcIndex);
                if (pPickedItem)
                    pPickedItem->HidePickedItem();
            }

#if defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_INFO, "MuMain", "[MobileInven] Move Equip->Bag srcIndex=%d targetSlot=%d", srcIndex, targetSlot);
#endif
            SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, srcIndex, &itemCopy, REQUEST_EQUIPMENT_INVENTORY, targetSlot);
            PlayBuffer(SOUND_GET_ITEM01);
            return;
        }

        // 3. Bag to bag move or swap
        if (srcType == SLOT_TYPE_BAG && dstType == SLOT_TYPE_BAG)
        {
            CNewUIInventoryCtrl* ctrl = g_pMyInventory ? g_pMyInventory->GetInventoryCtrl() : nullptr;
            if (!ctrl) return;

            const ITEM_ATTRIBUTE* pAttr = (ItemAttribute != nullptr) ? &ItemAttribute[pSrcItem->Type] : nullptr;
            const int w = (pAttr && pAttr->Width > 0) ? pAttr->Width : 1;
            const int h = (pAttr && pAttr->Height > 0) ? pAttr->Height : 1;

            const int dstCol = dstIndex % 8;
            const int dstRow = dstIndex / 8;

            // Grid boundaries
            if (dstCol + w > 8 || dstRow + h > 8)
            {
                PlayBuffer(SOUND_CLICK01);
                return;
            }

            // Same origin position check
            if (dstCol == pSrcItem->x && dstRow == pSrcItem->y)
            {
                return;
            }

            ITEM* pCollidedItem = nullptr;
            int collideCount = 0;
            const size_t numItems = ctrl->GetNumberOfItems();
            for (size_t i = 0; i < numItems; ++i)
            {
                ITEM* pOther = ctrl->GetItem(static_cast<int>(i));
                if (!pOther || pOther == pSrcItem) continue;

                const ITEM_ATTRIBUTE* pOAttr = (ItemAttribute != nullptr) ? &ItemAttribute[pOther->Type] : nullptr;
                const int ow = (pOAttr && pOAttr->Width > 0) ? pOAttr->Width : 1;
                const int oh = (pOAttr && pOAttr->Height > 0) ? pOAttr->Height : 1;

                if (dstCol < pOther->x + ow && dstCol + w > pOther->x &&
                    dstRow < pOther->y + oh && dstRow + h > pOther->y)
                {
                    pCollidedItem = pOther;
                    collideCount++;
                }
            }

            if (collideCount > 1)
            {
                // Cannot place on multiple existing items
                PlayBuffer(SOUND_CLICK01);
                return;
            }

            int sSlot = ctrl->GetIndexByItem(pSrcItem);
            if (sSlot < 12) sSlot = 12 + (pSrcItem->y * 8 + pSrcItem->x);

            int dSlot = 12 + dstIndex;
            if (collideCount == 1 && pCollidedItem != nullptr)
            {
                int cSlot = ctrl->GetIndexByItem(pCollidedItem);
                if (cSlot < 12) cSlot = 12 + (pCollidedItem->y * 8 + pCollidedItem->x);
                dSlot = cSlot;
            }

            // Sync with PC Inventory Ctrl by creating PickedItem and removing from ctrl before sending
            ITEM itemCopy = *pSrcItem;
            if (CNewUIInventoryCtrl::CreatePickedItem(ctrl, pSrcItem))
            {
                CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();
                if (pPickedItem)
                {
                    pPickedItem->HidePickedItem();
                }
                ctrl->RemoveItem(pSrcItem);
            }

#if defined(__ANDROID__)
            __android_log_print(ANDROID_LOG_INFO, "MuMain", "[MobileInven] Move Bag->Bag sSlot=%d dSlot=%d (type=%d)", sSlot, dSlot, itemCopy.Type);
#endif
            SendRequestEquipmentItem(REQUEST_EQUIPMENT_INVENTORY, sSlot, &itemCopy, REQUEST_EQUIPMENT_INVENTORY, dSlot);
            PlayBuffer(SOUND_GET_ITEM01);
        }
    }

    bool CUIMobileInventory::Update()
    {
        if (!m_bIsOpen) return true;
        if (CUIMobileInventoryExtension::GetInstance() && CUIMobileInventoryExtension::GetInstance()->IsOpen())
        {
            CUIMobileInventoryExtension::GetInstance()->Update();
        }
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

        if (CUIMobileInventoryExtension::GetInstance() && CUIMobileInventoryExtension::GetInstance()->IsOpen())
        {
            CUIMobileInventoryExtension::GetInstance()->Render();
        }

        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        RenderBackdrop();
        RenderEquipmentSlots();
        RenderUtilityButtons();
        RenderBagGrid();

        if (m_bShowActionCard)
        {
            RenderActionCardBackground();
        }

        DisableAlphaBlend();
        return true;
    }

    void CUIMobileInventory::UI2DEffectCallback(LPVOID pClass, DWORD, DWORD)
    {
        CUIMobileInventory* self = static_cast<CUIMobileInventory*>(pClass);
        if (!self || !self->m_bIsOpen) return;

        EnableAlphaTest();
        self->RenderBagItemOverlays();

        if (self->m_bShowActionCard)
        {
            self->RenderActionCard();
        }

        if (self->m_bDragging)
        {
            self->RenderDraggedItem();
        }

        if (CUIMobileInventoryExtension::GetInstance() && CUIMobileInventoryExtension::GetInstance()->IsOpen())
        {
            if (CUIMobileInventoryExtension::GetInstance()->IsActionCardVisible())
            {
                CUIMobileInventoryExtension::GetInstance()->RenderActionCard();
            }
        }

        DisableAlphaBlend();
    }

    void CUIMobileInventory::Render3D()
    {
        if (!m_bIsOpen) return;

        if (CUIMobileInventoryExtension::GetInstance() && CUIMobileInventoryExtension::GetInstance()->IsOpen())
        {
            CUIMobileInventoryExtension::GetInstance()->Render3D();
        }

        // 1. Render 3D items inside equipment slots
        for (int i = 0; i < 12; ++i)
        {
            if (m_bDragging && m_dragSrcType == SLOT_TYPE_EQUIPMENT && m_dragSrcIndex == i)
            {
                continue;
            }

            ITEM* pItem = GetEquipItem(i);
            if (pItem && pItem->Type >= 0)
            {
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                float y = m_equipSlots[i].y + 1.0f;
                if (i == EQUIPMENT_ARMOR) y -= 4.0f;
                RenderItem3D(
                    m_equipSlots[i].x + 1.0f,
                    y,
                    m_equipSlots[i].w - 2.0f,
                    m_equipSlots[i].h - 2.0f,
                    pItem->Type,
                    pItem->Level,
                    pItem->Option1,
                    pItem->ExtOption,
                    false
                );
            }
        }

        // 2. Render 3D items inside bag grid with authentic W x H sizing!
        if (g_pMyInventory && g_pMyInventory->GetInventoryCtrl())
        {
            CNewUIInventoryCtrl* ctrl = g_pMyInventory->GetInventoryCtrl();
            const size_t numItems = ctrl->GetNumberOfItems();
            ITEM* pDragBagItem = (m_bDragging && m_dragSrcType == SLOT_TYPE_BAG) ? GetBagItem(m_dragSrcIndex) : nullptr;

            for (size_t i = 0; i < numItems; ++i)
            {
                ITEM* pItem = ctrl->GetItem(static_cast<int>(i));
                if (!pItem || pItem->Type < 0) continue;

                // Don't render here if currently dragging this item
                if (pDragBagItem && (pItem == pDragBagItem || (pItem->x == pDragBagItem->x && pItem->y == pDragBagItem->y)))
                {
                    continue;
                }

                const ITEM_ATTRIBUTE* pAttr = &ItemAttribute[pItem->Type];
                const float itemX = m_bagStartX + (pItem->x * m_bagSlotSize);
                const float itemY = m_bagStartY + (pItem->y * m_bagSlotSize);
                const float itemW = pAttr->Width * m_bagSlotSize;
                const float itemH = pAttr->Height * m_bagSlotSize;

                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                RenderItem3D(
                    itemX + 1.0f,
                    itemY + 1.0f,
                    itemW - 2.0f,
                    itemH - 2.0f,
                    pItem->Type,
                    pItem->Level,
                    pItem->Option1,
                    pItem->ExtOption,
                    false
                );
            }
        }

        // 4. Dragged Item 3D preview floating above finger
        if (m_bDragging)
        {
            ITEM* pDragItem = (m_dragSrcType == SLOT_TYPE_BAG) ? GetBagItem(m_dragSrcIndex) : GetEquipItem(m_dragSrcIndex);
            if (pDragItem && pDragItem->Type >= 0)
            {
                const float dragW = 44.0f;
                const float dragH = 44.0f;
                const float dragX = m_dragCurX - (dragW * 0.5f);
                const float dragY = m_dragCurY - 45.0f - (dragH * 0.5f);

                glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
                RenderItem3D(
                    dragX,
                    dragY,
                    dragW,
                    dragH,
                    pDragItem->Type,
                    pDragItem->Level,
                    pDragItem->Option1,
                    pDragItem->ExtOption,
                    false
                );
            }
        }

        // 5. Post-3D 2D Overlays (level text, Action Card, Drag overlay)
        if (m_pNewUI3DRenderMng)
        {
            m_pNewUI3DRenderMng->RenderUI2DEffect(INVENTORY_CAMERA_Z_ORDER, UI2DEffectCallback, this, 0, 0);
        }
    }

    void CUIMobileInventory::RenderBackdrop()
    {
        // Draw the full sliced MU panel with golden header and borders
        UIMobile::DrawSlicedFrame(m_panelX, m_panelY, m_panelW, m_panelH, "HÒM ĐỒ & TRANG BỊ");

        // Close button [X]
        UIMobile::DrawCloseButton(m_closeBtnX, m_closeBtnY, m_closeBtnSize, false);
    }

    void CUIMobileInventory::RenderHeaderAndZen()
    {
        // Zen display at the bottom of the panel
        if (CharacterMachine && g_pRenderText)
        {
            EnableAlphaTest();
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            const float zenX = m_panelX + 16.0f;
            const float zenY = m_panelY + m_panelH - 34.0f;
            const float zenW = m_panelW - 32.0f;
            UIMobile::DrawStretched(BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 16, zenX, zenY, zenW, 20.0f);

            unicode::t_char zenNumStr[64] = { 0 };
            ConvertGold(CharacterMachine->Gold, zenNumStr);
            char fullZenStr[128];
            std::snprintf(fullZenStr, sizeof(fullZenStr), "%s Zen", zenNumStr);

            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(255, 230, 90, 255);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->RenderText(static_cast<int>(zenX + 28.0f), static_cast<int>(zenY + 4.0f), fullZenStr);
            DisableAlphaBlend();
        }
    }

    void CUIMobileInventory::RenderEquipmentSlots()
    {
        for (int i = 0; i < 12; ++i)
        {
            if (i == EQUIPMENT_HELM && Hero && gCharacterManager.GetBaseClass(Hero->Class) == CLASS_DARK)
            {
                continue;
            }

            const auto& s = m_equipSlots[i];
            const bool isSelected = (!m_bDragging && m_selectedType == SLOT_TYPE_EQUIPMENT && m_selectedIndex == i);
            const bool isDraggingThis = (m_bDragging && m_dragSrcType == SLOT_TYPE_EQUIPMENT && m_dragSrcIndex == i);
            ITEM* pItem = isDraggingThis ? nullptr : GetEquipItem(i);
            const bool hasItem = (pItem != nullptr);

            // Broken equipped item red background tint
            if (hasItem && pItem->Durability == 0)
            {
                EnableAlphaTest();
                glColor4f(1.0f, 0.0f, 0.0f, 0.4f);
                RenderColor(s.x, s.y, s.w, s.h);
                EndRenderColor();
            }

            // Hover compatibility check for dragging onto equipment slots
            bool isHoverValid = false;
            bool isHoverInvalid = false;
            if (m_bDragging && m_hoverDstType == SLOT_TYPE_EQUIPMENT && m_hoverDstIndex == i)
            {
                ITEM* pDragItem = (m_dragSrcType == SLOT_TYPE_BAG) ? GetBagItem(m_dragSrcIndex) : nullptr;
                if (pDragItem && IsRequireEquipItem(pDragItem))
                {
                    const ITEM_ATTRIBUTE* pAttr = (ItemAttribute != nullptr) ? &ItemAttribute[pDragItem->Type] : nullptr;
                    if (pAttr && (pAttr->m_byItemSlot == i ||
                        (i == EQUIPMENT_WEAPON_LEFT && pAttr->m_byItemSlot == EQUIPMENT_WEAPON_RIGHT) ||
                        (i == EQUIPMENT_RING_LEFT && pAttr->m_byItemSlot == EQUIPMENT_RING_RIGHT)))
                    {
                        isHoverValid = true;
                    }
                    else
                    {
                        isHoverInvalid = true;
                    }
                }
                else
                {
                    isHoverInvalid = true;
                }
            }

            UIMobile::DrawPaperdollSlot(s.x, s.y, s.w, s.h, hasItem ? 0 : m_equipSilhouettes[i], isSelected, isHoverValid);

            if (isHoverInvalid)
            {
                EnableAlphaTest();
                glColor4f(1.0f, 0.2f, 0.2f, 0.25f);
                RenderColor(s.x, s.y, s.w, s.h);
                EndRenderColor();
                UIMobile::DrawBorder(s.x, s.y, s.w, s.h, 2.0f, UIMobile::ColorRGBA(1.0f, 0.2f, 0.2f, 0.95f));
            }
            else if (isHoverValid)
            {
                EnableAlphaTest();
                glColor4f(0.2f, 1.0f, 0.2f, 0.25f);
                RenderColor(s.x, s.y, s.w, s.h);
                EndRenderColor();
            }
        }
    }

    void CUIMobileInventory::RenderUtilityButtons()
    {
        // 1. Repair All
        UIMobile::DrawButton(m_btnRepairX, m_btnRepairY, m_btnRepairW, m_btnRepairH, "Sửa All", false, true, UIMobile::Colors::BtnPrimary);

        // 2. Expand Inventory
        UIMobile::DrawButton(m_btnExpandX, m_btnExpandY, m_btnExpandW, m_btnExpandH, "Mở Rộng", false, true, UIMobile::Colors::BtnPrimary);

        // 3. Personal Shop (toggle text between "Cửa Hàng" and "Đóng Shop")
        const bool bShopOpen = (g_pNewUISystem && g_pNewUISystem->IsVisible(INTERFACE_MYSHOP_INVENTORY));
        UIMobile::DrawButton(m_btnMyShopX, m_btnMyShopY, m_btnMyShopW, m_btnMyShopH, bShopOpen ? "Đóng Shop" : "Cửa Hàng", false, true, bShopOpen ? UIMobile::Colors::BtnSuccess : UIMobile::Colors::BtnPrimary);
    }

    void CUIMobileInventory::RenderBagGrid()
    {
        const float bagW = 8.0f * m_bagSlotSize;
        const float bagH = 8.0f * m_bagSlotSize;
        const GLuint tSquare = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN; // newui_item_box.tga

        // 1. Build fast slot occupancy map (8x8 = 64 cells)
        ITEM* slotItems[64] = { nullptr };
        ITEM* pDragBagItem = (m_bDragging && m_dragSrcType == SLOT_TYPE_BAG) ? GetBagItem(m_dragSrcIndex) : nullptr;

        if (g_pMyInventory && g_pMyInventory->GetInventoryCtrl())
        {
            CNewUIInventoryCtrl* ctrl = g_pMyInventory->GetInventoryCtrl();
            const size_t numItems = ctrl->GetNumberOfItems();
            for (size_t i = 0; i < numItems; ++i)
            {
                ITEM* pItem = ctrl->GetItem(static_cast<int>(i));
                if (!pItem || pItem->Type < 0) continue;

                // Crucial: The item currently being dragged is considered EMPTY in the grid!
                if (pDragBagItem && (pItem == pDragBagItem || (pItem->x == pDragBagItem->x && pItem->y == pDragBagItem->y)))
                {
                    continue;
                }

                const ITEM_ATTRIBUTE* pAttr = (ItemAttribute != nullptr) ? &ItemAttribute[pItem->Type] : nullptr;
                const int w = (pAttr && pAttr->Width > 0) ? pAttr->Width : 1;
                const int h = (pAttr && pAttr->Height > 0) ? pAttr->Height : 1;

                for (int dy = 0; dy < h; ++dy)
                {
                    for (int dx = 0; dx < w; ++dx)
                    {
                        const int sx = pItem->x + dx;
                        const int sy = pItem->y + dy;
                        if (sx >= 0 && sx < 8 && sy >= 0 && sy < 8)
                        {
                            slotItems[sy * 8 + sx] = pItem;
                        }
                    }
                }
            }
        }

        // 2. Render each slot: authentic PC item background color, then slot texture overlay
        EnableAlphaTest();
        for (int r = 0; r < 8; ++r)
        {
            for (int c = 0; c < 8; ++c)
            {
                const int cellIdx = r * 8 + c;
                const float slotX = m_bagStartX + (c * m_bagSlotSize);
                const float slotY = m_bagStartY + (r * m_bagSlotSize);

                ITEM* pItem = slotItems[cellIdx];
                if (pItem != nullptr)
                {
                    // Item background color matching authentic PC MU client
                    if (pItem->Durability == 0)
                    {
                        // Broken item: light red background
                        glColor4f(1.0f, 0.0f, 0.0f, 0.4f);
                    }
                    else if (!IsConsumable(pItem) && !IsRequireEquipItem(pItem) && IsEquipableItem(pItem))
                    {
                        // Cannot equip: light red background
                        glColor4f(1.0f, 0.0f, 0.0f, 0.4f);
                    }
                    else if (pItem->byColorState == ITEM_COLOR_DURABILITY_50)
                    {
                        glColor4f(1.0f, 1.0f, 0.0f, 0.4f);
                    }
                    else if (pItem->byColorState == ITEM_COLOR_DURABILITY_70)
                    {
                        glColor4f(1.0f, 0.66f, 0.0f, 0.4f);
                    }
                    else if (pItem->byColorState == ITEM_COLOR_DURABILITY_80)
                    {
                        glColor4f(1.0f, 0.33f, 0.0f, 0.4f);
                    }
                    else if (pItem->byColorState == ITEM_COLOR_DURABILITY_100)
                    {
                        glColor4f(1.0f, 0.0f, 0.0f, 0.4f);
                    }
                    else
                    {
                        // Standard item: authentic MU cyan/teal background
                        glColor4f(0.3f, 0.5f, 0.5f, 0.6f);
                    }

                    RenderColor(slotX, slotY, m_bagSlotSize, m_bagSlotSize);
                    EndRenderColor();
                }

                // Render slot texture overlay on top
                EnableAlphaTest();
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                UIMobile::DrawStretched(tSquare, slotX, slotY, m_bagSlotSize, m_bagSlotSize);
            }
        }

        // 3. Render table frame borders around the 8x8 grid
        UIMobile::DrawTableFrameBorders(m_bagStartX, m_bagStartY, bagW, bagH);

        // 4. Highlight selected item (hidden during active drag)
        if (!m_bDragging && m_selectedType == SLOT_TYPE_BAG && m_selectedIndex >= 0 && m_selectedIndex < 64)
        {
            ITEM* pSel = GetBagItem(m_selectedIndex);
            if (pSel)
            {
                const ITEM_ATTRIBUTE* pAttr = &ItemAttribute[pSel->Type];
                const float selX = m_bagStartX + (pSel->x * m_bagSlotSize);
                const float selY = m_bagStartY + (pSel->y * m_bagSlotSize);
                const float selW = pAttr->Width * m_bagSlotSize;
                const float selH = pAttr->Height * m_bagSlotSize;
                UIMobile::DrawBorder(selX, selY, selW, selH, 2.0f, UIMobile::Colors::SlotHighlight);
            }
        }

        // 5. Drag Destination Preview Box (matches dragged item's dimensions!)
        if (m_bDragging && m_hoverDstType == SLOT_TYPE_BAG && m_hoverDstIndex >= 0 && m_hoverDstIndex < 64)
        {
            ITEM* pDragItem = (m_dragSrcType == SLOT_TYPE_BAG) ? GetBagItem(m_dragSrcIndex) : GetEquipItem(m_dragSrcIndex);
            if (pDragItem && pDragItem->Type >= 0)
            {
                const ITEM_ATTRIBUTE* pAttr = (ItemAttribute != nullptr) ? &ItemAttribute[pDragItem->Type] : nullptr;
                const int w = (pAttr && pAttr->Width > 0) ? pAttr->Width : 1;
                const int h = (pAttr && pAttr->Height > 0) ? pAttr->Height : 1;

                const int c = m_hoverDstIndex % 8;
                const int r = m_hoverDstIndex / 8;

                const float hX = m_bagStartX + (c * m_bagSlotSize);
                const float hY = m_bagStartY + (r * m_bagSlotSize);
                const float hW = w * m_bagSlotSize;
                const float hH = h * m_bagSlotSize;

                if (c + w <= 8 && r + h <= 8)
                {
                    std::vector<ITEM*> overlappedItems;
                    for (int dy = 0; dy < h; ++dy)
                    {
                        for (int dx = 0; dx < w; ++dx)
                        {
                            ITEM* pOther = slotItems[(r + dy) * 8 + (c + dx)];
                            if (pOther != nullptr)
                            {
                                if (std::find(overlappedItems.begin(), overlappedItems.end(), pOther) == overlappedItems.end())
                                {
                                    overlappedItems.push_back(pOther);
                                }
                            }
                        }
                    }

                    if (overlappedItems.empty())
                    {
                        // Clean empty slot (even if overlapping with self's original slots!) -> GREEN
                        EnableAlphaTest();
                        glColor4f(0.2f, 1.0f, 0.2f, 0.25f);
                        RenderColor(hX, hY, hW, hH);
                        EndRenderColor();
                        UIMobile::DrawBorder(hX, hY, hW, hH, 2.0f, UIMobile::ColorRGBA(0.2f, 1.0f, 0.2f, 0.95f));
                    }
                    else if (m_dragSrcType == SLOT_TYPE_BAG && overlappedItems.size() == 1)
                    {
                        // Bag-to-bag swap with 1 other item -> YELLOW
                        EnableAlphaTest();
                        glColor4f(1.0f, 0.85f, 0.2f, 0.25f);
                        RenderColor(hX, hY, hW, hH);
                        EndRenderColor();
                        UIMobile::DrawBorder(hX, hY, hW, hH, 2.0f, UIMobile::ColorRGBA(1.0f, 0.85f, 0.2f, 0.95f));
                    }
                    else
                    {
                        // Overlaps with 2+ items or equip->bag collision -> RED
                        EnableAlphaTest();
                        glColor4f(1.0f, 0.2f, 0.2f, 0.25f);
                        RenderColor(hX, hY, hW, hH);
                        EndRenderColor();
                        UIMobile::DrawBorder(hX, hY, hW, hH, 2.0f, UIMobile::ColorRGBA(1.0f, 0.2f, 0.2f, 0.95f));
                    }
                }
                else
                {
                    // Out of bag boundary -> RED
                    EnableAlphaTest();
                    glColor4f(1.0f, 0.2f, 0.2f, 0.25f);
                    RenderColor(hX, hY, hW, hH);
                    EndRenderColor();
                    UIMobile::DrawBorder(hX, hY, hW, hH, 2.0f, UIMobile::ColorRGBA(1.0f, 0.2f, 0.2f, 0.95f));
                }
            }
        }
    }

    void CUIMobileInventory::RenderBagItemOverlays()
    {
        if (!g_pMyInventory || !g_pMyInventory->GetInventoryCtrl() || !g_pRenderText) return;
        CNewUIInventoryCtrl* ctrl = g_pMyInventory->GetInventoryCtrl();
        const size_t numItems = ctrl->GetNumberOfItems();
        ITEM* pDragBagItem = (m_bDragging && m_dragSrcType == SLOT_TYPE_BAG) ? GetBagItem(m_dragSrcIndex) : nullptr;

        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetBgColor(0, 0, 0, 0);

        for (size_t i = 0; i < numItems; ++i)
        {
            ITEM* pItem = ctrl->GetItem(static_cast<int>(i));
            if (!pItem || pItem->Type < 0) continue;

            // Skip overlay text at old slot while dragging
            if (pDragBagItem && (pItem == pDragBagItem || (pItem->x == pDragBagItem->x && pItem->y == pDragBagItem->y)))
            {
                continue;
            }

            const ITEM_ATTRIBUTE* pAttr = &ItemAttribute[pItem->Type];
            const float itemX = m_bagStartX + (pItem->x * m_bagSlotSize);
            const float itemY = m_bagStartY + (pItem->y * m_bagSlotSize);
            const float itemW = pAttr->Width * m_bagSlotSize;
            const float itemH = pAttr->Height * m_bagSlotSize;

            const int itemLevel = (pItem->Level >> 3) & 15;

            // Level text (+1..+15)
            if (itemLevel > 0 && !IsConsumable(pItem))
            {
                char lvlStr[16];
                std::snprintf(lvlStr, sizeof(lvlStr), "+%d", itemLevel);
                g_pRenderText->SetTextColor(255, 230, 80, 255);
                g_pRenderText->RenderText(static_cast<int>(itemX + 2.0f), static_cast<int>(itemY + itemH - 11.0f), lvlStr);
            }

            // Stack count (for potions/jewels)
            if (IsConsumable(pItem) && pItem->Durability > 1)
            {
                char cntStr[16];
                std::snprintf(cntStr, sizeof(cntStr), "%d", pItem->Durability);
                g_pRenderText->SetTextColor(220, 240, 255, 255);
                const int cntX = static_cast<int>(itemX + itemW - 14.0f);
                g_pRenderText->RenderText(cntX, static_cast<int>(itemY + itemH - 11.0f), cntStr);
            }
        }
    }

    void CUIMobileInventory::RenderActionCardBackground()
    {
        // Authentic MU PC tooltip renders its own background and borders in RenderTipTextList
    }

    void CUIMobileInventory::RenderActionCard()
    {
        ITEM* pItem = (m_selectedType == SLOT_TYPE_BAG) ? GetBagItem(m_selectedIndex) : GetEquipItem(m_selectedIndex);
        if (!pItem || pItem->Type < 0) return;

        if (m_selectedType == SLOT_TYPE_EQUIPMENT)
            pItem->bySelectedSlotIndex = m_selectedIndex;
        else
            pItem->bySelectedSlotIndex = m_selectedIndex + 12;

        // Position tooltip right edge adjacent to inventory panel (or extension panel if open)
        const float targetRight = (CUIMobileInventoryExtension::GetInstance() && CUIMobileInventoryExtension::GetInstance()->IsOpen())
            ? (CUIMobileInventoryExtension::GetInstance()->GetWinX() - 6.0f)
            : (m_panelX - 6.0f);

        // Clamp tooltip right edge so it NEVER penetrates into the UI panel
        g_fTooltipMaxRight = targetRight;

        // Reset tooltip cache time so RenderTipTextList honors tooltipTargetY on every frame
        CacheTimeRenterTip1 = 0;

        // Call authentic PC MU RenderItemInfo!
        RenderItemInfo(static_cast<int>(targetRight - 100.0f), static_cast<int>(m_panelY), pItem, false, 0, false, false);

        // Clear max right clamp
        g_fTooltipMaxRight = 0.0f;

        // Retrieve exact layout calculated by RenderTipTextList
        const float tipLeft   = g_fTooltipActualLeft;
        const float tipTop    = g_fTooltipActualTop;
        const float tipWidth  = g_fTooltipActualWidth;
        const float tipHeight = g_fTooltipActualHeight;

        // Mobile Action Buttons placed on the SIDE (left edge of tooltip) instead of underneath!
        const float btnW       = 76.0f;
        const float btnH       = 34.0f;
        const float btnSpacing = 8.0f;
        float btnX             = tipLeft - btnW - 6.0f;
        if (btnX < 6.0f) btnX = 6.0f;

        const float btnY1 = tipTop + 4.0f;
        const float btnY2 = btnY1 + btnH + btnSpacing;

        const bool bHasPrimaryAction = (m_selectedType == SLOT_TYPE_EQUIPMENT) ||
            (m_selectedType == SLOT_TYPE_BAG && (IsEquipableItem(pItem) || IsConsumable(pItem)));

        // Reset all action buttons to default/hidden
        m_btnEquipW     = 0.0f;
        m_btnUseW       = 0.0f;
        m_btnDropW      = 0.0f;
        m_btnCloseCardW = 0.0f;
        m_btnCloseCardH = 0.0f;

        if (bHasPrimaryAction)
        {
            m_btnEquipX = btnX;
            m_btnEquipY = btnY1;
            m_btnEquipW = btnW;
            m_btnEquipH = btnH;

            m_btnUseX = m_btnEquipX;
            m_btnUseY = m_btnEquipY;
            m_btnUseW = m_btnEquipW;
            m_btnUseH = m_btnEquipH;

            m_btnDropX = btnX;
            m_btnDropY = btnY2;
            m_btnDropW = btnW;
            m_btnDropH = btnH;
        }
        else
        {
            m_btnDropX = btnX;
            m_btnDropY = btnY1;
            m_btnDropW = btnW;
            m_btnDropH = btnH;
        }

        // Card touch bounds (covers side buttons + tooltip)
        m_cardX = btnX - 4.0f;
        m_cardY = tipTop;
        m_cardW = (tipLeft + tipWidth) - m_cardX + 4.0f;
        m_cardH = (std::max)(tipHeight, (bHasPrimaryAction ? btnY2 : btnY1) + btnH + 4.0f - tipTop);

        // Render Action Buttons on the side
        if (m_selectedType == SLOT_TYPE_BAG)
        {
            if (IsEquipableItem(pItem))
            {
                const bool bCanEquip = IsRequireEquipItem(pItem);
                UIMobile::DrawButton(m_btnEquipX, m_btnEquipY, m_btnEquipW, m_btnEquipH, "Trang Bị", false, bCanEquip, UIMobile::Colors::BtnSuccess);
            }
            else if (IsConsumable(pItem))
            {
                UIMobile::DrawButton(m_btnUseX, m_btnUseY, m_btnUseW, m_btnUseH, "Sử Dụng", false, true, UIMobile::Colors::BtnPrimary);
            }

            UIMobile::DrawButton(m_btnDropX, m_btnDropY, m_btnDropW, m_btnDropH, "Vứt Bỏ", false, true, UIMobile::Colors::BtnDanger);
        }
        else if (m_selectedType == SLOT_TYPE_EQUIPMENT)
        {
            UIMobile::DrawButton(m_btnEquipX, m_btnEquipY, m_btnEquipW, m_btnEquipH, "Tháo Ra", false, true, UIMobile::Colors::BtnPrimary);
            UIMobile::DrawButton(m_btnDropX, m_btnDropY, m_btnDropW, m_btnDropH, "Vứt Bỏ", false, true, UIMobile::Colors::BtnDanger);
        }
        DisableAlphaBlend();
    }

    void CUIMobileInventory::RenderDraggedItem()
    {
        ITEM* pItem = (m_dragSrcType == SLOT_TYPE_BAG) ? GetBagItem(m_dragSrcIndex) : GetEquipItem(m_dragSrcIndex);
        if (!pItem) return;

        // Glowing indicator around finger tip
        const float renderX = m_dragCurX - 22.0f;
        const float renderY = m_dragCurY - 45.0f - 22.0f;

        UIMobile::DrawBorder(renderX, renderY, 44.0f, 44.0f, 2.0f, UIMobile::Colors::SlotHighlight);

        if (g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetBgColor(0, 0, 0, 0);

            const int itemLevel = (pItem->Level >> 3) & 15;
            if (itemLevel > 0 && !IsConsumable(pItem))
            {
                char lvlStr[16];
                std::snprintf(lvlStr, sizeof(lvlStr), "+%d", itemLevel);
                g_pRenderText->SetTextColor(255, 230, 80, 255);
                g_pRenderText->RenderText(static_cast<int>(renderX + 2.0f), static_cast<int>(renderY + 44.0f - 11.0f), lvlStr);
            }

            if (IsConsumable(pItem) && pItem->Durability > 1)
            {
                char cntStr[16];
                std::snprintf(cntStr, sizeof(cntStr), "%d", pItem->Durability);
                g_pRenderText->SetTextColor(220, 240, 255, 255);
                g_pRenderText->RenderText(static_cast<int>(renderX + 44.0f - 14.0f), static_cast<int>(renderY + 44.0f - 11.0f), cntStr);
            }
        }
    }
}


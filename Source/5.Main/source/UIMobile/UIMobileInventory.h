// =============================================================================
// UIMobileInventory.h
// Dedicated Mobile Inventory UI for MU Online (Touch-First Interaction).
// Replaces PC mouse hovering, right-click, and small grid drag-and-drop with:
//   1. Tap-to-Inspect + Smart Action Card (Trang bị / Tháo / Dùng / Bán / Vứt)
//   2. Double-Tap Quick Action (Thay thế chuột phải PC siêu tốc)
//   3. Touch Drag-and-Drop with Finger Offset (-40px)
//   4. Mobile Ergonomic Layout (Right-hand thumb zone + 36px touch targets)
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "NewUI3DRenderMng.h"
#include "UIMobileCommon.h"

#if defined(__ANDROID__) || defined(MU_IOS) || (defined(__has_include) && __has_include(<SDL3/SDL.h>))
#include <SDL3/SDL.h>
#else
typedef int64_t SDL_FingerID;
struct SDL_TouchFingerEvent {
    SDL_FingerID fingerID;
    float x;
    float y;
};
#endif

namespace SEASON3B
{
    class CUIMobileInventory : public CNewUIObj, public INewUI3DRenderObj
    {
    public:
        enum SLOT_TYPE
        {
            SLOT_TYPE_NONE = 0,
            SLOT_TYPE_EQUIPMENT,
            SLOT_TYPE_BAG,
        };

        struct TouchSlot
        {
            float x, y, w, h;
            int   slotIndex;
            SLOT_TYPE slotType;
        };

        CUIMobileInventory();
        virtual ~CUIMobileInventory();

        static CUIMobileInventory* GetInstance();

        bool Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng);
        void Release();

        // CNewUIObj interface
        bool Render() override;
        bool Update() override;
        bool UpdateMouseEvent() override { return true; }
        bool UpdateKeyEvent() override;

        float GetLayerDepth() override { return 4.54f; }
        float GetKeyEventOrder() override { return 3.1f; }

        // INewUI3DRenderObj interface
        void Render3D() override;
        static void UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB);

        // Visibility
        void Open();
        void Close();
        void Toggle();
        bool IsOpen() const { return m_bIsOpen; }
        bool IsVisible() const override { return m_bIsOpen; }

        float GetPanelX() const { return m_panelX; }
        float GetPanelY() const { return m_panelY; }
        float GetPanelW() const { return m_panelW; }
        float GetPanelH() const { return m_panelH; }

        // Touch Input Dispatch (called from MobileControls)
        bool OnFingerDown(const SDL_TouchFingerEvent& ev);
        bool OnFingerUp(const SDL_TouchFingerEvent& ev);
        bool OnFingerMotion(const SDL_TouchFingerEvent& ev);

        // Quick Actions (Touch replacement for PC Mouse Clicks)
        void ExecuteEquip(int bagSlotIndex);
        void ExecuteUnequip(int equipSlotIndex);
        void ExecuteUse(int bagSlotIndex);
        void ExecuteDrop(int slotIndex, SLOT_TYPE slotType);
        void ExecuteMoveOrSwap(SLOT_TYPE srcType, int srcIndex, SLOT_TYPE dstType, int dstIndex);
        void ExecuteRepairAll();

        // Helper getters for items
        ITEM* GetEquipItem(int equipIndex) const;
        ITEM* GetBagItem(int bagIndex) const;
        bool  IsConsumable(const ITEM* pItem) const;
        bool  IsEquipableItem(const ITEM* pItem) const;

    private:
        void EnsureTextures();
        void ComputeLayout();
        int  HitTestSlot(float x, float y, SLOT_TYPE& outType, bool bForDrop = false) const;
        bool HitTestButton(float x, float y, float bx, float by, float bw, float bh) const;

        // Render helpers
        void RenderBackdrop();
        void RenderEquipmentSlots();
        void RenderBagGrid();
        void RenderBagItemOverlays();
        void RenderActionCardBackground();
        void RenderActionCard();
        void RenderDraggedItem();
        void RenderHeaderAndZen();
        void RenderUtilityButtons();

        CNewUIManager*     m_pNewUIMng;
        CNewUI3DRenderMng* m_pNewUI3DRenderMng;

        bool m_bIsOpen;

        // Selection & Action Card State
        SLOT_TYPE m_selectedType;
        int       m_selectedIndex;
        bool      m_bShowActionCard;

        // Double-Tap Detector (< 300ms)
        uint32_t  m_lastTapTick;
        SLOT_TYPE m_lastTapType;
        int       m_lastTapIndex;

        // Touch Drag and Drop State
        bool         m_bDragging;
        SDL_FingerID m_dragFingerId;
        SLOT_TYPE    m_dragSrcType;
        int          m_dragSrcIndex;
        float        m_dragStartX;
        float        m_dragStartY;
        float        m_dragCurX;
        float        m_dragCurY;
        uint32_t     m_dragStartTick;
        int          m_hoverDstIndex;
        SLOT_TYPE    m_hoverDstType;

        // Layout bounds (virtual 640x480 or responsive coordinates)
        float m_panelX, m_panelY, m_panelW, m_panelH;
        float m_cardX, m_cardY, m_cardW, m_cardH;
        float m_closeBtnX, m_closeBtnY, m_closeBtnSize;
        float m_bagSlotSize, m_bagStartX, m_bagStartY;

        // Equipment slot hitboxes (12 slots)
        TouchSlot m_equipSlots[12];
        // Bag grid slot hitboxes (64 slots = 8x8)
        TouchSlot m_bagSlots[64];

        // Action Card button hitboxes
        float m_btnEquipX, m_btnEquipY, m_btnEquipW, m_btnEquipH;
        float m_btnUseX, m_btnUseY, m_btnUseW, m_btnUseH;
        float m_btnDropX, m_btnDropY, m_btnDropW, m_btnDropH;
        float m_btnCloseCardX, m_btnCloseCardY, m_btnCloseCardW, m_btnCloseCardH;

        // Utility button hitboxes (Sửa All, Mở Rộng, Cửa Hàng)
        float m_btnRepairX, m_btnRepairY, m_btnRepairW, m_btnRepairH;
        float m_btnExpandX, m_btnExpandY, m_btnExpandW, m_btnExpandH;
        float m_btnMyShopX, m_btnMyShopY, m_btnMyShopW, m_btnMyShopH;

        // Textures & silhouettes
        GLuint m_equipSilhouettes[12];
        DWORD m_texBackdrop;
        DWORD m_texSlotBg;
        DWORD m_texSlotActive;
        DWORD m_texActionCardBg;
        DWORD m_texBtnNormal;
        DWORD m_texBtnPressed;
        DWORD m_texBtnRepair;
        DWORD m_texBtnExpand;
        DWORD m_texBtnMyShop;
    };
}

#define g_pMobileInventory SEASON3B::CUIMobileInventory::GetInstance()

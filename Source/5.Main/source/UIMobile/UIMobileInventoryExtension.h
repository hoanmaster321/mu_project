// =============================================================================
// UIMobileInventoryExtension.h
// Mobile Inventory Extension Window (cloning PC UI look & feel, scaled for mobile)
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "NewUI3DRenderMng.h"
#include "UIMobileCommon.h"

namespace SEASON3B
{
    class CUIMobileInventoryExtension : public CNewUIObj, public INewUI3DRenderObj
    {
    public:
        enum IMAGE_LIST
        {
            IMAGE_EXTENSION_BACK     = SEASON3B::CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
            IMAGE_EXTENSION_TOP      = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 1, // newui_item_back04.tga
            IMAGE_EXTENSION_LEFT     = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 2, // newui_item_back02-L.tga
            IMAGE_EXTENSION_RIGHT    = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 3, // newui_item_back02-R.tga
            IMAGE_EXTENSION_BOTTOM   = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 4, // newui_item_back03.tga
            IMAGE_INVENTORY_EXIT_BTN = BITMAP_INTERFACE_NEW_PERSONALINVENTORY_BEGIN + 17,
            IMAGE_ITEM_SQUARE        = BITMAP_INTERFACE_NEW_INVENTORY_BASE_BEGIN,
            IMAGE_EXTENSION_EMPTY    = BITMAP_INTERFACE_NEW_INVENTORY_EXT_BEGIN,
            IMAGE_EXTENSION_TABLE    = BITMAP_INTERFACE_NEW_INVENTORY_EXT_BEGIN + 1,
            IMAGE_EXTENSION_NO1      = BITMAP_INTERFACE_NEW_INVENTORY_EXT_BEGIN + 2,
            IMAGE_EXTENSION_NO2      = BITMAP_INTERFACE_NEW_INVENTORY_EXT_BEGIN + 3,
            IMAGE_EXTENSION_NO3      = BITMAP_INTERFACE_NEW_INVENTORY_EXT_BEGIN + 4,
            IMAGE_EXTENSION_NO4      = BITMAP_INTERFACE_NEW_INVENTORY_EXT_BEGIN + 5,
        };

        CUIMobileInventoryExtension();
        virtual ~CUIMobileInventoryExtension();

        static CUIMobileInventoryExtension* GetInstance();

        bool Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng);
        void Release();

        bool Render() override;
        void Render3D() override;
        bool Update() override;
        bool UpdateMouseEvent() override { return true; }
        bool UpdateKeyEvent() override;
        float GetLayerDepth() override { return 10.6f; }

        bool OnFingerDown(const SDL_TouchFingerEvent& ev);
        bool OnFingerMotion(const SDL_TouchFingerEvent& ev);
        bool OnFingerUp(const SDL_TouchFingerEvent& ev);

        void Open();
        void Close();
        void Toggle();
        bool IsOpen() const { return m_bIsOpen; }
        bool IsVisible() const override { return m_bIsOpen; }
        bool IsDragging() const { return m_bDragging; }
        bool IsActionCardVisible() const { return m_bShowActionCard; }

        float GetWinX() const { return m_winX; }
        float GetWinY() const { return m_winY; }
        float GetWinW() const { return m_winW; }
        float GetWinH() const { return m_winH; }

        void ExecuteTransferToBag(int extIndex, int slotIndex);
        void ExecuteTransferFromBag(int bagSlotIndex);

        ITEM* GetExtensionItem(int extIndex, int slotIndex, int& outItemLinealIndex) const;

        void RenderActionCard();
        void RenderDraggedItem();

    private:
        void EnsureTextures();
        void ComputeLayout();
        void RenderFrame();
        void RenderBoxes();

        int  HitTestSlot(float tx, float ty, int& outExtIndex) const;

        CNewUIManager*      m_pNewUIMng;
        CNewUI3DRenderMng*  m_pNewUI3DRenderMng;
        bool m_bIsOpen;

        // Window Layout
        float m_winX, m_winY, m_winW, m_winH;
        float m_closeBtnX, m_closeBtnY, m_closeBtnSize;
        float m_exitBtnX, m_exitBtnY, m_exitBtnW, m_exitBtnH;

        // Grid sizing
        float m_slotSize;
        float m_boxStartX;
        float m_boxStartY[2];

        // Selection & Action Card
        int   m_selectedExt;
        int   m_selectedSlot;
        bool  m_bShowActionCard;
        float m_cardX, m_cardY, m_cardW, m_cardH;
        float m_btnToBagX, m_btnToBagY, m_btnToBagW, m_btnToBagH;
        float m_btnCloseCardX, m_btnCloseCardY, m_btnCloseCardW, m_btnCloseCardH;

        // Drag and drop state
        bool     m_bDragging;
        int64_t  m_dragFingerId;
        int      m_dragExt;
        int      m_dragSlot;
        float    m_dragStartX, m_dragStartY;
        float    m_dragCurX, m_dragCurY;
        uint32_t m_dragStartTick;

        // Double tap detection
        uint32_t m_lastTapTick;
        int      m_lastTapExt;
        int      m_lastTapSlot;
    };
}

#define g_pMobileInventoryExt SEASON3B::CUIMobileInventoryExtension::GetInstance()

// =============================================================================
// UIMobileMoveCommand.h
// Mobile Map Warp / Teleport Window with smooth touch-scrolling list.
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "UIMobileCommon.h"
#include "MoveCommandData.h"

#include <vector>

namespace SEASON3B
{
    class CUIMobileMoveCommand : public CNewUIObj
    {
    public:
        struct MapEntry
        {
            int  index;
            char name[64];
            int  reqLevel;
            int  reqZen;
            bool canMove;
        };

        CUIMobileMoveCommand();
        virtual ~CUIMobileMoveCommand();

        bool Create(CNewUIManager* pNewUIMng);
        void Release();

        bool Render() override;
        bool Update() override;
        bool UpdateMouseEvent() override { return true; }
        bool UpdateKeyEvent() override;
        float GetLayerDepth() override { return 10.5f; }

        bool OnFingerDown(const SDL_TouchFingerEvent& ev);
        bool OnFingerMotion(const SDL_TouchFingerEvent& ev);
        bool OnFingerUp(const SDL_TouchFingerEvent& ev);

        void Open();
        void Close();
        void Toggle();
        bool IsOpen() const { return m_bIsOpen; }

    private:
        void RefreshMapList();
        void ComputeLayout();
        void ExecuteWarp(int mapIndex);

        CNewUIManager* m_pNewUIMng;
        bool m_bIsOpen;

        // Window Bounds
        float m_winX, m_winY, m_winW, m_winH;
        float m_closeBtnX, m_closeBtnY, m_closeBtnSize;

        // Scrollable List
        float m_listX, m_listY, m_listW, m_listH;
        float m_scrollY;
        float m_maxScrollY;
        bool  m_isScrolling;
        SDL_FingerID m_scrollFingerId;
        float m_touchDownY;
        float m_lastTouchY;
        uint32_t m_touchDownTick;

        std::vector<MapEntry> m_maps;
        int m_pressedIndex;
        bool m_closePressed;
    };
}

#define g_pMobileMoveCommand (UIMobile::CUIMobileSystem::GetInstance() ? UIMobile::CUIMobileSystem::GetInstance()->GetMoveCommand() : nullptr)

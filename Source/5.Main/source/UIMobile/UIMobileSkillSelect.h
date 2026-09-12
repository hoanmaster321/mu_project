// =============================================================================
// UIMobileSkillSelect.h
// Mobile Skill Selection & Quick Combat Slot Assignment Window.
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "UIMobileCommon.h"

#include <vector>

namespace SEASON3B
{
    class CUIMobileSkillSelect : public CNewUIObj
    {
    public:
        struct SkillEntry
        {
            int skillIndex;
            int skillType;
            int assignedSlot; // 1..4 if assigned, else -1
        };

        CUIMobileSkillSelect();
        virtual ~CUIMobileSkillSelect();

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
        void RefreshSkills();
        void ComputeLayout();
        void ExecuteAssignSlot(int slotNumber);

        CNewUIManager* m_pNewUIMng;
        bool m_bIsOpen;

        // Window Bounds
        float m_winX, m_winY, m_winW, m_winH;
        float m_closeBtnX, m_closeBtnY, m_closeBtnSize;

        // Skill Grid Bounds
        float m_gridX, m_gridY, m_gridW, m_gridH;
        float m_scrollY;
        float m_maxScrollY;
        bool  m_isScrolling;
        SDL_FingerID m_scrollFingerId;
        float m_touchDownY;
        float m_lastTouchY;

        // Detail Card & Assign Buttons
        float m_cardX, m_cardY, m_cardW, m_cardH;
        float m_assignBtnX[4];
        float m_assignBtnY[4];
        float m_assignBtnW, m_assignBtnH;

        std::vector<SkillEntry> m_skills;
        int m_selectedIdx;
        int m_pressedAssignSlot;
        bool m_closePressed;
    };
}

#define g_pMobileSkillSelect (UIMobile::CUIMobileSystem::GetInstance() ? UIMobile::CUIMobileSystem::GetInstance()->GetSkillSelect() : nullptr)

// =============================================================================
// UIMobileNPCDialogue.h
// Mobile Visual RPG NPC Dialogue Interface (Touch-First Story & Choices).
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "UIMobileCommon.h"

namespace SEASON3B
{
    class CUIMobileNPCDialogue : public CNewUIObj
    {
    public:
        CUIMobileNPCDialogue();
        virtual ~CUIMobileNPCDialogue();

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
        void ComputeLayout();
        void ExecuteChoice(int choiceIndex);

        CNewUIManager* m_pNewUIMng;
        bool m_bIsOpen;

        // Dialogue Box Layout
        float m_boxX, m_boxY, m_boxW, m_boxH;
        float m_closeBtnX, m_closeBtnY, m_closeBtnSize;

        // Choices Layout
        struct ChoiceCard
        {
            UIMobile::UIRect rect;
            int index;
        };
        static const int MAX_CHOICES = 6;
        ChoiceCard m_choices[MAX_CHOICES];
        int m_choiceCount;
    };
}

#define g_pMobileNPCDialogue (UIMobile::CUIMobileSystem::GetInstance() ? UIMobile::CUIMobileSystem::GetInstance()->GetNPCDialogue() : nullptr)

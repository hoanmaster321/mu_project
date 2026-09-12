// =============================================================================
// UIMobileSystem.h
// Central Mobile UI Coordinator and Event Dispatcher.
// =============================================================================

#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "NewUI3DRenderMng.h"
#include "UIMobileCommon.h"

namespace SEASON3B
{
    class CNewUIManager;
    class CNewUI3DRenderMng;
    class CUIMobileInventory;
    class CUIMobileCharacterInfo;
    class CUIMobileMoveCommand;
    class CUIMobileSkillSelect;
    class CUIMobileParty;
    class CUIMobileNPCShop;
    class CUIMobileStorage;
    class CUIMobileMix;
    class CUIMobileTrade;
    class CUIMobileOption;
    class CUIMobileHelper;
    class CUIMobileNPCDialogue;
}

namespace UIMobile
{
    enum MOBILE_UI_TYPE
    {
        UI_NONE = 0,
        UI_INVENTORY,
        UI_CHARACTER_INFO,
        UI_MOVE_COMMAND,
        UI_SKILL_SELECT,
        UI_PARTY,
        UI_NPC_SHOP,
        UI_STORAGE,
        UI_MIX,
        UI_TRADE,
        UI_OPTION,
        UI_HELPER,
        UI_NPC_DIALOGUE,
        UI_COUNT
    };

    class CUIMobileSystem
    {
    public:
        CUIMobileSystem();
        ~CUIMobileSystem();

        static CUIMobileSystem* GetInstance();

        bool Create(SEASON3B::CNewUIManager* pNewUIMng, SEASON3B::CNewUI3DRenderMng* pNewUI3DRenderMng);
        void Release();

        // Display control
        void Show(MOBILE_UI_TYPE type);
        void Hide(MOBILE_UI_TYPE type);
        void Toggle(MOBILE_UI_TYPE type);
        void HideAll();
        bool IsVisible(MOBILE_UI_TYPE type) const;
        bool IsAnyUIVisible() const;

        // Event routing
        bool OnFingerDown(const SDL_TouchFingerEvent& ev);
        bool OnFingerMotion(const SDL_TouchFingerEvent& ev);
        bool OnFingerUp(const SDL_TouchFingerEvent& ev);

        void Update();
        void Render();
        void Render3D();

        // Getters
        SEASON3B::CUIMobileInventory*      GetInventory() const;
        SEASON3B::CUIMobileCharacterInfo*  GetCharacterInfo() const;
        SEASON3B::CUIMobileMoveCommand*    GetMoveCommand() const;
        SEASON3B::CUIMobileSkillSelect*    GetSkillSelect() const;
        SEASON3B::CUIMobileParty*          GetParty() const;
        SEASON3B::CUIMobileNPCShop*        GetNPCShop() const;
        SEASON3B::CUIMobileStorage*        GetStorage() const;
        SEASON3B::CUIMobileMix*            GetMix() const;
        SEASON3B::CUIMobileTrade*          GetTrade() const;
        SEASON3B::CUIMobileOption*         GetOption() const;
        SEASON3B::CUIMobileHelper*         GetHelper() const;
        SEASON3B::CUIMobileNPCDialogue*    GetNPCDialogue() const;

    private:
        SEASON3B::CNewUIManager*      m_pNewUIMng;
        SEASON3B::CNewUI3DRenderMng*  m_pNewUI3DRenderMng;

        SEASON3B::CUIMobileCharacterInfo* m_pCharInfo;
        SEASON3B::CUIMobileMoveCommand*   m_pMoveCommand;
        SEASON3B::CUIMobileSkillSelect*   m_pSkillSelect;
        SEASON3B::CUIMobileParty*         m_pParty;
        SEASON3B::CUIMobileNPCShop*       m_pNPCShop;
        SEASON3B::CUIMobileStorage*       m_pStorage;
        SEASON3B::CUIMobileMix*           m_pMix;
        SEASON3B::CUIMobileTrade*         m_pTrade;
        SEASON3B::CUIMobileOption*        m_pOption;
        SEASON3B::CUIMobileHelper*        m_pHelper;
        SEASON3B::CUIMobileNPCDialogue*   m_pNPCDialogue;
    };
}

#define g_pMobileSystem UIMobile::CUIMobileSystem::GetInstance()

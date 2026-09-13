// =============================================================================
// UIMobileSystem.cpp
// Implementation of Central Mobile UI Coordinator & Event Dispatcher.
// =============================================================================

#include "stdafx.h"
#include "UIMobileSystem.h"
#include "UIMobileInventory.h"
#include "UIMobileInventoryExtension.h"
#include "UIMobileCharacterInfo.h"
#include "UIMobileMoveCommand.h"
#include "UIMobileSkillSelect.h"
#include "UIMobileParty.h"
#include "UIMobileNPCShop.h"
#include "UIMobileStorage.h"
#include "UIMobileMix.h"
#include "UIMobileTrade.h"
#include "UIMobileOption.h"
#include "UIMobileHelper.h"
#include "UIMobileNPCDialogue.h"

static UIMobile::CUIMobileSystem* s_pSystemInstance = nullptr;

namespace UIMobile
{
    CUIMobileSystem::CUIMobileSystem()
        : m_pNewUIMng(nullptr)
        , m_pNewUI3DRenderMng(nullptr)
        , m_pInventoryExt(nullptr)
        , m_pCharInfo(nullptr)
        , m_pMoveCommand(nullptr)
        , m_pSkillSelect(nullptr)
        , m_pParty(nullptr)
        , m_pNPCShop(nullptr)
        , m_pStorage(nullptr)
        , m_pMix(nullptr)
        , m_pTrade(nullptr)
        , m_pOption(nullptr)
        , m_pHelper(nullptr)
        , m_pNPCDialogue(nullptr)
    {
        s_pSystemInstance = this;
    }

    CUIMobileSystem::~CUIMobileSystem()
    {
        Release();
        if (s_pSystemInstance == this) s_pSystemInstance = nullptr;
    }

    CUIMobileSystem* CUIMobileSystem::GetInstance()
    {
        return s_pSystemInstance;
    }

    bool CUIMobileSystem::Create(SEASON3B::CNewUIManager* pNewUIMng, SEASON3B::CNewUI3DRenderMng* pNewUI3DRenderMng)
    {
        m_pNewUIMng = pNewUIMng;
        m_pNewUI3DRenderMng = pNewUI3DRenderMng;

        m_pCharInfo = new SEASON3B::CUIMobileCharacterInfo;
        m_pCharInfo->Create(m_pNewUIMng);

        m_pMoveCommand = new SEASON3B::CUIMobileMoveCommand;
        m_pMoveCommand->Create(m_pNewUIMng);

        m_pSkillSelect = new SEASON3B::CUIMobileSkillSelect;
        m_pSkillSelect->Create(m_pNewUIMng);

        m_pParty = new SEASON3B::CUIMobileParty;
        m_pParty->Create(m_pNewUIMng);

        m_pNPCShop = new SEASON3B::CUIMobileNPCShop;
        m_pNPCShop->Create(m_pNewUIMng, m_pNewUI3DRenderMng);

        m_pStorage = new SEASON3B::CUIMobileStorage;
        m_pStorage->Create(m_pNewUIMng, m_pNewUI3DRenderMng);

        m_pMix = new SEASON3B::CUIMobileMix;
        m_pMix->Create(m_pNewUIMng, m_pNewUI3DRenderMng);

        m_pTrade = new SEASON3B::CUIMobileTrade;
        m_pTrade->Create(m_pNewUIMng, m_pNewUI3DRenderMng);

        m_pOption = new SEASON3B::CUIMobileOption;
        m_pOption->Create(m_pNewUIMng);

        m_pHelper = new SEASON3B::CUIMobileHelper;
        m_pHelper->Create(m_pNewUIMng);

        m_pNPCDialogue = new SEASON3B::CUIMobileNPCDialogue;
        m_pNPCDialogue->Create(m_pNewUIMng);

        m_pInventoryExt = new SEASON3B::CUIMobileInventoryExtension;
        m_pInventoryExt->Create(m_pNewUIMng, m_pNewUI3DRenderMng);

        return true;
    }

    void CUIMobileSystem::Release()
    {
        SAFE_DELETE(m_pInventoryExt);
        SAFE_DELETE(m_pCharInfo);
        SAFE_DELETE(m_pMoveCommand);
        SAFE_DELETE(m_pSkillSelect);
        SAFE_DELETE(m_pParty);
        SAFE_DELETE(m_pNPCShop);
        SAFE_DELETE(m_pStorage);
        SAFE_DELETE(m_pMix);
        SAFE_DELETE(m_pTrade);
        SAFE_DELETE(m_pOption);
        SAFE_DELETE(m_pHelper);
        SAFE_DELETE(m_pNPCDialogue);

        m_pNewUIMng = nullptr;
        m_pNewUI3DRenderMng = nullptr;
    }

    SEASON3B::CUIMobileInventory* CUIMobileSystem::GetInventory() const
    {
        return SEASON3B::CUIMobileInventory::GetInstance();
    }

    SEASON3B::CUIMobileCharacterInfo* CUIMobileSystem::GetCharacterInfo() const
    {
        return m_pCharInfo;
    }

    SEASON3B::CUIMobileMoveCommand* CUIMobileSystem::GetMoveCommand() const
    {
        return m_pMoveCommand;
    }

    SEASON3B::CUIMobileSkillSelect* CUIMobileSystem::GetSkillSelect() const
    {
        return m_pSkillSelect;
    }

    SEASON3B::CUIMobileParty* CUIMobileSystem::GetParty() const
    {
        return m_pParty;
    }

    SEASON3B::CUIMobileNPCShop* CUIMobileSystem::GetNPCShop() const
    {
        return m_pNPCShop;
    }

    SEASON3B::CUIMobileStorage* CUIMobileSystem::GetStorage() const
    {
        return m_pStorage;
    }

    SEASON3B::CUIMobileMix* CUIMobileSystem::GetMix() const
    {
        return m_pMix;
    }

    SEASON3B::CUIMobileTrade* CUIMobileSystem::GetTrade() const
    {
        return m_pTrade;
    }

    SEASON3B::CUIMobileOption* CUIMobileSystem::GetOption() const
    {
        return m_pOption;
    }

    SEASON3B::CUIMobileHelper* CUIMobileSystem::GetHelper() const
    {
        return m_pHelper;
    }

    SEASON3B::CUIMobileNPCDialogue* CUIMobileSystem::GetNPCDialogue() const
    {
        return m_pNPCDialogue;
    }

    SEASON3B::CUIMobileInventoryExtension* CUIMobileSystem::GetInventoryExt() const
    {
        return m_pInventoryExt;
    }

    void CUIMobileSystem::Show(MOBILE_UI_TYPE type)
    {
        // Exclusive window display unless opening paired windows like NPCShop/Storage/Extension + Inventory
        if (type != UI_INVENTORY && type != UI_INVENTORY_EXT && type != UI_NPC_SHOP && type != UI_STORAGE && type != UI_MIX && type != UI_TRADE)
        {
            HideAll();
        }

        switch (type)
        {
            case UI_INVENTORY:
                if (GetInventory()) GetInventory()->Open();
                break;
            case UI_INVENTORY_EXT:
                if (m_pInventoryExt) m_pInventoryExt->Open();
                break;
            case UI_CHARACTER_INFO:
                if (m_pCharInfo) m_pCharInfo->Open();
                break;
            case UI_MOVE_COMMAND:
                if (m_pMoveCommand) m_pMoveCommand->Open();
                break;
            case UI_SKILL_SELECT:
                if (m_pSkillSelect) m_pSkillSelect->Open();
                break;
            case UI_PARTY:
                if (m_pParty) m_pParty->Open();
                break;
            case UI_NPC_SHOP:
                if (m_pNPCShop) m_pNPCShop->Open();
                if (GetInventory() && !GetInventory()->IsOpen()) GetInventory()->Open();
                break;
            case UI_STORAGE:
                if (m_pStorage) m_pStorage->Open();
                if (GetInventory() && !GetInventory()->IsOpen()) GetInventory()->Open();
                break;
            case UI_MIX:
                if (m_pMix) m_pMix->Open();
                if (GetInventory() && !GetInventory()->IsOpen()) GetInventory()->Open();
                break;
            case UI_TRADE:
                if (m_pTrade) m_pTrade->Open();
                if (GetInventory() && !GetInventory()->IsOpen()) GetInventory()->Open();
                break;
            case UI_OPTION:
                if (m_pOption) m_pOption->Open();
                break;
            case UI_HELPER:
                if (m_pHelper) m_pHelper->Open();
                break;
            case UI_NPC_DIALOGUE:
                if (m_pNPCDialogue) m_pNPCDialogue->Open();
                break;
            default: break;
        }
    }

    void CUIMobileSystem::Hide(MOBILE_UI_TYPE type)
    {
        switch (type)
        {
            case UI_INVENTORY:
                if (GetInventory()) GetInventory()->Close();
                break;
            case UI_INVENTORY_EXT:
                if (m_pInventoryExt) m_pInventoryExt->Close();
                break;
            case UI_CHARACTER_INFO:
                if (m_pCharInfo) m_pCharInfo->Close();
                break;
            case UI_MOVE_COMMAND:
                if (m_pMoveCommand) m_pMoveCommand->Close();
                break;
            case UI_SKILL_SELECT:
                if (m_pSkillSelect) m_pSkillSelect->Close();
                break;
            case UI_PARTY:
                if (m_pParty) m_pParty->Close();
                break;
            case UI_NPC_SHOP:
                if (m_pNPCShop) m_pNPCShop->Close();
                break;
            case UI_STORAGE:
                if (m_pStorage) m_pStorage->Close();
                break;
            case UI_MIX:
                if (m_pMix) m_pMix->Close();
                break;
            case UI_TRADE:
                if (m_pTrade) m_pTrade->Close();
                break;
            case UI_OPTION:
                if (m_pOption) m_pOption->Close();
                break;
            case UI_HELPER:
                if (m_pHelper) m_pHelper->Close();
                break;
            case UI_NPC_DIALOGUE:
                if (m_pNPCDialogue) m_pNPCDialogue->Close();
                break;
            default: break;
        }
    }

    void CUIMobileSystem::Toggle(MOBILE_UI_TYPE type)
    {
        if (IsVisible(type))
        {
            Hide(type);
        }
        else
        {
            Show(type);
        }
    }

    void CUIMobileSystem::HideAll()
    {
        if (m_pInventoryExt && m_pInventoryExt->IsOpen()) m_pInventoryExt->Close();
        if (GetInventory() && GetInventory()->IsOpen()) GetInventory()->Close();
        if (m_pCharInfo && m_pCharInfo->IsOpen()) m_pCharInfo->Close();
        if (m_pMoveCommand && m_pMoveCommand->IsOpen()) m_pMoveCommand->Close();
        if (m_pSkillSelect && m_pSkillSelect->IsOpen()) m_pSkillSelect->Close();
        if (m_pParty && m_pParty->IsOpen()) m_pParty->Close();
        if (m_pNPCShop && m_pNPCShop->IsOpen()) m_pNPCShop->Close();
        if (m_pStorage && m_pStorage->IsOpen()) m_pStorage->Close();
        if (m_pMix && m_pMix->IsOpen()) m_pMix->Close();
        if (m_pTrade && m_pTrade->IsOpen()) m_pTrade->Close();
        if (m_pOption && m_pOption->IsOpen()) m_pOption->Close();
        if (m_pHelper && m_pHelper->IsOpen()) m_pHelper->Close();
        if (m_pNPCDialogue && m_pNPCDialogue->IsOpen()) m_pNPCDialogue->Close();
    }

    bool CUIMobileSystem::IsVisible(MOBILE_UI_TYPE type) const
    {
        switch (type)
        {
            case UI_INVENTORY:
                return (GetInventory() && GetInventory()->IsOpen());
            case UI_INVENTORY_EXT:
                return (m_pInventoryExt && m_pInventoryExt->IsOpen());
            case UI_CHARACTER_INFO:
                return (m_pCharInfo && m_pCharInfo->IsOpen());
            case UI_MOVE_COMMAND:
                return (m_pMoveCommand && m_pMoveCommand->IsOpen());
            case UI_SKILL_SELECT:
                return (m_pSkillSelect && m_pSkillSelect->IsOpen());
            case UI_PARTY:
                return (m_pParty && m_pParty->IsOpen());
            case UI_NPC_SHOP:
                return (m_pNPCShop && m_pNPCShop->IsOpen());
            case UI_STORAGE:
                return (m_pStorage && m_pStorage->IsOpen());
            case UI_MIX:
                return (m_pMix && m_pMix->IsOpen());
            case UI_TRADE:
                return (m_pTrade && m_pTrade->IsOpen());
            case UI_OPTION:
                return (m_pOption && m_pOption->IsOpen());
            case UI_HELPER:
                return (m_pHelper && m_pHelper->IsOpen());
            case UI_NPC_DIALOGUE:
                return (m_pNPCDialogue && m_pNPCDialogue->IsOpen());
            default: return false;
        }
    }

    bool CUIMobileSystem::IsAnyUIVisible() const
    {
        return (IsVisible(UI_INVENTORY)
            || IsVisible(UI_INVENTORY_EXT)
            || IsVisible(UI_CHARACTER_INFO)
            || IsVisible(UI_MOVE_COMMAND)
            || IsVisible(UI_SKILL_SELECT)
            || IsVisible(UI_PARTY)
            || IsVisible(UI_NPC_SHOP)
            || IsVisible(UI_STORAGE)
            || IsVisible(UI_MIX)
            || IsVisible(UI_TRADE)
            || IsVisible(UI_OPTION)
            || IsVisible(UI_HELPER)
            || IsVisible(UI_NPC_DIALOGUE));
    }

    bool CUIMobileSystem::OnFingerDown(const SDL_TouchFingerEvent& ev)
    {
        // Top modal dialogs first
        if (m_pOption && m_pOption->IsOpen() && m_pOption->OnFingerDown(ev)) return true;
        if (m_pHelper && m_pHelper->IsOpen() && m_pHelper->OnFingerDown(ev)) return true;
        if (m_pNPCDialogue && m_pNPCDialogue->IsOpen() && m_pNPCDialogue->OnFingerDown(ev)) return true;

        if (m_pInventoryExt && m_pInventoryExt->IsOpen() && m_pInventoryExt->OnFingerDown(ev)) return true;

        if (m_pNPCShop && m_pNPCShop->IsOpen() && m_pNPCShop->OnFingerDown(ev)) return true;
        if (m_pStorage && m_pStorage->IsOpen() && m_pStorage->OnFingerDown(ev)) return true;
        if (m_pMix && m_pMix->IsOpen() && m_pMix->OnFingerDown(ev)) return true;
        if (m_pTrade && m_pTrade->IsOpen() && m_pTrade->OnFingerDown(ev)) return true;

        if (m_pCharInfo && m_pCharInfo->IsOpen() && m_pCharInfo->OnFingerDown(ev)) return true;
        if (m_pMoveCommand && m_pMoveCommand->IsOpen() && m_pMoveCommand->OnFingerDown(ev)) return true;
        if (m_pSkillSelect && m_pSkillSelect->IsOpen() && m_pSkillSelect->OnFingerDown(ev)) return true;
        if (m_pParty && m_pParty->IsOpen() && m_pParty->OnFingerDown(ev)) return true;
        if (GetInventory() && GetInventory()->IsOpen() && GetInventory()->OnFingerDown(ev)) return true;

        return false;
    }

    bool CUIMobileSystem::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (m_pOption && m_pOption->IsOpen() && m_pOption->OnFingerMotion(ev)) return true;
        if (m_pHelper && m_pHelper->IsOpen() && m_pHelper->OnFingerMotion(ev)) return true;
        if (m_pNPCDialogue && m_pNPCDialogue->IsOpen() && m_pNPCDialogue->OnFingerMotion(ev)) return true;

        if (m_pInventoryExt && m_pInventoryExt->IsOpen() && m_pInventoryExt->OnFingerMotion(ev)) return true;

        if (m_pNPCShop && m_pNPCShop->IsOpen() && m_pNPCShop->OnFingerMotion(ev)) return true;
        if (m_pStorage && m_pStorage->IsOpen() && m_pStorage->OnFingerMotion(ev)) return true;
        if (m_pMix && m_pMix->IsOpen() && m_pMix->OnFingerMotion(ev)) return true;
        if (m_pTrade && m_pTrade->IsOpen() && m_pTrade->OnFingerMotion(ev)) return true;

        if (m_pCharInfo && m_pCharInfo->IsOpen() && m_pCharInfo->OnFingerMotion(ev)) return true;
        if (m_pMoveCommand && m_pMoveCommand->IsOpen() && m_pMoveCommand->OnFingerMotion(ev)) return true;
        if (m_pSkillSelect && m_pSkillSelect->IsOpen() && m_pSkillSelect->OnFingerMotion(ev)) return true;
        if (m_pParty && m_pParty->IsOpen() && m_pParty->OnFingerMotion(ev)) return true;
        if (GetInventory() && GetInventory()->IsOpen() && GetInventory()->OnFingerMotion(ev)) return true;

        return false;
    }

    bool CUIMobileSystem::OnFingerUp(const SDL_TouchFingerEvent& ev)
    {
        if (m_pOption && m_pOption->IsOpen() && m_pOption->OnFingerUp(ev)) return true;
        if (m_pHelper && m_pHelper->IsOpen() && m_pHelper->OnFingerUp(ev)) return true;
        if (m_pNPCDialogue && m_pNPCDialogue->IsOpen() && m_pNPCDialogue->OnFingerUp(ev)) return true;

        if (m_pInventoryExt && m_pInventoryExt->IsOpen() && m_pInventoryExt->OnFingerUp(ev)) return true;

        if (m_pNPCShop && m_pNPCShop->IsOpen() && m_pNPCShop->OnFingerUp(ev)) return true;
        if (m_pStorage && m_pStorage->IsOpen() && m_pStorage->OnFingerUp(ev)) return true;
        if (m_pMix && m_pMix->IsOpen() && m_pMix->OnFingerUp(ev)) return true;
        if (m_pTrade && m_pTrade->IsOpen() && m_pTrade->OnFingerUp(ev)) return true;

        if (m_pCharInfo && m_pCharInfo->IsOpen() && m_pCharInfo->OnFingerUp(ev)) return true;
        if (m_pMoveCommand && m_pMoveCommand->IsOpen() && m_pMoveCommand->OnFingerUp(ev)) return true;
        if (m_pSkillSelect && m_pSkillSelect->IsOpen() && m_pSkillSelect->OnFingerUp(ev)) return true;
        if (m_pParty && m_pParty->IsOpen() && m_pParty->OnFingerUp(ev)) return true;
        if (GetInventory() && GetInventory()->IsOpen() && GetInventory()->OnFingerUp(ev)) return true;

        return false;
    }

    void CUIMobileSystem::Update()
    {
        if (GetInventory()) GetInventory()->Update();
        if (m_pInventoryExt) m_pInventoryExt->Update();
        if (m_pCharInfo) m_pCharInfo->Update();
        if (m_pMoveCommand) m_pMoveCommand->Update();
        if (m_pSkillSelect) m_pSkillSelect->Update();
        if (m_pParty) m_pParty->Update();
        if (m_pNPCShop) m_pNPCShop->Update();
        if (m_pStorage) m_pStorage->Update();
        if (m_pMix) m_pMix->Update();
        if (m_pTrade) m_pTrade->Update();
        if (m_pOption) m_pOption->Update();
        if (m_pHelper) m_pHelper->Update();
        if (m_pNPCDialogue) m_pNPCDialogue->Update();
    }

    void CUIMobileSystem::Render()
    {
        if (GetInventory() && GetInventory()->IsOpen()) GetInventory()->Render();
        if (m_pInventoryExt && m_pInventoryExt->IsOpen()) m_pInventoryExt->Render();
        if (m_pCharInfo && m_pCharInfo->IsOpen()) m_pCharInfo->Render();
        if (m_pMoveCommand && m_pMoveCommand->IsOpen()) m_pMoveCommand->Render();
        if (m_pSkillSelect && m_pSkillSelect->IsOpen()) m_pSkillSelect->Render();
        if (m_pParty && m_pParty->IsOpen()) m_pParty->Render();
        if (m_pNPCShop && m_pNPCShop->IsOpen()) m_pNPCShop->Render();
        if (m_pStorage && m_pStorage->IsOpen()) m_pStorage->Render();
        if (m_pMix && m_pMix->IsOpen()) m_pMix->Render();
        if (m_pTrade && m_pTrade->IsOpen()) m_pTrade->Render();
        if (m_pNPCDialogue && m_pNPCDialogue->IsOpen()) m_pNPCDialogue->Render();
        if (m_pHelper && m_pHelper->IsOpen()) m_pHelper->Render();
        if (m_pOption && m_pOption->IsOpen()) m_pOption->Render();
    }

    void CUIMobileSystem::Render3D()
    {
        if (GetInventory() && GetInventory()->IsOpen()) GetInventory()->Render3D();
        if (m_pInventoryExt && m_pInventoryExt->IsOpen()) m_pInventoryExt->Render3D();
        if (m_pNPCShop && m_pNPCShop->IsOpen()) m_pNPCShop->Render3D();
        if (m_pStorage && m_pStorage->IsOpen()) m_pStorage->Render3D();
        if (m_pMix && m_pMix->IsOpen()) m_pMix->Render3D();
        if (m_pTrade && m_pTrade->IsOpen()) m_pTrade->Render3D();
    }
}

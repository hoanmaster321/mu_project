// =============================================================================
// UIMobileSystem.cpp
// Implementation of Central Mobile UI Coordinator & Event Dispatcher.
// =============================================================================

#include "stdafx.h"
#include "UIMobileSystem.h"
#include "UIMobileInventory.h"
#include "UIMobileCharacterInfo.h"
#include "UIMobileMoveCommand.h"
#include "UIMobileSkillSelect.h"
#include "UIMobileParty.h"

static UIMobile::CUIMobileSystem* s_pSystemInstance = nullptr;

namespace UIMobile
{
    CUIMobileSystem::CUIMobileSystem()
        : m_pNewUIMng(nullptr)
        , m_pNewUI3DRenderMng(nullptr)
        , m_pCharInfo(nullptr)
        , m_pMoveCommand(nullptr)
        , m_pSkillSelect(nullptr)
        , m_pParty(nullptr)
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

        return true;
    }

    void CUIMobileSystem::Release()
    {
        SAFE_DELETE(m_pCharInfo);
        SAFE_DELETE(m_pMoveCommand);
        SAFE_DELETE(m_pSkillSelect);
        SAFE_DELETE(m_pParty);

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

    void CUIMobileSystem::Show(MOBILE_UI_TYPE type)
    {
        // Exclusive window display
        HideAll();

        switch (type)
        {
            case UI_INVENTORY:
                if (GetInventory()) GetInventory()->Open();
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
        if (GetInventory() && GetInventory()->IsOpen()) GetInventory()->Close();
        if (m_pCharInfo && m_pCharInfo->IsOpen()) m_pCharInfo->Close();
        if (m_pMoveCommand && m_pMoveCommand->IsOpen()) m_pMoveCommand->Close();
        if (m_pSkillSelect && m_pSkillSelect->IsOpen()) m_pSkillSelect->Close();
        if (m_pParty && m_pParty->IsOpen()) m_pParty->Close();
    }

    bool CUIMobileSystem::IsVisible(MOBILE_UI_TYPE type) const
    {
        switch (type)
        {
            case UI_INVENTORY:
                return (GetInventory() && GetInventory()->IsOpen());
            case UI_CHARACTER_INFO:
                return (m_pCharInfo && m_pCharInfo->IsOpen());
            case UI_MOVE_COMMAND:
                return (m_pMoveCommand && m_pMoveCommand->IsOpen());
            case UI_SKILL_SELECT:
                return (m_pSkillSelect && m_pSkillSelect->IsOpen());
            case UI_PARTY:
                return (m_pParty && m_pParty->IsOpen());
            default: return false;
        }
    }

    bool CUIMobileSystem::IsAnyUIVisible() const
    {
        return (IsVisible(UI_INVENTORY)
            || IsVisible(UI_CHARACTER_INFO)
            || IsVisible(UI_MOVE_COMMAND)
            || IsVisible(UI_SKILL_SELECT)
            || IsVisible(UI_PARTY));
    }

    bool CUIMobileSystem::OnFingerDown(const SDL_TouchFingerEvent& ev)
    {
        if (m_pCharInfo && m_pCharInfo->IsOpen() && m_pCharInfo->OnFingerDown(ev))
            return true;
        if (m_pMoveCommand && m_pMoveCommand->IsOpen() && m_pMoveCommand->OnFingerDown(ev))
            return true;
        if (m_pSkillSelect && m_pSkillSelect->IsOpen() && m_pSkillSelect->OnFingerDown(ev))
            return true;
        if (m_pParty && m_pParty->IsOpen() && m_pParty->OnFingerDown(ev))
            return true;
        if (GetInventory() && GetInventory()->IsOpen() && GetInventory()->OnFingerDown(ev))
            return true;

        return false;
    }

    bool CUIMobileSystem::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (m_pCharInfo && m_pCharInfo->IsOpen() && m_pCharInfo->OnFingerMotion(ev))
            return true;
        if (m_pMoveCommand && m_pMoveCommand->IsOpen() && m_pMoveCommand->OnFingerMotion(ev))
            return true;
        if (m_pSkillSelect && m_pSkillSelect->IsOpen() && m_pSkillSelect->OnFingerMotion(ev))
            return true;
        if (m_pParty && m_pParty->IsOpen() && m_pParty->OnFingerMotion(ev))
            return true;
        if (GetInventory() && GetInventory()->IsOpen() && GetInventory()->OnFingerMotion(ev))
            return true;

        return false;
    }

    bool CUIMobileSystem::OnFingerUp(const SDL_TouchFingerEvent& ev)
    {
        if (m_pCharInfo && m_pCharInfo->IsOpen() && m_pCharInfo->OnFingerUp(ev))
            return true;
        if (m_pMoveCommand && m_pMoveCommand->IsOpen() && m_pMoveCommand->OnFingerUp(ev))
            return true;
        if (m_pSkillSelect && m_pSkillSelect->IsOpen() && m_pSkillSelect->OnFingerUp(ev))
            return true;
        if (m_pParty && m_pParty->IsOpen() && m_pParty->OnFingerUp(ev))
            return true;
        if (GetInventory() && GetInventory()->IsOpen() && GetInventory()->OnFingerUp(ev))
            return true;

        return false;
    }

    void CUIMobileSystem::Update()
    {
        if (GetInventory()) GetInventory()->Update();
        if (m_pCharInfo) m_pCharInfo->Update();
        if (m_pMoveCommand) m_pMoveCommand->Update();
        if (m_pSkillSelect) m_pSkillSelect->Update();
        if (m_pParty) m_pParty->Update();
    }

    void CUIMobileSystem::Render()
    {
        if (GetInventory() && GetInventory()->IsOpen()) GetInventory()->Render();
        if (m_pCharInfo && m_pCharInfo->IsOpen()) m_pCharInfo->Render();
        if (m_pMoveCommand && m_pMoveCommand->IsOpen()) m_pMoveCommand->Render();
        if (m_pSkillSelect && m_pSkillSelect->IsOpen()) m_pSkillSelect->Render();
        if (m_pParty && m_pParty->IsOpen()) m_pParty->Render();
    }

    void CUIMobileSystem::Render3D()
    {
        if (GetInventory() && GetInventory()->IsOpen())
        {
            GetInventory()->Render3D();
        }
    }
}

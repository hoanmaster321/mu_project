// =============================================================================
// UIMobileCharacterInfo.cpp
// Implementation of Mobile Character Stats & Point Distribution.
// =============================================================================

#include "stdafx.h"
#include "UIMobileCharacterInfo.h"
#include "ZzzInfomation.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "CharacterManager.h"
#include "DSPlaySound.h"
#include "wsclientinline.h"

#include <cstdio>
#include <algorithm>

namespace SEASON3B
{
    CUIMobileCharacterInfo::CUIMobileCharacterInfo()
        : m_pNewUIMng(nullptr)
        , m_bIsOpen(false)
        , m_winX(0.0f), m_winY(0.0f), m_winW(0.0f), m_winH(0.0f)
        , m_closeBtnX(0.0f), m_closeBtnY(0.0f), m_closeBtnSize(32.0f)
        , m_resetBtnX(0.0f), m_resetBtnY(0.0f), m_resetBtnW(0.0f), m_resetBtnH(0.0f)
        , m_pressedStat(-1), m_pressedStep(-1)
        , m_resetPressed(false)
        , m_closePressed(false)
    {
        std::memset(m_statBtns, 0, sizeof(m_statBtns));
    }

    CUIMobileCharacterInfo::~CUIMobileCharacterInfo()
    {
        Release();
    }

    bool CUIMobileCharacterInfo::Create(CNewUIManager* pNewUIMng)
    {
        m_pNewUIMng = pNewUIMng;
        if (m_pNewUIMng)
        {
            m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_CHARACTER, this);
        }
        return true;
    }

    void CUIMobileCharacterInfo::Release()
    {
        if (m_pNewUIMng)
        {
            m_pNewUIMng->RemoveUIObj(this);
            m_pNewUIMng = nullptr;
        }
        m_bIsOpen = false;
    }

    void CUIMobileCharacterInfo::Open()
    {
        m_bIsOpen = true;
        ComputeLayout();
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileCharacterInfo::Close()
    {
        m_bIsOpen = false;
        m_pressedStat = -1;
        m_pressedStep = -1;
        m_resetPressed = false;
        m_closePressed = false;
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileCharacterInfo::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    void CUIMobileCharacterInfo::ComputeLayout()
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        m_winW = 540.0f;
        m_winH = 400.0f;
        m_winX = (winW - m_winW) * 0.5f;
        m_winY = (winH - m_winH) * 0.5f;

        m_closeBtnSize = 30.0f;
        m_closeBtnX = m_winX + m_winW - m_closeBtnSize - 8.0f;
        m_closeBtnY = m_winY + 8.0f;

        m_resetBtnW = 100.0f;
        m_resetBtnH = 30.0f;
        m_resetBtnX = m_winX + 24.0f;
        m_resetBtnY = m_winY + m_winH - m_resetBtnH - 14.0f;

        // 5 Stat rows (+1, +10, +100, +MAX buttons)
        const float rowStartY = m_winY + 110.0f;
        const float rowH = 46.0f;
        const float btnW = 38.0f;
        const float btnH = 26.0f;
        const float btnsStartX = m_winX + 160.0f;

        for (int i = 0; i < STAT_COUNT; ++i)
        {
            m_statBtns[i].btnY = rowStartY + (i * rowH);
            m_statBtns[i].btnW = btnW;
            m_statBtns[i].btnH = btnH;
            for (int s = 0; s < 4; ++s)
            {
                m_statBtns[i].btnX[s] = btnsStartX + (s * (btnW + 4.0f));
            }
        }
    }

    void CUIMobileCharacterInfo::ExecuteAddPoint(int statIndex, int amount)
    {
        if (!CharacterAttribute) return;
        const int availablePoints = static_cast<int>(CharacterAttribute->PrintPlayer.ViewPoint);
        if (availablePoints <= 0) return;

        int addAmount = amount;
        if (addAmount < 0 || addAmount > availablePoints)
        {
            addAmount = availablePoints; // MAX
        }

        if (addAmount <= 0) return;

        char szCmd[64];
        switch (statIndex)
        {
            case STAT_STR: std::snprintf(szCmd, sizeof(szCmd), "/addstr %d", addAmount); break;
            case STAT_AGI: std::snprintf(szCmd, sizeof(szCmd), "/addagi %d", addAmount); break;
            case STAT_VIT: std::snprintf(szCmd, sizeof(szCmd), "/addvit %d", addAmount); break;
            case STAT_ENE: std::snprintf(szCmd, sizeof(szCmd), "/addene %d", addAmount); break;
            case STAT_CMD: std::snprintf(szCmd, sizeof(szCmd), "/addcmd %d", addAmount); break;
            default: return;
        }

        SendChat(szCmd);
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileCharacterInfo::ExecuteResetPoints()
    {
        SendChat("/taydiem all");
        PlayBuffer(SOUND_CLICK01);
    }

    bool CUIMobileCharacterInfo::OnFingerDown(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        float tx = 0.0f, ty = 0.0f;
        UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);

        // Outside modal -> ignore or consume touch
        if (!UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH))
        {
            return false;
        }

        // Close button
        if (UIMobile::HitTestRect(tx, ty, m_closeBtnX, m_closeBtnY, m_closeBtnSize, m_closeBtnSize))
        {
            m_closePressed = true;
            return true;
        }

        // Reset points button
        if (UIMobile::HitTestRect(tx, ty, m_resetBtnX, m_resetBtnY, m_resetBtnW, m_resetBtnH))
        {
            m_resetPressed = true;
            return true;
        }

        // Stepper buttons for 5 stats
        const bool isDL = (CharacterAttribute && gCharacterManager.GetCharacterClass(CharacterAttribute->Class) == CLASS_DARK_LORD);
        const int numStats = isDL ? STAT_COUNT : (STAT_COUNT - 1);

        for (int i = 0; i < numStats; ++i)
        {
            for (int s = 0; s < 4; ++s)
            {
                if (UIMobile::HitTestRect(tx, ty, m_statBtns[i].btnX[s], m_statBtns[i].btnY, m_statBtns[i].btnW, m_statBtns[i].btnH))
                {
                    m_pressedStat = i;
                    m_pressedStep = s;
                    return true;
                }
            }
        }

        return true;
    }

    bool CUIMobileCharacterInfo::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        float tx = 0.0f, ty = 0.0f;
        UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);
        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileCharacterInfo::OnFingerUp(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        float tx = 0.0f, ty = 0.0f;
        UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);

        if (m_closePressed)
        {
            m_closePressed = false;
            if (UIMobile::HitTestRect(tx, ty, m_closeBtnX, m_closeBtnY, m_closeBtnSize, m_closeBtnSize))
            {
                Close();
                return true;
            }
        }

        if (m_resetPressed)
        {
            m_resetPressed = false;
            if (UIMobile::HitTestRect(tx, ty, m_resetBtnX, m_resetBtnY, m_resetBtnW, m_resetBtnH))
            {
                ExecuteResetPoints();
                return true;
            }
        }

        if (m_pressedStat >= 0 && m_pressedStep >= 0)
        {
            const int stat = m_pressedStat;
            const int step = m_pressedStep;
            m_pressedStat = -1;
            m_pressedStep = -1;

            if (UIMobile::HitTestRect(tx, ty, m_statBtns[stat].btnX[step], m_statBtns[stat].btnY, m_statBtns[stat].btnW, m_statBtns[stat].btnH))
            {
                int amount = 1;
                if (step == 0) amount = 1;
                else if (step == 1) amount = 10;
                else if (step == 2) amount = 100;
                else if (step == 3) amount = -1; // MAX

                ExecuteAddPoint(stat, amount);
                return true;
            }
        }

        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileCharacterInfo::Update()
    {
        if (!m_bIsOpen) return true;
        ComputeLayout();
        return true;
    }

    bool CUIMobileCharacterInfo::UpdateKeyEvent()
    {
        if (!m_bIsOpen) return true;
        if (IsPress(VK_ESCAPE) || IsPress('C'))
        {
            Close();
            return false;
        }
        return true;
    }

    bool CUIMobileCharacterInfo::Render()
    {
        if (!m_bIsOpen || !CharacterAttribute) return true;

        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // Header Title
        char titleBuf[128];
        std::snprintf(titleBuf, sizeof(titleBuf), "[NHAN VAT] %s  (Lv.%d)", CharacterAttribute->Name, CharacterAttribute->Level);
        UIMobile::DrawPanel(m_winX, m_winY, m_winW, m_winH, titleBuf, true);

        // Close button
        UIMobile::DrawCloseButton(m_closeBtnX, m_closeBtnY, m_closeBtnSize, m_closePressed);

        if (g_pRenderText)
        {
            // Available Points
            const int points = static_cast<int>(CharacterAttribute->PrintPlayer.ViewPoint);
            char pointBuf[64];
            std::snprintf(pointBuf, sizeof(pointBuf), "DIEM TIEM NANG: %d", points);

            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(255, 230, 80, 255);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->RenderText(static_cast<int>(m_winX + 24.0f), static_cast<int>(m_winY + 44.0f), pointBuf);

            // Left Column: 5 Stats
            const char* statNames[STAT_COUNT] = { "Suc Manh (STR)", "Nhanh Nhen (AGI)", "The Luc (VIT)", "Nang Luong (ENE)", "Menh Lenh (CMD)" };
            const int baseStats[STAT_COUNT] = {
                CharacterAttribute->Strength,
                CharacterAttribute->Dexterity,
                CharacterAttribute->Vitality,
                CharacterAttribute->Energy,
                CharacterAttribute->Charisma
            };
            const int addStats[STAT_COUNT] = {
                CharacterAttribute->AddStrength,
                CharacterAttribute->AddDexterity,
                CharacterAttribute->AddVitality,
                CharacterAttribute->AddEnergy,
                CharacterAttribute->AddCharisma
            };

            const bool isDL = (gCharacterManager.GetCharacterClass(CharacterAttribute->Class) == CLASS_DARK_LORD);
            const int numStats = isDL ? STAT_COUNT : (STAT_COUNT - 1);

            const char* stepLabels[4] = { "+1", "+10", "+100", "MAX" };
            const bool hasPoints = (points > 0);

            for (int i = 0; i < numStats; ++i)
            {
                char statText[64];
                if (addStats[i] > 0)
                    std::snprintf(statText, sizeof(statText), "%s: %d (+%d)", statNames[i], baseStats[i], addStats[i]);
                else
                    std::snprintf(statText, sizeof(statText), "%s: %d", statNames[i], baseStats[i]);

                g_pRenderText->SetFont(g_hFont);
                g_pRenderText->SetTextColor(220, 235, 255, 255);
                g_pRenderText->RenderText(static_cast<int>(m_winX + 24.0f), static_cast<int>(m_statBtns[i].btnY + 6.0f), statText);

                // Stepper Buttons
                for (int s = 0; s < 4; ++s)
                {
                    const bool isPressed = (m_pressedStat == i && m_pressedStep == s);
                    UIMobile::DrawButton(
                        m_statBtns[i].btnX[s],
                        m_statBtns[i].btnY,
                        m_statBtns[i].btnW,
                        m_statBtns[i].btnH,
                        stepLabels[s],
                        isPressed,
                        hasPoints,
                        (s == 3) ? UIMobile::Colors::BtnSuccess : UIMobile::Colors::BtnPrimary
                    );
                }
            }

            // Divider Line between columns
            const float divX = m_winX + 340.0f;
            UIMobile::DrawSolidRect(divX, m_winY + 44.0f, 1.5f, m_winH - 64.0f, UIMobile::Colors::SlotBorder);

            // Right Column: Combat Attributes
            const float rightX = divX + 16.0f;
            float rY = m_winY + 48.0f;
            constexpr float rStep = 26.0f;

            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(100, 200, 255, 255);
            g_pRenderText->RenderText(static_cast<int>(rightX), static_cast<int>(rY), "CHI SO CHIEN DAU");
            rY += 28.0f;

            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetTextColor(210, 220, 235, 255);

            char buf[64];
            std::snprintf(buf, sizeof(buf), "Tan Cong: %d ~ %d", CharacterAttribute->AttackDamageMinRight, CharacterAttribute->AttackDamageMaxRight);
            g_pRenderText->RenderText(static_cast<int>(rightX), static_cast<int>(rY), buf);
            rY += rStep;

            std::snprintf(buf, sizeof(buf), "Sat Thuong Phep: %d ~ %d", CharacterAttribute->MagicDamageMin, CharacterAttribute->MagicDamageMax);
            g_pRenderText->RenderText(static_cast<int>(rightX), static_cast<int>(rY), buf);
            rY += rStep;

            std::snprintf(buf, sizeof(buf), "Toc Do Danh: %d", CharacterAttribute->AttackSpeed);
            g_pRenderText->RenderText(static_cast<int>(rightX), static_cast<int>(rY), buf);
            rY += rStep;

            std::snprintf(buf, sizeof(buf), "Phong Thu: %d", CharacterAttribute->Defense);
            g_pRenderText->RenderText(static_cast<int>(rightX), static_cast<int>(rY), buf);
            rY += rStep;

            std::snprintf(buf, sizeof(buf), "Ty Le Danh: %d", CharacterAttribute->AttackRating);
            g_pRenderText->RenderText(static_cast<int>(rightX), static_cast<int>(rY), buf);
            rY += rStep;

            std::snprintf(buf, sizeof(buf), "Ty Le Ne Tranh: %d", CharacterAttribute->SuccessfulBlocking);
            g_pRenderText->RenderText(static_cast<int>(rightX), static_cast<int>(rY), buf);
            rY += rStep;

            std::snprintf(buf, sizeof(buf), "Mau (HP): %d / %d", CharacterAttribute->Life, CharacterAttribute->LifeMax);
            g_pRenderText->SetTextColor(255, 110, 110, 255);
            g_pRenderText->RenderText(static_cast<int>(rightX), static_cast<int>(rY), buf);
            rY += rStep;

            std::snprintf(buf, sizeof(buf), "Mana (MP): %d / %d", CharacterAttribute->Mana, CharacterAttribute->ManaMax);
            g_pRenderText->SetTextColor(110, 180, 255, 255);
            g_pRenderText->RenderText(static_cast<int>(rightX), static_cast<int>(rY), buf);
            rY += rStep;

            std::snprintf(buf, sizeof(buf), "Giap (SD): %d / %d", CharacterAttribute->Shield, CharacterAttribute->ShieldMax);
            g_pRenderText->SetTextColor(255, 215, 100, 255);
            g_pRenderText->RenderText(static_cast<int>(rightX), static_cast<int>(rY), buf);
        }

        // Reset points button
        UIMobile::DrawButton(m_resetBtnX, m_resetBtnY, m_resetBtnW, m_resetBtnH, "TAY DIEM", m_resetPressed, true, UIMobile::Colors::BtnDanger);

        DisableAlphaBlend();
        return true;
    }
}

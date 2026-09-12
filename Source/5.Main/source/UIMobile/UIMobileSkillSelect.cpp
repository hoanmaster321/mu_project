// =============================================================================
// UIMobileSkillSelect.cpp
// Implementation of Mobile Skill Selection & Quick Combat Slot Assignment.
// =============================================================================

#include "stdafx.h"
#include "UIMobileSkillSelect.h"
#include "NewUISystem.h"
#include "NewUIMainFrameWindow.h"
#include "ZzzInfomation.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "DSPlaySound.h"
#include "SkillManager.h"

#include <cstdio>
#include <algorithm>

namespace SEASON3B
{
    CUIMobileSkillSelect::CUIMobileSkillSelect()
        : m_pNewUIMng(nullptr)
        , m_bIsOpen(false)
        , m_winX(0.0f), m_winY(0.0f), m_winW(0.0f), m_winH(0.0f)
        , m_closeBtnX(0.0f), m_closeBtnY(0.0f), m_closeBtnSize(30.0f)
        , m_gridX(0.0f), m_gridY(0.0f), m_gridW(0.0f), m_gridH(0.0f)
        , m_scrollY(0.0f), m_maxScrollY(0.0f)
        , m_isScrolling(false), m_scrollFingerId(0)
        , m_touchDownY(0.0f), m_lastTouchY(0.0f)
        , m_cardX(0.0f), m_cardY(0.0f), m_cardW(0.0f), m_cardH(0.0f)
        , m_assignBtnW(0.0f), m_assignBtnH(0.0f)
        , m_selectedIdx(-1), m_pressedAssignSlot(-1), m_closePressed(false)
    {
        std::memset(m_assignBtnX, 0, sizeof(m_assignBtnX));
        std::memset(m_assignBtnY, 0, sizeof(m_assignBtnY));
    }

    CUIMobileSkillSelect::~CUIMobileSkillSelect()
    {
        Release();
    }

    bool CUIMobileSkillSelect::Create(CNewUIManager* pNewUIMng)
    {
        m_pNewUIMng = pNewUIMng;
        if (m_pNewUIMng)
        {
            m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_SKILL_LIST, this);
        }
        return true;
    }

    void CUIMobileSkillSelect::Release()
    {
        if (m_pNewUIMng)
        {
            m_pNewUIMng->RemoveUIObj(this);
            m_pNewUIMng = nullptr;
        }
        m_bIsOpen = false;
        m_skills.clear();
    }

    void CUIMobileSkillSelect::Open()
    {
        m_bIsOpen = true;
        m_scrollY = 0.0f;
        m_isScrolling = false;
        m_selectedIdx = -1;
        m_pressedAssignSlot = -1;
        m_closePressed = false;
        RefreshSkills();
        ComputeLayout();
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileSkillSelect::Close()
    {
        m_bIsOpen = false;
        m_isScrolling = false;
        m_selectedIdx = -1;
        m_pressedAssignSlot = -1;
        m_closePressed = false;
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileSkillSelect::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    void CUIMobileSkillSelect::RefreshSkills()
    {
        m_skills.clear();
        if (!CharacterAttribute) return;

        for (int i = 0; i < MAX_MAGIC; ++i)
        {
            const int sType = CharacterAttribute->Skill[i];
            if (sType > 0 && !(sType >= AT_SKILL_STUN && sType <= AT_SKILL_REMOVAL_BUFF))
            {
                SkillEntry entry;
                entry.skillIndex = i;
                entry.skillType = sType;
                entry.assignedSlot = -1;

                if (g_pSkillList != nullptr)
                {
                    for (int k = 1; k <= 4; ++k)
                    {
                        if (g_pSkillList->GetHotKey(k) == sType)
                        {
                            entry.assignedSlot = k;
                            break;
                        }
                    }
                }

                m_skills.push_back(entry);
            }
        }

        if (m_selectedIdx == -1 && !m_skills.empty())
        {
            m_selectedIdx = 0;
        }

        // Calculate max scroll for 4-column grid
        constexpr float slotSize = 48.0f;
        constexpr float slotGap = 8.0f;
        const int numRows = (static_cast<int>(m_skills.size()) + 3) / 4;
        const float totalH = static_cast<float>(numRows) * (slotSize + slotGap);
        m_maxScrollY = (std::max)(0.0f, totalH - m_gridH);
    }

    void CUIMobileSkillSelect::ComputeLayout()
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        m_winW = 540.0f;
        m_winH = 410.0f;
        m_winX = (winW - m_winW) * 0.5f;
        m_winY = (winH - m_winH) * 0.5f;

        m_closeBtnSize = 30.0f;
        m_closeBtnX = m_winX + m_winW - m_closeBtnSize - 8.0f;
        m_closeBtnY = m_winY + 8.0f;

        // Left Grid Bounds (4 columns)
        m_gridX = m_winX + 16.0f;
        m_gridY = m_winY + 48.0f;
        m_gridW = 250.0f;
        m_gridH = m_winH - 64.0f;

        // Right Detail Card Bounds
        m_cardX = m_gridX + m_gridW + 16.0f;
        m_cardY = m_winY + 48.0f;
        m_cardW = m_winW - (m_cardX - m_winX) - 16.0f;
        m_cardH = m_gridH;

        // 4 Assign Buttons (2x2 grid)
        m_assignBtnW = (m_cardW - 32.0f) * 0.5f;
        m_assignBtnH = 36.0f;
        const float btnRow1Y = m_cardY + m_cardH - (m_assignBtnH * 2.0f) - 24.0f;
        const float btnRow2Y = btnRow1Y + m_assignBtnH + 8.0f;

        m_assignBtnX[0] = m_cardX + 12.0f;
        m_assignBtnY[0] = btnRow1Y;

        m_assignBtnX[1] = m_cardX + 12.0f + m_assignBtnW + 8.0f;
        m_assignBtnY[1] = btnRow1Y;

        m_assignBtnX[2] = m_cardX + 12.0f;
        m_assignBtnY[2] = btnRow2Y;

        m_assignBtnX[3] = m_cardX + 12.0f + m_assignBtnW + 8.0f;
        m_assignBtnY[3] = btnRow2Y;
    }

    void CUIMobileSkillSelect::ExecuteAssignSlot(int slotNumber)
    {
        if (m_selectedIdx < 0 || m_selectedIdx >= static_cast<int>(m_skills.size())) return;
        if (g_pSkillList == nullptr) return;

        const int sType = m_skills[m_selectedIdx].skillType;
        g_pSkillList->SetHotKey(slotNumber, sType);
        PlayBuffer(SOUND_CLICK01);
        RefreshSkills();
    }

    bool CUIMobileSkillSelect::OnFingerDown(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        float tx = 0.0f, ty = 0.0f;
        UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);

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

        // Inside Grid
        if (UIMobile::HitTestRect(tx, ty, m_gridX, m_gridY, m_gridW, m_gridH))
        {
            m_isScrolling = true;
            m_scrollFingerId = ev.fingerID;
            m_touchDownY = ty;
            m_lastTouchY = ty;

            constexpr float slotSize = 48.0f;
            constexpr float slotGap = 8.0f;
            const float relX = tx - m_gridX - 8.0f;
            const float relY = (ty - m_gridY - 8.0f) + m_scrollY;

            const int col = static_cast<int>(relX / (slotSize + slotGap));
            const int row = static_cast<int>(relY / (slotSize + slotGap));

            if (col >= 0 && col < 4 && row >= 0)
            {
                const int idx = row * 4 + col;
                if (idx >= 0 && idx < static_cast<int>(m_skills.size()))
                {
                    m_selectedIdx = idx;
                    PlayBuffer(SOUND_CLICK01);
                }
            }
            return true;
        }

        // Inside Assign Buttons
        for (int k = 0; k < 4; ++k)
        {
            if (UIMobile::HitTestRect(tx, ty, m_assignBtnX[k], m_assignBtnY[k], m_assignBtnW, m_assignBtnH))
            {
                m_pressedAssignSlot = k + 1;
                return true;
            }
        }

        return true;
    }

    bool CUIMobileSkillSelect::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        float tx = 0.0f, ty = 0.0f;
        UIMobile::TouchToVirtual(ev.x, ev.y, tx, ty);

        if (m_isScrolling && ev.fingerID == m_scrollFingerId)
        {
            const float deltaY = ty - m_lastTouchY;
            m_lastTouchY = ty;
            m_scrollY -= deltaY;
            m_scrollY = std::clamp(m_scrollY, 0.0f, m_maxScrollY);
            return true;
        }

        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileSkillSelect::OnFingerUp(const SDL_TouchFingerEvent& ev)
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

        if (m_isScrolling && ev.fingerID == m_scrollFingerId)
        {
            m_isScrolling = false;
            return true;
        }

        if (m_pressedAssignSlot > 0)
        {
            const int slot = m_pressedAssignSlot;
            m_pressedAssignSlot = -1;
            if (UIMobile::HitTestRect(tx, ty, m_assignBtnX[slot - 1], m_assignBtnY[slot - 1], m_assignBtnW, m_assignBtnH))
            {
                ExecuteAssignSlot(slot);
                return true;
            }
        }

        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileSkillSelect::Update()
    {
        if (!m_bIsOpen) return true;
        ComputeLayout();
        return true;
    }

    bool CUIMobileSkillSelect::UpdateKeyEvent()
    {
        if (!m_bIsOpen) return true;
        if (IsPress(VK_ESCAPE) || IsPress('K'))
        {
            Close();
            return false;
        }
        return true;
    }

    bool CUIMobileSkillSelect::Render()
    {
        if (!m_bIsOpen) return true;

        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // Header Title
        UIMobile::DrawPanel(m_winX, m_winY, m_winW, m_winH, "[DANH SACH KY NANG] - CHON DE GAN VAO PHIM DANH", true);

        // Close Button
        UIMobile::DrawCloseButton(m_closeBtnX, m_closeBtnY, m_closeBtnSize, m_closePressed);

        // Left Grid Container
        UIMobile::DrawSolidRect(m_gridX, m_gridY, m_gridW, m_gridH, UIMobile::Colors::SlotBg);
        UIMobile::DrawBorder(m_gridX, m_gridY, m_gridW, m_gridH, 1.5f, UIMobile::Colors::SlotBorder);

        // Render Skill Slots
        constexpr float slotSize = 48.0f;
        constexpr float slotGap = 8.0f;
        const float visibleStartY = m_gridY;
        const float visibleEndY = m_gridY + m_gridH;

        for (size_t i = 0; i < m_skills.size(); ++i)
        {
            const int col = static_cast<int>(i % 4);
            const int row = static_cast<int>(i / 4);

            const float sx = m_gridX + 8.0f + (static_cast<float>(col) * (slotSize + slotGap));
            const float sy = m_gridY + 8.0f + (static_cast<float>(row) * (slotSize + slotGap)) - m_scrollY;

            if (sy + slotSize < visibleStartY || sy > visibleEndY) continue;

            const bool isSelected = (m_selectedIdx == static_cast<int>(i));

            // Slot Background
            UIMobile::DrawSolidRect(sx, sy, slotSize, slotSize, isSelected ? UIMobile::ColorRGBA(0.2f, 0.35f, 0.55f, 0.9f) : UIMobile::ColorRGBA(0.1f, 0.14f, 0.2f, 0.85f));

            // Render Skill Icon via CNewUISkillList preview
            if (g_pSkillList != nullptr)
            {
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                g_pSkillList->RenderSkillIconPreview(m_skills[i].skillIndex, sx + 4.0f, sy + 4.0f, slotSize - 8.0f, slotSize - 8.0f);
            }

            // Border
            if (isSelected)
            {
                UIMobile::DrawBorder(sx, sy, slotSize, slotSize, 2.0f, UIMobile::Colors::SlotHighlight);
            }
            else
            {
                UIMobile::DrawBorder(sx, sy, slotSize, slotSize, 1.0f, UIMobile::Colors::SlotBorder);
            }

            // Assigned slot badge
            if (m_skills[i].assignedSlot > 0 && g_pRenderText)
            {
                char badge[8];
                std::snprintf(badge, sizeof(badge), "P%d", m_skills[i].assignedSlot);
                UIMobile::DrawSolidRect(sx + slotSize - 16.0f, sy, 16.0f, 14.0f, UIMobile::ColorRGBA(0.9f, 0.7f, 0.1f, 0.95f));

                g_pRenderText->SetFont(g_hFontBold);
                g_pRenderText->SetTextColor(0, 0, 0, 255);
                g_pRenderText->SetBgColor(0, 0, 0, 0);
                g_pRenderText->RenderText(static_cast<int>(sx + slotSize - 14.0f), static_cast<int>(sy + 1.0f), badge);
            }
        }

        // Right Detail Card
        UIMobile::DrawSolidRect(m_cardX, m_cardY, m_cardW, m_cardH, UIMobile::ColorRGBA(0.08f, 0.12f, 0.18f, 0.90f));
        UIMobile::DrawBorder(m_cardX, m_cardY, m_cardW, m_cardH, 1.5f, UIMobile::Colors::PanelBorder);

        if (m_selectedIdx >= 0 && m_selectedIdx < static_cast<int>(m_skills.size()) && SkillAttribute)
        {
            const int sType = m_skills[m_selectedIdx].skillType;
            SKILL_ATTRIBUTE* pAttr = &SkillAttribute[sType];

            if (g_pRenderText && pAttr)
            {
                // Skill Name
                g_pRenderText->SetFont(g_hFontBold);
                g_pRenderText->SetTextColor(255, 230, 100, 255);
                g_pRenderText->SetBgColor(0, 0, 0, 0);
                g_pRenderText->RenderText(static_cast<int>(m_cardX + 16.0f), static_cast<int>(m_cardY + 16.0f), pAttr->Name);

                // Stats
                g_pRenderText->SetFont(g_hFont);
                g_pRenderText->SetTextColor(220, 230, 245, 255);

                char buf[64];
                std::snprintf(buf, sizeof(buf), "Tieu hao Mana: %d", pAttr->Mana);
                g_pRenderText->RenderText(static_cast<int>(m_cardX + 16.0f), static_cast<int>(m_cardY + 46.0f), buf);

                std::snprintf(buf, sizeof(buf), "Tieu hao AG: %d", pAttr->AbilityGuage);
                g_pRenderText->RenderText(static_cast<int>(m_cardX + 16.0f), static_cast<int>(m_cardY + 70.0f), buf);

                std::snprintf(buf, sizeof(buf), "Khoang cach: %u o", pAttr->Distance);
                g_pRenderText->RenderText(static_cast<int>(m_cardX + 16.0f), static_cast<int>(m_cardY + 94.0f), buf);

                std::snprintf(buf, sizeof(buf), "Sat thuong co ban: %d", pAttr->Damage);
                g_pRenderText->RenderText(static_cast<int>(m_cardX + 16.0f), static_cast<int>(m_cardY + 118.0f), buf);

                // Instruction prompt
                g_pRenderText->SetFont(g_hFontBold);
                g_pRenderText->SetTextColor(100, 220, 255, 255);
                g_pRenderText->RenderText(static_cast<int>(m_cardX + 16.0f), static_cast<int>(m_assignBtnY[0] - 22.0f), "CHON O GAN VAO HUD:");
            }

            // 4 Assign Buttons
            const char* slotNames[4] = { "GAN SLOT 1", "GAN SLOT 2", "GAN SLOT 3", "GAN SLOT 4" };
            for (int k = 0; k < 4; ++k)
            {
                const bool isPressed = (m_pressedAssignSlot == (k + 1));
                const bool isCurAssigned = (m_skills[m_selectedIdx].assignedSlot == (k + 1));

                UIMobile::DrawButton(
                    m_assignBtnX[k],
                    m_assignBtnY[k],
                    m_assignBtnW,
                    m_assignBtnH,
                    slotNames[k],
                    isPressed,
                    true,
                    isCurAssigned ? UIMobile::Colors::BtnSuccess : UIMobile::Colors::BtnPrimary
                );
            }
        }

        DisableAlphaBlend();
        return true;
    }
}

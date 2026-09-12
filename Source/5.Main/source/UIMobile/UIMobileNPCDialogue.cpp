// =============================================================================
// UIMobileNPCDialogue.cpp
// Implementation of Mobile Visual RPG NPC Dialogue Interface.
// =============================================================================

#include "stdafx.h"
#include "UIMobileNPCDialogue.h"
#include "NewUISystem.h"
#include "NewUINPCDialogue.h"
#include "ZzzScene.h"
#include "ZzzOpenglUtil.h"
#include "DSPlaySound.h"

namespace SEASON3B
{
    CUIMobileNPCDialogue::CUIMobileNPCDialogue()
        : m_pNewUIMng(nullptr)
        , m_bIsOpen(false)
        , m_boxX(0.0f), m_boxY(0.0f), m_boxW(0.0f), m_boxH(0.0f)
        , m_closeBtnX(0.0f), m_closeBtnY(0.0f), m_closeBtnSize(0.0f)
        , m_choiceCount(0)
    {
    }

    CUIMobileNPCDialogue::~CUIMobileNPCDialogue()
    {
        Release();
    }

    bool CUIMobileNPCDialogue::Create(CNewUIManager* pNewUIMng)
    {
        m_pNewUIMng = pNewUIMng;
        ComputeLayout();
        return true;
    }

    void CUIMobileNPCDialogue::Release()
    {
        m_pNewUIMng = nullptr;
        m_bIsOpen = false;
    }

    void CUIMobileNPCDialogue::ComputeLayout()
    {
        float screenW = (float)UIMobile::Screen::GetWidth();
        float screenH = (float)UIMobile::Screen::GetHeight();

        m_boxW = (std::min)(screenW * 0.85f, 680.0f);
        m_boxH = 175.0f;
        m_boxX = (screenW - m_boxW) * 0.5f;
        m_boxY = screenH - m_boxH - 24.0f;

        m_closeBtnSize = 34.0f;
        m_closeBtnX = m_boxX + m_boxW - m_closeBtnSize - 10.0f;
        m_closeBtnY = m_boxY + 10.0f;

        // Choice buttons arranged vertically on the upper-right of dialogue or floating right
        float choiceW = 200.0f;
        float choiceH = 34.0f;
        float choiceX = m_boxX + m_boxW - choiceW - 14.0f;
        float choiceStartY = m_boxY - 8.0f;

        m_choiceCount = 0;
        if (g_pNPCDialogue)
        {
            int cnt = g_pNPCDialogue->GetSelTextCount();
            m_choiceCount = (std::min)(cnt, MAX_CHOICES);
        }

        for (int i = 0; i < MAX_CHOICES; ++i)
        {
            m_choices[i].index = i;
            // Float above the dialogue box
            m_choices[i].rect = {
                choiceX,
                choiceStartY - (float)(i + 1) * (choiceH + 6.0f),
                choiceW,
                choiceH
            };
        }
    }

    void CUIMobileNPCDialogue::Open()
    {
        m_bIsOpen = true;
        ComputeLayout();
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileNPCDialogue::Close()
    {
        m_bIsOpen = false;
        if (g_pNPCDialogue)
        {
            g_pNPCDialogue->ProcessClosing();
        }
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileNPCDialogue::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    bool CUIMobileNPCDialogue::Update()
    {
        if (m_bIsOpen)
        {
            // Sync choices count if dialogue changed
            if (g_pNPCDialogue)
            {
                int cnt = g_pNPCDialogue->GetSelTextCount();
                m_choiceCount = (std::min)(cnt, MAX_CHOICES);
            }
        }
        return true;
    }

    bool CUIMobileNPCDialogue::UpdateKeyEvent()
    {
        if (m_bIsOpen && SEASON3B::IsPress(VK_ESCAPE))
        {
            Close();
            return false;
        }
        return true;
    }

    void CUIMobileNPCDialogue::ExecuteChoice(int choiceIndex)
    {
        if (g_pNPCDialogue && choiceIndex >= 0)
        {
            g_pNPCDialogue->SelectAnswer(choiceIndex);
            PlayBuffer(SOUND_CLICK01);
            ComputeLayout();
        }
    }

    bool CUIMobileNPCDialogue::OnFingerDown(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;

        float touchX = ev.x * (float)UIMobile::Screen::GetWidth();
        float touchY = ev.y * (float)UIMobile::Screen::GetHeight();

        // Close Button
        if (touchX >= m_closeBtnX && touchX <= m_closeBtnX + m_closeBtnSize &&
            touchY >= m_closeBtnY && touchY <= m_closeBtnY + m_closeBtnSize)
        {
            Close();
            return true;
        }

        // Choice cards
        for (int i = 0; i < m_choiceCount; ++i)
        {
            if (m_choices[i].rect.Contains(touchX, touchY))
            {
                ExecuteChoice(i);
                return true;
            }
        }

        // Inside Dialogue Card (Tapping advances / closes)
        if (touchX >= m_boxX && touchX <= m_boxX + m_boxW &&
            touchY >= m_boxY && touchY <= m_boxY + m_boxH)
        {
            if (m_choiceCount == 0)
            {
                Close();
            }
            return true;
        }

        return false;
    }

    bool CUIMobileNPCDialogue::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        float touchX = ev.x * (float)UIMobile::Screen::GetWidth();
        float touchY = ev.y * (float)UIMobile::Screen::GetHeight();
        return (touchX >= m_boxX && touchX <= m_boxX + m_boxW && touchY >= m_boxY && touchY <= m_boxY + m_boxH);
    }

    bool CUIMobileNPCDialogue::OnFingerUp(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        float touchX = ev.x * (float)UIMobile::Screen::GetWidth();
        float touchY = ev.y * (float)UIMobile::Screen::GetHeight();
        return (touchX >= m_boxX && touchX <= m_boxX + m_boxW && touchY >= m_boxY && touchY <= m_boxY + m_boxH);
    }

    bool CUIMobileNPCDialogue::Render()
    {
        if (!m_bIsOpen) return true;

        // Dialogue Box Glass Panel
        UIMobile::Render::DrawGlassCard(m_boxX, m_boxY, m_boxW, m_boxH, 0.94f);
        UIMobile::Render::DrawPanelBorder(m_boxX, m_boxY, m_boxW, m_boxH, RGBA(255, 200, 80, 220));

        // NPC Portrait placeholder / Icon
        float portraitSize = 80.0f;
        float portraitX = m_boxX + 16.0f;
        float portraitY = m_boxY + 20.0f;
        UIMobile::Render::DrawGlassCard(portraitX, portraitY, portraitSize, portraitSize, 0.6f);
        UIMobile::Render::DrawPanelBorder(portraitX, portraitY, portraitSize, portraitSize, RGBA(200, 180, 100, 180));

        // Text & Header
        if (g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(RGBA(255, 220, 80, 255));
            g_pRenderText->SetBgColor(0);

            // NPC Name
            g_pRenderText->RenderText((int)(m_boxX + portraitSize + 32.0f), (int)(m_boxY + 18.0f), "NPC DIALOGUE");

            // Close Button
            g_pRenderText->SetTextColor(RGBA(255, 100, 100, 255));
            g_pRenderText->RenderText((int)(m_closeBtnX + 10.0f), (int)(m_closeBtnY + 8.0f), "X");

            // NPC Dialogue Words
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetTextColor(RGBA(240, 240, 250, 255));

            float textX = m_boxX + portraitSize + 32.0f;
            float textY = m_boxY + 44.0f;
            float lineH = 18.0f;

            if (g_pNPCDialogue)
            {
                for (int line = 0; line < 5; ++line)
                {
                    const char* pLine = g_pNPCDialogue->GetNPCWord(line);
                    if (pLine && pLine[0] != '\0')
                    {
                        g_pRenderText->RenderText((int)textX, (int)(textY + line * lineH), (char*)pLine);
                    }
                }
            }
            else
            {
                g_pRenderText->RenderText((int)textX, (int)textY, "Xin chào dũng sĩ lục địa MU!");
            }
        }

        // Render Floating Choice Buttons
        for (int i = 0; i < m_choiceCount; ++i)
        {
            const auto& c = m_choices[i];
            UIMobile::Render::DrawGlassCard(c.rect.x, c.rect.y, c.rect.w, c.rect.h, 0.88f);
            UIMobile::Render::DrawPanelBorder(c.rect.x, c.rect.y, c.rect.w, c.rect.h, RGBA(100, 200, 255, 220));

            if (g_pRenderText)
            {
                const char* pChoiceText = g_pNPCDialogue ? g_pNPCDialogue->GetSelText(i) : nullptr;
                g_pRenderText->SetFont(g_hFont);
                g_pRenderText->SetTextColor(RGBA(255, 230, 120, 255));
                g_pRenderText->RenderText((int)(c.rect.x + 14.0f), (int)(c.rect.y + 8.0f),
                    (pChoiceText && pChoiceText[0]) ? (char*)pChoiceText : "Tiếp tục...");
            }
        }

        return true;
    }
}

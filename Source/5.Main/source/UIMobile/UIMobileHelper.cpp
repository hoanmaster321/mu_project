// =============================================================================
// UIMobileHelper.cpp
// Implementation of Mobile MU Helper Interface.
// =============================================================================

#include "stdafx.h"
#include "UIMobileHelper.h"
#include "NewUISystem.h"
#include "NewUIMuHelper.h"
#include "CB_MUHelper.h"
#include "ZzzScene.h"
#include "ZzzOpenglUtil.h"

namespace SEASON3B
{
    CUIMobileHelper::CUIMobileHelper()
        : m_pNewUIMng(nullptr)
        , m_bIsOpen(false)
        , m_winX(0.0f), m_winY(0.0f), m_winW(0.0f), m_winH(0.0f)
        , m_closeBtnX(0.0f), m_closeBtnY(0.0f), m_closeBtnSize(0.0f)
        , m_bStarted(false)
        , m_huntingRange(6)
        , m_potionPercent(70)
        , m_bAutoPotion(true)
        , m_bPickJewel(true)
        , m_bPickZen(true)
        , m_bPickExc(true)
        , m_bPickSet(true)
        , m_bAutoBuffSelf(true)
        , m_bAutoBuffParty(true)
        , m_bReturnOriginal(true)
    {
    }

    CUIMobileHelper::~CUIMobileHelper()
    {
        Release();
    }

    bool CUIMobileHelper::Create(CNewUIManager* pNewUIMng)
    {
        m_pNewUIMng = pNewUIMng;
        ComputeLayout();
        return true;
    }

    void CUIMobileHelper::Release()
    {
        m_pNewUIMng = nullptr;
        m_bIsOpen = false;
    }

    void CUIMobileHelper::ComputeLayout()
    {
        m_winW = 490.0f;
        m_winH = 370.0f;
        m_winX = ((float)UIMobile::Screen::GetWidth() - m_winW) * 0.5f;
        m_winY = ((float)UIMobile::Screen::GetHeight() - m_winH) * 0.5f;

        m_closeBtnSize = 34.0f;
        m_closeBtnX = m_winX + m_winW - m_closeBtnSize - 12.0f;
        m_closeBtnY = m_winY + 10.0f;

        float startY = m_winY + 50.0f;
        float rowHeight = 44.0f;

        // Row 1: Hunting Range
        m_rcRangeMinus = { m_winX + m_winW - 140.0f, startY, 34.0f, 30.0f };
        m_rcRangePlus  = { m_winX + m_winW - 58.0f,  startY, 34.0f, 30.0f };

        // Row 2: Auto HP Potion
        float potBtnW = 48.0f;
        float potStartX = m_winX + m_winW - 220.0f;
        m_rcPot30 = { potStartX,                 startY + rowHeight, potBtnW, 30.0f };
        m_rcPot50 = { potStartX + potBtnW + 6,   startY + rowHeight, potBtnW, 30.0f };
        m_rcPot70 = { potStartX + (potBtnW+6)*2, startY + rowHeight, potBtnW, 30.0f };
        m_rcPot85 = { potStartX + (potBtnW+6)*3, startY + rowHeight, potBtnW, 30.0f };

        // Row 3: Auto Loot Items
        float lootBtnW = 98.0f;
        float lootStartX = m_winX + 24.0f;
        float lootY = startY + rowHeight * 2.0f + 16.0f;
        m_rcPickJewel = { lootStartX,                     lootY, lootBtnW, 32.0f };
        m_rcPickZen   = { lootStartX + (lootBtnW + 12),   lootY, lootBtnW, 32.0f };
        m_rcPickExc   = { lootStartX + (lootBtnW + 12)*2, lootY, lootBtnW, 32.0f };
        m_rcPickSet   = { lootStartX + (lootBtnW + 12)*3, lootY, lootBtnW, 32.0f };

        // Row 4: Buffs
        float buffBtnW = 140.0f;
        float buffY = lootY + 44.0f;
        m_rcBuffSelf  = { lootStartX,               buffY, buffBtnW, 32.0f };
        m_rcBuffParty = { lootStartX + buffBtnW + 16.0f, buffY, buffBtnW, 32.0f };

        // Big Start / Stop Button
        m_rcStartStopBtn = { m_winX + (m_winW - 220.0f) * 0.5f, m_winY + m_winH - 52.0f, 220.0f, 38.0f };
    }

    void CUIMobileHelper::SyncFromCore()
    {
        if (g_pNewUISystem && g_pNewUISystem->Get_pNewUIMuHelper())
        {
            auto& data = g_pNewUISystem->Get_pNewUIMuHelper()->DataAutoMu;
            m_bStarted = data.Started;
            m_huntingRange = data.Range[0] > 0 ? data.Range[0] : 6;
            m_potionPercent = data.PotPercent > 0 ? data.PotPercent : 70;
            m_bAutoPotion = (data.AutoPotion != 0);
            m_bPickJewel = (data.PickJewel != 0);
            m_bPickZen = (data.PickMoney != 0);
            m_bPickExc = (data.PickExc != 0);
            m_bPickSet = (data.PickSet != 0);
            m_bAutoBuffSelf = (data.AutoBuff != 0);
            m_bAutoBuffParty = (data.PartyAutoBuff != 0);
            m_bReturnOriginal = (data.OriginalPosition != 0);
        }
    }

    void CUIMobileHelper::SyncToCore()
    {
        if (g_pNewUISystem && g_pNewUISystem->Get_pNewUIMuHelper())
        {
            auto& data = g_pNewUISystem->Get_pNewUIMuHelper()->DataAutoMu;
            data.Started = m_bStarted;
            data.Range[0] = m_huntingRange;
            data.PotPercent = m_potionPercent;
            data.AutoPotion = m_bAutoPotion ? 1 : 0;
            data.PickJewel = m_bPickJewel ? 1 : 0;
            data.PickMoney = m_bPickZen ? 1 : 0;
            data.PickExc = m_bPickExc ? 1 : 0;
            data.PickSet = m_bPickSet ? 1 : 0;
            data.AutoBuff = m_bAutoBuffSelf ? 1 : 0;
            data.PartyAutoBuff = m_bAutoBuffParty ? 1 : 0;
            data.OriginalPosition = m_bReturnOriginal ? 1 : 0;

            if (m_bStarted && Hero)
            {
                data.StartX = Hero->PositionX;
                data.StartY = Hero->PositionY;
            }
        }
    }

    bool CUIMobileHelper::IsHelperActive() const
    {
        if (g_pNewUISystem && g_pNewUISystem->Get_pNewUIMuHelper())
        {
            return g_pNewUISystem->Get_pNewUIMuHelper()->DataAutoMu.Started;
        }
        return m_bStarted;
    }

    void CUIMobileHelper::ToggleHelperState()
    {
        SyncFromCore();
        m_bStarted = !m_bStarted;
        SyncToCore();
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileHelper::Open()
    {
        m_bIsOpen = true;
        SyncFromCore();
        ComputeLayout();
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileHelper::Close()
    {
        SyncToCore();
        m_bIsOpen = false;
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileHelper::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    bool CUIMobileHelper::Update()
    {
        return true;
    }

    bool CUIMobileHelper::UpdateKeyEvent()
    {
        if (m_bIsOpen && SEASON3B::IsPress(VK_ESCAPE))
        {
            Close();
            return false;
        }
        return true;
    }

    bool CUIMobileHelper::OnFingerDown(const SDL_TouchFingerEvent& ev)
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

        // Range controls
        if (m_rcRangeMinus.Contains(touchX, touchY))
        {
            if (m_huntingRange > 1) m_huntingRange--;
            SyncToCore();
            PlayBuffer(SOUND_CLICK01);
            return true;
        }
        if (m_rcRangePlus.Contains(touchX, touchY))
        {
            if (m_huntingRange < 8) m_huntingRange++;
            SyncToCore();
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        // Potion thresholds
        if (m_rcPot30.Contains(touchX, touchY)) { m_potionPercent = 30; SyncToCore(); PlayBuffer(SOUND_CLICK01); return true; }
        if (m_rcPot50.Contains(touchX, touchY)) { m_potionPercent = 50; SyncToCore(); PlayBuffer(SOUND_CLICK01); return true; }
        if (m_rcPot70.Contains(touchX, touchY)) { m_potionPercent = 70; SyncToCore(); PlayBuffer(SOUND_CLICK01); return true; }
        if (m_rcPot85.Contains(touchX, touchY)) { m_potionPercent = 85; SyncToCore(); PlayBuffer(SOUND_CLICK01); return true; }

        // Loot toggles
        if (m_rcPickJewel.Contains(touchX, touchY)) { m_bPickJewel = !m_bPickJewel; SyncToCore(); PlayBuffer(SOUND_CLICK01); return true; }
        if (m_rcPickZen.Contains(touchX, touchY))   { m_bPickZen   = !m_bPickZen;   SyncToCore(); PlayBuffer(SOUND_CLICK01); return true; }
        if (m_rcPickExc.Contains(touchX, touchY))   { m_bPickExc   = !m_bPickExc;   SyncToCore(); PlayBuffer(SOUND_CLICK01); return true; }
        if (m_rcPickSet.Contains(touchX, touchY))   { m_bPickSet   = !m_bPickSet;   SyncToCore(); PlayBuffer(SOUND_CLICK01); return true; }

        // Buff toggles
        if (m_rcBuffSelf.Contains(touchX, touchY))  { m_bAutoBuffSelf  = !m_bAutoBuffSelf;  SyncToCore(); PlayBuffer(SOUND_CLICK01); return true; }
        if (m_rcBuffParty.Contains(touchX, touchY)) { m_bAutoBuffParty = !m_bAutoBuffParty; SyncToCore(); PlayBuffer(SOUND_CLICK01); return true; }

        // Start / Stop Button
        if (m_rcStartStopBtn.Contains(touchX, touchY))
        {
            ToggleHelperState();
            return true;
        }

        // Absorb touch within window bounds
        if (touchX >= m_winX && touchX <= m_winX + m_winW &&
            touchY >= m_winY && touchY <= m_winY + m_winH)
        {
            return true;
        }

        return false;
    }

    bool CUIMobileHelper::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        float touchX = ev.x * (float)UIMobile::Screen::GetWidth();
        float touchY = ev.y * (float)UIMobile::Screen::GetHeight();
        return (touchX >= m_winX && touchX <= m_winX + m_winW && touchY >= m_winY && touchY <= m_winY + m_winH);
    }

    bool CUIMobileHelper::OnFingerUp(const SDL_TouchFingerEvent& ev)
    {
        if (!m_bIsOpen) return false;
        float touchX = ev.x * (float)UIMobile::Screen::GetWidth();
        float touchY = ev.y * (float)UIMobile::Screen::GetHeight();
        return (touchX >= m_winX && touchX <= m_winX + m_winW && touchY >= m_winY && touchY <= m_winY + m_winH);
    }

    bool CUIMobileHelper::Render()
    {
        if (!m_bIsOpen) return true;

        // Window Background Glass Panel
        UIMobile::Render::DrawGlassCard(m_winX, m_winY, m_winW, m_winH, 0.94f);
        UIMobile::Render::DrawPanelBorder(m_winX, m_winY, m_winW, m_winH,
            m_bStarted ? RGBA(100, 255, 120, 240) : RGBA(90, 160, 240, 220));

        // Header Title
        if (g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(RGBA(255, 220, 80, 255));
            g_pRenderText->SetBgColor(0);
            g_pRenderText->RenderText((int)(m_winX + 24.0f), (int)(m_winY + 16.0f), "MU HELPER - TỰ ĐỘNG CHIẾN ĐẤU");

            // Close Button
            g_pRenderText->SetTextColor(RGBA(255, 100, 100, 255));
            g_pRenderText->RenderText((int)(m_closeBtnX + 10.0f), (int)(m_closeBtnY + 8.0f), "X");
        }

        // Row 1: Range
        float contentX = m_winX + 24.0f;
        if (g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetTextColor(RGBA(220, 220, 240, 255));
            g_pRenderText->RenderText((int)contentX, (int)(m_rcRangeMinus.y + 6.0f), "Phạm vi đánh (Range 1-8):");
        }
        UIMobile::Render::DrawGlassCard(m_rcRangeMinus.x, m_rcRangeMinus.y, m_rcRangeMinus.w, m_rcRangeMinus.h, 0.7f);
        UIMobile::Render::DrawGlassCard(m_rcRangePlus.x,  m_rcRangePlus.y,  m_rcRangePlus.w,  m_rcRangePlus.h,  0.7f);
        if (g_pRenderText)
        {
            g_pRenderText->RenderText((int)(m_rcRangeMinus.x + 12.0f), (int)(m_rcRangeMinus.y + 6.0f), "-");
            g_pRenderText->RenderText((int)(m_rcRangePlus.x + 11.0f),  (int)(m_rcRangePlus.y + 6.0f),  "+");

            char buf[16];
            sprintf_s(buf, "%d", m_huntingRange);
            g_pRenderText->SetTextColor(RGBA(120, 220, 255, 255));
            g_pRenderText->RenderText((int)(m_rcRangeMinus.x + 42.0f), (int)(m_rcRangeMinus.y + 6.0f), buf);
        }

        // Row 2: Auto HP
        if (g_pRenderText)
        {
            g_pRenderText->SetTextColor(RGBA(220, 220, 240, 255));
            g_pRenderText->RenderText((int)contentX, (int)(m_rcPot30.y + 6.0f), "Tự bơm máu (HP %):");
        }
        UIMobile::Render::DrawGlassCard(m_rcPot30.x, m_rcPot30.y, m_rcPot30.w, m_rcPot30.h, m_potionPercent == 30 ? 0.9f : 0.4f);
        UIMobile::Render::DrawGlassCard(m_rcPot50.x, m_rcPot50.y, m_rcPot50.w, m_rcPot50.h, m_potionPercent == 50 ? 0.9f : 0.4f);
        UIMobile::Render::DrawGlassCard(m_rcPot70.x, m_rcPot70.y, m_rcPot70.w, m_rcPot70.h, m_potionPercent == 70 ? 0.9f : 0.4f);
        UIMobile::Render::DrawGlassCard(m_rcPot85.x, m_rcPot85.y, m_rcPot85.w, m_rcPot85.h, m_potionPercent == 85 ? 0.9f : 0.4f);
        if (g_pRenderText)
        {
            g_pRenderText->SetTextColor(m_potionPercent == 30 ? RGBA(255, 120, 120, 255) : RGBA(180, 180, 190, 255));
            g_pRenderText->RenderText((int)(m_rcPot30.x + 10.0f), (int)(m_rcPot30.y + 6.0f), "30%");

            g_pRenderText->SetTextColor(m_potionPercent == 50 ? RGBA(255, 200, 100, 255) : RGBA(180, 180, 190, 255));
            g_pRenderText->RenderText((int)(m_rcPot50.x + 10.0f), (int)(m_rcPot50.y + 6.0f), "50%");

            g_pRenderText->SetTextColor(m_potionPercent == 70 ? RGBA(100, 255, 150, 255) : RGBA(180, 180, 190, 255));
            g_pRenderText->RenderText((int)(m_rcPot70.x + 10.0f), (int)(m_rcPot70.y + 6.0f), "70%");

            g_pRenderText->SetTextColor(m_potionPercent == 85 ? RGBA(100, 255, 150, 255) : RGBA(180, 180, 190, 255));
            g_pRenderText->RenderText((int)(m_rcPot85.x + 10.0f), (int)(m_rcPot85.y + 6.0f), "85%");
        }

        // Row 3: Auto Loot Items
        if (g_pRenderText)
        {
            g_pRenderText->SetTextColor(RGBA(255, 220, 120, 255));
            g_pRenderText->RenderText((int)contentX, (int)(m_rcPickJewel.y - 18.0f), "Tự động nhặt đồ (Auto Pick):");
        }
        UIMobile::Render::DrawGlassCard(m_rcPickJewel.x, m_rcPickJewel.y, m_rcPickJewel.w, m_rcPickJewel.h, m_bPickJewel ? 0.85f : 0.35f);
        UIMobile::Render::DrawGlassCard(m_rcPickZen.x,   m_rcPickZen.y,   m_rcPickZen.w,   m_rcPickZen.h,   m_bPickZen   ? 0.85f : 0.35f);
        UIMobile::Render::DrawGlassCard(m_rcPickExc.x,   m_rcPickExc.y,   m_rcPickExc.w,   m_rcPickExc.h,   m_bPickExc   ? 0.85f : 0.35f);
        UIMobile::Render::DrawGlassCard(m_rcPickSet.x,   m_rcPickSet.y,   m_rcPickSet.w,   m_rcPickSet.h,   m_bPickSet   ? 0.85f : 0.35f);
        if (g_pRenderText)
        {
            g_pRenderText->SetTextColor(m_bPickJewel ? RGBA(255, 220, 80, 255) : RGBA(160, 160, 170, 255));
            g_pRenderText->RenderText((int)(m_rcPickJewel.x + 14.0f), (int)(m_rcPickJewel.y + 8.0f), "Ngọc (Jewel)");

            g_pRenderText->SetTextColor(m_bPickZen ? RGBA(255, 220, 80, 255) : RGBA(160, 160, 170, 255));
            g_pRenderText->RenderText((int)(m_rcPickZen.x + 20.0f), (int)(m_rcPickZen.y + 8.0f), "Tiền Zen");

            g_pRenderText->SetTextColor(m_bPickExc ? RGBA(100, 255, 150, 255) : RGBA(160, 160, 170, 255));
            g_pRenderText->RenderText((int)(m_rcPickExc.x + 12.0f), (int)(m_rcPickExc.y + 8.0f), "Đồ H.Hảo");

            g_pRenderText->SetTextColor(m_bPickSet ? RGBA(100, 220, 255, 255) : RGBA(160, 160, 170, 255));
            g_pRenderText->RenderText((int)(m_rcPickSet.x + 14.0f), (int)(m_rcPickSet.y + 8.0f), "Đồ Thần Set");
        }

        // Row 4: Buffs
        UIMobile::Render::DrawGlassCard(m_rcBuffSelf.x,  m_rcBuffSelf.y,  m_rcBuffSelf.w,  m_rcBuffSelf.h,  m_bAutoBuffSelf  ? 0.85f : 0.35f);
        UIMobile::Render::DrawGlassCard(m_rcBuffParty.x, m_rcBuffParty.y, m_rcBuffParty.w, m_rcBuffParty.h, m_bAutoBuffParty ? 0.85f : 0.35f);
        if (g_pRenderText)
        {
            g_pRenderText->SetTextColor(m_bAutoBuffSelf ? RGBA(120, 255, 200, 255) : RGBA(160, 160, 170, 255));
            g_pRenderText->RenderText((int)(m_rcBuffSelf.x + 16.0f), (int)(m_rcBuffSelf.y + 8.0f), "Tự Buff Bản Thân");

            g_pRenderText->SetTextColor(m_bAutoBuffParty ? RGBA(120, 255, 200, 255) : RGBA(160, 160, 170, 255));
            g_pRenderText->RenderText((int)(m_rcBuffParty.x + 18.0f), (int)(m_rcBuffParty.y + 8.0f), "Tự Buff Tổ Đội");
        }

        // Big Start / Stop Button
        UIMobile::Render::DrawGlassCard(m_rcStartStopBtn.x, m_rcStartStopBtn.y, m_rcStartStopBtn.w, m_rcStartStopBtn.h, 0.92f);
        UIMobile::Render::DrawPanelBorder(m_rcStartStopBtn.x, m_rcStartStopBtn.y, m_rcStartStopBtn.w, m_rcStartStopBtn.h,
            m_bStarted ? RGBA(255, 80, 80, 255) : RGBA(80, 255, 120, 255));
        if (g_pRenderText)
        {
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(m_bStarted ? RGBA(255, 100, 100, 255) : RGBA(100, 255, 140, 255));
            g_pRenderText->RenderText((int)(m_rcStartStopBtn.x + (m_bStarted ? 46.0f : 34.0f)),
                                      (int)(m_rcStartStopBtn.y + 10.0f),
                                      m_bStarted ? "DỪNG AUTO (STOP)" : "BẮT ĐẦU AUTO (START)");
        }

        return true;
    }
}

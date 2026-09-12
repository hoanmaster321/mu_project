// =============================================================================
// UIMobileMoveCommand.cpp
// Implementation of Mobile Map Warp / Teleport Window.
// =============================================================================

#include "stdafx.h"
#include "UIMobileMoveCommand.h"
#include "NewUISystem.h"
#include "NewUIMoveCommandWindow.h"
#include "ZzzInfomation.h"
#include "ZzzInventory.h"
#include "ZzzInterface.h"
#include "DSPlaySound.h"
#include "wsclientinline.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

namespace SEASON3B
{
    CUIMobileMoveCommand::CUIMobileMoveCommand()
        : m_pNewUIMng(nullptr)
        , m_bIsOpen(false)
        , m_winX(0.0f), m_winY(0.0f), m_winW(0.0f), m_winH(0.0f)
        , m_closeBtnX(0.0f), m_closeBtnY(0.0f), m_closeBtnSize(30.0f)
        , m_listX(0.0f), m_listY(0.0f), m_listW(0.0f), m_listH(0.0f)
        , m_scrollY(0.0f), m_maxScrollY(0.0f)
        , m_isScrolling(false), m_scrollFingerId(0)
        , m_touchDownY(0.0f), m_lastTouchY(0.0f), m_touchDownTick(0)
        , m_pressedIndex(-1), m_closePressed(false)
    {
    }

    CUIMobileMoveCommand::~CUIMobileMoveCommand()
    {
        Release();
    }

    bool CUIMobileMoveCommand::Create(CNewUIManager* pNewUIMng)
    {
        m_pNewUIMng = pNewUIMng;
        if (m_pNewUIMng)
        {
            m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_MOVEMAP, this);
        }
        return true;
    }

    void CUIMobileMoveCommand::Release()
    {
        if (m_pNewUIMng)
        {
            m_pNewUIMng->RemoveUIObj(this);
            m_pNewUIMng = nullptr;
        }
        m_bIsOpen = false;
        m_maps.clear();
    }

    void CUIMobileMoveCommand::Open()
    {
        m_bIsOpen = true;
        m_scrollY = 0.0f;
        m_isScrolling = false;
        m_pressedIndex = -1;
        m_closePressed = false;
        RefreshMapList();
        ComputeLayout();
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileMoveCommand::Close()
    {
        m_bIsOpen = false;
        m_isScrolling = false;
        m_pressedIndex = -1;
        m_closePressed = false;
        PlayBuffer(SOUND_CLICK01);
    }

    void CUIMobileMoveCommand::Toggle()
    {
        if (m_bIsOpen) Close();
        else Open();
    }

    void CUIMobileMoveCommand::RefreshMapList()
    {
        m_maps.clear();
        if (!CMoveCommandData::GetInstance()) return;

        const auto& listData = CMoveCommandData::GetInstance()->GetMoveCommandDatalist();
        const int myLevel = CharacterAttribute ? CharacterAttribute->Level : 0;
        const DWORD myZen = CharacterMachine ? CharacterMachine->Gold : 0;

        for (const auto* pInfo : listData)
        {
            if (!pInfo) continue;
            MapEntry entry;
            entry.index = pInfo->_ReqInfo.index;
            std::snprintf(entry.name, sizeof(entry.name), "%s", pInfo->_ReqInfo.szMainMapName);
            entry.reqLevel = pInfo->_ReqInfo.iReqLevel;
            entry.reqZen = pInfo->_ReqInfo.iReqZen;

            const bool levelOk = (myLevel >= entry.reqLevel);
            const bool zenOk = (myZen >= static_cast<DWORD>(entry.reqZen));
            entry.canMove = (levelOk && zenOk);

            m_maps.push_back(entry);
        }

        // Compute max scroll
        constexpr float itemH = 46.0f;
        const float contentH = static_cast<float>(m_maps.size()) * itemH;
        m_maxScrollY = (std::max)(0.0f, contentH - m_listH);
    }

    void CUIMobileMoveCommand::ComputeLayout()
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        m_winW = 500.0f;
        m_winH = 420.0f;
        m_winX = (winW - m_winW) * 0.5f;
        m_winY = (winH - m_winH) * 0.5f;

        m_closeBtnSize = 30.0f;
        m_closeBtnX = m_winX + m_winW - m_closeBtnSize - 8.0f;
        m_closeBtnY = m_winY + 8.0f;

        m_listX = m_winX + 16.0f;
        m_listY = m_winY + 46.0f;
        m_listW = m_winW - 32.0f;
        m_listH = m_winH - 58.0f;

        constexpr float itemH = 46.0f;
        const float contentH = static_cast<float>(m_maps.size()) * itemH;
        m_maxScrollY = (std::max)(0.0f, contentH - m_listH);
    }

    void CUIMobileMoveCommand::ExecuteWarp(int mapIndex)
    {
        DWORD key = 0;
        if (g_pMoveCommandWindow)
        {
            key = g_pMoveCommandWindow->GetMoveCommandKey();
        }
        SendRequestMoveMap(key, static_cast<WORD>(mapIndex));
        PlayBuffer(SOUND_CLICK01);
        Close();
    }

    bool CUIMobileMoveCommand::OnFingerDown(const SDL_TouchFingerEvent& ev)
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

        // Inside list
        if (UIMobile::HitTestRect(tx, ty, m_listX, m_listY, m_listW, m_listH))
        {
            m_isScrolling = true;
            m_scrollFingerId = ev.fingerID;
            m_touchDownY = ty;
            m_lastTouchY = ty;
            m_touchDownTick = SDL_GetTicks();

            // Check which item touched
            constexpr float itemH = 46.0f;
            const float relY = (ty - m_listY) + m_scrollY;
            const int idx = static_cast<int>(relY / itemH);
            if (idx >= 0 && idx < static_cast<int>(m_maps.size()))
            {
                m_pressedIndex = idx;
            }
            return true;
        }

        return true;
    }

    bool CUIMobileMoveCommand::OnFingerMotion(const SDL_TouchFingerEvent& ev)
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

            // If dragged significantly, cancel button tap
            if (std::abs(ty - m_touchDownY) > 8.0f)
            {
                m_pressedIndex = -1;
            }
            return true;
        }

        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileMoveCommand::OnFingerUp(const SDL_TouchFingerEvent& ev)
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

            // If tap without drag (< 8px movement and < 400ms duration)
            const uint32_t dur = SDL_GetTicks() - m_touchDownTick;
            if (std::abs(ty - m_touchDownY) <= 8.0f && dur < 400 && m_pressedIndex >= 0)
            {
                const int idx = m_pressedIndex;
                m_pressedIndex = -1;
                if (idx >= 0 && idx < static_cast<int>(m_maps.size()))
                {
                    if (m_maps[idx].canMove)
                    {
                        ExecuteWarp(m_maps[idx].index);
                        return true;
                    }
                }
            }
            m_pressedIndex = -1;
            return true;
        }

        return UIMobile::HitTestRect(tx, ty, m_winX, m_winY, m_winW, m_winH);
    }

    bool CUIMobileMoveCommand::Update()
    {
        if (!m_bIsOpen) return true;
        ComputeLayout();
        return true;
    }

    bool CUIMobileMoveCommand::UpdateKeyEvent()
    {
        if (!m_bIsOpen) return true;
        if (IsPress(VK_ESCAPE) || IsPress('M'))
        {
            Close();
            return false;
        }
        return true;
    }

    bool CUIMobileMoveCommand::Render()
    {
        if (!m_bIsOpen) return true;

        EnableAlphaTest();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // Header Title
        const DWORD myZen = CharacterMachine ? CharacterMachine->Gold : 0;
        char titleBuf[128];
        std::snprintf(titleBuf, sizeof(titleBuf), "[DICH CHUYEN BAN DO]  -  ZEN: %u", myZen);
        UIMobile::DrawPanel(m_winX, m_winY, m_winW, m_winH, titleBuf, true);

        // Close Button
        UIMobile::DrawCloseButton(m_closeBtnX, m_closeBtnY, m_closeBtnSize, m_closePressed);

        // List View Background & Border
        UIMobile::DrawSolidRect(m_listX, m_listY, m_listW, m_listH, UIMobile::Colors::SlotBg);
        UIMobile::DrawBorder(m_listX, m_listY, m_listW, m_listH, 1.5f, UIMobile::Colors::SlotBorder);

        // Render Map Rows
        constexpr float itemH = 46.0f;
        const float visibleStartY = m_listY;
        const float visibleEndY = m_listY + m_listH;

        for (size_t i = 0; i < m_maps.size(); ++i)
        {
            const float rowY = m_listY + (static_cast<float>(i) * itemH) - m_scrollY;
            if (rowY + itemH < visibleStartY || rowY > visibleEndY)
            {
                continue; // clipped
            }

            const auto& m = m_maps[i];
            const bool isPressed = (m_pressedIndex == static_cast<int>(i));

            // Row background
            UIMobile::ColorRGBA rowBg = isPressed
                ? UIMobile::ColorRGBA(0.18f, 0.32f, 0.50f, 0.90f)
                : ((i % 2 == 0) ? UIMobile::ColorRGBA(0.08f, 0.12f, 0.18f, 0.70f) : UIMobile::ColorRGBA(0.10f, 0.15f, 0.22f, 0.70f));

            UIMobile::DrawSolidRect(m_listX + 2.0f, rowY + 2.0f, m_listW - 4.0f, itemH - 4.0f, rowBg);

            if (g_pRenderText)
            {
                // Map Name
                g_pRenderText->SetFont(g_hFontBold);
                if (m.canMove)
                    g_pRenderText->SetTextColor(255, 230, 100, 255);
                else
                    g_pRenderText->SetTextColor(150, 150, 150, 255);

                g_pRenderText->SetBgColor(0, 0, 0, 0);
                g_pRenderText->RenderText(static_cast<int>(m_listX + 16.0f), static_cast<int>(rowY + 8.0f), m.name);

                // Level Req & Zen Cost
                g_pRenderText->SetFont(g_hFont);
                char infoBuf[64];
                std::snprintf(infoBuf, sizeof(infoBuf), "Y/C: Lv.%d   |   %d Zen", m.reqLevel, m.reqZen);

                if (m.canMove)
                    g_pRenderText->SetTextColor(120, 220, 150, 255);
                else
                    g_pRenderText->SetTextColor(220, 100, 100, 255);

                g_pRenderText->RenderText(static_cast<int>(m_listX + 16.0f), static_cast<int>(rowY + 26.0f), infoBuf);
            }

            // WARP Button on the right
            const float warpW = 75.0f;
            const float warpH = 30.0f;
            const float warpX = m_listX + m_listW - warpW - 12.0f;
            const float warpY = rowY + (itemH - warpH) * 0.5f;

            UIMobile::DrawButton(
                warpX, warpY, warpW, warpH,
                "WARP",
                isPressed,
                m.canMove,
                m.canMove ? UIMobile::Colors::BtnPrimary : UIMobile::Colors::BtnDisabled
            );
        }

        // Scrollbar Indicator on the right edge if content exceeds height
        if (m_maxScrollY > 0.0f)
        {
            const float barH = (std::max)(30.0f, m_listH * (m_listH / (m_listH + m_maxScrollY)));
            const float barRatio = m_scrollY / m_maxScrollY;
            const float barY = m_listY + (barRatio * (m_listH - barH));
            const float barX = m_listX + m_listW - 5.0f;

            UIMobile::DrawSolidRect(barX, barY, 4.0f, barH, UIMobile::ColorRGBA(0.4f, 0.7f, 1.0f, 0.75f));
        }

        DisableAlphaBlend();
        return true;
    }
}

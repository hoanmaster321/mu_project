// NewUIMainFrameMobile.h: Dedicated Mobile Main Frame UI
#pragma once

#include "NewUIBase.h"
#include "NewUIManager.h"
#include "NewUI3DRenderMng.h"

#if defined(__ANDROID__) || defined(MU_IOS)
#include <SDL3/SDL.h>
#else
typedef int64_t SDL_FingerID;
struct SDL_TouchFingerEvent {
    SDL_FingerID fingerID;
    float x;
    float y;
};
#endif

namespace SEASON3B
{
    class CNewUIMainFrameMobile : public CNewUIObj
    {
    public:
        CNewUIMainFrameMobile();
        virtual ~CNewUIMainFrameMobile();

        static CNewUIMainFrameMobile* GetInstance();

        bool Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng);
        void Release();

        bool Render() override;
        bool Update() override;
        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;

        float GetLayerDepth() override { return 10.8f; }
        float GetKeyEventOrder() override { return 3.0f; }

        // Touch Input Dispatch
        bool OnFingerDown(const SDL_TouchFingerEvent& ev);
        bool OnFingerUp(const SDL_TouchFingerEvent& ev);
        bool OnFingerMotion(const SDL_TouchFingerEvent& ev);

        // Combat & Skills
        void ExecuteSkillAt(int slotIndex, int targetMonster, int aimTileX, int aimTileY, bool isManualAim = false);
        void ExecuteCombatSkill(int slotIndex);
        void TriggerAttack();
        void TriggerQuickPotion();
        void GetSkillForSlot(int k, int& outSkillType, int& outSlotIndex);

    private:
        void EnsureTextures();
        void ResetJoystick();
        void GetCombatLayout(float& atkX, float& atkY, float& atkR,
                             float skillX[4], float skillY[4], float& skillR,
                             float& potX, float& potY, float& potR);

        CNewUIManager*     m_pNewUIMng;
        CNewUI3DRenderMng* m_pNewUI3DRenderMng;

        // Joystick state
        bool         m_joystickActive;
        SDL_FingerID m_joystickFingerId;
        float        m_joystickCenterX;
        float        m_joystickCenterY;
        float        m_knobX;
        float        m_knobY;
        float        m_dirX;
        float        m_dirY;
        float        m_lastDirX;
        float        m_lastDirY;
        float        m_lastSentWDirX;
        float        m_lastSentWDirY;
        float        m_strength;
        uint32_t     m_lastMoveTick;
        uint32_t     m_joystickPauseUntil;
        bool         m_wasDrivingMove;

        // Combat touch state
        bool         m_attackPressed;
        SDL_FingerID m_attackFingerId;

        bool         m_skillPressed[4];
        SDL_FingerID m_skillFingerId[4];

        bool         m_potPressed;
        SDL_FingerID m_potFingerId;

        uint32_t     m_lastCombatTick;

        // MOBA Skill Aiming state
        bool         m_aimActive;
        int          m_aimSlot;           // 0..3 for skills, 4 for ATK
        SDL_FingerID m_aimFingerId;
        float        m_aimStartX;
        float        m_aimStartY;
        float        m_aimCurX;
        float        m_aimCurY;
        float        m_aimDragDist;
        bool         m_aimIsDragging;
        bool         m_aimCancel;
        float        m_aimDirX;
        float        m_aimDirY;
        int          m_aimTargetTileX;
        int          m_aimTargetTileY;
        int          m_aimTargetMonster;
        uint32_t     m_aimHoldTick;

        // Textures
        uint32_t     m_texCircle;
        uint32_t     m_texRing;
        uint32_t     m_texJoyBase;
        uint32_t     m_texJoyKnob;
        uint32_t     m_texArrow;
        uint32_t     m_texReticle;

        // Skill page / quick swap
        int          m_skillPage;
    };
}

#define g_pMainFrameMobile SEASON3B::CNewUIMainFrameMobile::GetInstance()

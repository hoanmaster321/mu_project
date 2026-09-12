// NewUIMainFrameMobile.cpp: Dedicated Mobile Main Frame UI
#include "stdafx.h"
#include "NewUIMainFrameMobile.h"
#include "ZzzOpenglUtil.h"
#include "ZzzTexture.h"
#include "ZzzCharacter.h"
#include "ZzzScene.h"
#include "ZzzInterface.h"
#include "ZzzAI.h"
#include "ZzzLodTerrain.h"
#include "NewUISystem.h"
#include "NewUIMainFrameWindow.h"
#include "SkillManager.h"
#include "CharacterManager.h"
#include "DSPlaySound.h"
#include "VulkanTextureManager.h"
#include "ProtocolSend.h"
#include "GIPetManager.h"
#include "wsclientinline.h"
#include "UIMobile/UIMobileInventory.h"
#include "UIMobile/UIMobileSystem.h"

#include <cmath>
#include <algorithm>
#include <vector>

extern int DisplayWin;
extern int DisplayHeight;
extern int MouseX;
extern int MouseY;
extern bool MouseLButton;
extern bool MouseLButtonPush;
extern bool MouseLButtonPop;
extern bool MouseRButton;
extern bool MouseRButtonPush;
extern bool MouseRButtonPop;
extern int SelectedCharacter;
extern int Attacking;
extern int ActionTarget;
extern int TargetX;
extern int TargetY;
extern float SelectXF;
extern float SelectYF;
extern vec3_t CollisionPosition;
extern MovementSkill g_MovementSkill;
extern void SendMove(CHARACTER* c, OBJECT* o);
extern void Attack(CHARACTER* c);
extern int ExecuteSkill(CHARACTER* c, int Skill, float Distance);
extern void Action(CHARACTER* c, OBJECT* o, bool bAttack = false);
extern int FindCharacterIndex(int Key);
extern bool CheckWall(int sx1, int sy1, int sx2, int sy2);
extern float RequestTerrainHeight(float x, float y);
extern float CreateAngle(float x1, float y1, float x2, float y2);
extern int HeroKey;
extern int getTargetCharacterKey(CHARACTER* c, int SelectedCharacter);
extern void SetPlayerMagic(CHARACTER* c);
extern void SetAttackSpeed();
extern void SendRequestUse(int Index, int SubIndex);
extern int GetHeroCharacterIndex();

extern float CameraMatrix[3][4];
extern float PerspectiveX;
extern float PerspectiveY;
extern int ScreenCenterX;
extern int ScreenCenterY;
extern float g_fScreenRate_x;
extern float g_fScreenRate_y;
extern int g_iNoMouseTime;

static SEASON3B::CNewUIMainFrameMobile* s_pInstance = nullptr;

namespace SEASON3B
{
    static constexpr float kJoystickRadius = 58.0f;
    static constexpr float kKnobRadius     = 26.0f;
    static constexpr float kDeadZone       = 8.0f;

    static bool IsTileWalkable(int x, int y)
    {
        if (x < 0 || y < 0 || x >= 256 || y >= 256) return false;
        int attr = TerrainWall[y * 256 + x];
        if ((attr & TW_ACTION) == TW_ACTION) attr -= TW_ACTION;
        if ((attr & TW_HEIGHT) == TW_HEIGHT) attr -= TW_HEIGHT;
        if ((attr & TW_CAMERA_UP) == TW_CAMERA_UP) attr -= TW_CAMERA_UP;
        return (attr < TW_NOMOVE);
    }

    static bool IsTargetRequiredSkill(int skillType)
    {
        return (skillType >= AT_SKILL_SWORD1 && skillType <= AT_SKILL_SWORD5)
            || (skillType == AT_SKILL_ONETOONE)
            || (skillType >= MASTER_SKILL_ADD_DEATH_STAB_IMPROVED && skillType <= MASTER_SKILL_ADD_DEATH_STAB_MASTERED)
            || (skillType == MASTER_SKILL_ADD_FALLING_SLASH_IMPROVED)
            || (skillType == MASTER_SKILL_ADD_CYCLONE_IMPROVED1 || skillType == MASTER_SKILL_ADD_CYCLONE_IMPROVED2)
            || (skillType == MASTER_SKILL_ADD_SLASH_IMPROVED)
            || (skillType == MASTER_SKILL_ADD_LUNGE_IMPROVED)
            || (skillType >= AT_SKILL_BLOW_UP && skillType <= AT_SKILL_BLOW_UP + 4);
    }

    static int FindSkillIndexByType(int skillType)
    {
        if (CharacterAttribute == nullptr || skillType <= 0 || skillType >= MAX_SKILLS)
        {
            return -1;
        }
        for (int i = 0; i < MAX_MAGIC; ++i)
        {
            if (CharacterAttribute->Skill[i] == skillType)
            {
                return i;
            }
        }
        return -1;
    }

    static int ResolveSkillSlotIndex(int skillSlotOrType)
    {
        if (skillSlotOrType >= AT_PET_COMMAND_DEFAULT && skillSlotOrType < AT_PET_COMMAND_END)
        {
            return skillSlotOrType;
        }
        if (CharacterAttribute == nullptr)
        {
            return -1;
        }
        if (skillSlotOrType >= 0 && skillSlotOrType < MAX_MAGIC && CharacterAttribute->Skill[skillSlotOrType] > 0)
        {
            return skillSlotOrType;
        }
        return FindSkillIndexByType(skillSlotOrType);
    }

    static int ResolveSkillType(int skillSlotOrType)
    {
        if (skillSlotOrType >= AT_PET_COMMAND_DEFAULT && skillSlotOrType < AT_PET_COMMAND_END)
        {
            return skillSlotOrType;
        }
        if (CharacterAttribute == nullptr)
        {
            return -1;
        }
        if (skillSlotOrType >= 0 && skillSlotOrType < MAX_MAGIC && CharacterAttribute->Skill[skillSlotOrType] > 0)
        {
            return CharacterAttribute->Skill[skillSlotOrType];
        }
        if (skillSlotOrType > 0 && skillSlotOrType < MAX_SKILLS)
        {
            if (FindSkillIndexByType(skillSlotOrType) >= 0)
            {
                return skillSlotOrType;
            }
        }
        return -1;
    }

    static bool IsGroundTargetSkill(ActionSkillType skillType)
    {
        switch (skillType)
        {
        case AT_SKILL_BLAST_FREEZE:
        case AT_SKILL_ICE_UP:
        case AT_SKILL_ICE_UP + 1:
        case AT_SKILL_ICE_UP + 2:
        case AT_SKILL_BLAST_POISON:
        case MASTER_SKILL_ADD_DECAY_IMPROVED:
        case AT_SKILL_FLAME:
        case MASTER_SKILL_ADD_FLAME_IMPROVED1:
        case MASTER_SKILL_ADD_FLAME_IMPROVED2:
            return true;
        default:
            return false;
        }
    }

    static bool IsUntargetedAoeSkill(ActionSkillType skillType)
    {
        switch (skillType)
        {
        case AT_SKILL_WHEEL:
        case MASTER_SKILL_ADD_TWISTING_SLASH_IMPROVED1:
        case MASTER_SKILL_ADD_TWISTING_SLASH_IMPROVED2:
        case MASTER_SKILL_ADD_TWISTING_SLASH_ENHANCED:
        case AT_SKILL_TORNADO_SWORDA_UP:
        case AT_SKILL_TORNADO_SWORDA_UP + 1:
        case AT_SKILL_TORNADO_SWORDA_UP + 2:
        case AT_SKILL_TORNADO_SWORDA_UP + 3:
        case AT_SKILL_TORNADO_SWORDA_UP + 4:
        case AT_SKILL_TORNADO_SWORDB_UP:
        case AT_SKILL_TORNADO_SWORDB_UP + 1:
        case AT_SKILL_TORNADO_SWORDB_UP + 2:
        case AT_SKILL_TORNADO_SWORDB_UP + 3:
        case AT_SKILL_TORNADO_SWORDB_UP + 4:
        case AT_SKILL_EVIL:
        case AT_SKILL_EVIL_SPIRIT_UP:
        case AT_SKILL_EVIL_SPIRIT_UP_M:
        case MASTER_SKILL_ADD_EVIL_SPIRIT_IMPROVED1:
        case MASTER_SKILL_ADD_EVIL_SPIRIT_IMPROVED2:
        case AT_SKILL_STORM:
        case AT_SKILL_GIGANTIC_STORM:
        case AT_SKILL_HELL:
        case AT_SKILL_HELL_FIRE_UP:
        case MASTER_SKILL_ADD_HELL_FIRE_IMPROVED:
        case AT_SKILL_INFERNO:
        case MASTER_SKILL_ADD_INFERNO_IMPROVED1:
        case MASTER_SKILL_ADD_INFERNO_IMPROVED2:
        case AT_SKILL_DARK_HORSE:
        case MASTER_SKILL_ADD_EARTHQUAKE_IMPROVED:
        case MASTER_SKILL_ADD_EARTHQUAKE_ENHANCED:
        case AT_SKILL_FLASH:
            return true;
        default:
            return false;
        }
    }

    static bool SafeProject(const vec3_t pos, float& outX, float& outY)
    {
        vec3_t trans;
        VectorTransform(const_cast<float*>(pos), CameraMatrix, trans);
        if (trans[2] <= 1.0f) return false;
        outX = (ScreenCenterX - (trans[0] / PerspectiveX / trans[2])) / g_fScreenRate_x;
        outY = (ScreenCenterY + (trans[1] / PerspectiveY / trans[2])) / g_fScreenRate_y;
        return true;
    }

    CNewUIMainFrameMobile::CNewUIMainFrameMobile()
        : m_pNewUIMng(nullptr)
        , m_pNewUI3DRenderMng(nullptr)
        , m_joystickActive(false)
        , m_joystickFingerId(-1)
        , m_joystickCenterX(115.0f)
        , m_joystickCenterY(340.0f)
        , m_knobX(115.0f)
        , m_knobY(340.0f)
        , m_dirX(0.0f)
        , m_dirY(0.0f)
        , m_lastDirX(0.0f)
        , m_lastDirY(0.0f)
        , m_lastSentWDirX(0.0f)
        , m_lastSentWDirY(0.0f)
        , m_strength(0.0f)
        , m_lastMoveTick(0)
        , m_joystickPauseUntil(0)
        , m_wasDrivingMove(false)
        , m_attackPressed(false)
        , m_attackFingerId(-1)
        , m_potPressed(false)
        , m_potFingerId(-1)
        , m_lastCombatTick(0)
        , m_aimActive(false)
        , m_aimSlot(-1)
        , m_aimFingerId(-1)
        , m_aimStartX(0.0f)
        , m_aimStartY(0.0f)
        , m_aimCurX(0.0f)
        , m_aimCurY(0.0f)
        , m_aimDragDist(0.0f)
        , m_aimIsDragging(false)
        , m_aimCancel(false)
        , m_aimDirX(0.0f)
        , m_aimDirY(0.0f)
        , m_aimTargetTileX(0)
        , m_aimTargetTileY(0)
        , m_aimTargetMonster(-1)
        , m_aimHoldTick(0)
        , m_texCircle(0)
        , m_texRing(0)
        , m_texJoyBase(0)
        , m_texJoyKnob(0)
        , m_texArrow(0)
        , m_texReticle(0)
        , m_skillPage(0)
    {
        for (int i = 0; i < 4; ++i)
        {
            m_skillPressed[i] = false;
            m_skillFingerId[i] = -1;
        }
        s_pInstance = this;
    }

    CNewUIMainFrameMobile::~CNewUIMainFrameMobile()
    {
        Release();
        if (s_pInstance == this) s_pInstance = nullptr;
    }

    CNewUIMainFrameMobile* CNewUIMainFrameMobile::GetInstance()
    {
        return s_pInstance;
    }

    bool CNewUIMainFrameMobile::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng)
    {
        m_pNewUIMng = pNewUIMng;
        m_pNewUI3DRenderMng = pNewUI3DRenderMng;

        if (m_pNewUIMng)
        {
            m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_MAINFRAME_MOBILE, this);
        }

        Show(true);
        Enable(true);
        EnsureTextures();
        return true;
    }

    void CNewUIMainFrameMobile::Release()
    {
        if (m_pNewUIMng)
        {
            m_pNewUIMng->RemoveUIObj(this);
            m_pNewUIMng = nullptr;
        }
        m_pNewUI3DRenderMng = nullptr;

        auto destroyTex = [](uint32_t& tex) {
            if (tex != 0)
            {
                VulkanTextureManager::Instance().DestroyTexture(tex);
                tex = 0;
            }
        };

        destroyTex(m_texCircle);
        destroyTex(m_texRing);
        destroyTex(m_texJoyBase);
        destroyTex(m_texJoyKnob);
        destroyTex(m_texArrow);
        destroyTex(m_texReticle);
    }

    static void TouchToVirtual(float normX, float normY, float& outX, float& outY)
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;
        outX = normX * winW;
        outY = normY * winH;
    }

    void CNewUIMainFrameMobile::EnsureTextures()
    {
        if (m_texCircle != 0 && m_texRing != 0 && m_texJoyBase != 0 &&
            m_texJoyKnob != 0 && m_texArrow != 0 && m_texReticle != 0)
        {
            return;
        }

        // 1. Soft Circle (64x64)
        if (m_texCircle == 0)
        {
            constexpr int size = 64;
            constexpr float radius = (size - 2) * 0.5f;
            constexpr float center = size * 0.5f;
            std::vector<uint8_t> pixels(size * size * 4, 0);
            for (int y = 0; y < size; ++y)
            {
                for (int x = 0; x < size; ++x)
                {
                    const float dist = std::hypot(x + 0.5f - center, y + 0.5f - center);
                    const float a = std::clamp(radius - dist + 1.0f, 0.0f, 1.0f);
                    const int idx = (y * size + x) * 4;
                    pixels[idx + 0] = 255;
                    pixels[idx + 1] = 255;
                    pixels[idx + 2] = 255;
                    pixels[idx + 3] = static_cast<uint8_t>(a * 255.0f);
                }
            }
            m_texCircle = VulkanTextureManager::Instance().CreateTexture(size, size, 4, pixels.data(), true, true);
        }

        // 2. Clean Ring (64x64)
        if (m_texRing == 0)
        {
            constexpr int size = 64;
            constexpr float radius = (size - 2) * 0.5f;
            constexpr float center = size * 0.5f;
            constexpr float innerR = radius - 4.5f;
            std::vector<uint8_t> pixels(size * size * 4, 0);
            for (int y = 0; y < size; ++y)
            {
                for (int x = 0; x < size; ++x)
                {
                    const float dist = std::hypot(x + 0.5f - center, y + 0.5f - center);
                    float a = 0.0f;
                    if (dist <= radius && dist >= innerR)
                    {
                        const float edgeOut = std::clamp(radius - dist + 1.0f, 0.0f, 1.0f);
                        const float edgeIn  = std::clamp(dist - innerR + 1.0f, 0.0f, 1.0f);
                        a = (std::min)(edgeOut, edgeIn);
                    }
                    const int idx = (y * size + x) * 4;
                    pixels[idx + 0] = 255;
                    pixels[idx + 1] = 255;
                    pixels[idx + 2] = 255;
                    pixels[idx + 3] = static_cast<uint8_t>(a * 255.0f);
                }
            }
            m_texRing = VulkanTextureManager::Instance().CreateTexture(size, size, 4, pixels.data(), true, true);
        }

        // 3. High-Detail Joystick Base Plate (128x128)
        if (m_texJoyBase == 0)
        {
            constexpr int size = 128;
            constexpr float center = size * 0.5f;
            constexpr float outerR = (size - 4) * 0.5f;
            constexpr float borderW = 5.0f;
            constexpr float innerR = outerR - borderW;

            std::vector<uint8_t> pixels(size * size * 4, 0);
            for (int y = 0; y < size; ++y)
            {
                for (int x = 0; x < size; ++x)
                {
                    const float dx = x + 0.5f - center;
                    const float dy = y + 0.5f - center;
                    const float dist = std::hypot(dx, dy);
                    const int idx = (y * size + x) * 4;

                    if (dist > outerR + 1.0f)
                    {
                        continue;
                    }

                    float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;

                    if (dist >= innerR)
                    {
                        const float edgeOut = std::clamp(outerR - dist + 1.0f, 0.0f, 1.0f);
                        const float edgeIn  = std::clamp(dist - innerR + 1.0f, 0.0f, 1.0f);
                        const float rimAlpha = (std::min)(edgeOut, edgeIn);

                        const float light = 0.65f + 0.35f * (-dx - dy) / (dist * 1.414f + 0.01f);
                        r = 0.45f * light;
                        g = 0.75f * light;
                        b = 1.00f * light;
                        a = rimAlpha * 0.92f;
                    }
                    else
                    {
                        const float normDist = dist / innerR;
                        const float guideRing = std::exp(-std::pow((normDist - 0.65f) * 12.0f, 2.0f));
                        const float dishAlpha = 0.55f - normDist * 0.20f + guideRing * 0.15f;

                        r = 0.06f + guideRing * 0.15f;
                        g = 0.10f + guideRing * 0.25f;
                        b = 0.22f + guideRing * 0.45f;
                        a = std::clamp(dishAlpha, 0.0f, 1.0f);
                    }

                    // Cardinal tick notches
                    const float absDx = std::abs(dx);
                    const float absDy = std::abs(dy);
                    if (dist >= (innerR - 10.0f) && dist <= (outerR + 0.5f))
                    {
                        if ((absDx < 2.2f && absDy > 35.0f) || (absDy < 2.2f && absDx > 35.0f))
                        {
                            r = 0.90f;
                            g = 0.95f;
                            b = 1.00f;
                            a = 0.95f;
                        }
                    }

                    pixels[idx + 0] = static_cast<uint8_t>(std::clamp(r * 255.0f, 0.0f, 255.0f));
                    pixels[idx + 1] = static_cast<uint8_t>(std::clamp(g * 255.0f, 0.0f, 255.0f));
                    pixels[idx + 2] = static_cast<uint8_t>(std::clamp(b * 255.0f, 0.0f, 255.0f));
                    pixels[idx + 3] = static_cast<uint8_t>(std::clamp(a * 255.0f, 0.0f, 255.0f));
                }
            }
            m_texJoyBase = VulkanTextureManager::Instance().CreateTexture(size, size, 4, pixels.data(), true, true);
        }

        // 4. High-Detail 3D Joystick Knob (128x128)
        if (m_texJoyKnob == 0)
        {
            constexpr int size = 128;
            constexpr float center = size * 0.5f;
            constexpr float outerR = (size - 6) * 0.5f;
            constexpr float rimW   = 6.0f;
            constexpr float coreR  = outerR - rimW;

            std::vector<uint8_t> pixels(size * size * 4, 0);
            for (int y = 0; y < size; ++y)
            {
                for (int x = 0; x < size; ++x)
                {
                    const float dx = x + 0.5f - center;
                    const float dy = y + 0.5f - center;
                    const float dist = std::hypot(dx, dy);
                    const int idx = (y * size + x) * 4;

                    if (dist > outerR + 1.0f)
                    {
                        continue;
                    }

                    float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;

                    if (dist >= coreR)
                    {
                        const float edgeOut = std::clamp(outerR - dist + 1.0f, 0.0f, 1.0f);
                        const float edgeIn  = std::clamp(dist - coreR + 1.0f, 0.0f, 1.0f);
                        const float rimAlpha = (std::min)(edgeOut, edgeIn);

                        const float light = 0.60f + 0.40f * (-dx - dy) / (dist * 1.414f + 0.01f);
                        r = 0.70f * light;
                        g = 0.85f * light;
                        b = 1.00f * light;
                        a = rimAlpha * 0.98f;
                    }
                    else
                    {
                        const float nx = dx / coreR;
                        const float ny = dy / coreR;
                        const float nz = std::sqrt((std::max)(0.0f, 1.0f - nx * nx - ny * ny));

                        const float lx = -0.32f, ly = -0.42f, lz = 0.85f;
                        const float diff = (std::max)(0.0f, nx * lx + ny * ly + nz * lz);

                        const float hx = lx, hy = ly, hz = lz + 1.0f;
                        const float hlen = std::hypot(hx, std::hypot(hy, hz));
                        const float spec = std::pow((std::max)(0.0f, (nx * hx + ny * hy + nz * hz) / hlen), 16.0f);

                        r = 0.15f + diff * 0.40f + spec * 0.75f;
                        g = 0.35f + diff * 0.55f + spec * 0.75f;
                        b = 0.80f + diff * 0.20f + spec * 0.80f;
                        a = 0.95f;

                        const float absDx = std::abs(dx);
                        const float absDy = std::abs(dy);
                        if (dist >= 14.0f && dist <= 34.0f)
                        {
                            if ((absDx < 2.0f && absDy > 14.0f) || (absDy < 2.0f && absDx > 14.0f))
                            {
                                r *= 0.45f;
                                g *= 0.45f;
                                b *= 0.55f;
                            }
                        }
                    }

                    pixels[idx + 0] = static_cast<uint8_t>(std::clamp(r * 255.0f, 0.0f, 255.0f));
                    pixels[idx + 1] = static_cast<uint8_t>(std::clamp(g * 255.0f, 0.0f, 255.0f));
                    pixels[idx + 2] = static_cast<uint8_t>(std::clamp(b * 255.0f, 0.0f, 255.0f));
                    pixels[idx + 3] = static_cast<uint8_t>(std::clamp(a * 255.0f, 0.0f, 255.0f));
                }
            }
            m_texJoyKnob = VulkanTextureManager::Instance().CreateTexture(size, size, 4, pixels.data(), true, true);
        }

        // 5. Directional Arrow Chevron (64x64)
        if (m_texArrow == 0)
        {
            constexpr int size = 64;
            constexpr float center = size * 0.5f;
            std::vector<uint8_t> pixels(size * size * 4, 0);

            for (int y = 0; y < size; ++y)
            {
                for (int x = 0; x < size; ++x)
                {
                    const float px = x - center;
                    const float py = y - center;
                    const int idx = (y * size + x) * 4;

                    const float absX = std::abs(px);
                    float a = 0.0f;
                    if (py >= -24.0f && py <= 20.0f)
                    {
                        const float arrowW = (py + 24.0f) * 0.55f;
                        if (absX <= arrowW && absX >= (arrowW - 7.0f))
                        {
                            a = 1.0f - (absX - (arrowW - 7.0f)) / 7.0f;
                        }
                    }

                    pixels[idx + 0] = 255;
                    pixels[idx + 1] = 255;
                    pixels[idx + 2] = 255;
                    pixels[idx + 3] = static_cast<uint8_t>(std::clamp(a * 255.0f, 0.0f, 255.0f));
                }
            }
            m_texArrow = VulkanTextureManager::Instance().CreateTexture(size, size, 4, pixels.data(), true, true);
        }

        // 6. Targeting Reticle (64x64)
        if (m_texReticle == 0)
        {
            constexpr int size = 64;
            constexpr float center = size * 0.5f;
            constexpr float radius = 28.0f;
            std::vector<uint8_t> pixels(size * size * 4, 0);

            for (int y = 0; y < size; ++y)
            {
                for (int x = 0; x < size; ++x)
                {
                    const float dx = x - center;
                    const float dy = y - center;
                    const float dist = std::hypot(dx, dy);
                    const int idx = (y * size + x) * 4;

                    float a = 0.0f;
                    if (dist >= (radius - 3.5f) && dist <= radius)
                    {
                        const float angle = std::fmod(std::abs(std::atan2(dy, dx)) * 180.0f / 3.14159f, 90.0f);
                        if (angle >= 25.0f && angle <= 65.0f)
                        {
                            a = 0.95f;
                        }
                    }
                    if (dist <= 3.5f)
                    {
                        a = 1.0f;
                    }

                    pixels[idx + 0] = 255;
                    pixels[idx + 1] = 255;
                    pixels[idx + 2] = 255;
                    pixels[idx + 3] = static_cast<uint8_t>(std::clamp(a * 255.0f, 0.0f, 255.0f));
                }
            }
            m_texReticle = VulkanTextureManager::Instance().CreateTexture(size, size, 4, pixels.data(), true, true);
        }
    }

    void CNewUIMainFrameMobile::ResetJoystick()
    {
        if (m_wasDrivingMove || (Hero != nullptr && Hero->Movement))
        {
            m_wasDrivingMove = false;
            if (Hero != nullptr)
            {
                Hero->Movement = false;
                Hero->Path.PathNum = 0;
                Hero->Path.CurrentPath = 0;
                SetPlayerStop(Hero);

                BYTE stopX = static_cast<BYTE>(Hero->PositionX);
                BYTE stopY = static_cast<BYTE>(Hero->PositionY);
                TargetX = stopX;
                TargetY = stopY;
#ifdef NEW_PROTOCOL_SYSTEM
                gProtocolSend.SendCharacterMoveNew(Hero->Key, Hero->Object.Angle[2], 1, &stopX, &stopY, stopX, stopY);
#else
                SendCharacterMove(Hero->Key, Hero->Object.Angle[2], 1, &stopX, &stopY, stopX, stopY);
#endif
            }
        }

        m_joystickActive = false;
        m_joystickFingerId = -1;
        m_dirX = 0.0f;
        m_dirY = 0.0f;
        m_lastDirX = 0.0f;
        m_lastDirY = 0.0f;
        m_lastSentWDirX = 0.0f;
        m_lastSentWDirY = 0.0f;
        m_strength = 0.0f;

        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;
        m_joystickCenterX = 115.0f;
        m_joystickCenterY = winH - 140.0f;
        m_knobX = m_joystickCenterX;
        m_knobY = m_joystickCenterY;
    }

    void CNewUIMainFrameMobile::GetCombatLayout(float& atkX, float& atkY, float& atkR,
                                                float skillX[4], float skillY[4], float& skillR,
                                                float& potX, float& potY, float& potR)
    {
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        atkX = winW - 72.0f;
        atkY = winH - 72.0f;
        atkR = 38.0f;

        skillR = 26.0f;
        // Skill 1 (bottom left of ATK)
        skillX[0] = winW - 152.0f;
        skillY[0] = winH - 60.0f;

        // Skill 2 (diagonal up-left of ATK)
        skillX[1] = winW - 146.0f;
        skillY[1] = winH - 128.0f;

        // Skill 3 (top-left of ATK)
        skillX[2] = winW - 104.0f;
        skillY[2] = winH - 170.0f;

        // Skill 4 (top of ATK)
        skillX[3] = winW - 40.0f;
        skillY[3] = winH - 172.0f;

        // Quick Potion (HP)
        potX = winW - 214.0f;
        potY = winH - 60.0f;
        potR = 22.0f;
    }

    void CNewUIMainFrameMobile::GetSkillForSlot(int k, int& outSkillType, int& outSlotIndex)
    {
        outSkillType = -1;
        outSlotIndex = -1;
        if (CharacterAttribute == nullptr) return;

        std::vector<int> validSlots;
        for (int i = 0; i < MAX_MAGIC; ++i)
        {
            const int s = CharacterAttribute->Skill[i];
            if (s > 0 && !(s >= AT_SKILL_STUN && s <= AT_SKILL_REMOVAL_BUFF))
            {
                validSlots.push_back(i);
            }
        }

        if (validSlots.empty()) return;

        const int hotKeyIndex = k + 1;
        if (g_pSkillList != nullptr)
        {
            const int hkSlot = g_pSkillList->GetHotKey(hotKeyIndex);
            const int actualSlot = ResolveSkillSlotIndex(hkSlot);
            const int sType = ResolveSkillType(hkSlot);
            if (actualSlot >= 0 && sType > 0)
            {
                outSlotIndex = actualSlot;
                outSkillType = sType;
                return;
            }
        }

        const int pageOffset = m_skillPage * 4;
        const int targetIdx = pageOffset + k;
        if (targetIdx < static_cast<int>(validSlots.size()))
        {
            outSlotIndex = validSlots[targetIdx];
            outSkillType = CharacterAttribute->Skill[outSlotIndex];
        }
        else if (k < static_cast<int>(validSlots.size()))
        {
            outSlotIndex = validSlots[k];
            outSkillType = CharacterAttribute->Skill[outSlotIndex];
        }
    }

    static int FindNearestMonsterInRange(float maxDistTiles)
    {
        if (Hero == nullptr) return -1;
        int nearest = -1;
        float minDist = maxDistTiles;

        for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
        {
            CHARACTER* c = &CharactersClient[i];
            OBJECT* o = &c->Object;
            if (KIND_MONSTER == o->Kind && o->Live && o->Visible && o->Alpha > 0.0f && o->CurrentAction != 6 && c->Dead == 0)
            {
                const float dx = static_cast<float>(c->PositionX - Hero->PositionX);
                const float dy = static_cast<float>(c->PositionY - Hero->PositionY);
                const float d = std::hypot(dx, dy);
                if (d <= minDist)
                {
                    minDist = d;
                    nearest = i;
                }
            }
        }
        return nearest;
    }

    static int FindMonsterNearTile(int tileX, int tileY, float maxDist)
    {
        if (Hero == nullptr) return -1;
        int bestMonster = -1;
        float bestDist = maxDist;

        for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
        {
            CHARACTER* c = &CharactersClient[i];
            OBJECT* o = &c->Object;
            if (KIND_MONSTER == o->Kind && o->Live && o->Visible && o->Alpha > 0.0f && o->CurrentAction != 6 && c->Dead == 0)
            {
                const float dx = static_cast<float>(c->PositionX - tileX);
                const float dy = static_cast<float>(c->PositionY - tileY);
                const float d = std::hypot(dx, dy);
                if (d < bestDist)
                {
                    bestDist = d;
                    bestMonster = i;
                }
            }
        }
        return bestMonster;
    }

    static int GetHeroCharacterIndex()
    {
        if (CharactersClient == nullptr || Hero == nullptr) return -1;
        if (Hero < &CharactersClient[0] || Hero >= (&CharactersClient[0] + MAX_CHARACTERS_CLIENT)) return -1;
        return static_cast<int>(Hero - &CharactersClient[0]);
    }

    static bool PerformNormalAttackOnTarget(int targetIdx)
    {
        if (Hero == nullptr || targetIdx < 0 || targetIdx >= MAX_CHARACTERS_CLIENT) return false;
        CHARACTER* targetChar = &CharactersClient[targetIdx];
        if (targetChar->Dead > 0 || !targetChar->Object.Live) return false;

        SelectedCharacter = targetIdx;
        ActionTarget = targetIdx;
        TargetX = static_cast<int>(targetChar->Object.Position[0] / TERRAIN_SCALE);
        TargetY = static_cast<int>(targetChar->Object.Position[1] / TERRAIN_SCALE);
        VectorCopy(targetChar->Object.Position, Hero->TargetPosition);
        Hero->Object.Angle[2] = CreateAngle(Hero->Object.Position[0], Hero->Object.Position[1],
                                            Hero->TargetPosition[0], Hero->TargetPosition[1]);

        Attacking = 1;
        Hero->MovementType = MOVEMENT_ATTACK;

        if (!CheckWall(Hero->PositionX, Hero->PositionY, TargetX, TargetY))
        {
            return false;
        }

        if (!PathFinding2(Hero->PositionX, Hero->PositionY, TargetX, TargetY, &Hero->Path))
        {
            Action(Hero, &Hero->Object, true);
            return true;
        }

        const bool ranged = (gCharacterManager.GetEquipedBowType() != BOWTYPE_NONE) || (Hero->MonsterIndex == 0);
        if (ranged)
        {
            Action(Hero, &Hero->Object, true);
        }
        else
        {
            SendMove(Hero, &Hero->Object);
        }
        return true;
    }

    void CNewUIMainFrameMobile::ExecuteSkillAt(int slotIndex, int targetMonster, int aimTileX, int aimTileY, bool isManualAim)
    {
        if (slotIndex < 0 || Hero == nullptr || CharacterAttribute == nullptr || Hero->Dead > 0) return;

        if (m_wasDrivingMove)
        {
            m_wasDrivingMove = false;
        }

        m_joystickPauseUntil = SDL_GetTicks() + 350;

        Hero->Movement = false;
        Hero->Path.PathNum = 0;
        Hero->Path.CurrentPath = 0;
        SetPlayerStop(Hero);

        // Reset animation if not in a valid stop action so ExecuteSkill doesn't reject it
        {
            OBJECT* ho = &Hero->Object;
            const bool inStopAction =
                (ho->CurrentAction >= PLAYER_STOP_MALE && ho->CurrentAction <= PLAYER_STOP_RIDE_WEAPON)
                || ho->CurrentAction == PLAYER_STOP_TWO_HAND_SWORD_TWO
                || ho->CurrentAction == PLAYER_SKILL_HELL_BEGIN
                || ho->CurrentAction == PLAYER_DARKLORD_STAND
                || ho->CurrentAction == PLAYER_STOP_RIDE_HORSE
                || ho->CurrentAction == PLAYER_FENRIR_STAND
                || ho->CurrentAction == PLAYER_FENRIR_STAND_TWO_SWORD
                || ho->CurrentAction == PLAYER_FENRIR_STAND_ONE_RIGHT
                || ho->CurrentAction == PLAYER_FENRIR_STAND_ONE_LEFT
                || (ho->CurrentAction >= PLAYER_RAGE_FENRIR_STAND && ho->CurrentAction <= PLAYER_RAGE_FENRIR_STAND_ONE_LEFT)
                || ho->CurrentAction == PLAYER_RAGE_UNI_STOP_ONE_RIGHT
                || ho->CurrentAction == PLAYER_STOP_RAGEFIGHTER;
            if (!inStopAction)
            {
                ho->CurrentAction = PLAYER_STOP_MALE;
                ho->AnimationFrame = 0.0f;
            }
        }

        if (slotIndex >= AT_PET_COMMAND_DEFAULT && slotIndex < AT_PET_COMMAND_END)
        {
            if (Hero->m_pPet == nullptr) return;
            Hero->CurrentSkill = static_cast<BYTE>(slotIndex);
            giPetManager::SendPetCommand(Hero, Hero->CurrentSkill);
            return;
        }

        const int actualSkillIndex = ResolveSkillSlotIndex(slotIndex);
        const int rawSkillType = ResolveSkillType(slotIndex);
        if (actualSkillIndex < 0 || rawSkillType <= 0 || rawSkillType >= MAX_SKILLS) return;

        Hero->CurrentSkill = static_cast<BYTE>(actualSkillIndex);
        const ActionSkillType skillType = static_cast<ActionSkillType>(rawSkillType);
        const float skillDistance = gSkillManager.GetSkillDistance(skillType, Hero);
        g_MovementSkill.m_bMagic = TRUE;
        g_MovementSkill.m_iSkill = actualSkillIndex;
        Attacking = 2;

        if (!gSkillManager.CheckSkillDelay(Hero->CurrentSkill))
        {
            return;
        }

        int iMana = 0, iSkillMana = 0;
        gSkillManager.GetSkillInformation(skillType, 1, NULL, &iMana, NULL, &iSkillMana);
        if (CharacterAttribute->Mana < iMana)
        {
            int itemIdx = g_pMyInventory ? g_pMyInventory->FindManaItemIndex() : -1;
            if (itemIdx != -1)
            {
                SendRequestUse(itemIdx, 0);
            }
            return;
        }
        if (CharacterAttribute->SkillMana < iSkillMana)
        {
            return;
        }

        // Support / Self / Buff skills
        const bool isSupport = (skillType == AT_SKILL_HEALING || skillType == AT_SKILL_DEFENSE
            || skillType == AT_SKILL_ATTACK || skillType == AT_SKILL_WIZARDDEFENSE
            || skillType == AT_SKILL_VITALITY || skillType == AT_SKILL_INFINITY_ARROW
            || skillType == AT_SKILL_SWELL_OF_MAGICPOWER || skillType == AT_SKILL_RECOVER
            || skillType == AT_SKILL_ALICE_BERSERKER
            || (skillType >= AT_SKILL_DEF_POWER_UP && skillType <= AT_SKILL_DEF_POWER_UP + 4)
            || (skillType >= AT_SKILL_ATT_POWER_UP && skillType <= AT_SKILL_ATT_POWER_UP + 4)
            || (skillType >= AT_SKILL_SOUL_UP && skillType <= AT_SKILL_SOUL_UP + 4)
            || (skillType >= AT_SKILL_HEAL_UP && skillType <= AT_SKILL_HEAL_UP + 4)
            || (skillType >= AT_SKILL_LIFE_UP && skillType <= AT_SKILL_LIFE_UP + 4)
            || skillType == AT_SKILL_IMPROVE_AG || skillType == AT_SKILL_ADD_CRITICAL
            || skillType == AT_SKILL_PARTY_TELEPORT);

        if (isSupport)
        {
            const int heroIdx = GetHeroCharacterIndex();
            SelectedCharacter = (heroIdx >= 0) ? heroIdx : 0;
            ActionTarget = SelectedCharacter;
            g_MovementSkill.m_iTarget = SelectedCharacter;
            TargetX = Hero->PositionX;
            TargetY = Hero->PositionY;
            VectorCopy(Hero->Object.Position, Hero->TargetPosition);

            SendRequestMagic(skillType, HeroKey);
            SetPlayerMagic(Hero);
            return;
        }

        // Target calculation
        if (targetMonster >= 0 && targetMonster < MAX_CHARACTERS_CLIENT && CharactersClient[targetMonster].Object.Live && CharactersClient[targetMonster].Dead == 0)
        {
            SelectedCharacter = targetMonster;
            ActionTarget = targetMonster;
            g_MovementSkill.m_iTarget = targetMonster;
            TargetX = static_cast<int>(CharactersClient[targetMonster].Object.Position[0] / TERRAIN_SCALE);
            TargetY = static_cast<int>(CharactersClient[targetMonster].Object.Position[1] / TERRAIN_SCALE);
            VectorCopy(CharactersClient[targetMonster].Object.Position, Hero->TargetPosition);
            Hero->Object.Angle[2] = CreateAngle(Hero->Object.Position[0], Hero->Object.Position[1],
                                                Hero->TargetPosition[0], Hero->TargetPosition[1]);
        }
        else if (isManualAim)
        {
            SelectedCharacter = -1;
            ActionTarget = -1;
            g_MovementSkill.m_iTarget = -1;
            TargetX = std::clamp(aimTileX, 0, 255);
            TargetY = std::clamp(aimTileY, 0, 255);
            Hero->TargetPosition[0] = static_cast<float>(TargetX * TERRAIN_SCALE + 50.0f);
            Hero->TargetPosition[1] = static_cast<float>(TargetY * TERRAIN_SCALE + 50.0f);
            Hero->TargetPosition[2] = RequestTerrainHeight(Hero->TargetPosition[0], Hero->TargetPosition[1]);
            Hero->Object.Angle[2] = CreateAngle(Hero->Object.Position[0], Hero->Object.Position[1],
                                                Hero->TargetPosition[0], Hero->TargetPosition[1]);
            SelectXF = static_cast<float>(TargetX);
            SelectYF = static_cast<float>(TargetY);
            VectorCopy(Hero->TargetPosition, CollisionPosition);
        }
        else
        {
            const float searchRange = (std::max)(skillDistance + 2.0f, 6.5f);
            const int nearest = FindNearestMonsterInRange(searchRange);
            if (nearest >= 0 && nearest < MAX_CHARACTERS_CLIENT)
            {
                SelectedCharacter = nearest;
                ActionTarget = nearest;
                g_MovementSkill.m_iTarget = nearest;
                TargetX = static_cast<int>(CharactersClient[nearest].Object.Position[0] / TERRAIN_SCALE);
                TargetY = static_cast<int>(CharactersClient[nearest].Object.Position[1] / TERRAIN_SCALE);
                VectorCopy(CharactersClient[nearest].Object.Position, Hero->TargetPosition);
                Hero->Object.Angle[2] = CreateAngle(Hero->Object.Position[0], Hero->Object.Position[1],
                                                    Hero->TargetPosition[0], Hero->TargetPosition[1]);
            }
            else
            {
                SelectedCharacter = -1;
                ActionTarget = -1;
                g_MovementSkill.m_iTarget = -1;
                const float rad = glm::radians(Hero->Object.Angle[2]);
                TargetX = std::clamp(Hero->PositionX + static_cast<int>(std::round(-sinf(rad) * 4.0f)), 0, 255);
                TargetY = std::clamp(Hero->PositionY + static_cast<int>(std::round(cosf(rad) * 4.0f)), 0, 255);
                Hero->TargetPosition[0] = static_cast<float>(TargetX * TERRAIN_SCALE + 50.0f);
                Hero->TargetPosition[1] = static_cast<float>(TargetY * TERRAIN_SCALE + 50.0f);
                Hero->TargetPosition[2] = RequestTerrainHeight(Hero->TargetPosition[0], Hero->TargetPosition[1]);
                SelectXF = static_cast<float>(TargetX);
                SelectYF = static_cast<float>(TargetY);
                VectorCopy(Hero->TargetPosition, CollisionPosition);
            }
        }

        // Ground / Position Target Skills (Ice Storm / Mưa Băng Tuyết, Flame, Decay / Poison, Ice Up)
        if (IsGroundTargetSkill(skillType))
        {
            WORD targetKey = 0xffff;
            if (SelectedCharacter >= 0 && SelectedCharacter < MAX_CHARACTERS_CLIENT
                && CharactersClient[SelectedCharacter].Object.Live
                && CharactersClient[SelectedCharacter].Dead == 0)
            {
                targetKey = static_cast<WORD>(getTargetCharacterKey(Hero, SelectedCharacter));
            }

            const BYTE tx = static_cast<BYTE>(std::clamp(TargetX, 0, 255));
            const BYTE ty = static_cast<BYTE>(std::clamp(TargetY, 0, 255));
            const BYTE angle = static_cast<BYTE>(Hero->Object.Angle[2] / 360.0f * 256.0f);

            SendRequestMagicContinue(skillType, tx, ty, angle, 0, 0, targetKey, 0);
            SetPlayerMagic(Hero);
            Hero->Movement = 0;
            return;
        }

        // Untargeted AOE Skills (Twisting Slash, Evil Spirit, Hellfire, Inferno, Dark Horse, etc.)
        if (IsUntargetedAoeSkill(skillType))
        {
            WORD targetKey = 0xffff;
            if (SelectedCharacter >= 0 && SelectedCharacter < MAX_CHARACTERS_CLIENT && CharactersClient[SelectedCharacter].Object.Live)
            {
                targetKey = static_cast<WORD>(getTargetCharacterKey(Hero, SelectedCharacter));
                g_MovementSkill.m_iTarget = SelectedCharacter;
            }
            else
            {
                g_MovementSkill.m_iTarget = -1;
            }

            if (skillType == AT_SKILL_WHEEL || (skillType >= AT_SKILL_TORNADO_SWORDA_UP && skillType <= AT_SKILL_TORNADO_SWORDB_UP + 4)
                || skillType == MASTER_SKILL_ADD_TWISTING_SLASH_IMPROVED1 || skillType == MASTER_SKILL_ADD_TWISTING_SLASH_IMPROVED2
                || skillType == MASTER_SKILL_ADD_TWISTING_SLASH_ENHANCED)
            {
                BYTE PathX[1] = { static_cast<BYTE>(Hero->PositionX) };
                BYTE PathY[1] = { static_cast<BYTE>(Hero->PositionY) };
#ifdef NEW_PROTOCOL_SYSTEM
                gProtocolSend.SendCharacterMoveNew(Hero->Key, Hero->Object.Angle[2], 1, &PathX[0], &PathY[0], TargetX, TargetY);
#else
                SendCharacterMove(Hero->Key, Hero->Object.Angle[2], 1, &PathX[0], &PathY[0], TargetX, TargetY);
#endif
                SendRequestMagicContinue(skillType, Hero->PositionX, Hero->PositionY,
                    static_cast<BYTE>(Hero->Object.Angle[2] / 360.0f * 256.0f), 0, 0, targetKey, 0);
                SetAttackSpeed();
                SetAction(&Hero->Object, PLAYER_ATTACK_SKILL_WHEEL);
                Hero->Movement = 0;
                return;
            }

            if (skillType == AT_SKILL_EVIL || skillType == AT_SKILL_EVIL_SPIRIT_UP || skillType == AT_SKILL_EVIL_SPIRIT_UP_M
                || skillType == MASTER_SKILL_ADD_EVIL_SPIRIT_IMPROVED1 || skillType == MASTER_SKILL_ADD_EVIL_SPIRIT_IMPROVED2
                || skillType == AT_SKILL_STORM || skillType == AT_SKILL_GIGANTIC_STORM)
            {
                SendRequestMagicContinue(skillType, Hero->PositionX, Hero->PositionY,
                    static_cast<BYTE>(Hero->Object.Angle[2] / 360.0f * 256.0f), 0, 0, 0xffff, &Hero->Object.m_bySkillSerialNum);
                SetPlayerMagic(Hero);
                Hero->Movement = 0;
                return;
            }

            if (skillType == AT_SKILL_HELL || skillType == AT_SKILL_HELL_FIRE_UP || skillType == MASTER_SKILL_ADD_HELL_FIRE_IMPROVED)
            {
                SendRequestMagicContinue(skillType, Hero->PositionX, Hero->PositionY,
                    static_cast<BYTE>(Hero->Object.Angle[2] / 360.0f * 256.0f), 0, 0, 0xffff, 0);
                SetAttackSpeed();
                SetAction(&Hero->Object, PLAYER_SKILL_HELL);
                Hero->Movement = 0;
                return;
            }

            if (skillType == AT_SKILL_INFERNO || skillType == MASTER_SKILL_ADD_INFERNO_IMPROVED1 || skillType == MASTER_SKILL_ADD_INFERNO_IMPROVED2)
            {
                SendRequestMagicContinue(skillType, Hero->PositionX, Hero->PositionY,
                    static_cast<BYTE>(Hero->Object.Angle[2] / 360.0f * 256.0f), 0, 0, 0xffff, 0);
                SetAttackSpeed();
                SetAction(&Hero->Object, PLAYER_SKILL_INFERNO);
                Hero->Movement = 0;
                return;
            }

            if (skillType == AT_SKILL_DARK_HORSE || skillType == MASTER_SKILL_ADD_EARTHQUAKE_IMPROVED || skillType == MASTER_SKILL_ADD_EARTHQUAKE_ENHANCED)
            {
                SendRequestMagicContinue(skillType, Hero->PositionX, Hero->PositionY,
                    static_cast<BYTE>(Hero->Object.Angle[2] / 360.0f * 256.0f), 0, 0, 0xffff, 0);
                SetAttackSpeed();
                SetAction(&Hero->Object, PLAYER_ATTACK_DARKHORSE);
                Hero->Movement = 0;
                return;
            }

            SendRequestMagicContinue(skillType, Hero->PositionX, Hero->PositionY,
                static_cast<BYTE>(Hero->Object.Angle[2] / 360.0f * 256.0f), 0, 0, targetKey, 0);
            SetPlayerMagic(Hero);
            Hero->Movement = 0;
            return;
        }

        // Targeted & Weapon Skills
        if (SelectedCharacter >= 0 && SelectedCharacter < MAX_CHARACTERS_CLIENT
            && CharactersClient[SelectedCharacter].Object.Live
            && CharactersClient[SelectedCharacter].Dead == 0)
        {
            CHARACTER* targetChar = &CharactersClient[SelectedCharacter];
            const float dx = (Hero->Object.Position[0] - targetChar->Object.Position[0]) / TERRAIN_SCALE;
            const float dy = (Hero->Object.Position[1] - targetChar->Object.Position[1]) / TERRAIN_SCALE;
            const float dist = std::sqrt(dx * dx + dy * dy);
            const float effDist = (std::max)(skillDistance, 2.0f);

            if (dist <= effDist)
            {
                int res = ExecuteSkill(Hero, skillType, skillDistance);
                if (res != 0) return;

                WORD tKey = static_cast<WORD>(getTargetCharacterKey(Hero, SelectedCharacter));
                SendRequestMagic(skillType, tKey);
                SetPlayerMagic(Hero);
                return;
            }
            else if (!isManualAim)
            {
                if (PathFinding2(Hero->PositionX, Hero->PositionY, TargetX, TargetY, &Hero->Path, effDist * 0.85f))
                {
                    Hero->Movement = true;
                    Hero->MovementType = MOVEMENT_SKILL;
                    SendMove(Hero, &Hero->Object);
                    return;
                }
            }
        }
        else
        {
            SendRequestMagicContinue(skillType, static_cast<BYTE>(TargetX), static_cast<BYTE>(TargetY),
                static_cast<BYTE>(Hero->Object.Angle[2] / 360.0f * 256.0f), 0, 0, 0xffff, 0);
            SetPlayerMagic(Hero);
            return;
        }
    }

    void CNewUIMainFrameMobile::ExecuteCombatSkill(int slotIndex)
    {
        if (slotIndex < 0 || Hero == nullptr || Hero->Dead > 0) return;

        const int rawSkillType = ResolveSkillType(slotIndex);
        if (rawSkillType <= 0) return;

        const float range = gSkillManager.GetSkillDistance(static_cast<ActionSkillType>(rawSkillType), Hero);
        const int monster = FindNearestMonsterInRange(range + 1.5f);
        int tx = Hero->PositionX;
        int ty = Hero->PositionY;
        if (monster < 0)
        {
            const float rad = glm::radians(Hero->Object.Angle[2]);
            tx = std::clamp(Hero->PositionX + static_cast<int>(std::round(-sinf(rad) * 4.0f)), 0, 255);
            ty = std::clamp(Hero->PositionY + static_cast<int>(std::round(cosf(rad) * 4.0f)), 0, 255);
        }
        ExecuteSkillAt(slotIndex, monster, tx, ty);
    }

    void CNewUIMainFrameMobile::TriggerAttack()
    {
        if (Hero == nullptr || Hero->Dead > 0) return;

        if (m_wasDrivingMove)
        {
            m_wasDrivingMove = false;
            MouseLButton = false;
            MouseLButtonPush = false;
            MouseLButtonPop = false;
        }

        m_joystickPauseUntil = SDL_GetTicks() + 250;

        const int target = FindNearestMonsterInRange(6.0f);
        if (target >= 0 && PerformNormalAttackOnTarget(target))
        {
            return;
        }

        Attacking = 1;
        Action(Hero, &Hero->Object, true);
    }

    void CNewUIMainFrameMobile::TriggerQuickPotion()
    {
        if (g_pMainFrame != nullptr)
        {
            g_pMainFrame->UseItemHotKey(HOTKEY_Q);
            PlayBuffer(SOUND_CLICK01);
        }
    }

    bool CNewUIMainFrameMobile::OnFingerDown(const SDL_TouchFingerEvent& ev)
    {
        if (SceneFlag != MAIN_SCENE || Hero == nullptr) return false;

        float touchX = 0.0f, touchY = 0.0f;
        TouchToVirtual(ev.x, ev.y, touchX, touchY);

        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        float atkX, atkY, atkR;
        float skillX[4], skillY[4], skillR;
        float potX, potY, potR;
        GetCombatLayout(atkX, atkY, atkR, skillX, skillY, skillR, potX, potY, potR);

        // 1. Attack Button (Slot 4)
        const float dAtkX = touchX - atkX;
        const float dAtkY = touchY - atkY;
        if ((dAtkX * dAtkX + dAtkY * dAtkY) <= (atkR + 10.0f) * (atkR + 10.0f))
        {
            m_attackPressed = true;
            m_attackFingerId = ev.fingerID;
            m_aimHoldTick = SDL_GetTicks();
            m_lastCombatTick = m_aimHoldTick;
            m_aimActive = true;
            m_aimSlot = 4;
            m_aimFingerId = ev.fingerID;
            m_aimStartX = atkX;
            m_aimStartY = atkY;
            m_aimCurX = touchX;
            m_aimCurY = touchY;
            m_aimDragDist = 0.0f;
            m_aimIsDragging = false;
            m_aimCancel = false;
            m_aimTargetMonster = FindNearestMonsterInRange(4.0f);
            m_aimTargetTileX = Hero->PositionX;
            m_aimTargetTileY = Hero->PositionY;

            // Instant trigger on tap!
            TriggerAttack();
            return true;
        }

        // 2. Skill Buttons 1..4 (Slots 0..3)
        for (int i = 0; i < 4; ++i)
        {
            const float dx = touchX - skillX[i];
            const float dy = touchY - skillY[i];
            if ((dx * dx + dy * dy) <= (skillR + 8.0f) * (skillR + 8.0f))
            {
                m_skillPressed[i] = true;
                m_skillFingerId[i] = ev.fingerID;
                m_aimHoldTick = SDL_GetTicks();
                m_lastCombatTick = m_aimHoldTick;
                m_aimActive = true;
                m_aimSlot = i;
                m_aimFingerId = ev.fingerID;
                m_aimStartX = skillX[i];
                m_aimStartY = skillY[i];
                m_aimCurX = touchX;
                m_aimCurY = touchY;
                m_aimDragDist = 0.0f;
                m_aimIsDragging = false;
                m_aimCancel = false;

                int sType = -1, sSlot = -1;
                GetSkillForSlot(i, sType, sSlot);
                float skillRange = 4.0f;
                if (sType > 0)
                {
                    skillRange = gSkillManager.GetSkillDistance(static_cast<ActionSkillType>(sType), Hero);
                    if (skillRange < 2.0f) skillRange = 3.0f;
                }

                m_aimTargetMonster = FindNearestMonsterInRange(skillRange + 1.0f);
                m_aimTargetTileX = (m_aimTargetMonster >= 0) ? CharactersClient[m_aimTargetMonster].PositionX : Hero->PositionX;
                m_aimTargetTileY = (m_aimTargetMonster >= 0) ? CharactersClient[m_aimTargetMonster].PositionY : Hero->PositionY;

                // Instant trigger on tap!
                if (sSlot >= 0)
                {
                    Hero->CurrentSkill = static_cast<BYTE>(sSlot);
                    ExecuteSkillAt(sSlot, m_aimTargetMonster, m_aimTargetTileX, m_aimTargetTileY);
                }
                return true;
            }
        }

        // 3. Quick Potion Button
        const float dPotX = touchX - potX;
        const float dPotY = touchY - potY;
        if ((dPotX * dPotX + dPotY * dPotY) <= (potR + 8.0f) * (potR + 8.0f))
        {
            m_potPressed = true;
            m_potFingerId = ev.fingerID;
            TriggerQuickPotion();
            return true;
        }

        // 4. Skill Swap Button
        const float swapX = winW - 178.0f;
        const float swapY = winH - 132.0f;
        constexpr float swapR = 18.0f;
        const float dSwapX = touchX - swapX;
        const float dSwapY = touchY - swapY;
        if ((dSwapX * dSwapX + dSwapY * dSwapY) <= (swapR + 6.0f) * (swapR + 6.0f))
        {
            m_skillPage = (m_skillPage + 1) % 4;
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        // 5. Mobile Navigation Dock Buttons (BAG, STAT, SKL, MAP, PTY, AUTO, OPT)
        constexpr float dockR = 19.0f;
        const float dockX = winW - 34.0f;
        const float dockY[7] = {
            winH - 224.0f, // 0: BAG
            winH - 264.0f, // 1: STAT
            winH - 304.0f, // 2: SKL
            winH - 344.0f, // 3: MAP
            winH - 384.0f, // 4: PTY
            winH - 424.0f, // 5: AUTO
            winH - 464.0f  // 6: OPT
        };
        const UIMobile::MOBILE_UI_TYPE dockTypes[7] = {
            UIMobile::UI_INVENTORY,
            UIMobile::UI_CHARACTER_INFO,
            UIMobile::UI_SKILL_SELECT,
            UIMobile::UI_MOVE_COMMAND,
            UIMobile::UI_PARTY,
            UIMobile::UI_HELPER,
            UIMobile::UI_OPTION
        };

        for (int d = 0; d < 7; ++d)
        {
            const float dx = touchX - dockX;
            const float dy = touchY - dockY[d];
            if ((dx * dx + dy * dy) <= (dockR + 8.0f) * (dockR + 8.0f))
            {
                if (g_pMobileSystem)
                {
                    g_pMobileSystem->Toggle(dockTypes[d]);
                }
                else if (d == 0 && g_pMobileInventory)
                {
                    g_pMobileInventory->Toggle();
                }
                return true;
            }
        }

        // 6. Joystick on Left Side
        if (touchX < (winW * 0.45f) && touchY > (winH * 0.30f) && !m_joystickActive)
        {
            m_joystickActive = true;
            m_joystickFingerId = ev.fingerID;
            m_joystickCenterX = std::clamp(touchX, kJoystickRadius + 15.0f, (winW * 0.45f) - kJoystickRadius);
            m_joystickCenterY = std::clamp(touchY, kJoystickRadius + 15.0f, winH - kJoystickRadius - 35.0f);
            m_knobX = m_joystickCenterX;
            m_knobY = m_joystickCenterY;
            m_dirX = 0.0f;
            m_dirY = 0.0f;
            m_strength = 0.0f;
            m_lastMoveTick = 0;
            return true;
        }

        return false;
    }

    bool CNewUIMainFrameMobile::OnFingerUp(const SDL_TouchFingerEvent& ev)
    {
        // 1. Joystick release: STOP IMMEDIATELY! Do not call SendMove!
        if (m_joystickActive && ev.fingerID == m_joystickFingerId)
        {
            ResetJoystick();
            if (Hero != nullptr)
            {
                Hero->Movement = false;
                Hero->Path.PathNum = 0;
                Hero->Path.CurrentPath = 0;
                SetPlayerStop(Hero);

                // Send immediate stop packet to server: PathNum = 1, current tile
                BYTE stopX = static_cast<BYTE>(Hero->PositionX);
                BYTE stopY = static_cast<BYTE>(Hero->PositionY);
                TargetX = stopX;
                TargetY = stopY;
#ifdef NEW_PROTOCOL_SYSTEM
                gProtocolSend.SendCharacterMoveNew(Hero->Key, Hero->Object.Angle[2], 1, &stopX, &stopY, stopX, stopY);
#else
                SendCharacterMove(Hero->Key, Hero->Object.Angle[2], 1, &stopX, &stopY, stopX, stopY);
#endif
            }
            MouseLButton = false;
            MouseLButtonPush = false;
            MouseLButtonPop = false;
            return true;
        }

        // 2. Combat button release
        if (m_aimActive && ev.fingerID == m_aimFingerId)
        {
            if (m_aimCancel)
            {
                PlayBuffer(SOUND_CLICK01);
            }
            else if (m_aimIsDragging)
            {
                // Drag release: final shot at aimed location
                if (m_aimSlot >= 0 && m_aimSlot < 4)
                {
                    int sType = -1, sSlot = -1;
                    GetSkillForSlot(m_aimSlot, sType, sSlot);
                    if (sSlot >= 0)
                    {
                        ExecuteSkillAt(sSlot, m_aimTargetMonster, m_aimTargetTileX, m_aimTargetTileY, true);
                    }
                }
                else if (m_aimSlot == 4)
                {
                    TriggerAttack();
                }
            }

            if (m_aimSlot >= 0 && m_aimSlot < 4)
            {
                m_skillPressed[m_aimSlot] = false;
                m_skillFingerId[m_aimSlot] = -1;
            }
            else if (m_aimSlot == 4)
            {
                m_attackPressed = false;
                m_attackFingerId = -1;
            }

            m_aimActive = false;
            m_aimIsDragging = false;
            m_aimCancel = false;
            m_aimSlot = -1;
            m_aimFingerId = -1;
            m_aimTargetMonster = -1;
            return true;
        }

        if (m_attackPressed && ev.fingerID == m_attackFingerId)
        {
            m_attackPressed = false;
            m_attackFingerId = -1;
            return true;
        }

        for (int i = 0; i < 4; ++i)
        {
            if (m_skillPressed[i] && ev.fingerID == m_skillFingerId[i])
            {
                m_skillPressed[i] = false;
                m_skillFingerId[i] = -1;
                return true;
            }
        }

        if (m_potPressed && ev.fingerID == m_potFingerId)
        {
            m_potPressed = false;
            m_potFingerId = -1;
            return true;
        }

        return false;
    }

    bool CNewUIMainFrameMobile::OnFingerMotion(const SDL_TouchFingerEvent& ev)
    {
        // 1. Aiming drag tracking
        if (m_aimActive && ev.fingerID == m_aimFingerId)
        {
            float touchX = 0.0f, touchY = 0.0f;
            TouchToVirtual(ev.x, ev.y, touchX, touchY);

            m_aimCurX = touchX;
            m_aimCurY = touchY;
            const float dx = touchX - m_aimStartX;
            const float dy = touchY - m_aimStartY;
            m_aimDragDist = std::hypot(dx, dy);

            if (m_aimDragDist > 8.0f)
            {
                m_aimIsDragging = true;
                m_aimDirX = dx / m_aimDragDist;
                m_aimDirY = dy / m_aimDragDist;

                if (dy < -110.0f || m_aimDragDist > 140.0f)
                {
                    m_aimCancel = true;
                }
                else
                {
                    m_aimCancel = false;
                }

                // Invert Y: touch down = negative map Y, rotated 45 deg for MU isometric camera
                const float screenDirX = m_aimDirX;
                const float screenDirY = -m_aimDirY;
                const float rad = glm::radians(45.0f);
                const float wDirX = screenDirX * cosf(rad) - screenDirY * sinf(rad);
                const float wDirY = screenDirX * sinf(rad) + screenDirY * cosf(rad);

                float maxRange = 7.0f;
                if (m_aimSlot >= 0 && m_aimSlot < 4)
                {
                    int sType = -1, sSlot = -1;
                    GetSkillForSlot(m_aimSlot, sType, sSlot);
                    if (sType > 0)
                    {
                        maxRange = std::clamp(gSkillManager.GetSkillDistance(static_cast<ActionSkillType>(sType), Hero), 3.0f, 12.0f);
                    }
                }

                const float dragFactor = std::clamp((m_aimDragDist - 8.0f) / 55.0f, 0.0f, 1.0f);
                const float aimTiles = 1.5f + dragFactor * (maxRange - 1.5f);

                m_aimTargetTileX = std::clamp(Hero->PositionX + static_cast<int>(std::round(wDirX * aimTiles)), 0, 255);
                m_aimTargetTileY = std::clamp(Hero->PositionY + static_cast<int>(std::round(wDirY * aimTiles)), 0, 255);

                const float targetWorldX = static_cast<float>(m_aimTargetTileX * TERRAIN_SCALE + 50.0f);
                const float targetWorldY = static_cast<float>(m_aimTargetTileY * TERRAIN_SCALE + 50.0f);
                Hero->Object.Angle[2] = CreateAngle(Hero->Object.Position[0], Hero->Object.Position[1], targetWorldX, targetWorldY);

                m_aimTargetMonster = FindMonsterNearTile(m_aimTargetTileX, m_aimTargetTileY, 2.5f);
            }
            else
            {
                m_aimIsDragging = false;
                m_aimCancel = false;
            }
            return true;
        }

        // 2. Joystick drag tracking
        if (!m_joystickActive || ev.fingerID != m_joystickFingerId) return false;

        float touchX = 0.0f, touchY = 0.0f;
        TouchToVirtual(ev.x, ev.y, touchX, touchY);

        const float dx = touchX - m_joystickCenterX;
        const float dy = touchY - m_joystickCenterY;
        const float dist = std::hypot(dx, dy);

        if (dist > kDeadZone)
        {
            const float maxMove = kJoystickRadius;
            const float clampedDist = (std::min)(dist, maxMove);
            m_dirX = dx / dist;
            m_dirY = dy / dist;
            m_strength = std::clamp((clampedDist - kDeadZone) / (maxMove - kDeadZone), 0.0f, 1.0f);
            m_knobX = m_joystickCenterX + m_dirX * clampedDist;
            m_knobY = m_joystickCenterY + m_dirY * clampedDist;

            if (m_strength > 0.20f && (m_lastDirX != 0.0f || m_lastDirY != 0.0f))
            {
                const float dot = m_dirX * m_lastDirX + m_dirY * m_lastDirY;
                if (dot < 0.65f)
                {
                    m_lastMoveTick = 0;
                }
            }
        }
        else
        {
            m_dirX = 0.0f;
            m_dirY = 0.0f;
            m_strength = 0.0f;
            m_knobX = m_joystickCenterX;
            m_knobY = m_joystickCenterY;
        }

        return true;
    }

    bool CNewUIMainFrameMobile::Update()
    {
        if (SceneFlag != MAIN_SCENE || Hero == nullptr) return true;

        const uint32_t now = SDL_GetTicks();

        // 1. Joystick movement: Direct smooth steering with PathFinding2 & SendMove
        if (m_joystickActive && m_strength > 0.15f)
        {
            if (now >= m_joystickPauseUntil)
            {
                // Invert screen Y for map coordinates, rotated 45 deg for MU isometric camera
                const float screenDirX = m_dirX;
                const float screenDirY = -m_dirY;
                const float rad = glm::radians(45.0f);
                const float wDirX = screenDirX * cosf(rad) - screenDirY * sinf(rad);
                const float wDirY = screenDirX * sinf(rad) + screenDirY * cosf(rad);

                const float dot = (m_lastSentWDirX != 0.0f || m_lastSentWDirY != 0.0f)
                    ? (wDirX * m_lastSentWDirX + wDirY * m_lastSentWDirY)
                    : 1.0f;

                const bool isSharpTurn = (dot < 0.65f);
                const bool isPathNearEnd = (!Hero->Movement || Hero->Path.PathNum <= 2 || (Hero->Path.PathNum - Hero->Path.CurrentPath) <= 2);
                const uint32_t elapsed = now - m_lastMoveTick;

                if ((isSharpTurn && elapsed >= 80) || (isPathNearEnd && elapsed >= 200) || (!Hero->Movement && elapsed >= 100))
                {
                    m_lastMoveTick = now;
                    m_lastSentWDirX = wDirX;
                    m_lastSentWDirY = wDirY;

                    const int step = static_cast<int>(3.0f + m_strength * 2.5f); // 4 to 5 tiles
                    bool found = false;
                    int bestTx = Hero->PositionX;
                    int bestTy = Hero->PositionY;

                    // Try full direction with decreasing steps
                    for (int s = step; s >= 1; --s)
                    {
                        const int tx = std::clamp(Hero->PositionX + static_cast<int>(std::round(wDirX * s)), 0, 255);
                        const int ty = std::clamp(Hero->PositionY + static_cast<int>(std::round(wDirY * s)), 0, 255);
                        if (tx == Hero->PositionX && ty == Hero->PositionY) continue;

                        if (IsTileWalkable(tx, ty) && PathFinding2(Hero->PositionX, Hero->PositionY, tx, ty, &Hero->Path))
                        {
                            bestTx = tx;
                            bestTy = ty;
                            found = true;
                            break;
                        }
                    }

                    // Wall sliding fallback: if diagonal blocked, try horizontal or vertical slide
                    if (!found)
                    {
                        if (std::abs(wDirX) > 0.35f)
                        {
                            const int tx = std::clamp(Hero->PositionX + static_cast<int>(std::round(wDirX * 3)), 0, 255);
                            const int ty = Hero->PositionY;
                            if (tx != Hero->PositionX && IsTileWalkable(tx, ty) && PathFinding2(Hero->PositionX, Hero->PositionY, tx, ty, &Hero->Path))
                            {
                                bestTx = tx;
                                bestTy = ty;
                                found = true;
                            }
                        }
                        if (!found && std::abs(wDirY) > 0.35f)
                        {
                            const int tx = Hero->PositionX;
                            const int ty = std::clamp(Hero->PositionY + static_cast<int>(std::round(wDirY * 3)), 0, 255);
                            if (ty != Hero->PositionY && IsTileWalkable(tx, ty) && PathFinding2(Hero->PositionX, Hero->PositionY, tx, ty, &Hero->Path))
                            {
                                bestTx = tx;
                                bestTy = ty;
                                found = true;
                            }
                        }
                    }

                    if (found)
                    {
                        TargetX = bestTx;
                        TargetY = bestTy;
                        Hero->MovementType = MOVEMENT_MOVE;
                        SendMove(Hero, &Hero->Object);
                        m_wasDrivingMove = true;
                    }
                }
            }
        }
        else if (m_wasDrivingMove)
        {
            ResetJoystick();
        }

        // 2. Long-press continuous attack at aimed ground/target ("ấn lì thì nó tự đánh")
        if (m_aimActive && !m_aimCancel)
        {
            if ((now - m_aimHoldTick >= 180) && (now - m_lastCombatTick >= 250))
            {
                m_lastCombatTick = now;
                if (m_aimSlot >= 0 && m_aimSlot < 4)
                {
                    int sType = -1, sSlot = -1;
                    GetSkillForSlot(m_aimSlot, sType, sSlot);
                    if (sSlot >= 0)
                    {
                        ExecuteSkillAt(sSlot, m_aimTargetMonster, m_aimTargetTileX, m_aimTargetTileY, m_aimIsDragging);
                    }
                }
                else if (m_aimSlot == 4)
                {
                    TriggerAttack();
                }
            }
        }

        return true;
    }

    bool CNewUIMainFrameMobile::UpdateMouseEvent()
    {
        return true;
    }

    bool CNewUIMainFrameMobile::UpdateKeyEvent()
    {
        return true;
    }

    bool CNewUIMainFrameMobile::Render()
    {
        if (SceneFlag != MAIN_SCENE || Hero == nullptr) return true;
        if (!IsVisible())
        {
            Show(true);
        }

        EnsureTextures();
        if (m_texCircle == 0 || m_texJoyBase == 0 || m_texJoyKnob == 0) return true;

        EnableAlphaBlend();

        // ── 1. High-Detail Joystick ──
        glColor4f(1.0f, 1.0f, 1.0f, m_joystickActive ? 0.95f : 0.70f);
        RenderBitmap(m_texJoyBase, m_joystickCenterX - kJoystickRadius, m_joystickCenterY - kJoystickRadius,
                     kJoystickRadius * 2.0f, kJoystickRadius * 2.0f, 0, 0, 1, 1, true, true);

        if (m_joystickActive)
        {
            glColor4f(0.30f, 0.80f, 1.0f, 0.45f);
            RenderBitmap(m_texCircle, m_knobX - kKnobRadius * 1.25f, m_knobY - kKnobRadius * 1.25f,
                         kKnobRadius * 2.5f, kKnobRadius * 2.5f, 0, 0, 1, 1, true, true);
        }
        glColor4f(1.0f, 1.0f, 1.0f, m_joystickActive ? 1.0f : 0.85f);
        RenderBitmap(m_texJoyKnob, m_knobX - kKnobRadius, m_knobY - kKnobRadius,
                     kKnobRadius * 2.0f, kKnobRadius * 2.0f, 0, 0, 1, 1, true, true);

        // ── 2. Combat Layout ──
        const float winW = (DisplayWin > 0) ? static_cast<float>(DisplayWin) : 640.0f;
        const float winH = (DisplayHeight > 0) ? static_cast<float>(DisplayHeight) : 480.0f;

        float atkX, atkY, atkR;
        float skillX[4], skillY[4], skillR;
        float potX, potY, potR;
        GetCombatLayout(atkX, atkY, atkR, skillX, skillY, skillR, potX, potY, potR);

        // Quick Potion (HP)
        glColor4f(0.18f, 0.03f, 0.03f, m_potPressed ? 0.90f : 0.65f);
        RenderBitmap(m_texCircle, potX - potR, potY - potR, potR * 2.0f, potR * 2.0f, 0, 0, 1, 1, true, true);
        glColor4f(1.0f, 0.28f, 0.28f, m_potPressed ? 1.0f : 0.78f);
        RenderBitmap(m_texRing, potX - potR, potY - potR, potR * 2.0f, potR * 2.0f, 0, 0, 1, 1, true, true);
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(255, 140, 140, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->RenderText(static_cast<int>(potX - 8.0f), static_cast<int>(potY - 6.0f), "HP");

        // Skill Swap / Page Button
        const float swapX = winW - 178.0f;
        const float swapY = winH - 132.0f;
        constexpr float swapR = 18.0f;
        glColor4f(0.08f, 0.12f, 0.20f, 0.65f);
        RenderBitmap(m_texCircle, swapX - swapR, swapY - swapR, swapR * 2.0f, swapR * 2.0f, 0, 0, 1, 1, true, true);
        glColor4f(0.40f, 0.75f, 1.0f, 0.80f);
        RenderBitmap(m_texRing, swapX - swapR, swapY - swapR, swapR * 2.0f, swapR * 2.0f, 0, 0, 1, 1, true, true);
        char pageStr[8];
        snprintf(pageStr, sizeof(pageStr), "P%d", m_skillPage + 1);
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(160, 220, 255, 230);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->RenderText(static_cast<int>(swapX - 7.0f), static_cast<int>(swapY - 6.0f), pageStr);

        // Skill Buttons 1..4
        const int activeSlot = Hero->CurrentSkill;
        for (int i = 0; i < 4; ++i)
        {
            int sType = -1, sSlot = -1;
            GetSkillForSlot(i, sType, sSlot);
            const bool isPressed = m_skillPressed[i];
            const bool isCurrent = (sSlot >= 0 && sSlot == activeSlot);

            if (isCurrent)
                glColor4f(0.35f, 0.25f, 0.05f, 0.85f);
            else if (isPressed)
                glColor4f(0.12f, 0.25f, 0.45f, 0.90f);
            else
                glColor4f(0.05f, 0.09f, 0.18f, 0.65f);
            RenderBitmap(m_texCircle, skillX[i] - skillR, skillY[i] - skillR, skillR * 2.0f, skillR * 2.0f, 0, 0, 1, 1, true, true);

            if (isCurrent)
                glColor4f(1.0f, 0.88f, 0.25f, 0.98f);
            else if (isPressed)
                glColor4f(0.45f, 0.88f, 1.0f, 0.98f);
            else
                glColor4f(0.35f, 0.65f, 0.95f, 0.75f);
            RenderBitmap(m_texRing, skillX[i] - skillR, skillY[i] - skillR, skillR * 2.0f, skillR * 2.0f, 0, 0, 1, 1, true, true);

            if (sSlot >= 0 && g_pSkillList != nullptr)
            {
                constexpr float iconW = 22.0f;
                constexpr float iconH = 30.0f;
                g_pSkillList->RenderSkillIconPreview(sSlot, skillX[i] - iconW * 0.5f, skillY[i] - iconH * 0.5f, iconW, iconH);
            }

            char numStr[4];
            snprintf(numStr, sizeof(numStr), "%d", i + 1);
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(255, 230, 130, 240);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->RenderText(static_cast<int>(skillX[i] + skillR - 12.0f), static_cast<int>(skillY[i] - skillR), numStr);
        }

        // ATK Button
        if (m_attackPressed)
            glColor4f(0.35f, 0.12f, 0.08f, 0.90f);
        else
            glColor4f(0.12f, 0.08f, 0.20f, 0.70f);
        RenderBitmap(m_texCircle, atkX - atkR, atkY - atkR, atkR * 2.0f, atkR * 2.0f, 0, 0, 1, 1, true, true);

        if (m_attackPressed)
            glColor4f(1.0f, 0.55f, 0.20f, 1.0f);
        else
            glColor4f(0.90f, 0.45f, 1.0f, 0.85f);
        RenderBitmap(m_texRing, atkX - atkR, atkY - atkR, atkR * 2.0f, atkR * 2.0f, 0, 0, 1, 1, true, true);

        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(255, 255, 255, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->RenderText(static_cast<int>(atkX - 12.0f), static_cast<int>(atkY - 6.0f), "ATK");

        // Mobile Navigation Dock Buttons (BAG, STAT, SKILL, MAP, PTY)
        constexpr float dockR = 19.0f;
        const float dockX = winW - 34.0f;
        const float dockY[7] = {
            winH - 224.0f, // 0: BAG
            winH - 264.0f, // 1: STAT
            winH - 304.0f, // 2: SKL
            winH - 344.0f, // 3: MAP
            winH - 384.0f, // 4: PTY
            winH - 424.0f, // 5: AUTO
            winH - 464.0f  // 6: OPT
        };
        const char* dockLabels[7] = { "BAG", "STAT", "SKL", "MAP", "PTY", "AUTO", "OPT" };
        const UIMobile::MOBILE_UI_TYPE dockTypes[7] = {
            UIMobile::UI_INVENTORY,
            UIMobile::UI_CHARACTER_INFO,
            UIMobile::UI_SKILL_SELECT,
            UIMobile::UI_MOVE_COMMAND,
            UIMobile::UI_PARTY,
            UIMobile::UI_HELPER,
            UIMobile::UI_OPTION
        };

        for (int d = 0; d < 7; ++d)
        {
            const bool isOpen = g_pMobileSystem ? g_pMobileSystem->IsVisible(dockTypes[d]) : (d == 0 && g_pMobileInventory && g_pMobileInventory->IsOpen());

            if (isOpen)
                glColor4f(0.35f, 0.28f, 0.08f, 0.90f);
            else
                glColor4f(0.08f, 0.12f, 0.22f, 0.70f);
            RenderBitmap(m_texCircle, dockX - dockR, dockY[d] - dockR, dockR * 2.0f, dockR * 2.0f, 0, 0, 1, 1, true, true);

            if (isOpen)
                glColor4f(1.0f, 0.85f, 0.20f, 1.0f);
            else
                glColor4f(0.35f, 0.65f, 0.95f, 0.85f);
            RenderBitmap(m_texRing, dockX - dockR, dockY[d] - dockR, dockR * 2.0f, dockR * 2.0f, 0, 0, 1, 1, true, true);

            g_pRenderText->SetFont(g_hFontBold);
            if (isOpen)
                g_pRenderText->SetTextColor(255, 230, 100, 255);
            else
                g_pRenderText->SetTextColor(200, 220, 245, 255);
            g_pRenderText->SetBgColor(0, 0, 0, 0);

            const int textOffX = (std::strlen(dockLabels[d]) == 4) ? 14 : 11;
            g_pRenderText->RenderText(static_cast<int>(dockX - static_cast<float>(textOffX)), static_cast<int>(dockY[d] - 5.0f), dockLabels[d]);
        }

        // ── 3. Ground Reticle Indicator ("cái chấm ở dưới chân kéo đi đâu là nó đánh ở đó") ──
        const uint32_t nowTime = SDL_GetTicks();
        const bool showAimIndicator = m_aimActive && ((nowTime - m_aimHoldTick >= 120) || m_aimIsDragging);

        if (showAimIndicator)
        {
            // A. Cancel Zone
            const float cancelX = winW - 100.0f;
            const float cancelY = winH - 235.0f;
            constexpr float cancelR = 28.0f;

            if (m_aimCancel)
            {
                glColor4f(0.90f, 0.15f, 0.15f, 0.92f);
                RenderBitmap(m_texCircle, cancelX - cancelR, cancelY - cancelR, cancelR * 2.0f, cancelR * 2.0f, 0, 0, 1, 1, true, true);
                glColor4f(1.0f, 0.45f, 0.45f, 1.0f);
                RenderBitmap(m_texRing, cancelX - cancelR, cancelY - cancelR, cancelR * 2.0f, cancelR * 2.0f, 0, 0, 1, 1, true, true);
            }
            else
            {
                glColor4f(0.20f, 0.05f, 0.05f, 0.60f);
                RenderBitmap(m_texCircle, cancelX - cancelR, cancelY - cancelR, cancelR * 2.0f, cancelR * 2.0f, 0, 0, 1, 1, true, true);
                glColor4f(0.90f, 0.35f, 0.35f, 0.70f);
                RenderBitmap(m_texRing, cancelX - cancelR, cancelY - cancelR, cancelR * 2.0f, cancelR * 2.0f, 0, 0, 1, 1, true, true);
            }

            g_pRenderText->SetFont(g_hFontBold);
            if (m_aimCancel)
                g_pRenderText->SetTextColor(255, 255, 255, 255);
            else
                g_pRenderText->SetTextColor(255, 170, 170, 220);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->RenderText(static_cast<int>(cancelX - 16.0f), static_cast<int>(cancelY - 6.0f), "CANCEL");

            // B. Skill Button Guide Ring & Knob
            constexpr float aimGuideR = 64.0f;
            if (m_aimCancel)
                glColor4f(1.0f, 0.25f, 0.25f, 0.60f);
            else
                glColor4f(0.30f, 0.80f, 1.0f, 0.60f);
            RenderBitmap(m_texRing, m_aimStartX - aimGuideR, m_aimStartY - aimGuideR,
                         aimGuideR * 2.0f, aimGuideR * 2.0f, 0, 0, 1, 1, true, true);

            const float clampedAimDrag = (std::min)(m_aimDragDist, aimGuideR);
            const float knobX = m_aimStartX + m_aimDirX * clampedAimDrag;
            const float knobY = m_aimStartY + m_aimDirY * clampedAimDrag;
            constexpr float aimKnobR = 15.0f;

            if (m_aimCancel)
                glColor4f(1.0f, 0.30f, 0.30f, 0.95f);
            else
                glColor4f(0.40f, 0.95f, 1.0f, 0.95f);
            RenderBitmap(m_texCircle, knobX - aimKnobR, knobY - aimKnobR, aimKnobR * 2.0f, aimKnobR * 2.0f, 0, 0, 1, 1, true, true);

            // C. Ground Target Indicator ("cái chấm ở dưới chân")
            if (!m_aimCancel)
            {
                vec3_t targetWorldPos;
                if (m_aimIsDragging)
                {
                    targetWorldPos[0] = static_cast<float>(m_aimTargetTileX * TERRAIN_SCALE + 50.0f);
                    targetWorldPos[1] = static_cast<float>(m_aimTargetTileY * TERRAIN_SCALE + 50.0f);
                }
                else
                {
                    targetWorldPos[0] = Hero->Object.Position[0];
                    targetWorldPos[1] = Hero->Object.Position[1];
                }
                targetWorldPos[2] = RequestTerrainHeight(targetWorldPos[0], targetWorldPos[1]) + 8.0f;

                float targetSx = 0.0f, targetSy = 0.0f;
                const bool targetValid = SafeProject(targetWorldPos, targetSx, targetSy);

                float heroSx = 0.0f, heroSy = 0.0f;
                const bool heroValid = SafeProject(Hero->Object.Position, heroSx, heroSy);

                if (targetValid && heroValid)
                {
                    if (m_aimIsDragging)
                    {
                        for (int s = 1; s <= 4; ++s)
                        {
                            const float t = s / 5.0f;
                            vec3_t dotPos;
                            dotPos[0] = Hero->Object.Position[0] + (targetWorldPos[0] - Hero->Object.Position[0]) * t;
                            dotPos[1] = Hero->Object.Position[1] + (targetWorldPos[1] - Hero->Object.Position[1]) * t;
                            dotPos[2] = RequestTerrainHeight(dotPos[0], dotPos[1]) + 8.0f;

                            float dotSx = 0.0f, dotSy = 0.0f;
                            if (SafeProject(dotPos, dotSx, dotSy))
                            {
                                const float dotR = 3.5f + t * 2.0f;
                                glColor4f(0.30f, 0.85f, 1.0f, 0.40f + t * 0.45f);
                                RenderBitmap(m_texCircle, dotSx - dotR, dotSy - dotR, dotR * 2.0f, dotR * 2.0f, 0, 0, 1, 1, true, true);
                            }
                        }
                    }

                    // Ground target circle ("cái chấm")
                    if (m_aimTargetMonster >= 0)
                    {
                        constexpr float reticleR = 28.0f;
                        glColor4f(1.0f, 0.25f, 0.15f, 0.95f);
                        RenderBitmap(m_texReticle, targetSx - reticleR, targetSy - reticleR, reticleR * 2.0f, reticleR * 2.0f, 0, 0, 1, 1, true, true);
                        glColor4f(1.0f, 0.65f, 0.20f, 0.85f);
                        RenderBitmap(m_texCircle, targetSx - 6.0f, targetSy - 6.0f, 12.0f, 12.0f, 0, 0, 1, 1, true, true);
                    }
                    else
                    {
                        constexpr float reticleR = 24.0f;
                        glColor4f(0.25f, 0.90f, 1.0f, 0.90f);
                        RenderBitmap(m_texReticle, targetSx - reticleR, targetSy - reticleR, reticleR * 2.0f, reticleR * 2.0f, 0, 0, 1, 1, true, true);
                        glColor4f(0.40f, 0.95f, 1.0f, 0.75f);
                        RenderBitmap(m_texCircle, targetSx - 5.0f, targetSy - 5.0f, 10.0f, 10.0f, 0, 0, 1, 1, true, true);
                    }
                }
            }
        }

        return true;
    }
}

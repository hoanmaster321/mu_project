
#pragma once

#include "../CBMu_RenderConfig.h"

struct CBMu_EffectBudgetLimits
{
	int MaxParticlesSubmitted;
	int MaxJointsSubmitted;
	int MaxLargeGlowSubmitted;
	int MaxTransparentSpritesSubmitted;
};

struct CBMu_EffectBudgetFrame
{
	int ParticlesSubmitted;
	int JointsSubmitted;
	int LargeGlowSubmitted;
	int TransparentSpritesSubmitted;
};

class CBMu_EffectBudget
{
public:
	CBMu_EffectBudget();

	void ResetFrame();
	void SetLimits(const CBMu_EffectBudgetLimits& limits);
	const CBMu_EffectBudgetLimits& GetLimits() const;
	const CBMu_EffectBudgetFrame& GetFrame() const;

	bool CanSubmitParticle() const;
	bool CanSubmitJoint() const;
	bool CanSubmitLargeGlow() const;
	bool CanSubmitTransparentSprite() const;

	void AddParticleSubmitted();
	void AddJointSubmitted();
	void AddLargeGlowSubmitted();
	void AddTransparentSpriteSubmitted();

private:
	CBMu_EffectBudgetLimits m_Limits;
	CBMu_EffectBudgetFrame m_Frame;
};

extern CBMu_EffectBudget g_CBMuEffectBudget;

// CustomWingEffect.cpp: implementation of the CCustomWingEffect class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CustomWing.h"
#include "DynamicEffect.h"

CDynamicWingEffect gDynamicWingEffect;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDynamicWingEffect::CDynamicWingEffect() // OK
{
	this->m_DynamicWingEffectInfo.clear();
	this->m_DynamicWingByItemIndex.clear();
}

CDynamicWingEffect::~CDynamicWingEffect() // OK
{

}


void CDynamicWingEffect::Load(DYNAMIC_WING_EFFECT_INFO* info) // OK
{
	this->m_DynamicWingEffectInfo.clear();
	this->m_DynamicWingByItemIndex.clear();
	for(int n=0;n < MAX_DYNAMIC_WING_EFFECT;n++)
	{
		if(info[n].Index < 0 || info[n].Index >= MAX_DYNAMIC_WING_EFFECT)
		{
			return;
		}
		this->m_DynamicWingEffectInfo.insert(std::pair<int,DYNAMIC_WING_EFFECT_INFO>(info[n].Index,info[n]));
		this->m_DynamicWingByItemIndex[info[n].ItemIndex].push_back(info[n]);
	}
}
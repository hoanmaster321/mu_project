///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ZzzOpenglUtil.h"
#include "ZzzBMD.h"
#include "ZzzInfomation.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "ZzzLodTerrain.h"
#include "ZzzTexture.h"
#include "ZzzAi.h"
#include "ZzzEffect.h"
#include "DSPlaySound.h"
#include "WSClient.h"
#include "NewUISystem.h"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace
{
constexpr float kSpriteFrameMin = 0.2f;
constexpr float kSpriteFrameMax = 1.0f;
constexpr float kSpriteFrameStep = 0.1f;

float ComputeFrameDelta()
{
    return kSpriteFrameStep * FPS_ANIMATION_FACTOR;
}

float UpdateAnimationFrame(float currentFrame, bool isVisible)
{
    const float delta = ComputeFrameDelta();
    currentFrame += isVisible ? delta : -delta;
    return std::clamp(currentFrame, kSpriteFrameMin, kSpriteFrameMax);
}

}

OBJECT	Sprites   [MAX_SPRITES];
inline SpinLock* g_CreateSprite_lock = new SpinLock();
int CreateSprite(int Type,vec3_t Position,float Scale,vec3_t Light,OBJECT *Owner,float Rotation,int SubType)
{
	if (!g_pNewUISystem->GetUI_NewOptionWindow()->OnOffGrap[g_pNewUISystem->GetUI_NewOptionWindow()->eEffectStatic]) return false;

#if defined(__ANDROID__) || defined(MU_IOS)
	if (ShouldThrottleAdaptiveEffectSpawn(ADAPTIVE_EFFECT_SPRITE, Type, Position, SubType, Scale, Owner))
	{
		return false;
	}
#endif

	//g_CreateSprite_lock->lock();
	for(int i=0;i<MAX_SPRITES;i++)
	{
		OBJECT *o = &Sprites[i];
		//== Fix Effect
		if (i >= (MAX_SPRITES - 2) && o->Live)
		{
			o->Live = false;
			o->Visible = false;
		}
		if(!o->Live)
		{
			o->Live           = true;
			o->Type           = Type;
			o->SubType        = SubType;
			o->Owner          = Owner;
			o->AnimationFrame = 1.f;
    		o->Scale          = Scale;
			o->Angle[2]       = Rotation;
			VectorCopy(Position,o->Position);
			VectorCopy(Position,o->StartPosition);
			VectorCopy(Light,o->Light);
			//g_CreateSprite_lock->unlock();
			return i;
		}
	}
	//g_CreateSprite_lock->unlock();
	return false;
}

void RenderSprite(OBJECT *o,OBJECT *Owner)
{
	if (GetRenderEffect() == false) return;

	if (o->Visible)
	{
		o->AnimationFrame += 0.1f * static_cast<float>(FPS_ANIMATION_FACTOR);
		if (o->AnimationFrame > 1.f)
		{
			o->AnimationFrame = 1.f;
		}
	}
	else
	{
		o->AnimationFrame -= 0.1f * static_cast<float>(FPS_ANIMATION_FACTOR);
		if (o->AnimationFrame < 0.2f)
		{
			o->AnimationFrame = 0.2f;
		}
	}
	float Scale = o->AnimationFrame*o->Scale;
	
	BITMAP_t* pBitmap = Bitmaps.GetTexture(o->Type);
	float Width  = pBitmap->Width * Scale;
	float Height = pBitmap->Height * Scale;

    if ( o->Type==BITMAP_FORMATION_MARK )
    {
		float u = 0.0f, v = 0.0f, uw, vw;
		uw=0.33f; vw=0.33f;
        switch ( o->SubType )
        {
        case 0:
            u=0.f; v=0.f;
            break;

        case 1:
            u=0.33f; v=0.f;
            break;

        case 2:
            u=0.66f; v=0.f;
            break;

        case 3:
            u=0.f; v=0.33f;
            break;

        case 4:
            u=0.33f; v=0.33f;
            break;

        case 5:
            u=0.66f; v=0.33f;
            break;

        case 6:
            u=0.f; v=0.66f;
            break;

        case 7:
            u=0.33f; v=0.66f;
            break;
        }

        RenderSprite( o->Type, o->Position, 64, 64, o->Light, o->Angle[2], u, v, uw, vw );
    }
    else
    {
        RenderSprite(o->Type,o->Position,Width,Height,o->Light,o->Angle[2]);
    }
}

void RenderSprites ( BYTE byRenderOneMore )
{
	for(int i=0;i<MAX_SPRITES;i++)
	{
		OBJECT *o = &Sprites[i];
        if( byRenderOneMore == 1 )
        {
            if ( o->Position[2] > 150.f ) 
			{
				continue;
			}
        }
        else if( byRenderOneMore == 2 )
        {
            if( o->Position[2] <= 100.f )
            {
                o->Live = false;
                continue;
            }
        }

		if(o->Live)
		{
            if( o->Type == BITMAP_FORMATION_MARK )
            {
                EnableAlphaTest ();
            }
            else if(o->SubType == 0)
			{
          	    EnableAlphaBlend();
			}
			else if( o->SubType==1 )
			{
               	EnableAlphaBlendMinus();
			}
            else if( o->SubType==2 )
			{
                EnableAlphaTest();
			}
            else if( o->SubType==3 )
			{
                EnableAlphaBlend2();
			}
    		RenderSprite(o,o->Owner);

            if( byRenderOneMore == 0 || byRenderOneMore == 2 )
            {
                o->Live = false;
            }
		}
	}
}

void CheckSprites()
{
	for(int i=0; i<MAX_SPRITES; i++)
	{
		OBJECT *o = &Sprites[i];
		if(o->Live)
		{
         	o->Visible = true;
		}
	}
}

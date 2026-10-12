#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	enum EnterFlags : UInt8
	{
		kEnterAnimationNone = 0,
		kEnterAnimationFade = 1,
		kEnterAnimationSize = 2,

		//special, for enterSize
		kEnterNoClip = 32,
	};


	void Enter(Object & object, UInt8 flags = kEnterAnimationFade | kEnterAnimationSize);

	void Exit(Object & object, bool detach, UInt8 or_flags = 0);


	void SkipEnter(Object & object, UInt8 flags = kEnterAnimationFade | kEnterAnimationSize);

	void SkipExit(Object & object, bool detach, UInt8 or_flags = 0);	//exit when not detached should do same thing


	bool HasEntered(const Object & object);

}




//
//Detail

namespace Reflex::GLX::Detail
{

	void EnableEnterExitRenderToTexture(Object & object, bool enable = true);	//enable for high-quality fade, this can be slow on large areas on some GPUs, so GLX defaults to opacity fade

}




//
//impl

inline void Reflex::GLX::Detail::EnableEnterExitRenderToTexture(Object & object, bool enable)
{
	auto render_mode = enable ? ComputedStyle::kRenderAuto : ComputedStyle::kRenderFalse;

	object.SetMod(Reflex::Detail::MergeHashes(MakeKey32("entering"), kopacity), ComputedStyle::Create(1.0f, 1.0f, render_mode));
}

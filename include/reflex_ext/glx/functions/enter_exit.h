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


	[[deprecated("use kEnterAnimationFade")]] constexpr auto kEnterFade = kEnterAnimationFade;

}

namespace Reflex::GLX::Detail
{

	inline void SetEnterExitRenderMode(Object & object, ComputedStyle::Render render)
	{
		object.SetMod(Reflex::Detail::MergeHashes(MakeKey32("entering"), kopacity), ComputedStyle::Create(1.0f, 1.0f, render));
	}

	inline void EnableEnterExitRender(Object & object, bool enable = true)
	{
		SetEnterExitRenderMode(object, enable ? ComputedStyle::kRenderTrue : ComputedStyle::kRenderFalse);
	}

}

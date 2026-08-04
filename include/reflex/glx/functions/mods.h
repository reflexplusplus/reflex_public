#pragma once

#include "../object.h"




//
//decl

namespace Reflex::GLX
{

	void EnableDraw(Object & object, Key32 id, bool enable);


	void UnsetClip(Object & object, Key32 id = kNullKey);

	void SetClip(Object & object, Key32 id = kNullKey, bool x = true, bool y = true);

	Pair <bool> GetClip(const Object & object, Key32 id);


	void UnsetOpacity(Object & object, Key32 id);

	void SetOpacity(Object & object, Key32 id, Float opacity, Detail::ComputedStyle::Render render = Detail::ComputedStyle::kRenderAuto);

	Float GetOpacity(const Object & object, Key32 id);


	void UnsetBounds(Object & object, Key32 id);

	void SetBounds(Object & object, Key32 id, const Size & min, const Size & max = GLX::kLarge);

	const Pair <Size> & GetBounds(const Object & object, Key32 id);


	[[deprecated]] void UnsetMagnification(Object & object, Key32 id);

	[[deprecated]] void SetMagnification(Object & object, Key32 id, Float scale, Detail::ComputedStyle::Render render = Detail::ComputedStyle::kRenderAuto);

	[[deprecated]] Float GetMagnification(const Object & object, Key32 id);

}




//
//impl

REFLEX_NS(Reflex::GLX::Detail)
REFLEX_DECLARE_KEY32(clip);
REFLEX_DECLARE_KEY32(bounds);
REFLEX_DECLARE_KEY32(magnification);
REFLEX_END

REFLEX_INLINE void Reflex::GLX::EnableDraw(Object & object, Key32 id, bool enable)
{
	if (enable)
	{
		UnsetOpacity(object, id);
	}
	else
	{
		SetOpacity(object, id, 0.0f, GLX::Detail::ComputedStyle::kRenderFalse);
	}
}

REFLEX_INLINE void Reflex::GLX::UnsetClip(Object & object, Key32 id)
{
	object.ClearMod(Reflex::Detail::MergeHashes(id.value, Detail::kclip));
}

REFLEX_INLINE Reflex::Pair <bool> Reflex::GLX::GetClip(const Object & object, Key32 id)
{
	return object.GetMod(Reflex::Detail::MergeHashes(id.value, Detail::kclip))->GetClip();
}

REFLEX_INLINE void Reflex::GLX::UnsetOpacity(Object & object, Key32 id)
{
	object.ClearMod(Reflex::Detail::MergeHashes(id.value, kopacity));
}

REFLEX_INLINE void Reflex::GLX::SetOpacity(Object & object, Key32 id, Float opacity, Detail::ComputedStyle::Render render)
{
	auto mod_id = Reflex::Detail::MergeHashes(id.value, kopacity);

	if (opacity < 1.0f)
	{
		object.SetMod(mod_id, Detail::ComputedStyle::Create(1.0f, opacity, render));
	}
	else
	{
		object.ClearMod(mod_id);
	}
}

REFLEX_INLINE Reflex::Float Reflex::GLX::GetOpacity(const Object & object, Key32 id)
{
	return object.GetMod(Reflex::Detail::MergeHashes(id.value, kopacity))->GetOpacity();
}

REFLEX_INLINE void Reflex::GLX::UnsetBounds(Object & object, Key32 id)
{
	object.ClearMod(Reflex::Detail::MergeHashes(id.value, Detail::kbounds));
}

REFLEX_INLINE void Reflex::GLX::SetBounds(Object & object, Key32 id, const Size & min, const Size & max)
{
	object.SetMod(Reflex::Detail::MergeHashes(id.value, Detail::kbounds), Detail::ComputedStyle::Create(min, max));
}

REFLEX_INLINE const Reflex::Pair <Reflex::GLX::Size> & Reflex::GLX::GetBounds(const Object & object, Key32 id)
{
	return object.GetMod(Reflex::Detail::MergeHashes(id.value, Detail::kbounds))->GetMinMax();
}

REFLEX_INLINE void Reflex::GLX::UnsetMagnification(Object & object, Key32 id)
{
	object.ClearMod(Reflex::Detail::MergeHashes(id.value, Detail::kmagnification));
}

REFLEX_INLINE void Reflex::GLX::SetMagnification(Object & object, Key32 id, Float scale, Detail::ComputedStyle::Render render)
{
	object.SetMod(Reflex::Detail::MergeHashes(id.value, Detail::kmagnification), Detail::ComputedStyle::Create(scale, 1.0f, render));
}

REFLEX_INLINE Reflex::Float Reflex::GLX::GetMagnification(const Object & object, Key32 id)
{
	return object.GetMod(Reflex::Detail::MergeHashes(id.value, Detail::kmagnification))->GetScale();
}

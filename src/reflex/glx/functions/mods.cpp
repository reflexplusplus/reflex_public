#include "../../../../include/reflex/glx/functions/mods.h"




//
//impl

void Reflex::GLX::SetClip(Object & object, Key32 id, bool x, bool y)
{
	auto modid = Reflex::Detail::MergeHashes(id.value, Detail::kclip);

	if (Reinterpret<UInt8>(x) | Reinterpret<UInt8>(y))
	{
		object.SetMod(modid, Detail::ComputedStyle::Create(x, y));
	}
	else
	{
		object.UnsetMod(modid);
	}
}
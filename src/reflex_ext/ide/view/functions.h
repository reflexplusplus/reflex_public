#pragma once

#include "../../../../include/reflex/ide/console.h"




REFLEX_NS(Reflex::IDE)

const Reflex::WString & Reflex::GLX::GetResourcePath(File::ResourcePool::Lock & lock, const Object & s)
{
	return File::GetPath(lock, MakeAddress<StyleSheet>(s.path));
}

REFLEX_END
#pragma once

#include "core.h"




//
//Secondary API

namespace Reflex::GLX
{

	REFLEX_DECLARE_KEY32(EmulateMobile);

	REFLEX_DECLARE_KEY32(Animations);


	Reference <Reflex::Object> Start(File::ResourcePool & resourcepool, const System::Renderer::Config & config = {});

	void Restart();


	Key32 RegisterKey(CString::View key);	//for debug only

	ConstTRef <Data::KeyMap> GetKeyMap();	//for debug only

	CString::View GetKey(Key32 id);	//for debug only


	extern Reflex::Detail::Module module;	//no dependencies, initalised by Start, use as parent of UI nulls

	extern const bool & kIsMobile;

	extern const Pair <bool,Float> & kSystemTheme;	//cached from System

}




//
//impl

#if !REFLEX_DEBUG
inline Reflex::Key32 Reflex::GLX::RegisterKey(CString::View key) { return key; }
inline Reflex::ConstTRef <Reflex::Data::KeyMap> Reflex::GLX::GetKeyMap() { return {}; }
inline Reflex::CString::View Reflex::GLX::GetKey(Key32 id) { return {}; }
#endif

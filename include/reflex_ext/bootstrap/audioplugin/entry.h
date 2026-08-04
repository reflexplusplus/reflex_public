#pragma once

#include "audioplugin.h"




//
//Secondary API

namespace Reflex::Bootstrap
{

	template <class INSTANCE> TRef <Global> StartAudioPlugin(System::AudioPlugin::Configuration & config, const CString::View & vendor, const CString::View & product, Key32 resources_subdomain, const char * entry);

}




//
//impl

REFLEX_NS(Reflex::Bootstrap::Detail)

using ParamDefsProperty = ObjectOf < Array < Pair < Key32, ConstReference <ParamDesc> > > >;

REFLEX_END

template <class INSTANCE> inline Reflex::TRef <Reflex::Bootstrap::Global> Reflex::Bootstrap::StartAudioPlugin(System::AudioPlugin::Configuration & config, const CString::View & vendor, const CString::View & product, Key32 resources_subdomain, const char * entry)
{
	auto global = Global::Acquire(vendor, product, Detail::ExtractProjectDir(entry), resources_subdomain);
	
	auto & cls = config.classes.Push(INSTANCE::MakeClass());

	auto paramdefs_property = New<Detail::ParamDefsProperty>();

	auto & paramdefs = paramdefs_property->value;

	paramdefs.SetSize(cls.num_params);

	INSTANCE::PopulateParameters(cls, ToRegion(paramdefs));

#if REFLEX_DEBUG
	Map <Key32, UInt> uids;
	for (auto & [id, def] : paramdefs)
	{
		REFLEX_ASSERT_EX(!uids.Acquire(id)++, "duplicate parameter id");
	}
#endif

	global->SetProperty(MakeKey32("bootstrap.paramdefs"), paramdefs_property);

	config.instance_ctr = [](Object & global, const System::AudioPlugin::Configuration::Class & cls, System::AudioPlugin & instance)
	{
		return Detail::Initialise(New<INSTANCE>(instance));
	};

	return global;
}

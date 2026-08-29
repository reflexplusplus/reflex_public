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

template <class INSTANCE> inline Reflex::TRef <Reflex::Bootstrap::Global> Reflex::Bootstrap::StartAudioPlugin(System::AudioPlugin::Configuration & config, const CString::View & vendor, const CString::View & product, Key32 resources_subdomain, const char * entry)
{
	auto global = Global::Acquire(vendor, product, Detail::ExtractProjectDir(entry), resources_subdomain);
	
	config.classes = INSTANCE::MakeClasses();

	auto class_paramdefs = Data::AcquirePropertySet(global, K32("bootstrap.paramdefs"));

	for (const auto & cls : config.classes)
	{
		auto defs_property = New<Detail::ParamDefs>();
		auto & paramdefs = defs_property->value;
		paramdefs.SetSize(cls.num_params);
		INSTANCE::PopulateParameters(cls, ToRegion(paramdefs));
#if REFLEX_DEBUG
		Map <Key32, UInt> uids;
		for (auto & [id, def] : paramdefs) REFLEX_ASSERT_EX(!uids.Acquire(id)++, "duplicate parameter id");
#endif
		auto key = MakeKey32(cls.clap.uid);

		REFLEX_ASSERT_EX(!QueryAbstractProperty(class_paramdefs, key), "duplicate CLAP class key");
		
		SetAbstractProperty(class_paramdefs, key, defs_property);
	}

	config.instance_ctr = [](Object & global, const System::AudioPlugin::Configuration::Class & cls, System::AudioPlugin & instance)
	{
		return Detail::Initialise(INSTANCE::Create(cls, instance));
	};

	return global;
}

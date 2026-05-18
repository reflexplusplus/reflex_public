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
	
	config.classes = ToView(INSTANCE::MakeClass());

	config.instance_ctr = [](Object & global, const System::AudioPlugin::Configuration::Class & cls, System::AudioPlugin & instance)
	{
		return Detail::Initialise(New<INSTANCE>(instance));
	};

	global->SetProperty(MakeKey32("bootstrap.paramdefs"), INSTANCE::CreateParamDefs());

	return global;
}

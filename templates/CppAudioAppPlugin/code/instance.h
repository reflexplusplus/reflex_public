#pragma once

#include "reflex_ext.h"




//
//declarations

namespace _PRODUCT-NAME-SYMBOL_
{
	
	class Instance;

	extern Reflex::Output output;
	
}




//
//_PRODUCT-NAME_ Instance

class _PRODUCT-NAME-SYMBOL_::Instance : public Reflex::Bootstrap::AudioPlugin
{
public:

	static constexpr Reflex::WString::View kFileExt = L"_PRODUCT-NAME-SYMBOL-LOWER_";



	//config

	static Reflex::System::AudioPlugin::Configuration::Class MakeClass();

	static Reflex::TRef <ParamDefs> CreateParamDefs();



	//ctr

	static Reflex::TRef <Instance> Create(Reflex::System::AudioPlugin & instance);



protected:

	using Reflex::Bootstrap::AudioPlugin::AudioPlugin;
};

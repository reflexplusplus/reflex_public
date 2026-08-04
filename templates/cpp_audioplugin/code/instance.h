#pragma once

#include "reflex_ext.h"




//
//declarations

namespace _PRODUCT-NAME-SYMBOL_
{
	
	class Instance;						// Plugin instance. Exposes the plugin interface and manages instance state, and dispatches state change notifications


	extern Reflex::Output output;		// Debug output node for this namespace

}




//
//_PRODUCT-NAME_ Instance

class _PRODUCT-NAME-SYMBOL_::Instance : public Reflex::Bootstrap::AudioPlugin
{
public:

	static constexpr Reflex::WString::View kFileExt = L"_PACKAGE-ID-PRODUCT_";



	//config

	static Class MakeClass();

	static void PopulateParameters(const Class & cls, Reflex::ArrayRegion < Reflex::Pair <Reflex::Key32, Reflex::ConstReference <Reflex::Bootstrap::ParamDesc> > > paramdefs);



	//ctr

	static Reflex::TRef <Instance> Create(Reflex::System::AudioPlugin & instance);



protected:

	using Reflex::Bootstrap::AudioPlugin::AudioPlugin;
};

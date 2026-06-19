#pragma once

#include "reflex_ext.h"




//
//declarations

namespace _PRODUCT-NAME-SYMBOL_
{
	
	class App;							// Application model. Exposes the main interface, manages core state, and dispatches state change notifications

	
	extern Reflex::Output output;		// Debug output node for this namespace
	
}




//
//_PRODUCT-NAME_ App

class _PRODUCT-NAME-SYMBOL_::App : public Reflex::Bootstrap::App
{
public:

	static constexpr Reflex::WString::View kFileExt = L"_PACKAGE-ID-PRODUCT_";



	//ctr for abstract class

	static Reflex::TRef <App> Create();



	//put your app interface here



protected:

	using Reflex::Bootstrap::App::App;

};

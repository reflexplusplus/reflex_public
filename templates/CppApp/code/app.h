#pragma once

#include "reflex_ext.h"




//
//declarations

namespace _PRODUCT-NAME-SYMBOL_
{
	
	class App;

	extern Reflex::Output output;
	
}




//
//_PRODUCT-NAME_ App

class _PRODUCT-NAME-SYMBOL_::App : public Reflex::Bootstrap::App
{
public:

	static constexpr Reflex::WString::View kFileExt = L"_PRODUCT-NAME-SYMBOL-LOWER_";



	//ctr for abstract class

	static Reflex::TRef <App> Create();



	//put your app interface here



protected:

	using Reflex::Bootstrap::App::App;

};

#pragma once

#include "reflex_ext.h"




//
//declarations

namespace CustomDrawing
{
	
	class App;

	extern Reflex::Output output;
	
}




//
//Custom Drawing App

class CustomDrawing::App : public Reflex::Bootstrap::App
{
public:

	static constexpr Reflex::WString::View kFileExt = L"customdrawing";



	//ctr for abstract class

	static Reflex::TRef <App> Create();



	//put your app interface here



protected:

	using Reflex::Bootstrap::App::App;

};

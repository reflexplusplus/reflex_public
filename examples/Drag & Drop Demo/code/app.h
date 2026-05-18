#pragma once

#include "reflex_ext.h"




//
//declarations

namespace DragDropDemo
{
	
	class App;

	extern Reflex::Output output;
	
}




//
//Drag & Drop Demo App

class DragDropDemo::App : public Reflex::Bootstrap::App
{
public:

	static constexpr Reflex::WString::View kFileExt = L"dragdropdemo";



	//ctr for abstract class

	static Reflex::TRef <App> Create();



	//put your app interface here



protected:

	using Reflex::Bootstrap::App::App;

};

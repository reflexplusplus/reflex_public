#pragma once

#include "[require].h"




//
//declarations

namespace Widgets
{

	class App;					//app instance, no functionality in this pure UI demo

	extern Output output;		//log output

}




//
//App

class Widgets::App : public Bootstrap::App
{
public:

	static Unretained <App> Create();



protected:
	
	using Bootstrap::App::App;
};

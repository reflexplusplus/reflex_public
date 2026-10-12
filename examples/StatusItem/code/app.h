#pragma once

#include "reflex_ext.h"




//
//declarations

namespace StatusItemExample
{

	using namespace Reflex;

	class App;

	extern Reflex::Output output;

}




//
//Status Item App

class StatusItemExample::App : public Reflex::Bootstrap::App
{
public:

	//ctr for abstract class

	static Reflex::Unretained <App> Create();



protected:

	using Reflex::Bootstrap::App::App;

};

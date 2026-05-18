#pragma once

#include "app.h"




//
//declarations

namespace SVGDemo
{
	
	class View;
	
}




//
//Asterix Test View

class SVGDemo::View : public Bootstrap::View
{
public:

	static TRef <View> Create(App & app);



protected:

	using Bootstrap::View::View;

};

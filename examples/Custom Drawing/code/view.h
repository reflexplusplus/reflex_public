#pragma once

#include "app.h"




//
//declarations

namespace CustomDrawing
{
	
	class View;
	
}




//
//Custom Drawing View

class CustomDrawing::View : public Reflex::Bootstrap::View
{
public:

	static Reflex::TRef <View> Create(App & app);



protected:

	using Reflex::Bootstrap::View::View;

};

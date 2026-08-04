#pragma once

#include "app.h"




//
//declarations

namespace DragAndDrop
{
	
	class View;
	
}




//
//Drag & Drop Demo View

class DragAndDrop::View : public Reflex::Bootstrap::View
{
public:

	static Reflex::TRef <View> Create(App & app);



protected:

	using Reflex::Bootstrap::View::View;

};

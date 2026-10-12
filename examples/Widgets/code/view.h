#pragma once

#include "app.h"




//
//declarations

namespace Widgets 
{

	class View;		//aop root view housing various widgets

}




//
//View

class Widgets::View : public Bootstrap::View
{
public:
	
	static Unretained <View> Create(App & app);



protected:

	using Bootstrap::View::View;

};

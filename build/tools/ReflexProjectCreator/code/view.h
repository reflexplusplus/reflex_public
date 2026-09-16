#pragma once

#include "app.h"




//
//declarations

namespace ReflexProjectCreator
{

	class View;

}




//
//View

class ReflexProjectCreator::View : public Reflex::Bootstrap::View
{

public:

	static Reflex::Unretained <View> Create(App & app);



protected:

	using Reflex::Bootstrap::View::View;

};

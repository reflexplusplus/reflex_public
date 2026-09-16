#pragma once

#include "app.h"




//
//View

namespace Notes
{

	class View : public Bootstrap::View
	{
	public:

		static Unretained <View> Create(App & app);



	protected:

		using Bootstrap::View::View;

	};

}

REFLEX_SET_TRAIT(Notes::View,IsAbstract);

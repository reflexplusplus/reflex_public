#pragma once

#include "app.h"




//
//View

namespace ResourceBuilder
{

	class View : public Bootstrap::View
	{
	public:

		static Unretained <View> Create(App & app);



	protected:

		using Bootstrap::View::View;

	};

}

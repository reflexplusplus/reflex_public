#pragma once

#include "app.h"




//
//View

namespace NotesCppApp
{

	class View : public Bootstrap::View
	{
	public:

		static TRef <View> Create(App & app);



	protected:

		using Bootstrap::View::View;

	};

}

REFLEX_SET_TRAIT(NotesCppApp::View,IsAbstract);
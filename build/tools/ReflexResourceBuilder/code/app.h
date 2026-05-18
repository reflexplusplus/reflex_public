#pragma once

#include "parser.h"




//
//App

namespace ResourceBuilder
{

	class App : public Bootstrap::App
	{
	public:

		REFLEX_OBJECT(App, Bootstrap::App);


		//reflex ctr for abstract class

		static TRef <App> Create();



		//compile .xml in bg thread

		virtual TRef <System::Thread> Compile(const WString & path, ObjectOf <Float> & progress) = 0;



	protected:

		using Bootstrap::App::App;

	};

	extern Output output;

}

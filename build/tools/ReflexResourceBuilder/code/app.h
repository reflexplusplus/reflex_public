#pragma once

#include "reflex_ext.h"




//
//declarations

namespace ResourceBuilder
{

	using namespace Reflex;


	class App;

	TRef <System::Task> Compile(const WString::View & path);

	extern Output output;

}




//
//App

class ResourceBuilder::App : public Bootstrap::App
{
public:

	REFLEX_OBJECT(App, Bootstrap::App);


	//reflex ctr for abstract class

	static TRef <App> Create();



	//compile .xml in bg thread

	virtual TRef <System::Task> Compile(const WString & path, ObjectOf <Float> & progress) = 0;



protected:

	using Bootstrap::App::App;

};

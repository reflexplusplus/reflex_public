#pragma once

#include "../window.h"




//
//Secondary API

namespace Reflex::System
{

	class App;

}




//
//App

class Reflex::System::App : public Item <System::App,false>
{
public:

	REFLEX_OBJECT(System::App, Object);



	//types

	struct Configuration
	{
		Function <TRef<Object>(Object & global, App & instance)> instance_ctr;

		Function <TRef<Window>(App & instance, void * host_window)> view_ctr;

		Array < Tuple<WString, WChar, Function<void()>> > app_menu;	//macos
	};

	

	//app defined entry callback

	static TRef <Object> OnStart(const ArrayView <CString::View> & cmdline, Configuration & config);	//return your app global

	static void Quit();



	//interface

	virtual TRef <Object> GetClient() = 0;


	virtual void OpenEditor() = 0;

	virtual void CloseEditor() = 0;



	//global

	static List list;

};

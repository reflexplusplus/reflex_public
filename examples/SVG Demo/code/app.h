#pragma once

#include "reflex_ext.h"




//
//declarations

namespace SVGDemo
{

	using namespace Reflex;


	class App;

	extern Output output;
	
}




//
//SVG Demo App

class SVGDemo::App : public Bootstrap::App
{
public:

	static constexpr WString::View kFileExt = L"asterixtest";



	//ctr for abstract class

	static TRef <App> Create();



	//put your app interface here

	virtual void SetPath(const WString & path) = 0;

	virtual const WString & GetPath() const = 0;



protected:

	using Bootstrap::App::App;

};

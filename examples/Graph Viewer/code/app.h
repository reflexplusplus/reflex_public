#pragma once

#include "reflex_ext.h"
#include "reflex/import.h"	//using namespace Reflex:: and macros




//
//declarations

namespace GraphViewer
{

	class App;

	extern Output output;

}




//
//GraphViewer::App

class GraphViewer::App : public Bootstrap::App
{
public:

	static constexpr WString::View kFileExt = L"GraphViewer";



	//ctr for abstract class

	static TRef <App> Create();



	//interface

	virtual void AddGraph() = 0;

	virtual void RemoveGraph(UInt idx) = 0;

	virtual UInt GetNumGraph() const = 0;

	virtual void SetCode(UInt idx, const Data::Archive::View & code) = 0;

	virtual Data::Archive::View GetCode(UInt idx) const = 0;


	virtual bool IsCompiled() const = 0;

	virtual bool Compute(UInt idx, ArrayView <Float> x_in, ArrayRegion <Float> y_out) = 0;	//ArrayRegion means values can be changed



protected:

	using Bootstrap::App::App;

};

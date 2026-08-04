#pragma once

#include "reflex_ext.h"




//
//declarations

namespace NotesCppApp
{

	using namespace Reflex;

	class App;

	extern Output output;

}




//
//App 

class NotesCppApp::App: public Reflex::Bootstrap::App
{
public:

	static constexpr WString::View kFileExt = L"notes";



	//ctr for abstract class

	static TRef <App> Create();



	//app interface

	virtual void AddNote() = 0;

	virtual void SetNote(UInt idx, const Data::Archive & utf8) = 0;

	virtual void DeleteNote(UInt idx) = 0;


	virtual UInt GetNumNotes() const = 0;

	virtual Data::Archive::View GetNote(UInt idx) const = 0;



protected:

	using Bootstrap::App::App;

};

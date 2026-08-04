#pragma once

#include "streamable.h"




//
//Primary API

namespace Reflex::Bootstrap
{

	class App;

}




//
//App

class Reflex::Bootstrap::App :
	public Data::PropertySet,
	public Streamable,
	public State
{
public:

	//access

	void Reset();

	bool Open(const WString & path);

	bool Save(const WString & path);

	WString::View GetFilename() const { return m_filename; }

	bool IsEdited() const { return m_edited; }



	//info

	const UInt32 magic;		//unique id for fileformat

	const TRef <File::PersistentPropertySet> session;



protected:

	App(UInt32 magic, UInt16 chunkversion);

	void Notify(bool edited);


	virtual bool OnImport(const WString::View & path, const Data::Archive::View & bytes) { return false; };


	void OnReleaseData() override;



private:

	bool Open(const WString::View & path, const Data::Archive::View & bytes);

	void AttachSessionListener();


	WString m_filename;

	bool m_edited;

	Reference <Object> m_session_listener;
	
};




//
//impl

inline void Reflex::Bootstrap::App::Reset()
{
	session->Reset(File::PersistentPropertySet::kContextPreset);
}

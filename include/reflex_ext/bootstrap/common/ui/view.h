#pragma once

#include "[require].h"
#include "../app.h"




//
//Primary API

namespace Reflex::Bootstrap
{

	class View; 

}




//
//View

class Reflex::Bootstrap::View : 
	public GLX::Object,
	public Streamable
{
public:

	REFLEX_OBJECT(Bootstrap::View, GLX::Object);

	View(App & app, UInt16 chunk_version, WString::View stylesheet_path) : View(app.session, MakeKey32("bootstrap.view"), chunk_version, stylesheet_path) { ConnectState(app); }

	View(File::PersistentPropertySet & state, Key32 chunk_id, UInt16 chunk_version, WString::View stylesheet_path);



protected:

	void ConnectState(State & state);	//by default this the state passed to ctor


	//Streamable callbacks

	virtual void OnResetState(Key32 context) = 0;

	virtual void OnRestoreState(Data::Archive::View & stream, Key32 context) = 0;

	virtual void OnStoreState(Data::Archive & stream) const = 0;


	//GLX::Object callbacks

	virtual void OnClock(Float delta) override;

	virtual void OnAttachWindow() override;

	virtual void OnDetachWindow() override;

	virtual bool OnEvent(GLX::Object & source, GLX::Event & e) override;



private:

	void OnReset(Key32 context) final;

	void OnRestore(Data::Archive::View & stream, Key32 context) final;

	void OnStore(Data::Archive & stream) const final;


	Reference <Reflex::Object> m_prefs_listener;

	WString m_stylesheet_path;

	State::Monitor m_monitor;
};




//
//impl

inline void Reflex::Bootstrap::View::ConnectState(State & state)
{
	m_monitor.Connect(state);
}

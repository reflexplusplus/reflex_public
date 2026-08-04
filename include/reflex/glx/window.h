#pragma once

#include "object.h"
#include "event.h"




//
//declarations

namespace Reflex::GLX
{

	class WindowClient;

}




//
//WindowClient

class Reflex::GLX::WindowClient : public Core::WindowClient
{
public:

	REFLEX_OBJECT(GLX::WindowClient, Core::WindowClient);

	static WindowClient & null;



	//events

	REFLEX_GLX_EVENT_ID(WindowResize);			//emitted on content Object
	REFLEX_GLX_EVENT_ID(RequestAutoFit);		//received, not emitted



	//lifetime

	WindowClient(bool profile);

	~WindowClient();



	//content

	void SetContent(TRef <GLX::Object> object);

	TRef <GLX::Object> GetContent();

	TRef <GLX::Object> GetForeground();			//used for overlaying objects, for example context menu



	//coordinates

	void SetPosition(const Point & position);

	void AutoFit();



	//base

	using Core::WindowClient::BeginDragDrop;



protected:

	WindowClient();	//for null case


	virtual void OnClose() override;

	virtual System::ScreenOrientation OnGetScreenOrientation() override;

	virtual System::iSize OnGetContentSize() override;

	virtual void OnSetRect(System::WindowDisplay state, const System::iRect & rect, const System::iRect & interactable, Int32 dpifactor) override;



private:

	struct Container;

	TRef <Container> m_container;

};

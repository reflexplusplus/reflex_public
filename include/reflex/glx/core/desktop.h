#pragma once

#include "window.h"




//
//Internal

REFLEX_NS(Reflex::GLX::Core)

REFLEX_DECLARE_KEY32(Engine);				//System::Renderer::Config option
REFLEX_DECLARE_KEY32(IncrementalMouse);		//System::Renderer::Config option

class Desktop;

extern const TRef <Core::Desktop> desktop;

REFLEX_END




//
//Core::Desktop

class Reflex::GLX::Core::Desktop :
	public Reflex::Object,
	public State
{
public:
	enum Notification : UInt8
	{
		kNotificationMouseOver,
		kNotificationFocus,
		kNotificationDragDropBegin,
		kNotificationDragDropEnd,
		kNotificationDragDropTarget,

		kNumNotification
	};



	//info
	
	virtual const System::Renderer::Config & GetGraphicsConfig() = 0;



	//notifications

	virtual TRef <Reflex::Object> CreateAnimationClock(const Function <void(Float32)> & listener) = 0;

	virtual TRef <Reflex::Object> CreateListener(Notification notification, const Function <void()> & listener) = 0;



	//windows

	virtual void EnumerateWindows(const Function <void(GLX::WindowClient&)> & callback) = 0;

	virtual const Pointer * QueryPointer(UInt8 slot) const = 0;

	virtual ArrayView <Pointer> GetPointers() const = 0;		//returns the first pressed pointer, or the mousepointer



	//mouseover

	virtual TRef <GLX::Object> GetMouseOver() = 0;



	//focus

	virtual void SetFocus(GLX::Object & object) = 0;

	virtual TRef <GLX::Object> GetFocus() = 0;



	//drag & drop

	virtual void StartDragDrop(UInt8 pointer_slot, TRef <Reflex::Object> data, System::MouseCursor dragover = System::kMouseCursorInvisible, System::MouseCursor block = System::kMouseCursorInvisible) = 0;

	virtual void CompleteDragDrop() = 0;

	virtual void CancelDragDrop() = 0;


	virtual TRef <Reflex::Object> GetDragDropData() = 0;

	virtual TRef <GLX::Object> GetDragDropTarget() = 0;



	//special

	virtual void EndPointerCapture(UInt8 slot) = 0;

	virtual void ResetClock() = 0;



	//root

	const TRef <File::ResourcePool> resourcepool;



protected:

	Desktop(File::ResourcePool & resourcepool);

};

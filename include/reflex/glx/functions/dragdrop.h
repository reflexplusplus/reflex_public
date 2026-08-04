#pragma once

#include "../object.h"




//
//Primary API

namespace Reflex::GLX
{

	void StartDragDrop(TRef <Reflex::Object> data, MouseCursor dragover = kMouseCursorInvisible, MouseCursor block = kMouseCursorInvisible);

	void CancelDragDrop();


	template <class TYPE> inline TYPE * QueryDragDropData(GLX::Event & e);


	TRef <Reflex::Object> CreateDragDropBeginListener(const Function <void(Reflex::Object&)> & callback);

	TRef <Reflex::Object> CreateDragDropEndListener(const Function <void()> & callback);

	TRef <Reflex::Object> CreateDragDropTargetListener(const Function <void(GLX::Object&)> & callback);

}




//
//impl

inline void Reflex::GLX::StartDragDrop(TRef <Reflex::Object> data, MouseCursor accept, MouseCursor deny)
{
	Core::desktop->StartDragDrop(data, accept, deny);
}

inline void Reflex::GLX::CancelDragDrop()
{
	Core::desktop->CancelDragDrop();
}

template <class TYPE> inline TYPE * Reflex::GLX::QueryDragDropData(GLX::Event & e)
{
	return DynamicCast<TYPE>(GetDragDropData(e));
}

inline Reflex::TRef <Reflex::Object> Reflex::GLX::CreateDragDropBeginListener(const Function <void(Reflex::Object&)> & callback)
{
	return Core::desktop->CreateListener(Core::Desktop::kNotificationDragDropBegin, [callback]()
	{
		callback(Core::desktop->GetDragDropData());
	});
}

inline Reflex::TRef <Reflex::Object> Reflex::GLX::CreateDragDropEndListener(const Function <void()> & callback)
{
	return Core::desktop->CreateListener(Core::Desktop::kNotificationDragDropEnd, callback);
}

inline Reflex::TRef <Reflex::Object> Reflex::GLX::CreateDragDropTargetListener(const Function <void(GLX::Object&)> & callback)
{
	return Core::desktop->CreateListener(Core::Desktop::kNotificationDragDropTarget, [callback]()
	{
		callback(Core::desktop->GetDragDropTarget());
	});
}

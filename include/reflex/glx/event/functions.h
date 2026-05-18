#pragma once

#include "../object.h"




//
//Primary API

namespace Reflex::GLX
{

	void SetEventDelegate(Object & object, Key32 delegate_id, const Function <bool(Object&, Event&)> & callback);	//receive all events


	void BindEvent(Object & object, Key32 event_id, const Function <bool(Object&, Event&)> & callback);				//receive only events with event_id

	void BindEventVoid(Object & object, Key32 event_id, const Function <void()> & callback);

	void UnbindEvent(Object & object, Key32 event_id);


	TRef <Object> BindClick(Object & object, const Function <void()> & fn);


	void BeginEventForwarding(Object & object, Key32 event_id, Object & to);


	//helpers for common events

	UInt8 GetClickFlags(const Event & e);		//see ClickFlags

	bool IsLeftClick(const Event & e);

	bool IsRightClick(const Event & e);

	bool IsDoubleClick(const Event & e);


	Point GetMouseDelta(const Event & e);		//for kMouseDrag & kMouseWheel


	UInt8 GetModifierKeys(const Event & e);		//see ModifierKeys

	KeyCode GetKeyCode(const Event & e);

	WChar GetKeyCharacter(const Event & e);


	void EmitTransaction(Object & object, TransactionStage stage, UInt index = 0, Float32 value = 0.0f);

	TransactionStage GetTransactionStage(const Event & e);


	UInt GetIndex(const Event & e);			//helper for Data::GetUInt32(e, kindex), emitted by lists etc

	TRef <Object> GetItem(Event & e);		//helper for e.QueryProperty<GLX::Object>(kitem), emitted by lists etc


	TRef <Reflex::Object> GetDragDropData(Event & e);


	const Event * QueryAntecedent(const Event & e, Key32 id, const Event * fallback = nullptr);


	//emit

	bool Emit(Object & src, Key32 id, Data::PropertySet && params = {});

	bool Send(Object & src, Key32 id, Data::PropertySet && params = {});


	template <class ... VARGS> bool Emit(Object & src, Key32 id, VARGS && ... vargs);

	template <class ... VARGS> bool Send(Object & src, Key32 id, VARGS && ... vargs);


	bool EmitCloseRequest(Object & child);		//emits kCloseRequest, parent Window or Overlay will respond


	bool EmitRequest(Object & src, Key32 id, bool allow_default = true);

	void PermitRequest(Event & e, bool allow);

}

REFLEX_NS(Reflex::GLX::Detail)

bool EmitRequest(Core::WeakReference & src, Event & e, bool allow_default = true);

REFLEX_END




//
//impl

inline Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::BindClick(Object & object, const Function <void()> & fn)
{
	BindEventVoid(object, kMouseDown, fn); 
	
	return object;
}

inline void Reflex::GLX::UnbindEvent(Object & object, Key32 id)
{
	object.ClearDelegate(id);
}

inline bool Reflex::GLX::IsLeftClick(const Event & e)
{
	return e.id == kMouseDown && !(GetClickFlags(e) & kClickFlagRmb);
}

inline bool Reflex::GLX::IsRightClick(const Event & e)
{
	return e.id == kMouseDown && (GetClickFlags(e) & kClickFlagRmb);
}

inline bool Reflex::GLX::IsDoubleClick(const Event & e)
{
	return e.id == kMouseDown && (GetClickFlags(e) & kClickFlagDbl);
}

REFLEX_INLINE Reflex::UInt8 Reflex::GLX::GetModifierKeys(const Event & e)
{
	REFLEX_ASSERT(e.QueryProperty<Data::UInt8Property>(kmodifiers));

	return Data::GetUInt8(e, kmodifiers);	//raw system modifier flags
}

REFLEX_INLINE Reflex::UInt8 Reflex::GLX::GetClickFlags(const Event & e)
{
	return Data::GetUInt8(e, kflags);
}

REFLEX_INLINE Reflex::GLX::KeyCode Reflex::GLX::GetKeyCode(const Event & e)
{
	return KeyCode(Data::GetUInt8(e, kkeycode, UInt8(kKeyCodeNull)));
}

REFLEX_INLINE Reflex::GLX::TransactionStage Reflex::GLX::GetTransactionStage(const Event & e)
{
	return TransactionStage(Data::GetUInt8(e, kstage, kTransactionStageNull));
}

REFLEX_INLINE Reflex::UInt Reflex::GLX::GetIndex(const Event & e)
{
	return Data::GetUInt32(e, kindex);
}

inline Reflex::TRef <Reflex::Object> Reflex::GLX::GetDragDropData(Event & e)
{
	return GetAbstractProperty(e, kdrag_data);
}

template <class ... VARGS> inline bool Reflex::GLX::Emit(Object & src, Key32 id, VARGS && ... vargs)
{
	return Emit(src, id, Data::Detail::MakePropertySet(std::forward<VARGS>(vargs)...));
}

template <class ... VARGS> inline bool Reflex::GLX::Send(Object & src, Key32 id, VARGS && ... vargs)
{
	return Send(src, id, Data::Detail::MakePropertySet(std::forward<VARGS>(vargs)...));
}

inline bool Reflex::GLX::EmitRequest(Object & src, Key32 id, bool allow_default)
{
	Core::WeakReference src_ref(src);

	return Detail::EmitRequest(src_ref, Make<Event>(id), allow_default);
}

REFLEX_INLINE void Reflex::GLX::PermitRequest(Event & e, bool allow)
{
	Data::SetBool(e, kallow, allow);
}

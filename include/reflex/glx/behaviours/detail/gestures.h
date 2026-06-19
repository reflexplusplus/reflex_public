#pragma once

#include "../../object.h"




//
//Detail

namespace Reflex::GLX::Detail
{

	class AbstractDraggable;

	class AbstractGestureRecognizer;

}




//
//AbstractDraggable 

class Reflex::GLX::Detail::AbstractDraggable : public GLX::Object::Delegate
{
public:

	REFLEX_OBJECT(AbstractDraggable, GLX::Object::Delegate);



protected:

	friend class AbstractGestureRecognizer;

	virtual Core::Trap OnDragTender(UInt8 slot, UInt8 flags) = 0;

};




//
//AbstractGestureRecognizer 

class Reflex::GLX::Detail::AbstractGestureRecognizer : public GLX::Detail::AbstractDraggable
{
public:

	REFLEX_OBJECT(AbstractGestureRecognizer, AbstractDraggable);

	AbstractGestureRecognizer(bool emulate_pointer_down = false);



protected:

	virtual bool OnTransactionTender(UInt8 pointer_flags, Float32 time_delta, Point position_delta) = 0;

	virtual bool OnTransactionStage(TransactionStage stage, Float32 time, Point position_delta) = 0;


	void OnDetachObject() override;



private:

	friend class AbstractDraggable;

	struct State;

	Core::Trap OnDragTender(UInt8 slot, UInt8 flags) final;

	bool OnEvent(GLX::Object & src, Event & e) override;

	bool OnPointerTender(Core::PointerAction action, const Core::Pointer & pointer, UInt8 flags, Core::Trap & trap) final;


	bool m_emulate_pointer_down;

	UInt8 m_capture_flags;

	Reference <Reflex::Object> m_state;
};

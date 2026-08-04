#pragma once

#include "reflex/glx.h"




//
//declarations

namespace Reflex::GLX
{

	struct AbstractDragBehaviour;

	struct ResizeBehaviour;

	struct MoveBehaviour;

}




//
//AbstractDragBehaviour

struct Reflex::GLX::AbstractDragBehaviour : public Object::Delegate
{
	void EnableBounds(bool enable);

	virtual void EnableAxis(bool x, bool y);



protected:

	AbstractDragBehaviour();


	UInt8 m_impl[4];
};




//
//ResizeBehaviour 

struct Reflex::GLX::ResizeBehaviour : public AbstractDragBehaviour
{
	REFLEX_OBJECT(GLX::ResizeBehaviour, AbstractDragBehaviour);

	static constexpr UInt32 kClassID = MakeKey32("ResizeBehaviour");


	static TRef <ResizeBehaviour> Create();

	virtual void SetHandleSize(Float size) = 0;

	virtual void SetRect(const Rect & rect) = 0;

	virtual Rect GetRect() const = 0;
};

REFLEX_SET_TRAIT(Reflex::GLX::ResizeBehaviour, IsAbstract);




//
//MoveBehaviour

struct Reflex::GLX::MoveBehaviour : public AbstractDragBehaviour
{
	REFLEX_OBJECT(GLX::MoveBehaviour, AbstractDragBehaviour);

	static constexpr UInt32 kClassID = ResizeBehaviour::kClassID + 1;


	static TRef <MoveBehaviour> Create();

	using AbstractDragBehaviour::AbstractDragBehaviour;
};

REFLEX_SET_TRAIT(Reflex::GLX::MoveBehaviour, IsAbstract);
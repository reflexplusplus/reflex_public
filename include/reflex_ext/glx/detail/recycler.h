#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX::Detail
{

	class Recycler;

}




//
//Detail::Recycler

class Reflex::GLX::Detail::Recycler
{
public:

	struct ItemPositioning
	{
		Positioning mode = kPositioningInline;
		Orientation axis = kOrientationNear;
		Orientation ortho = kOrientationFit;
	};



	//lifetime
	
	Recycler(Object & parent, const Style & style, UInt8 enter_flags, const Function <TRef<Object>()> & ctr = &CreateImpl);

	Recycler(Object & parent, const Style & style = Style::null, const Function <TRef<Object>()> & ctr = &CreateImpl) : Recycler(parent, style, kEnterAnimationNone, ctr) {}

	~Recycler();


	
	//access
	
	TRef <Object> Acquire(Key32 id, ItemPositioning positioning, const Style & style, const Function <TRef<Object>()> & ctr);
	
	TRef <Object> Acquire(Key32 id, ItemPositioning positioning, const Function <TRef<Object>()> & ctr);

	TRef <Object> Acquire(Key32 id, ItemPositioning positioning);

	TRef <Object> Acquire(Key32 id) { return Acquire(id, {}); }		//CLANG WORKAROUND

	void Keep(GLX::Object & object);


	[[deprecated("use Acquire(Key32 id)")]] TRef <Object> Create(Key32 id) { return Acquire(id); }



private:

	static TRef <Object> CreateImpl() { return New<Object>(); }

	TRef <Object> AcquireImpl(const Function <TRef<Object>()> & ctr, Key32 id, ItemPositioning positioning, const Style & style);


	TRef <Object> m_parent;

	ConstTRef <Style> m_style;

	UInt8 m_enter_flags;

	decltype (&Enter) m_enter;

	Function <TRef<Object>()> m_ctr;

	Array <Object*> m_kept;
};




//
//impl

inline Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::Detail::Recycler::Acquire(Key32 id, ItemPositioning positioning, const Style & style, const Function <TRef<Object>()> & ctr)
{
	return AcquireImpl(ctr, id, positioning, style);
}

inline Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::Detail::Recycler::Acquire(Key32 id, ItemPositioning positioning, const Function <TRef<Object>()> & ctr)
{
	return AcquireImpl(ctr, id, positioning, m_style);
}

inline Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::Detail::Recycler::Acquire(Key32 id, ItemPositioning positioning)
{
	return AcquireImpl(m_ctr, id, positioning, m_style);
}

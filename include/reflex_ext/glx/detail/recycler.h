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
	
	Recycler(Object & parent, const Style & style, UInt8 enter_flags, const Function <Unretained<Object>()> & ctr = &CreateImpl);

	Recycler(Object & parent, const Style & style = Style::null, const Function <Unretained<Object>()> & ctr = &CreateImpl) : Recycler(parent, style, kEnterAnimationNone, ctr) {}

	~Recycler();


	
	//access
	
	AlreadyRetained <Object> Acquire(Key32 id, ItemPositioning positioning, const Style & style, const Function <Unretained<Object>()> & ctr);
	
	AlreadyRetained <Object> Acquire(Key32 id, ItemPositioning positioning, const Function <Unretained<Object>()> & ctr);

	AlreadyRetained <Object> Acquire(Key32 id, ItemPositioning positioning);

	AlreadyRetained <Object> Acquire(Key32 id) { return Acquire(id, {}); }		//CLANG WORKAROUND

	void Keep(GLX::Object & object);



private:

	[[nodiscard]] static Unretained <Object> CreateImpl() { return New<Object>(); }

	AlreadyRetained <Object> AcquireImpl(const Function <Unretained<Object>()> & ctr, Key32 id, ItemPositioning positioning, const Style & style);


	AlreadyRetained <Object> m_parent;

	ConstAlreadyRetained <Style> m_style;

	UInt8 m_enter_flags;

	decltype (&Enter) m_enter;

	Function <Unretained<Object>()> m_ctr;

	Array <Object*> m_kept;
};




//
//impl

inline Reflex::AlreadyRetained <Reflex::GLX::Object> Reflex::GLX::Detail::Recycler::Acquire(Key32 id, ItemPositioning positioning, const Style & style, const Function <Unretained<Object>()> & ctr)
{
	return AcquireImpl(ctr, id, positioning, style);
}

inline Reflex::AlreadyRetained <Reflex::GLX::Object> Reflex::GLX::Detail::Recycler::Acquire(Key32 id, ItemPositioning positioning, const Function <Unretained<Object>()> & ctr)
{
	return AcquireImpl(ctr, id, positioning, m_style);
}

inline Reflex::AlreadyRetained <Reflex::GLX::Object> Reflex::GLX::Detail::Recycler::Acquire(Key32 id, ItemPositioning positioning)
{
	return AcquireImpl(m_ctr, id, positioning, m_style);
}

#pragma once

#include "forward.h"




//
//Internal

REFLEX_NS(Reflex::GLX::Core)

class Desktop;

class WindowClient;

class Object;

struct Context;

struct RenderContext;

struct Accessor;

using WeakReference = Reflex::Detail::LegacyWeakReference <GLX::Object>;


enum MouseAction : UInt8
{
	kMouseActionMove,
	kMouseActionClick,
	kMouseActionWheel,
	kMouseActionDragAndDrop,

	kNumMouseAction
};

enum Trap : UInt8
{
	kTrapThru,
	kTrapPassive,
	kTrapActive,

	kTrapCallbackSpecific,
	kTrapReject = kTrapCallbackSpecific,				//for OnMouseOver, OnDragOver
	kTrapActiveIncremental = kTrapCallbackSpecific,		//for OnMouseClick

	kNumTrap,
};

REFLEX_END




//
//overloads

REFLEX_NS(Reflex::System)

Colour operator+(const Colour & a, const Colour & b);

Colour operator-(const Colour & a, const Colour & b);

Colour operator*(const Colour & a, const Colour & b);

REFLEX_END




//
//impl

REFLEX_SET_TRAIT(GLX::Object, IsObject);	//FUCKKKKKKK

REFLEX_INLINE Reflex::System::Colour Reflex::System::operator+(const Colour & a, const Colour & b)
{
	return { a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a };
}

REFLEX_INLINE Reflex::System::Colour Reflex::System::operator-(const Colour & a, const Colour & b)
{
	return { a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a };
}

REFLEX_INLINE Reflex::System::Colour Reflex::System::operator*(const Colour & a, const Colour & b)
{
	return { a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a };
}

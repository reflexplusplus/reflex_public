#pragma once

#include "defines.h"




//
//Secondary API

REFLEX_NS(Reflex::GLX::Core)

struct Context;

struct RenderContext;

struct Pointer;

REFLEX_END




//
//Core::Context

struct Reflex::GLX::Core::Context : public Reflex::Detail::ContextScope
{
public:

	//info

	static bool IsActive();



	//lifetime

	Context();

	~Context();



private:

	static UInt st_count;

};




//
//Core::RenderContext

struct Reflex::GLX::Core::RenderContext
{
	System::Renderer::Canvas * canvas = nullptr;

	System::iRect viewport;

	SIMD::IntV4 clip_rect;

	System::Renderer::Transform transform;


	static RenderContext * st_current;
};




//
//impl

struct Reflex::GLX::Core::Pointer
{
	TRef <GLX::WindowClient> window;
	WeakReference target;
	UIntNative touch_id = ~UIntNative(0);
	UInt8 slot = 0;
	bool pressed = false;
	bool drag_and_drop = false;
	Trap capture_mode = kTrapThru;
	System::fPoint position;
	System::fPoint capture_origin;
	UInt64 drag_z = kMaxUInt64;
};




//
//impl

REFLEX_INLINE bool Reflex::GLX::Core::Context::IsActive()
{
	return True(st_count);
}

REFLEX_INLINE Reflex::GLX::Core::Context::Context()
	: Reflex::Detail::ContextScope(g_renderer->GetContextID())
{
	REFLEX_ASSERT_MAINTHREAD("GLX::Core::Context::Context");

	if (!st_count++)
	{
		auto & renderer = *g_renderer;

		Retain(renderer);

		renderer.BeginAccess();
	}
}

REFLEX_INLINE Reflex::GLX::Core::Context::~Context()
{
	REFLEX_ASSERT_MAINTHREAD("GLX::Core::Context::~Context");

	if (!--st_count)
	{
		auto & renderer = *g_renderer;

		renderer.EndAccess();

		Release(renderer);
	}
}

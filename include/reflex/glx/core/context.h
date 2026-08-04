#pragma once

#include "defines.h"




//
//Secondary API

REFLEX_NS(Reflex::GLX::Core)

struct Context;

struct RenderContext;

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

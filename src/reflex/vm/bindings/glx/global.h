#pragma once

#include "object.h"





//global

#define BIND_ENUM(ENUM) VM_QBIND_ENUM(GLX, ENUM)

REFLEX_NS(Reflex::GLXVM)

constexpr Key32 kGLX = K32("GLX");

void BindEvent(VM::Compiler::State & cstate, Key32 glx, VM::TypeRef dynamic_t, VM::TypeRef object_t, VM::TypeRef point_t);

struct Library : public Reflex::Object
{
	Library();

	~Library()
	{
		st_self = 0;
	}


	Reference <Allocator> m_allocator;

	Reference <Reflex::Object> m_ondragstart_end[2];

	GLXVM::Object::Callbacks m_callbacks;


	static inline Library * st_self = 0;

	static inline Reflex::Allocator * g_allocator = 0;
};

REFLEX_END

#pragma once

#include "[require].h"




//
//declarations

REFLEX_NS(Reflex::GLX::Detail)

struct Incrementer;

REFLEX_END




//
//incrementer

struct Reflex::GLX::Detail::Incrementer : public Reflex::Object
{
public:

	//lifetime

	[[nodiscard]] static TRef <Incrementer> Create();



	//setup

	virtual void SetRange(Float min, Float max, Float step, Float pixelrange = 128.0f, bool invert = false) = 0;



	//process

	virtual void Begin(Float32 initialvalue) = 0;

	virtual Float Process(Float drag, bool fine) = 0;

};

#pragma once

#include "[require].h"




//
//impl

REFLEX_NS(Reflex::IDE)

TRef <Object> AcquireConsole(TRef <GLX::Object> root, const Function <void()> & onclose);

REFLEX_END

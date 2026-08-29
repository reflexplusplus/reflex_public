#pragma once

#include "[require].h"




//
//impl

REFLEX_NS(Reflex::IDE)

[[nodiscard]] TRef <Object> AcquireConsole(TRef <GLX::Object> root, const Function <void()> & onclose);

REFLEX_END

#pragma once

#include "[require].h"




//
//impl

REFLEX_NS(Reflex::IDE)

[[nodiscard]] Unretained <Object> AcquireConsole(AlreadyRetained <GLX::Object> root, const Function <void()> & onclose);

REFLEX_END

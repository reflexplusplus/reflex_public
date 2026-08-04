#pragma once

#include "../context.h"




REFLEX_NS(Reflex::VM::Detail)

Reflex::Object * CrossContextCopy(Object & object, Context & to, bool mt = true, Object * fallback = nullptr);

REFLEX_END

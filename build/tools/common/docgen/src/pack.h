#pragma once

#include "../include/docgen/functions.h"
#include "../include/docgen/export.h"




REFLEX_NS(Docgen)

//Data property ids
REFLEX_DECLARE_KEY32(Constructors);
REFLEX_DECLARE_KEY32(Methods);
REFLEX_DECLARE_KEY32(Members);
REFLEX_DECLARE_KEY32(TemplateArgs);

constexpr Key32 kargs_targs[2] = { kArguments, kTemplateArgs };

REFLEX_END

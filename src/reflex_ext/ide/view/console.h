#pragma once

#include "../globalimpl.h"
#include "info_item.h"




//
//

REFLEX_NS(Reflex::IDE)

struct ComputedStyle : public Reflex::Object
{
	ComputedStyle(const GLX::Style & style);

	ConstTRef <GLX::Style> menu, menu_section, bar, subgroup, button, popup, list, list_item, texteditor, focus_rectangle, focus_parent_rectangle;
};

constexpr Key32 kOverflowPath = "path";

REFLEX_END

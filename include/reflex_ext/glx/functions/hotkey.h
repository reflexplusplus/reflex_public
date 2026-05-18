#pragma once

#include "reflex/glx.h"




//
//Addon API

#define GLX_KEY_CODE(keycode, modifier_keys) UInt16(keycode | ((modifier_keys) << 12))		//eg switch (GLX_KEY_CODE(GLX::GetKeyCode(e), GLX::GetModifierKeys(e)) { case GLX_KEY_CODE(GLX::kKeyCodeF5, GLX::kModifierKeyPrimary):

namespace Reflex::GLX
{

	void SetHotKey(WindowClient & window, KeyCode keycode, UInt8 modifiers, const Function <void()> & onkey);

	void RemoveHotKey(WindowClient & window, KeyCode keycode, UInt8 modifiers);

}

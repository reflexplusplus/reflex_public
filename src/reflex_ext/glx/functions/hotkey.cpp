#include "../../../../include/reflex_ext/glx/functions/hotkey.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

typedef ObjectOf < Map < UInt16, Function <void()> > > HotKeyMap;

REFLEX_END_INTERNAL

void Reflex::GLX::SetHotKey(WindowClient & window, KeyCode keycode, UInt8 modifiers, const Function <void()> & onkey)
{
	auto keycode_with_modifiers = GLX_KEY_CODE(keycode, modifiers);

	auto root = window.GetContent();

	auto keymap = AcquireProperty<HotKeyMap>(root, kNullKey);
	
	keymap->value.Set(keycode_with_modifiers, onkey);

	BindEvent(root, K32("SetHotKey"), [root, keymap, onkey](Object & src, Event & e)
	{
		if (auto ponkey = keymap->value.Search(GLX_KEY_CODE(GetKeyCode(e), GetModifierKeys(e))))
		{
			(*ponkey)();

			return true;
		}

		return false;
	});
}

void Reflex::GLX::RemoveHotKey(WindowClient & window, KeyCode keycode, UInt8 modifiers)
{
	auto keycode_with_modifiers = GLX_KEY_CODE(keycode, modifiers);

	auto root = window.GLX::Core::WindowClient::GetContent();

	auto & keymap = Data::Detail::AcquireProperty<HotKeyMap>(root, kNullKey)->value;

	keymap.Unset(keycode_with_modifiers);

	if (!keymap) UnbindEvent(root, K32("SetHotKey"));
}


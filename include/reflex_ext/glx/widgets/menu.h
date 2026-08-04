#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	class Menu;


	TRef <Object> AddMenuSection(Menu & menu, const WString::View & label);

	TRef <Object> AddMenuOption(Menu & menu, const WString::View & label, bool selected = false);


	void BindMenuSelect(Menu & menu, const Function <void(UInt)> & onselect);


	Reference <Menu> OpenContextMenu(Object & src, Key32 context = kNullKey, Key32 style = kmenu);

	void CloseContextMenu();


	TRef <Menu> GetMenu(Event & e);

	TRef <Menu> GetMenu(Event & e, Key32 context);

	Key32 GetMenuContext(const Event & e);

}




//
//Menu

class Reflex::GLX::Menu : public ScrollArea
{
public:

	REFLEX_OBJECT(GLX::Menu, ScrollArea);

	static Menu & null;



	//events

	REFLEX_GLX_EVENT_ID(MenuOpen);
	REFLEX_GLX_EVENT_ID(MenuSelect);	//item: Object;



	//lifetime

	[[nodiscard]] static TRef <Menu> Create();



	//content

	virtual void Clear() = 0;


	virtual TRef <Object> AddItem(const WString::View & label) = 0;

	virtual TRef <Object> AddSeparator() = 0;

	virtual TRef <Menu> AddSubMenu(const WString::View & label) = 0;


	virtual TRef <Object> AddItem(TRef <Object> item) = 0;

	virtual TRef <Object> AddSeparator(TRef <Object> item) = 0;

	virtual TRef <Menu> AddSubMenu(TRef <Object> item, TRef <Menu> menu = Menu::Create()) = 0;



	//advanced

	virtual bool OpenSubMenu(Object & item) = 0;

	virtual TRef <Object> GetParentItem() const = 0;



protected:

	using ScrollArea::ScrollArea;

};

REFLEX_SET_TRAIT(Reflex::GLX::Menu, IsSingleThreadExclusive);




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

inline TRef <Object> CreateMenuContent() { return Menu::Create(); };

bool PopupHasFocus(Object & menu, Object & focus);	//returns true if menu, or open sub menu contains focus

TRef <Menu> GetMenuProperty(Event & e);

bool EmitMenuOpenEvent(Object & src, Menu & menu, Key32 context);	//done automatically when menu attaches

void PlacePopup(Object & owner, Object & fg, TRef <Object> popup, const Rect & target_rect, Alignment alignment, Orientation justify);

Object * QueryPopup(Object & owner);

REFLEX_END

inline Reflex::Key32 Reflex::GLX::GetMenuContext(const Event & e)
{
	return Data::GetKey32(e, kcontext);
}

inline Reflex::TRef <Reflex::GLX::Menu> Reflex::GLX::GetMenu(Event & e)
{
	if (e.id == Menu::kMenuOpen)
	{
		return Detail::GetMenuProperty(e);
	}

	return {};
}

inline Reflex::TRef <Reflex::GLX::Menu> Reflex::GLX::GetMenu(Event & e, Key32 context)
{
	if (e.id == Menu::kMenuOpen && GetMenuContext(e) == context)
	{
		return Detail::GetMenuProperty(e);
	}

	return {};
}

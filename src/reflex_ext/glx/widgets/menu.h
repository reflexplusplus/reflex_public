#pragma once

#include "../../../../include/reflex_ext/glx/widgets/menu.h"





//
//menu

REFLEX_NS(Reflex::GLX)

struct MenuImpl : public Menu
{
	struct ComputedStyle;

	REFLEX_DECLARE_KEY32(separator);

	
	MenuImpl();


	void Clear() override;

	TRef <Object> AddItem(TRef <Object> item) override;

	TRef <Object> AddSeparator(TRef <Object> item) override;

	TRef <Menu> AddSubMenu(TRef <Object> item, TRef <Menu> menu) override;

	TRef <Object> AddItem(const WString::View & label) override;

	TRef <Object> AddSeparator() override;

	TRef <Menu> AddSubMenu(const WString::View & label) override;

	TRef <Object> GetParentItem() const override { return m_parent_item; }

	bool OpenSubMenu(Object & item) override;


	void OnSetStyle(const Style & style) override;

	bool OnEvent(Object & src, Event & e) override;

	void OnAttachWindow() override;

	void OnDetachWindow() override;


	ConstReference <ComputedStyle> m_cstyle;

	Key32 m_context;

	Object m_content;

	MenuImpl * m_root;

	Core::WeakReference m_parent_item;

};

REFLEX_END

struct Reflex::GLX::MenuImpl::ComputedStyle : public Reflex::Object
{
	ComputedStyle();

	ComputedStyle(const Style & style);

	const ConstTRef <Style> kFolder, kItem, kSeparator;
};

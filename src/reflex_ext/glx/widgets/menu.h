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

	AlreadyRetained <Object> AddItem(WillRetain <Object> item) override;

	AlreadyRetained <Object> AddSeparator(WillRetain <Object> item) override;

	AlreadyRetained <Menu> AddSubMenu(WillRetain <Object> item, WillRetain <Menu> menu) override;

	AlreadyRetained <Object> AddItem(const WString::View & label) override;

	AlreadyRetained <Object> AddSeparator() override;

	AlreadyRetained <Menu> AddSubMenu(const WString::View & label) override;

	AlreadyRetained <Object> GetParentItem() const override { return m_parent_item; }

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

	const ConstAlreadyRetained <Style> kFolder, kItem, kSeparator;
};

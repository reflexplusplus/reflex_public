#pragma once

#include "../../../include/reflex_ext/ide.h"




//
//decl

REFLEX_NS(Reflex::IDE)

struct Global :
	public Object,
	public State
{
};

struct NullResourceGroup : public ResourceGroup
{
	void Clear() override {}

	void AddItem(Address adr, ConstTRef <Object> object) override { AutoRelease(object); }

	void ForceRebuild(File::ResourcePool::Lock & lock) override {}
};

struct PlaceholderResourceGroup : public NullResourceGroup
{
	void Clear() override { m_items.Clear(); }

	void AddItem(Address adr, ConstTRef <Object> object) override { m_items.Push(object); }

	Array < ConstReference <Object> > m_items;
};

REFLEX_END

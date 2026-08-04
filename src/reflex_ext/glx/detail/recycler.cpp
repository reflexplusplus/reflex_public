#include "../../../../include/reflex_ext/glx/detail/recycler.h"




//
//Detail::Recycler

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

REFLEX_END_INTERNAL

Reflex::GLX::Detail::Recycler::Recycler(Object & parent, const Style & style, UInt8 enter_flags, const Function <TRef<Object>()> & ctr)
	: m_parent(parent),
	m_style(style),
	m_enter_flags(enter_flags),
	m_enter(m_enter_flags && GLX::AnimationScope::IsEnabled() ? &Enter : &SkipEnter),
	m_ctr(ctr)
{
}

Reflex::GLX::Detail::Recycler::~Recycler()
{
	auto exit = (m_enter_flags && GLX::AnimationScope::IsEnabled()) ? &GLX::Exit : &GLX::SkipExit;

	Array < Reference <Object> > remove;

	remove.Allocate(m_parent->GetNumItem());

	Idx new_order_z;

	bool needs_reorder = false;

	for (auto & i : m_parent)
	{
		if (Idx new_order = Search(m_kept, &i))
		{
			if (new_order_z && new_order_z.value > new_order.value)
			{
				needs_reorder = true;
			}

			new_order_z = new_order;
		}
		else
		{
			remove.Push<kAllocateNone>(i);
		}
	}

	if (needs_reorder) for (auto & i : m_kept) i->SendTop();

	REFLEX_RFOREACH(i, remove)
	{
		i->id = {};						//prevent reuse on next refresh

		exit(i, true, m_enter_flags);
	}
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::Detail::Recycler::AcquireImpl(const Function <TRef<Object>()> & ctr, Key32 id, ItemPositioning positioning, const Style & style)
{
	auto pobject = QueryChildById(m_parent, id, nullptr);

	if (!pobject)
	{
		pobject = ctr().Adr();

		pobject->id = id;

		pobject->SetStyle(style);

		AddItem(m_parent, *pobject, positioning.mode, positioning.axis, positioning.ortho);
		
		m_enter(*pobject, m_enter_flags);	//last, because size-in needs parent to know axis
	}

	m_kept.Push(pobject);

	return pobject;
}

void Reflex::GLX::Detail::Recycler::Keep(GLX::Object & object)
{
	if (!Search(m_kept, &object))
	{
		m_kept.Push(&object);
	}
}

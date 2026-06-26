#include "../../../../include/reflex_ext/glx/widgets/tree.h"
#include "../../../../include/reflex_ext/glx/functions/hotkey.h"




//
//helper

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct PointerCompare
{
	template <class TYPE> static bool eq(const Reference <TYPE> & itr, TYPE * value)
	{
		return itr.Adr() == value;
	}
};

struct FocusLock
{
	FocusLock(Object & object)
		: m_object(BranchContains(object, Core::desktop->GetFocus()) ? object : Reflex::Detail::GetNullInstance<Object>())
	{
	}

	~FocusLock()
	{
		if (m_object) m_object->Focus();
	}

	GLX::Core::WeakReference m_object;
};

REFLEX_NOINLINE void RemoveFromSelection(Array < Reference<Tree::Node> > & selection, Tree::Node * ptr)
{
	Remove<PointerCompare>(selection, ptr);
}

Tree::Node * FindNextNode(Tree::Node & node)
{
	if (auto tree = QueryParentByType<Tree>(node))
	{
		if (node.IsOpen() && node.body->GetNumItem())
		{
			return node.GetFirst();
		}
		else
		{
			if (auto next = node.GetNext())
			{
				return next;
			}
			else
			{
				auto parent = node.GetParent();

				while (parent != tree->root.Adr())
				{
					if (auto next = parent->GetNext()) return next;

					parent = parent->GetParent();
				}
			}
		}
	}

	return 0;
}

Tree::Node * FindPrevNode(Tree::Node & node)
{
	if (auto tree = QueryParentByType<Tree>(node))
	{
		if (auto prev = node.GetPrev())
		{
			return prev;
		}
		else
		{
			return node.GetParent();
		}
	}

	return 0;
}

REFLEX_END_INTERNAL

Reflex::GLX::Tree::Tree()
	: root(REFLEX_CREATE(Node, *this))
{
	Retain(root);

	root->Open(false);

	SetContent(root);
}

Reflex::GLX::Tree::~Tree()
{
	m_selection.Clear();

	SetContent(Null<Object>());

	root->Clear();

	Release(root);
}

void Reflex::GLX::Tree::Clear()
{
	FocusLock focuslock(*this);

	root->Clear();
}

void Reflex::GLX::Tree::SelectNone()
{
	REFLEX_RLOOP(idx, m_selection.GetSize())
	{
		m_selection[idx]->Deselect();
	}

	m_selection.Clear();
}

//void Reflex::GLX::Tree::SelectFirst()
//{
//	SelectNone();
//
//	if (root->body->GetNumItem())
//	{
//		Node * next = root->GetFirst();
//
//		while (And(next, !next->Select()))
//		{
//			next = FindNextNode(*next);
//		}
//	}
//}
//
//void Reflex::GLX::Tree::SelectLast()
//{
//	SelectNone();
//
//	if (root->body->GetNumItem())
//	{
//		auto node = root.Adr();
//
//		while (node->body)
//		{
//			if (auto last = node->GetLast())
//			{
//				node = last;
//			}
//			else
//			{
//				break;
//			}
//		}
//
//		node->Select();
//	}
//}

void Reflex::GLX::Tree::SelectPrev()
{
	if (m_selection)
	{
		if (auto next = FindPrevNode(*m_selection.GetLast()))
		{
			SelectNone();

			while (!next->Select())
			{
				next = FindPrevNode(*next);

				if (!next)
				{
					//SelectLast();

					break;
				}
			}
		}
		else
		{
			//SelectLast();
		}
	}
	else
	{
		//SelectLast();
	}

	if (m_selection) m_selection.GetLast()->Reveal();
}

void Reflex::GLX::Tree::SelectNext()
{
	if (m_selection)
	{
		if (auto next = FindNextNode(*m_selection.GetLast()))
		{
			SelectNone();

			while (!next->Select())
			{
				next = FindNextNode(*next);

				if (!next)
				{
					//SelectFirst();

					break;
				}
			}
		}
		//else
		//{
		//	SelectFirst();
		//}
	}
	//else
	//{
	//	SelectFirst();
	//}

	if (m_selection) m_selection.GetLast()->Reveal();
}

void Reflex::GLX::Tree::OnSetStyle(const Style & style)
{
	ScrollArea::OnSetStyle(style);

	m_node_style = style[kitem].Adr();

	root->SetStyle(m_node_style);
}

bool Reflex::GLX::Tree::OnEvent(Object & src, Event & e)
{
	if (e.id == kKeyDown)
	{
		switch (GLX_KEY_CODE(GetKeyCode(e), GetModifierKeys(e)))
		{
		case GLX_KEY_CODE(kKeyCodeUp, GLX::kModifierKeyAlt):
			if (m_selection)
			{
				TRef focus = m_selection.GetLast();

				if (auto prev = focus->GetPrev())
				{
					GLX::Emit(focus, kNodeRequestInsert, K32("source"), *prev, K32("after"), false);
				}
			}
			return true;

		case GLX_KEY_CODE(kKeyCodeUp, GLX::kModifierKeyNone):
			SelectPrev();
			return true;

		case GLX_KEY_CODE(kKeyCodeDown, GLX::kModifierKeyAlt):
			if (m_selection)
			{
				TRef focus = m_selection.GetLast();

				if (auto next = focus->GetNext())
				{
					GLX::Emit(focus, kNodeRequestInsert, K32("source"), *next, K32("after"), true);
				}
			}

			return true;

		case GLX_KEY_CODE(kKeyCodeDown, GLX::kModifierKeyNone):
			SelectNext();
			return true;

		case GLX_KEY_CODE(kKeyCodeLeft, GLX::kModifierKeyNone):
			if (m_selection)
			{
				TRef node = m_selection.GetLast();

				if (node != root)
				{
					if (node->IsOpen())
					{
						node->Close();
					}
					else
					{
						auto parent = node->GetParent();

						SelectNone();

						if (parent != root.Adr()) parent->Select();
					}
				}
			}
			return true;

		case GLX_KEY_CODE(kKeyCodeRight, GLX::kModifierKeyNone):
			if (m_selection) m_selection.GetLast()->Open();
			return true;

		case GLX_KEY_CODE(kKeyCodeEnter, GLX::kModifierKeyNone):
			if (m_selection) m_selection.GetLast()->Toggle();
			return true;

		case GLX_KEY_CODE(kKeyCodeDelete, GLX::kModifierKeyNone):
		case GLX_KEY_CODE(kKeyCodeBackspace, GLX::kModifierKeyNone):
			REFLEX_RLOOP(idx, m_selection.GetSize())
			{
				TRef item = m_selection[idx];

				if (EmitRequest(item, kNodeRemove, false))
				{
					item->Detach();
				}
			}
			return true;
		};
	}

	return ScrollArea::OnEvent(src, e);
}

Reflex::GLX::Tree::Node::Node(Tree & tree)
	: Accordion(false),
	tree(tree),
	m_parent(0)
{
	SetFlow(body, GLX::kFlowY);
}

Reflex::GLX::Tree::Node::Node(Tree::Node & parent)
	: Accordion(false),
	tree(parent.tree),
	m_parent(&parent)
{
	SetFlow(body, GLX::kFlowY);

	AddInline(parent.body, this);

	SetStyle(tree.m_node_style);
}

Reflex::GLX::Tree::Node::~Node()
{
	RemoveFromSelection(tree.m_selection, this);
}

void Reflex::GLX::Tree::Node::Open(bool animate, bool reveal)
{
	Select();

	Accordion::Open(animate, reveal);
}

void Reflex::GLX::Tree::Node::Clear()
{
	body->Clear();
}

Reflex::TRef <Reflex::GLX::Tree::Node> Reflex::GLX::Tree::Node::AddNode()
{
	return REFLEX_CREATE(Node, *this);
}

Reflex::GLX::Tree::Node * Reflex::GLX::Tree::Node::GetParent()
{
	return m_parent;
}

Reflex::GLX::Tree::Node * Reflex::GLX::Tree::Node::GetPrev()
{
	return Cast<Node>(Core::Object::GetPrev());
}

Reflex::GLX::Tree::Node * Reflex::GLX::Tree::Node::GetNext()
{
	return Cast<Node>(Core::Object::GetNext());
}

Reflex::GLX::Tree::Node * Reflex::GLX::Tree::Node::GetFirst()
{
	return Cast<Node>(body->GetFirst());
}

Reflex::GLX::Tree::Node * Reflex::GLX::Tree::Node::GetLast()
{
	return Cast<Node>(body->GetLast());
}

bool Reflex::GLX::Tree::Node::Selected() const
{
	return GLX::IsSelected(header);
}

bool Reflex::GLX::Tree::Node::Select(bool multi)
{
	//TODO unsafe, tree can die
	
	if (GLX::IsSelected(header)) return true;

	if (EmitRequest(*this, kNodeSelect))
	{
		RemoveFromSelection(tree.m_selection, this);

		if (!multi) tree.SelectNone();

		GLX::Select(header);

		tree.m_selection.Push(this);

		return true;
	}

	return false;
}

void Reflex::GLX::Tree::Node::Deselect()
{
	//TODO unsafe, tree can die

	if (GLX::IsSelected(header))
	{
		if (EmitRequest(*this, kNodeDeselect))
		{
			GLX::Select(header, false);

			RemoveFromSelection(tree.m_selection, this);
		}
	}
}

void Reflex::GLX::Tree::Node::Reveal(bool animate)
{
	Point pos = CalculateAbs(*this).a - CalculateAbs(tree.GetContent()).a;

	Float h = Detail::ComputeContentSize(*this).h;

	Float pad = header->contentsize.h;

	tree.ScrollArea::Reveal(true, pos.y, h, pad, animate);
}

void Reflex::GLX::Tree::Node::OnSetStyle(const Style & style)
{
	GLX::Accordion::OnSetStyle(style);

	for (auto & i : body)
	{
		i.SetStyle(style);
	}
}

bool Reflex::GLX::Tree::Node::OnEvent(Object & src, Event & e)
{
	if (&src == this)
	{
		if (e.id == kAccordionOpen)
		{
			if (Selected())
			{
				PermitRequest(e, EmitRequest(*this, kNodeOpen));
			}
			else
			{
				PermitRequest(e, false);

				Select();
			}

			return true;
		}
		else if (e.id == kAccordionClose)
		{
			if (Selected())
			{
				PermitRequest(e, EmitRequest(*this, kNodeClose));
			}
			else
			{
				PermitRequest(e, false);

				Select();
			}

			return true;
		}
	}

	return Accordion::OnEvent(src, e);
}

#include "../../../../include/reflex_ext/glx/functions/overlay.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

typedef ObjectOf < Function <TRef<Animation>()> > FocusHighlightCtr;

struct Overlay : public Object
{
	bool OnEvent(Object & src, Event & e) override
	{
		if (e.id == kMouseDown)
		{
			if (block_input)
			{
				return true;
			}
			else
			{
				Detail::DiscardOverlay(*this);

				return false;
			}
		}
		else if (e.id == kDragDropTender)
		{
			return block_dragdrop;
		}
		else if (e.id == kKeyDown)
		{
			auto keycode = GetKeyCode(e);

			if (keycode == kKeyCodeEscape && !block_input)
			{
				Detail::DiscardOverlay(*this);

				return true;
			}

			return block_input;
		}
		else if (e.id == kRequestClose)
		{
			Core::Context evnt;

			Object::id = kMaxUInt32;

			EnableMouse(*this, false, false);

			Exit(*this, true);

			return true;
		}

		return false;
	}

	bool block_input;

	bool block_dragdrop;
};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::Acquire(Object & parent, Key32 id, UInt8 enterflags, const Function<TRef<Object>()> & ctr)
{
	if (auto pobject = QueryChildById(parent, id))
	{
		return *pobject;
	}

	auto object = ctr();

	object->id = id;

	object->SetParent(parent);	//BEFORE ENTER

	Enter(object, enterflags);

	return object;
}

void Reflex::GLX::Discard(Object & parent, Key32 id)
{
	if (auto pobject = QueryChildById(parent, id))
	{
		pobject->id = {};

		Exit(*pobject, true);
	}
}

bool Reflex::GLX::Detail::DiscardOverlay(Object & overlay)
{
	Event e(kRequestClose);

	return overlay.ProcessEvent(overlay, e);
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::AcquireOverlay(Object & parent, Key32 id, bool block_input, bool block_dragdrop, Key32 style_id, const Function<void(Object&)> & oninit)
{
	auto overlay = Cast<Overlay>(Acquire(parent, id, kEnterAnimationFade, [&parent, style_id, oninit]()
	{
		TRef <Object> overlay = REFLEX_CREATE(Overlay);

		Reflex::Detail::SilentReference <Object> retain(overlay);

		EnableFloat(overlay, kOrientationFit, kOrientationFit);

		overlay->SetMod(kMaxUInt32 - 1, Detail::ComputedStyle::Create(1.0f, 1.0f, Detail::ComputedStyle::kRenderFalse));

		overlay->SetProperty(kDragDropTender, Null<Animation>());	//NAMESPACE HACK for Freestyle::DragManager

		Data::SetBool(overlay, kDragDropTender, false);					//NAMESPACE HACK for Freestyle::DragManager

		oninit(overlay);

		overlay->SetStyle(FindStyle(parent, style_id));

		return overlay;
	}));

	overlay->SendTop();

	overlay->block_input = block_input;

	overlay->block_dragdrop = block_dragdrop;

	return overlay;
}

bool Reflex::GLX::DiscardOverlay(Object & parent, Key32 id)
{
	if (auto overlay = QueryChildById(parent, id))
	{
		return Detail::DiscardOverlay(*overlay);
	}

	return false;
}

void Reflex::GLX::EnableFocusCycle(Object & object)
{
	REFLEX_DECLARE_KEY32(CycleFocus);

	REFLEX_LOCAL(void, CollectFocusable)(Object & container, Array <Object*> &list)
	{
		for (auto & i : container)
		{
			if (Data::GetBool(i, kWantsFocus))
			{
				list.Push(&i);
			}
			else
			{
				Call(i, list);
			}
		}
	}
	REFLEX_END

	SetEventDelegate(object, kCycleFocus, [&object](Object & src, Event & e)
	{
		if (e.id == kKeyDown)
		{
			auto keycode = GetKeyCode(e);

			if (keycode == kKeyCodeTab)
			{
				Array <Object*> list;

				CollectFocusable::Call(object, list);

				auto focus = Core::desktop->GetFocus();

				REFLEX_LOOP(idx, list.GetSize())
				{
					if (BranchContains(*list[idx], *focus))
					{
						auto inc = (GetModifierKeys(e) & kModifierKeyShift) ? -1 : 1;

						Object * next = list[Modulo(Int(idx) + inc, Int(list.GetSize()))];

						next->Focus();

						return true;
					}
				}

				if (list) list.GetFirst()->Focus();

				return true;
			}
		}

		return false;
	});
}

void Reflex::GLX::EnableFocusHighlight(Object & root)
{
	auto focus_ref = AcquireProperty<Detail::LegacyWeakReferenceObject>(root, kWantsFocus);

	SetEventDelegate(root, kWantsFocus, [focus_ref](Object & src, Event & e) mutable
	{
		if (e.id == kFocus)
		{
			Core::WeakReference & focus = focus_ref->value;

			for (auto & parent : Object::ParentRange(src))
			{
				if (Data::GetBool(parent, kWantsFocus))
				{
					if (focus.Adr() != &parent)
					{
						focus = parent;

						if (QueryAntecedent(e, kKeyDown))
						{
							if (auto ctr = Detail::SearchInheritedProperty<FocusHighlightCtr>(parent, kWantsFocus))
							{
								Run(parent, K32("highlight"), 1.0f, ctr->value());
							}
						}
					}

					return true;
				}
			}

			focus.Clear();
		}

		return false;
	});
}

void Reflex::GLX::DisableFocusHighlight(Object & root)
{
	root.ClearDelegate(kWantsFocus);

	root.UnsetProperty<Detail::LegacyWeakReferenceObject>(kWantsFocus);
}

void Reflex::GLX::SetFocusHighlight(Object & scope, const Function <TRef<Animation>()> & ctr)
{
	scope.SetProperty(kWantsFocus, REFLEX_CREATE(FocusHighlightCtr, ctr));
}

void Reflex::GLX::ClearFocusHighlight(Object & scope)
{
	scope.UnsetProperty<FocusHighlightCtr>(kWantsFocus);
}

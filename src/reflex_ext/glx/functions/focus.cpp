#include "../../../../include/reflex_ext/glx/functions/focus.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

typedef ObjectOf < Function <TRef<Animation>()> > FocusHighlightCtr;

void CollectFocusable(Object & container, Array <Object*> & list)
{
	for (auto & i : container)
	{
		if (Data::GetBool(i, kfocusable))
		{
			list.Push(&i);
		}
		else
		{
			CollectFocusable(i, list);
		}
	}
}

REFLEX_END_INTERNAL

void Reflex::GLX::FocusBranch(Object & object)
{
	if (!Reflex::BranchContains<GLX::Object>(object, GetFocus()))
	{
		object.Focus();
	}
}

void Reflex::GLX::RedirectFocus(Object & area, Object & object)
{
	auto itr = GetFocus();

	while (itr)
	{
		if (itr == area)
		{
			//area must contain focus, so we can focus object

			object.Focus();

			return;
		}
		else if (itr == object)
		{
			//object already contains focus, so dont need to do anything

			return;
		}

		itr = itr->GetParent();
	}
}

void Reflex::GLX::EnableTabNavigation(Object & object)
{
	REFLEX_DECLARE_KEY32(CycleFocus);

	SetEventDelegate(object, kCycleFocus, [&object](Object & src, Event & e)
	{
		if (e.id == kKeyDown)
		{
			auto keycode = GetKeyCode(e);

			if (keycode == kKeyCodeTab)
			{
				bool shift = True(GetModifierKeys(e) & kModifierKeyShift);

				Array <Object*> list;

				CollectFocusable(object, list);

				auto focus = GetFocus();

				REFLEX_LOOP(idx, list.GetSize())
				{
					if (BranchContains(*list[idx], *focus))
					{
						auto inc = shift ? -1 : 1;

						auto next = list[Modulo(Int(idx) + inc, Int(list.GetSize()))];

						next->Focus();

						return true;
					}
				}

				if (list)
				{
					(shift ? list.GetLast() : list.GetFirst())->Focus();
				}

				return true;
			}
		}

		return false;
	});
}

void Reflex::GLX::EnableFocusHighlight(Object & root)
{
	auto focus_ref = AcquireProperty<Detail::LegacyWeakReferenceObject>(root, kfocusable);

	SetEventDelegate(root, kfocusable, [focus_ref](Object & src, Event & e) mutable
	{
		if (e.id == kFocus)
		{
			Core::WeakReference & focus = focus_ref->value;

			for (auto & parent : Object::ParentRange(src))
			{
				if (Data::GetBool(parent, kfocusable))
				{
					if (focus.Adr() != &parent)
					{
						focus = parent;

						if (QueryAntecedent(e, kKeyDown))
						{
							if (auto ctr = Detail::SearchInheritedProperty<FocusHighlightCtr>(parent, kfocusable))
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
	root.ClearDelegate(kfocusable);

	root.UnsetProperty<Detail::LegacyWeakReferenceObject>(kfocusable);
}

void Reflex::GLX::SetFocusHighlight(Object & scope, const Function <TRef<Animation>()> & ctr)
{
	scope.SetProperty(kfocusable, REFLEX_CREATE(FocusHighlightCtr, ctr));
}

void Reflex::GLX::ClearFocusHighlight(Object & scope)
{
	scope.UnsetProperty<FocusHighlightCtr>(kfocusable);
}

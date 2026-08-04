#include "../../../../include/reflex_ext/glx/widgets/abstractlist.h"




//
//processor

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

REFLEX_NOINLINE Reference <Event> MakeIndexedEvent(Key32 id, UInt idx)
{
	auto e = Make<Event>(id);

	Data::SetUInt32(e, kindex, idx);

	return e;
}

inline Reference <Event> MakeIndexedEvent(Key32 id, UInt idx, bool status)
{
	auto e = MakeIndexedEvent(id, idx);

	Data::SetBool(e, kstate, status);

	return e;
}

REFLEX_NOINLINE bool ExtendSelection(AbstractList & self, Array <UInt> & selection, UInt idx)
{
	if (selection)
	{
		bool selected = false;

		UInt min = Reflex::Min(selection.GetFirst(), idx);

		UInt max = Reflex::Max(selection.GetLast(), idx);

		REFLEX_LOOP(x, (max - min) + 1) selected = Or(selected, self.Select(min + x));

		return selected;
	}
	else
	{
		return self.Select(idx);
	}
}

Idx g_abstract_list_idx;

bool g_abstract_list_dragging = false;

UInt8 g_abstract_list_modifierflags = 0;

constexpr KeyCode kPrevKey[2] = { kKeyCodeLeft, kKeyCodeUp };

constexpr KeyCode kNextKey[2] = { kKeyCodeRight, kKeyCodeDown };

REFLEX_END_INTERNAL

Reflex::GLX::AbstractList::AbstractList(Detail::LayoutModelCtr layout)
	: GLX::Object(layout)
	, m_selection_mode(kSelectionModeSingle)
	, m_multiselect_mask(0)
	, m_multiselect_flags(BitSet<UInt8>(0, 7))
{
	SetFlow(*this, kFlowY);
}

void Reflex::GLX::AbstractList::SetSelectionMode(SelectionMode mode)
{
	//bit 6 = manual multi select (select all)
	//bit 7 = always select all (kModeMultiToggle)

	m_selection_mode = mode;

	switch (mode)
	{
	case kSelectionModeSingle:
		m_multiselect_mask = 0;
		break;

	case kSelectionModeMulti:
		m_multiselect_mask = kModifierKeyShift | kModifierKeyPrimary | BitSet<UInt8>(0, 6);
		break;

	case kSelectionModeMultiToggle:
		m_multiselect_mask = BitSet<UInt8>(0, 7);
		break;
	}
}

void Reflex::GLX::AbstractList::SelectAll()
{
	if (m_selection_mode)
	{
		//m_multiselectflags = BitSet(m_multiselectflags, 6);

		SelectNone();

		REFLEX_LOOP(idx, OnGetSize()) Select(idx, true);

		//m_multiselectflags = BitClear(m_multiselectflags, 6);
	}
}

void Reflex::GLX::AbstractList::SelectNone()
{
	Core::WeakReference weakref(*this);

	auto selection = GetListSelection(*this);

	auto e = MakeIndexedEvent(kListSelect, 0, false);

	for (auto & idx : ReverseIterate(selection))
	{
		Data::SetUInt32(e, kindex, idx);

		if (Detail::EmitRequest(weakref, e))
		{
			OnDeselect(idx);
		}
	}
}

bool Reflex::GLX::AbstractList::Select(UInt idx, bool multi)
{
	Core::WeakReference weakref(*this);

	multi = multi || True((m_multiselect_flags | System::GetModifierKeys()) & m_multiselect_mask);

	if (!multi) SelectNone();

	auto e = MakeIndexedEvent(kListSelect, idx, true);

	Data::SetBool(e, K32("multi"), multi);

	if (Detail::EmitRequest(weakref, e))
	{
		OnSelect(idx);

		return true;
	}

	return false;
}

void Reflex::GLX::AbstractList::Deselect(UInt idx)
{
	Core::WeakReference weakref(*this);

	if (IsListSelected(*this, idx))
	{
		if (Detail::EmitRequest(weakref, MakeIndexedEvent(kListSelect, idx, false)))
		{
			OnDeselect(idx);
		}
	}
}

bool Reflex::GLX::AbstractList::SelectNext(bool extend)
{
	UInt size = OnGetSize();

	if (auto selection = GetListSelection(*this))
	{
		UInt last = Modulo(selection.GetLast() + 1, size);

		OnReveal(last);

		if (extend)
		{
			return ExtendSelection(*this, selection, last);
		}
		else
		{
			SelectNone();

			return Select(last);
		}
	}
	else if (size)
	{
		OnReveal(0);

		return Select(0);
	}

	return false;
}

bool Reflex::GLX::AbstractList::SelectPrev(bool extend)
{
	UInt size = OnGetSize();

	if (auto selection = GetListSelection(*this))
	{
		UInt first = Modulo(Int(selection.GetFirst() - 1), Int(size));

		OnReveal(first);

		if (extend)
		{
			return ExtendSelection(*this, selection, first);
		}
		else
		{
			SelectNone();

			return Select(first);
		}
	}
	else if (size)
	{
		OnReveal(size - 1);

		return Select(size - 1);
	}

	return false;
}

bool Reflex::GLX::AbstractList::OnEvent(GLX::Object & src, Event & e)
{
	constexpr auto ProcessClick = [](AbstractList & self, SelectionMode MODE, UInt idx, UInt8 click_flags, UInt8 key_flags)
	{
		g_abstract_list_idx = {};

		g_abstract_list_dragging = false;

		bool left = Not(click_flags & kClickFlagRmb);

		if (MODE)
		{
			if (key_flags & kModifierKeyShift)
			{
				if (!left) return false;

				auto selection = GetListSelection(self);

				ExtendSelection(self, selection, idx);

				return true;
			}
			else if (key_flags & kModifierKeyPrimary)
			{
				if (!left) return false;

				if (IsListSelected(self, idx))
				{
					self.Deselect(idx);
				}
				else
				{
					self.Select(idx);
				}

				return true;
			}
		}

		if (IsListSelected(self, idx))
		{
			if (!left) return false;

			if (MODE == AbstractList::kSelectionModeMultiToggle)
			{
				self.Deselect(idx);

				return true;
			}
			else
			{
				if (click_flags & kClickFlagDbl)
				{
					return self.Emit(MakeIndexedEvent(AbstractList::kListLoad, idx));
				}
				else
				{
					g_abstract_list_idx = idx;

					g_abstract_list_dragging = true;
				}

				return true;
			}
		}
		else if (self.Select(idx))
		{
			g_abstract_list_idx = idx;

			return true;
		}

		return false;
	};

	if (e.id == kMouseDown)
	{
		if (&src != this)
		{
			auto click_flags = GetClickFlags(e);

			if (Not(click_flags & kPointerFlagMulti))
			{
				if (auto branchidx = LookupBranchIndex(*this, src))
				{
					if (auto idx = OnGetIndex(LookupChildAtIndex(*this, branchidx.value)))
					{
						ProcessClick(*this, m_selection_mode, idx.value, click_flags, GetModifierKeys(e));

						if (!(click_flags & kClickFlagRmb)) return true;
					}
				}
			}
		}
	}
	else if (e.id == kMouseDrag)
	{
		if (g_abstract_list_idx && ExceedsDragThreshold(GetDelta(e), 4.0f))
		{
			UInt idx = g_abstract_list_idx.value;

			g_abstract_list_idx = {};

			return Emit(MakeIndexedEvent(kListStartDrag, idx));
		}

		return true;
	}
	else if (e.id == kMouseUp)
	{
		if (g_abstract_list_idx && g_abstract_list_dragging)
		{
			auto idx = Poll(g_abstract_list_idx).value;

			if (CountListSelection(*this) > 1)
			{
				SelectNone();

				Select(idx);
			}
			else
			{
				return Emit(MakeIndexedEvent(kListLoad, idx));
			}
		}

		return true;
	}
	else if (e.id == kKeyDown)
	{
		auto keycode = GetKeyCode(e);

		auto modifiers = GetModifierKeys(e);

		bool y = GetAxis(*this);

		if (keycode == kPrevKey[y])
		{
			if (SelectPrev(modifiers & kModifierKeyShift)) return true;
		}
		else if (keycode == kNextKey[y])
		{
			if (SelectNext(modifiers & kModifierKeyShift)) return true;
		}
		else if (keycode == kKeyCodeEnter)
		{
			if (auto idx = GetListFocus(*this))
			{
				if (Emit(MakeIndexedEvent(kListLoad, idx.value))) return true;
			}
		}
		else if (keycode == kKeyCodeA)
		{
			if (modifiers & kModifierKeyPrimary)
			{
				SelectAll();

				return true;
			}
		}
		else if (keycode == kKeyCodeD)
		{
			if (modifiers & kModifierKeyPrimary)
			{
				SelectNone();

				return true;
			}
		}
		else if (keycode == kKeyCodeDelete || keycode == kKeyCodeBackspace)
		{
			if (auto idx = GetListFocus(*this))
			{
				if (Emit(MakeIndexedEvent(kListRequestRemove, idx.value))) return true;
			}
		}
	}

	return Object::OnEvent(src, e);
}

void Reflex::GLX::AbstractList::OnSetProperty(Address address, Reflex::Object & object)
{
	/*if (adr == Address( { K32("selection"), K32("Array@Int32") }))
	{
		auto t = AutoRelease(this);

		auto array = Cast<VM::ValueArray>(object);

		auto src = AutoRelease(array->m_allocation);

		SelectNone();

		for (auto & i : array->GetView<UInt32>()) Select(Reinterpret<Int32>(i), true);

		return;
	}
	else */if (address == MakeAddress<Data::Key32Property>(K32("selection_mode")))
	{
		auto t = AutoRelease(object);

		switch (Cast<Data::Key32Property>(object)->value.value)
		{
		case K32("single"):
			SetSelectionMode(kSelectionModeSingle);
			break;

		case K32("multi"):
			SetSelectionMode(kSelectionModeMulti);
			break;

		case K32("toggle"):
			SetSelectionMode(kSelectionModeMultiToggle);
			break;
		}

		return;
	}

	return GLX::Object::OnSetProperty(address, object);
}

Reflex::Idx Reflex::GLX::GetListFocus(const AbstractList & list)
{
	try
	{
		list.EnumerateSelection(0, kMaxUInt32, [](UInt idx, UInt n)
		{
			throw(idx);
		});
	}
	catch (UInt idx)
	{
		return idx;
	}

	return {};
}

Reflex::Array <Reflex::UInt> Reflex::GLX::GetListSelection(const AbstractList & list)
{
	Array <UInt> rtn;

	list.EnumerateSelection(0, kMaxUInt32, [&rtn](UInt idx, UInt n)
	{
		for (auto & i : Extend(rtn, n)) i = idx++;
	});

	return rtn;
}

bool Reflex::GLX::IsListSelected(const AbstractList & list, UInt idx)
{
	bool selected = false;

	list.EnumerateSelection(idx, 1, [&selected](UInt idx, UInt n)
	{
		selected = True(n);
	});

	return selected;
}

Reflex::UInt Reflex::GLX::CountListSelection(const AbstractList & list)
{
	UInt nselected = 0;

	list.EnumerateSelection(0, kMaxUInt32, [&nselected](UInt start, UInt n)
	{
		nselected += n;
	});

	return nselected;
}
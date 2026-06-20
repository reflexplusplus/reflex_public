#include "../../../../include/reflex_ext/glx/widgets/virtuallist.h"
#include "../../../../include/reflex_ext/glx/functions/focus.h"




//
//implementation

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct VirtualListComputedStyle : public Reflex::Object
{
	VirtualListComputedStyle(const Style & style)
		: item(style[K32("item")])
		, item_size(item ? Detail::Compile<Detail::ComputedStyle>(item)->GetMinMax().a : MakeSize(20.0f))
	{
	}

	const ConstTRef <Style> item;

	const Size item_size;
};

Pair <UInt,UInt> GetRange(const Map <UInt> & selection, UInt start, UInt range)
{
	if (auto idx = selection.SearchGTE(start))
	{
		auto range_start = idx.value;

		if (auto end = selection.SearchLT(start + range))
		{
			return { range_start, (end.value - range_start) + 1 };
		}
	}

	return { 0, 0 };
}

REFLEX_DECLARE_KEY32(Callback);

REFLEX_END_INTERNAL

Reflex::GLX::VirtualList::VirtualList()
	: GLX::AbstractList([](GLX::Object & self) -> TRef <Detail::LayoutModel>
	{
		struct Layout : public Detail::LayoutModel
		{
			Pair <AccommodateFn,AlignFn> OnRebuild(GLX::Object & object, UInt8 layout_flags) override
			{
				if (Cast<VirtualList>(object)->m_pcallback)
				{
					bool yaxis = BitCheck(layout_flags, Detail::kStandardLayoutY);

					return CastLayoutFns<VirtualList>(yaxis ? &OnAccommodate<true> : &OnAccommodate<false>, yaxis ? &OnAlign<true> : &OnAlign<false>);
				}
				else
				{
					return
					{
						[](GLX::Object & object, bool & isresponsive, System::fSize & contentsize) {},
						[](GLX::Object & object, bool isresponsive, Float & contenth) {}
					};
				}
			}
		};

		return New<Layout>();
	})
	, m_itemsize({ 20.0f, 20.0f })
	, m_pcallback(nullptr)
	, m_nitem(0)
	, m_visible({ 0,-1 })
{
	SetFlow(*this, kFlowY);
}

void Reflex::GLX::VirtualList::SetPopulateCallback(const PopulateCallback & callback)
{
	m_pcallback = REFLEX_CREATE(PopulateCallbackProperty, callback);

	SetProperty(kCallback, m_pcallback);

	RebuildLayout();

	m_visible = { 0, -1 };
}

void Reflex::GLX::VirtualList::ClearItems()
{
	m_nitem = 0;

	m_selection.Clear();

	m_visible = { 0, -1 };

	Accommodate();
}

void Reflex::GLX::VirtualList::SetNumItem(UInt n, bool forcerebuild)
{
	if (SetFiltered(m_nitem, n) || forcerebuild)
	{
		auto & selection = Reinterpret<Sequence<UInt>>(m_selection);

		if (UInt nsel = selection.GetSize())
		{
			while (selection.GetLast().key >= n)
			{
				selection.Remove(--nsel);

				if (selection.Empty()) break;
			}
		}

		m_visible = { 0, -1 };

		Accommodate();
	}
}

void Reflex::GLX::VirtualList::Rebuild()
{
	auto n = m_nitem;

	m_nitem = 0;

	SetNumItem(n);
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::VirtualList::GetItem(UInt idx)
{
	if (Reflex::Inside(Reinterpret<Int>(idx), m_visible.a, m_visible.b)) return m_items[idx - m_visible.a];

	return {};
}

Reflex::UInt Reflex::GLX::VirtualList::OnGetSize() const
{
	return m_nitem;
}

Reflex::Idx Reflex::GLX::VirtualList::OnGetIndex(Object & child)
{
	if (auto idx = LookupIndex(child)) return idx.value + m_visible.a;

	return {};
}

void Reflex::GLX::VirtualList::OnSelect(UInt idx)
{
	auto first = m_visible.a;

	if (Reflex::Inside<Int>(idx, first, m_visible.b))
	{
		auto & item = *m_items[idx - first];

		GLX::Select(item);

		RedirectFocus(*this, item);
	}

	m_selection.Set(idx);
}

void Reflex::GLX::VirtualList::OnDeselect(UInt idx)
{
	auto first = m_visible.a;

	if (Reflex::Inside<Int>(idx, first, m_visible.b))
	{
		auto & item = *m_items[idx - first];

		GLX::Select(item, false);
	}

	m_selection.Unset(idx);
}

void Reflex::GLX::VirtualList::OnEnumerateSelection(UInt start, UInt range, const Function <void(UInt idx, UInt n)> & callback) const
{
	if (m_selection.GetSize() == m_nitem)
	{
		callback(0, m_nitem);
	}
	else
	{
		auto [idx,n] = GetRange(m_selection, start, range);

		auto pselection = m_selection.GetData();

		while (n--)
		{
			callback(pselection[idx++]->key, 1);
		}
	}
}

void Reflex::GLX::VirtualList::OnReveal(UInt idx)
{
	auto viewport = GetContainingViewPort(*this);

	auto y = GetAxis(*this);

	if (viewport->GetContent().Adr() == this)
	{
		auto itemsize = Detail::GetSize(y, m_itemsize);

		viewport->Reveal(y, idx * itemsize, itemsize, itemsize);
	}
}

void Reflex::GLX::VirtualList::OnSetStyle(const Style & style)
{
	auto cstyle = Detail::Compile<VirtualListComputedStyle>(style);

	m_item = cstyle->item;

	m_itemsize = Max(cstyle->item_size, GLX::kNormal);

	Rebuild();
}

bool Reflex::GLX::VirtualList::OnEvent(Object & src, Event & e)
{
	if (e.id == kFocus && &src != this)
	{
		Focus();

		return true;
	}

	return AbstractList::OnEvent(src, e);
}

template <bool Y> void Reflex::GLX::VirtualList::OnAccommodate(VirtualList & self, bool & isresponsive, Size & contentsize)
{
	typedef ConditionalType <Y,Detail::YAxis,Detail::XAxis> AXIS;

	auto cstyle = self.GetComputedStyle();

	contentsize = AXIS::MakeSize(self.m_nitem * AXIS::GetSize(self.m_itemsize)) + Sum(cstyle->GetPadding());

	contentsize = Max(contentsize, cstyle->GetMinMax().a);

	Detail::AccommodateRenderer(self, isresponsive, contentsize);
}

template <bool Y> void Reflex::GLX::VirtualList::OnAlign(VirtualList & self, bool isresponsive, Float & contenth)
{
	typedef ConditionalType <Y,Detail::YAxis,Detail::XAxis> AXIS;

	auto & vr = self.GetParent()->GetRect().size;// self.m_coordinates->GetViewRange();

	if (AXIS::GetSize(vr))
	{
		auto & rect = self.GetRect();

		auto cstyle = self.GetComputedStyle();

		Float vo_axis = -AXIS::GetPoint(rect.origin);

		auto inner = Indent(rect.size, cstyle->GetPadding());

		auto & size = inner.size;

		auto & items = self.m_items;

		Float axis_size = AXIS::GetSize(self.m_itemsize);

		Int range = Reflex::Max(ToInt32(AXIS::GetSize(vr) / axis_size) + 2, 0);

		Int first = Reflex::Truncate(vo_axis / axis_size);

		Int last = Reflex::Min(first + range, Int(self.m_nitem));

		range = Reflex::Max(last - first, Int(0));

		Rect itemrect = { AXIS::MakePoint(first * axis_size) + inner.origin, AXIS::MakeSize(axis_size, Reflex::Max(AXIS::Ortho::GetSize(size), 0.0f)) };

		auto visible = MakeTuple(first, range);

		if (SetFiltered(self.m_visible, visible))
		{
			AnimationScope scope(false);

			self.Core::Object::Clear();

			items.SetSize(range);

			self.m_pcallback->value(visible.a, items, self.m_item);

			for (auto & i : items)
			{
				auto item = AddInline(self, i);

				item->SetRect(itemrect);

				AXIS::SetPoint(itemrect.origin, AXIS::GetPoint(itemrect.origin) + axis_size);
			}

			auto selection_range = GetRange(self.m_selection, first, range);

			auto pselection = self.m_selection.GetData() + selection_range.a;

			auto n = selection_range.b;

			while (n--)
			{
				UInt selectionidx = (*pselection++)->key;

				GLX::Select(*items[selectionidx - first]);
			}
		}
		else
		{
			REFLEX_LOOP(idx, range)
			{
				auto & item = *items[idx];

				item.SetRect(itemrect);

				AXIS::SetPoint(itemrect.origin, AXIS::GetPoint(itemrect.origin) + axis_size);

				GLX::Select(item, True(self.m_selection.Search(idx + first)));
			}
		}
	}

	Detail::AlignRenderer(self, contenth);
}

#include "../../../../include/reflex_ext/glx/widgets/list.h"




//MSG REWORRK
//
//single ListReorderMsg, has "allow" = true (default) and kstage
//on begin: ReorderMsg: "allow":true, kstage:begin, "from":idx" "source":source
//on reorder: ReorderMsg: "allow":true, kstage:perform, "from":idx" "to":idx, "source":source
//on reorder: ReorderMsg: "allow":true, kstage:end, "from":idx" "to":idx, "source":source

//
//container

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

REFLEX_NOINLINE static bool EmitReorderRequest(Core::WeakReference & list, TransactionStage transaction, UInt from, UInt to, bool allow)
{
	auto e = Make<Event>(List::kListReorder);

	Data::SetUInt8(e, kstage, transaction);

	Data::SetUInt32(e, kfrom, from);

	Data::SetUInt32(e, kto, to);

	return Detail::EmitRequest(list, e, allow);
}

struct ListLayout : public Detail::StandardLayout
{
	using StandardLayout::StandardLayout;

	Pair <AccommodateFn,AlignFn> OnRebuild(GLX::Object & object, UInt8 layout_flags) override;

	static void OnAccommodate(List & list, bool & isresponsive, Size & contentsize);

	AccommodateFn std_accommodate;
};

struct ReorderContainer : public Reflex::Object
{
	typedef GLX::Object Object;

	ReorderContainer(List & list, UInt idx);

	~ReorderContainer();


	void OnMouseDrag();

	bool EmitRequest(TransactionStage transaction, UInt from, UInt to)
	{
		return EmitReorderRequest(m_list, transaction, from, to, true);
	}


	Core::WeakReference m_list;

	const bool yaxis;

	Size m_contentsize;

	Float m_padding;


	UInt m_from, m_to;

	Array < Pair <Reference <GLX::Object>, Float> > m_objects;

	Array < Pair <GLX::Object*, Float> > m_current;


	static inline const Float kLogFloor = Log(0.01f) / Log(2.0f);
};

ReorderContainer::ReorderContainer(List & list, UInt idx)
	: m_list(list),
	yaxis(GetAxis(list)),
	m_from(idx),
	m_to(idx)
{
	GetContainingViewPort(list)->EnableAutoScroll(0.25f, true);

	list.ComputeLayout();

	auto cstyle = list.GetComputedStyle();

	auto & padding = cstyle->GetPadding();

	m_contentsize = list.contentsize - Sum(padding);

	m_padding = Detail::GetSize(yaxis, padding.near);// *-1.0f;

	for (auto & i : list)
	{
		EnableAbsolute(i);

		i.ComputeLayout();

		Point position = i.GetRect().origin;

		auto axisposition = Detail::GetPoint(yaxis, position);

		Detail::SetPoint(yaxis, position, axisposition);

		m_objects.Push({ &i, axisposition });
	}

	m_current.SetSize(m_objects.GetSize());

	REFLEX_LOOP(idx, m_objects.GetSize())
	{
		auto & pair = m_objects[idx];

		auto & current = m_current[idx];

		current = { pair.a.Adr(), pair.b };
	}

	auto & object = *m_objects[m_from].a;

	object.SendTop();

	object.SetState(List::kReorderState);

	list.RebuildLayout();

	AttachAnimationClock(list, K32("onclock"), [this](Float delta)
	{
		delta += 0.0001f;

		for (auto & i : m_current)
		{
			Object & object = *i.a;

			Point position = object.GetRect().origin;

			Float axis = Detail::GetPoint(yaxis, position);

			Float dif = axis - i.b;

			Float sign = Sign(dif);

			Float mult = Min(Pow(2.0f, kLogFloor / (0.35f / delta)), 1.0f);

			Float t = RoundDown(Abs(dif) * mult);

			axis = RoundDown(i.b + (t * sign));

			Detail::SetPoint(yaxis, position, axis);

			object.SetPosition(position);
		}
	});
}

ReorderContainer::~ReorderContainer()
{
	if (DynamicCast<List>(*m_list))
	{
		m_list->RebuildLayout();

		DetachClock(*m_list, K32("onclock"));

		for (auto & i : m_objects) EnableInline<false>(i.a, GLX::kOrientationFit);

		m_objects[m_from].a->ClearState(List::kReorderState);

		if (m_from == m_to)
		{
			for (auto & i : m_objects) i.a->SendTop();

			EmitRequest(kTransactionCancel, m_from, m_to);
		}
		else
		{
			if (EmitRequest(kTransactionEnd, m_from, m_to))
			{
				for (auto & i : m_current) i.a->SendTop();
			}
		}

		if (auto plist = DynamicCast<List>(*m_list))
		{
			GetContainingViewPort(m_list)->DisableAutoScroll();
		}
	}
}

void ReorderContainer::OnMouseDrag()
{
	auto & list = *m_list;

	Point mousepos = GetMousePosition(list);

	UInt to = 0;

	REFLEX_RLOOP(idx, m_current.GetSize())
	{
		to = idx;

		if (Detail::GetPoint(yaxis, mousepos) > m_objects[idx].b) break;
	}

	Int dec = Sign<Int>(to - m_from);

	auto end = m_from - dec;

	do
	{
		if (EmitRequest(kTransactionPerform, m_from, to))
		{
			if (SetFiltered(m_to, to))
			{
				REFLEX_LOOP(idx, m_objects.GetSize())
				{
					auto & pair = m_objects[idx];

					auto & current = m_current[idx];

					current = { pair.a.Adr(), 0.0f };
				}

				Pair <Object*,Float> pair = m_current[m_from];

				m_current.Remove(m_from);

				m_current.Insert(to, pair);

				Float pos = m_padding;

				for (auto & pair : m_current)
				{
					auto & object = *pair.a;

					object.ComputeLayout();

					auto & margin = object.GetComputedStyle()->GetMargin();

					pos += Detail::GetSize(yaxis, margin.near);

					pair.b = pos;

					pos += Detail::GetSize(yaxis, object.GetRect().size) + Detail::GetSize(yaxis, margin.far);
				}
			}

			return;
		}

		to -= dec;
	}
	while (to != end);
}

Pair <ListLayout::AccommodateFn,ListLayout::AlignFn> ListLayout::OnRebuild(GLX::Object & object, UInt8 layout_flags)
{
	auto base = StandardLayout::OnRebuild(object, layout_flags);

	std_accommodate = base.a;

	if (auto reorder = object.QueryProperty<ReorderContainer>(kNullKey))
	{
		base.a = CastAccommodateFn<List>(&ListLayout::OnAccommodate);
	}
	
	return base;
}

void ListLayout::OnAccommodate(List & list, bool & isresponsive, Size & contentsize)
{
	auto layout = Cast<ListLayout>(list.GetLayoutModel().RemoveConst());

	layout->std_accommodate(list, isresponsive, contentsize);

	if (auto reorder = list.QueryProperty<ReorderContainer>(kNullKey))
	{
		contentsize = reorder->m_contentsize;
	}
}

REFLEX_END_INTERNAL

Reflex::GLX::List::List()
	: GLX::AbstractList([](GLX::Object & self) -> TRef <Detail::LayoutModel>
	{
		return New<ListLayout>(self);
	})
{
}

Reflex::UInt Reflex::GLX::List::OnGetSize() const
{
	return GLX::Object::GetNumItem();
}

Reflex::Idx Reflex::GLX::List::OnGetIndex(Object & child)
{
	return LookupIndex(child);
}

void Reflex::GLX::List::OnSelect(UInt idx)
{
	if (idx < GLX::Object::GetNumItem())
	{
		auto item = LookupChildAtIndex(*this, idx);

		item->SetState(kSelectedState);
	}
}

void Reflex::GLX::List::OnDeselect(UInt idx)
{
	if (idx < GLX::Object::GetNumItem())
	{
		auto item = LookupChildAtIndex(*this, idx);

		item->ClearState(kSelectedState);
	}
}

void Reflex::GLX::List::OnEnumerateSelection(UInt start, UInt range, const Function <void(UInt idx, UInt n)> & callback) const
{
	auto n = GetNumItem();

	if (start < n)
	{
		range = std::min(range, n - start);

		const Object * ptr = LookupItemAtIndex(*this, start);

		while (range--)
		{
			if (ptr->CheckState(kSelectedState)) callback(start, 1);

			start++;

			ptr = ptr->GetNext();
		}
	}
}

void Reflex::GLX::List::OnReveal(UInt idx)
{
	auto item = LookupChildAtIndex(*this, idx);

	bool y = GetAxis(*this);

	auto & rect = item->GetRect();

	Float itemsize = Detail::GetSize(y, rect.size);

	auto viewport = GetContainingViewPort(*this);

	if (viewport->GetContent().Adr() == this) viewport->Reveal(y, Detail::GetPoint(y, rect.origin), itemsize, itemsize);
}

void Reflex::GLX::List::OnUpdate()
{
	//TODO
	//update selection here based on Selection
	//maintain seperate state to item select state / dont set synchron
}

bool Reflex::GLX::List::OnEvent(Object & src, Event & e)
{
	if (e.id == kListStartDrag)
	{
		if (CountListSelection(*this) == 1)
		{
			Core::WeakReference self(*this);

			auto index = GetIndex(e);

			if (EmitReorderRequest(self, kTransactionBegin, index, index, false))
			{
				SetProperty(kNullKey, REFLEX_CREATE(ReorderContainer, *this, index));
			}
		}

		return true;
	}
	else if (e.id == kMouseDrag)
	{
		if (auto reorder = QueryProperty<ReorderContainer>(kNullKey))
		{
			reorder->OnMouseDrag();

			return true;
		}
	}
	else if (e.id == kMouseUp)
	{
		UnsetProperty<ReorderContainer>(kNullKey);
	}

	return AbstractList::OnEvent(src, e);
}

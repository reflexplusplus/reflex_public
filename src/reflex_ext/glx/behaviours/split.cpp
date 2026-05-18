#include "../../../../include/reflex_ext/glx/behaviours/split.h"
#include "../widgets/common.h"




//
//implementation

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct SplitState
{
	Float content_size;
	Float init_size;
	Float max;
	Float dir;
	Float scale;
	Idx idx;
};

SplitState g_split_state;

struct SplitBehaviourImpl : public SplitBehaviour
{
	void ClearSplitSize(GLX::Object & item) override;

	void SetSplitSize(GLX::Object & item, Float size) override;

	Float GetSplitSize(const GLX::Object & item) const override;

	bool OnMouseOver(Core::MouseAction mouseaction, UInt8 flags) override;

	bool OnEvent(GLX::Object & src, Event & e) override;
};

bool IsLastInline(const Object & object)
{
	auto itr = &object;

	while (auto next = itr->GetNext())
	{
		if (Detail::GetPositioning(*next).a == Detail::kPositioningInline) return false;

		itr = next;
	}

	return true;
}

TRef <Object> FindSplitTarget(Object & self)
{
	constexpr Float32 kInvert[2] = { 1.0f, -1.0f };

	const bool y = GetAxis(self);

	const bool is_inverted = IsInverted(self);

	TRef <Object> target;

	Float size = Detail::GetSize(y, self.GetRect().size);

	auto mousepos = GetMousePosition(self);

	Float axis_mousepos = Detail::GetPoint(y, mousepos);

	axis_mousepos = is_inverted ? size - axis_mousepos : axis_mousepos;

	Float axis_position = 0.0f;

	g_split_state.content_size = 0.0f;

	bool hasflex = false;

	for (auto & object : self)
	{
		auto & rect = object.GetRect();

		auto positioning = Detail::GetPositioning(object);

		if (positioning.a == Detail::kPositioningInline)
		{
			bool flex = positioning.b == kOrientationFit;

			hasflex = hasflex || flex;

			object.ComputeLayout();

			auto & margin = object.GetComputedStyle()->GetMargin();

			const Pair <Float> near_far = { Detail::GetSize(y, margin.near), Detail::GetSize(y, margin.far) };

			const Float sum = near_far.a + near_far.b;

			bool resize = Data::GetBool(object, kresize);

			if (target || flex || !resize)	//skip test
			{
				axis_position += Detail::GetSize(y, rect.size) + sum;

				g_split_state.content_size += Detail::GetSize(y, object.contentsize) + sum;
			}
			else
			{
				g_split_state.content_size = Detail::GetSize(y, object.contentsize) + sum;

				axis_position += (&near_far.a)[is_inverted]; //INVERT ? far : near;

				if (hasflex && IsLastInline(object))
				{
					if (Reflex::Inside(axis_mousepos, axis_position - 8.0f, 16.0f))
					{
						target = object;

						g_split_state.dir = kInvert[!is_inverted];	//INVERT ? 1.0f : -1.0f;
					}

					axis_position += Detail::GetSize(y, rect.size);
				}
				else
				{
					axis_position += Detail::GetSize(y, rect.size);

					if (Reflex::Inside(axis_mousepos, axis_position - 8.0f, 16.0f))
					{
						target = object;

						g_split_state.dir = kInvert[is_inverted];	//INVERT ? -1.0f : 1.0f;
					}
				}

				axis_position += (&near_far.a)[!is_inverted]; //INVERT ? near : far;
			}
		}
		else if (Contains(rect, mousepos))
		{
			return {};
		}
	}

	return target;
}

void SplitBehaviourImpl::ClearSplitSize(GLX::Object & item)
{
	UnsetBounds(item, SplitBehaviour::ksplit_size);
}

void SplitBehaviourImpl::SetSplitSize(GLX::Object & item, Float size)
{
	SetBounds(item, SplitBehaviour::ksplit_size, Detail::MakeSize(GetAxis(Delegate::object), size));
}

Reflex::Float SplitBehaviourImpl::GetSplitSize(const GLX::Object & item) const
{
	return Detail::GetSize(GetAxis(Delegate::object), GetBounds(item, SplitBehaviour::ksplit_size).a);
}

bool SplitBehaviourImpl::OnMouseOver(Core::MouseAction mouseaction, UInt8 flags)
{
	EnableMouse(object);

	if (mouseaction > Core::kMouseActionClick)
	{
		return false;
	}
	else if (FindSplitTarget(object))
	{
		object->SetMouseCursor(GetAxis(object) ? kMouseCursorTopBottom : kMouseCursorLeftRight);

		EnableMouse(object, true, true);
	}
	else
	{
		object->SetMouseCursor(kMouseCursorArrow);

		//EnableMouseCapture(object, false);
	}

	return false;
}

bool SplitBehaviourImpl::OnEvent(GLX::Object & src, Event & e)
{
	if (e.id == kMouseDown)
	{
		g_split_state.idx = {};

		if (!(GetClickFlags(e) & kClickFlagRmb))
		{
			if (auto target = FindSplitTarget(object))
			{
				object->SetProperty(K32("target"), REFLEX_CREATE(Detail::LegacyWeakReferenceObject, target));

				//TODO 
				//first: emit transaction begin
				//second: recompute entire contentisze and recalc max, this will allow clients to clear bounds that may mess up the max size

				bool y = GetAxis(object);

				g_split_state.init_size = Detail::GetSize(y, target->GetRect().size);

				g_split_state.max = Detail::GetSize(y, object->GetRect().size) - (g_split_state.content_size - Detail::GetSize(y, target->contentsize));

				g_split_state.scale = 1.0f / GetObject()->GetComputedStyle()->GetScale();

				g_split_state.idx = LookupIndex(target);

				EnableMouseCapture(object, true);

				EmitTransaction(object, kTransactionBegin, g_split_state.idx.value);

				return true;
			}
		}

		EnableMouseCapture(object, false);
	}
	else if (e.id == kMouseDrag)
	{
		if (g_split_state.idx)
		{
			auto drag = GetMouseDelta(e);

			bool y = GetAxis(object);

			auto & target = Data::Detail::AcquireProperty<Detail::LegacyWeakReferenceObject>(object, K32("target"))->value;

			Float delta = Detail::GetPoint(y, drag) * g_split_state.dir * g_split_state.scale;

			UnsetBounds(target, SplitBehaviour::ksplit_size);

			Float axis = Clip(g_split_state.init_size + delta, Detail::GetSize(y, Detail::ComputeContentSize(target)), g_split_state.max);

			SetBounds(target, SplitBehaviour::ksplit_size, Detail::MakeSize(y, axis), Detail::MakeSize(y, axis, kLarge.w));

			object->Realign();	//responsive flag bug

			EmitTransaction(object, kTransactionPerform, g_split_state.idx.value);

			return true;
		}
	}
	else if (e.id == kMouseUp)
	{
		if (g_split_state.idx)
		{
			object->UnsetProperty<Detail::LegacyWeakReferenceObject>(K32("target"));

			EmitTransaction(object, kTransactionEnd, g_split_state.idx.value);

			return true;
		}
	}

	return false;
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::GLX::SplitBehaviour> Reflex::GLX::SplitBehaviour::Create()
{
	return New<SplitBehaviourImpl>();
}

#include "../../../../include/reflex/glx/behaviours/resizer.h"




//
//private

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct AbstractDragBehaviourImpl
{
	enum Flags
	{
		kFlagX,
		kFlagY,
		kFlagBounds,
	};


	AbstractDragBehaviourImpl();

	void SetCursor(Object & object, MouseCursor cursor);

	void UnsetCursor(Object & object);


	bool Begin(Object & object, Alignment mouseover, Event & e, FunctionPointer <void(AbstractDragBehaviourImpl&, Object&, Size & min, Size & content)> get_size);

	void Process(Object & self, Alignment mouseover, Event & e, FunctionPointer <void(Object&,Size)> set_size);
		
	void End(Object & object)
	{
		UnsetCursor(object);

		EmitTransaction(object, kTransactionStageEnd);
	}

	template <class AXIS, Orientation ORIENTATION> static void DragAxis(Point drag);

	
	Flags8 m_flags;

	MouseCursor m_cursor_z;

	
	static inline Rect st_bounds;

	static inline Rect st_rect;

	static inline Size st_min;

	static inline Point st_drag_z;


	static constexpr UInt8 kDefaultDragBehaviourFlags = UInt8(MakeBit(kFlagX) | MakeBit(kFlagY) | MakeBit(kFlagBounds));

	static constexpr Rect kNoClip = { { -1048576.0f, -1048576.0f }, { 1048576.0f, 1048576.0f } };
};

REFLEX_STATIC_ASSERT(sizeof(AbstractDragBehaviourImpl) <= 4);

struct ResizeBehaviourImpl : public ResizeBehaviour
{
	ResizeBehaviourImpl();
	
	void SetHandleSize(Float size) override;

	void SetRect(const GLX::Rect & rect) override;

	Rect GetRect() const override;


	Core::Trap OnDragTender(UInt8 slot, UInt8 flags) override;

	void OnAttachObject() override;

	bool OnPointerTender(Core::PointerAction action, const Core::Pointer & pointer, UInt8 flags, Core::Trap & trap) override;

	bool OnEvent(GLX::Object & src, Event & e) override;
		

	template <bool AXIS_X, bool AXIS_Y> static Alignment FindEdge(const Rect & rect, Scale scale, Size handle_size, Point mousepos);
		
	template <class AXIS> static bool FarEdgeCase(Size bounds, Size handlesize, const Rect & rect)
	{
		auto & size = rect.size;

		return And(AXIS::GetSize(size) < AXIS::GetSize(handlesize), AXIS::GetPoint(rect.origin) + AXIS::GetSize(size) + Core::kRoundingTolerance > AXIS::GetSize(bounds));
	}

	template <class AXIS> static bool NearEdgeCase(Size bounds, Size handlesize, const Rect & rect)
	{
		auto & size = rect.size;

		return And(AXIS::GetSize(size) < AXIS::GetSize(handlesize), AXIS::GetPoint(rect.origin) - Core::kRoundingTolerance < 0.0f);
	}

	
	Float m_handle_size;

	Alignment m_target_edge;


	static const decltype (&FindEdge<false,false>) kFindEdgeFns[4];
};

struct MoveBehaviourImpl : public MoveBehaviour
{
	void EnableAxis(bool x, bool y) override;

	Core::Trap OnDragTender(UInt8 slot, UInt8 flags) override
	{
		return Core::kTrapPassive;
	}

	void OnAttachObject() override;

	bool OnEvent(GLX::Object & src, Event & e) override;
};

AbstractDragBehaviourImpl::AbstractDragBehaviourImpl()
	: m_flags(kDefaultDragBehaviourFlags),
	m_cursor_z(kNumMouseCursor)
{
}

void AbstractDragBehaviourImpl::SetCursor(Object & object, MouseCursor cursor)
{
	if (m_cursor_z == kNumMouseCursor)
	{
		m_cursor_z = object.GetMouseCursor();
	}

	object.SetMouseCursor(cursor);
}

void AbstractDragBehaviourImpl::UnsetCursor(Object & object)
{
	if (m_cursor_z != kNumMouseCursor)
	{
		object.SetMouseCursor(m_cursor_z);

		m_cursor_z = kNumMouseCursor;
	}
}

template <class AXIS, Reflex::GLX::Orientation ORIENTATION> void AbstractDragBehaviourImpl::DragAxis(Point drag)
{
	Rect & rect = st_rect;

	Float inc = AXIS::GetPoint(drag) - AXIS::GetPoint(st_drag_z);

	Float & point = (&rect.origin.x)[AXIS::kY];

	Float & size = (&rect.size.w)[AXIS::kY];

	switch (ORIENTATION)
	{
	case kOrientationNear:
	{
		Float tsize = Reflex::Max(size - inc, AXIS::GetSize(st_min));

		Float tpoint = Reflex::Max(point + (size - tsize), AXIS::GetPoint(st_bounds.origin));

		size -= tpoint - point;

		point = tpoint;
	}
	break;

	case kOrientationCenter:
		point += inc;
		point = Clip(point, AXIS::GetPoint(st_bounds.origin), AXIS::GetSize(st_bounds.size) - size);
		break;

	case kOrientationFar:
		size = Reflex::Min(size + inc, AXIS::GetSize(st_bounds.size) - point);
		break;

	default:
		break;
	};
}

bool AbstractDragBehaviourImpl::Begin(Object & object, Alignment mouseover, Event & e, FunctionPointer <void(AbstractDragBehaviourImpl&, Object&, Size & min, Size & size)> get_size)
{
	if (Not(GetClickFlags(e) & (kClickFlagRmb | kPointerFlagMulti)))
	{
		st_drag_z = kOrigin;

		st_rect = object.GetRect();

		EmitTransaction(object, kTransactionStageBegin, mouseover);

		auto orientation = Detail::kAlignmentToOrientation[mouseover];

		Size size;

		get_size(*this, object, st_min, size);

		bool resizex = orientation.a != kOrientationCenter;
		bool resizey = orientation.b != kOrientationCenter;

		st_rect.size.w = resizex ? st_rect.size.w : size.w;
		st_rect.size.h = resizey ? st_rect.size.h : size.h;

		Rect rect = { kOrigin, object.GetParent()->GetRect().size };

		st_bounds = m_flags.Check(kFlagBounds) ? rect : kNoClip;

		return true;
	}

	return false;
}

void AbstractDragBehaviourImpl::Process(Object & object, Alignment mouseover, Event & e, FunctionPointer <void(Object&, Size)> setsize)
{
	Point mousedrag = ScaleDelta(object.GetParent(), GetDelta(e));

	if (!ExceedsDragThreshold(mousedrag)) return;

	switch (mouseover)
	{
	case kAlignmentTopLeft:
		DragAxis<Detail::XAxis, kOrientationNear>(mousedrag);
		DragAxis<Detail::YAxis, kOrientationNear>(mousedrag);
		break;

	case kAlignmentTop:
		DragAxis<Detail::YAxis, kOrientationNear>(mousedrag);
		break;

	case kAlignmentTopRight:
		DragAxis<Detail::XAxis, kOrientationFar>(mousedrag);
		DragAxis<Detail::YAxis, kOrientationNear>(mousedrag);
		break;

	case kAlignmentLeft:
		DragAxis<Detail::XAxis, kOrientationNear>(mousedrag);
		break;

	case kAlignmentCenter:
		if (m_flags.Check(kFlagX)) DragAxis<Detail::XAxis, kOrientationCenter>(mousedrag);
		if (m_flags.Check(kFlagY)) DragAxis<Detail::YAxis, kOrientationCenter>(mousedrag);
		break;

	case kAlignmentRight:
		DragAxis<Detail::XAxis, kOrientationFar>(mousedrag);
		break;

	case kAlignmentBottomLeft:
		DragAxis<Detail::XAxis, kOrientationNear>(mousedrag);
		DragAxis<Detail::YAxis, kOrientationFar>(mousedrag);
		break;

	case kAlignmentBottom:
		DragAxis<Detail::YAxis, kOrientationFar>(mousedrag);
		break;

	case kAlignmentBottomRight:
		DragAxis<Detail::XAxis, kOrientationFar>(mousedrag);
		DragAxis<Detail::YAxis, kOrientationFar>(mousedrag);
		break;

	default:
		break;
	};

	st_drag_z = mousedrag;

	object.SetPosition(st_rect.origin);

	setsize(object, st_rect.size);

	object.Realign();	//in case rect.size is same as prev

	EmitTransaction(object, kTransactionStagePerform, mouseover);
}

ResizeBehaviourImpl::ResizeBehaviourImpl()
	: m_handle_size(8.0f),
	m_target_edge(kAlignmentCenter)
{
}

void ResizeBehaviourImpl::SetHandleSize(Float size)
{
	m_handle_size = size;
}

void ResizeBehaviourImpl::SetRect(const Rect & rect)
{
	SetBounds(object, kClassID, rect.size);

	object->SetRect(rect);
}

Rect ResizeBehaviourImpl::GetRect() const
{
	return { object->GetRect().origin, GetBounds(object, kClassID).a };
}

Core::Trap ResizeBehaviourImpl::OnDragTender(UInt8 slot, UInt8 flags)
{
	Core::Trap trap = Core::kTrapPassive;

	OnPointerTender(Core::kPointerActionPress, *Core::desktop->QueryPointer(slot), flags, trap);

	return trap;
}

void ResizeBehaviourImpl::OnAttachObject()
{
	EnableAbsolute(object);

	SetBounds(object, kClassID, kNormal, kLarge);
}

bool ResizeBehaviourImpl::OnPointerTender(Core::PointerAction action, const Core::Pointer & pointer, UInt8 flags, Core::Trap & trap)
{
	auto impl = Reinterpret<AbstractDragBehaviourImpl>(m_impl);

	if (Not(flags & (kClickFlagRmb | kPointerFlagMulti)))
	{
		Scale scale = object->GetParent()->GetScale();

		Size handle_size = Abs(MakeSize(m_handle_size) / scale);

		bool invert = True(scale.w < 0.0f) != True(scale.h < 0.0f);

		m_target_edge = (kFindEdgeFns[impl->m_flags.GetWord() & 3])(object->GetRect(), scale, handle_size, TransformPosition(object, pointer.position));

		MouseCursor cursor = kMouseCursorArrow;

		switch (m_target_edge)
		{
		case kAlignmentTopLeft:
			cursor = invert ? kMouseCursorBottomLeftTopRight : kMouseCursorTopLeftBottomRight;
			break;

		case kAlignmentTop:
			cursor = kMouseCursorTopBottom;
			break;

		case kAlignmentTopRight:
			cursor = invert ? kMouseCursorTopLeftBottomRight : kMouseCursorBottomLeftTopRight;
			break;

		case kAlignmentLeft:
			cursor = kMouseCursorLeftRight;
			break;

		case kAlignmentRight:
			cursor = kMouseCursorLeftRight;
			break;

		case kAlignmentBottomLeft:
			cursor = invert ? kMouseCursorTopLeftBottomRight : kMouseCursorBottomLeftTopRight;
			break;

		case kAlignmentBottom:
			cursor = kMouseCursorTopBottom;
			break;

		case kAlignmentBottomRight:
			cursor = invert ? kMouseCursorBottomLeftTopRight : kMouseCursorTopLeftBottomRight;
			break;

		default:
			impl->UnsetCursor(object);
			return false;
		}

		impl->SetCursor(object, cursor);

		trap = Core::kTrapActive;

		return true;
	}

	impl->UnsetCursor(object);

	return false;
}

bool ResizeBehaviourImpl::OnEvent(GLX::Object & src, Event & e)
{
	auto impl = Reinterpret<AbstractDragBehaviourImpl>(m_impl);

	if (m_target_edge != kNumAlignment)
	{
		if (e.id == kMouseDown)
		{
			return impl->Begin(object, m_target_edge, e, [](AbstractDragBehaviourImpl & self, GLX::Object & object, Size & min, Size & size)
			{
				object.SetMouseCursor(kMouseCursorInvisible);

				size = GetBounds(object, kClassID).a;

				UnsetBounds(object, kClassID);

				min = ComputeContentSize(object);

				SetBounds(object, kClassID, size);
			});
		}
		else if (e.id == kMouseDrag)
		{
			impl->Process(object, m_target_edge, e, [](GLX::Object & object, Size size)
			{
				SetBounds(object, kClassID, size);
			});

			return true;
		}
		else if (e.id == kMouseUp)
		{
			impl->End(object);

			return true;
		}
	}

	return false;
}

template <bool X, bool Y> Alignment ResizeBehaviourImpl::FindEdge(const Rect & rect, Scale scale, Size handle_size, Point mousepos)
{
	if constexpr (!(X | Y)) return kNumAlignment;

	auto size = rect.size;

	if (And(X, mousepos.x >= size.w - handle_size.w))
	{
		if constexpr (Y)
		{
			if (mousepos.y < handle_size.h)
			{
				return kAlignmentTopRight;
			}
			else if (mousepos.y >= size.h - handle_size.h)
			{
				return kAlignmentBottomRight;
			}
		}

		if (FarEdgeCase<Detail::XAxis>(AbstractDragBehaviourImpl::st_bounds.size, handle_size, rect))
		{
			return kAlignmentLeft;
		}
		else
		{
			return kAlignmentRight;
		}
	}
	else if (And(X, mousepos.x < handle_size.w))
	{
		if constexpr (Y)
		{
			if (mousepos.y < handle_size.h)
			{
				return kAlignmentTopLeft;
			}
			else if (mousepos.y >= size.h - handle_size.h)
			{
				return kAlignmentBottomLeft;
			}
		}

		return kAlignmentLeft;
	}
	else if (And(Y, mousepos.y < handle_size.h))
	{
		if (And(scale.h < 0.0f, NearEdgeCase<Detail::YAxis>(AbstractDragBehaviourImpl::st_bounds.size, handle_size, rect)))
		{
			return kAlignmentBottom;
		}
		else
		{
			return kAlignmentTop;
		}
	}
	else if (And(Y, mousepos.y >= size.h - handle_size.h))
	{
		if (FarEdgeCase<Detail::YAxis>(AbstractDragBehaviourImpl::st_bounds.size, handle_size, rect))
		{
			return kAlignmentTop;
		}
		else
		{
			return kAlignmentBottom;
		}
	}

	return kNumAlignment;
}

void MoveBehaviourImpl::EnableAxis(bool x, bool y)
{
	MoveBehaviour::EnableAxis(x, y);

	constexpr MouseCursor kCursors[] = { kMouseCursorArrow, kMouseCursorPointer, kMouseCursorPointer, kMouseCursorDrag };

	object->SetMouseCursor(kCursors[MakeBits(x, y)]);
}

void MoveBehaviourImpl::OnAttachObject()
{
	auto impl = Reinterpret<AbstractDragBehaviourImpl>(m_impl);

	EnableAxis(impl->m_flags.Check(AbstractDragBehaviourImpl::kFlagX), impl->m_flags.Check(AbstractDragBehaviourImpl::kFlagY));
}

bool MoveBehaviourImpl::OnEvent(GLX::Object & src, Event & e)
{
	auto impl = Reinterpret<AbstractDragBehaviourImpl>(m_impl);

	if (e.id == kMouseDown)
	{
		return impl->Begin(object, kAlignmentCenter, e, [](AbstractDragBehaviourImpl & impl, GLX::Object & object, Size & min, Size & size)
		{
			impl.SetCursor(object, kMouseCursorInvisible);

			size = object.GetRect().size;
		});
	}
	else if (e.id == kMouseDrag)
	{
		impl->Process(object, kAlignmentCenter, e, [](GLX::Object & object, Size size)
		{
		});

		return true;
	}
	else if (e.id == kMouseUp)
	{
		impl->End(object);

		return true;
	}

	return false;
}

const decltype (&ResizeBehaviourImpl::FindEdge<false,false>) ResizeBehaviourImpl::kFindEdgeFns[4] =
{
	&FindEdge<false, false>,
	&FindEdge<true, false>,
	&FindEdge<false, true>,
	&FindEdge<true, true>,
};

REFLEX_END_INTERNAL

Reflex::GLX::AbstractDragBehaviour::AbstractDragBehaviour()
{
	Reflex::Detail::Constructor<AbstractDragBehaviourImpl>::Construct(m_impl);
}

void Reflex::GLX::AbstractDragBehaviour::EnableBounds(bool enable)
{
	auto impl = Reinterpret<AbstractDragBehaviourImpl>(m_impl);

	impl->m_flags.Set(AbstractDragBehaviourImpl::kFlagBounds, enable);
}

void Reflex::GLX::AbstractDragBehaviour::EnableAxis(bool x, bool y)
{
	auto impl = Reinterpret<AbstractDragBehaviourImpl>(m_impl);

	impl->m_flags.Set(AbstractDragBehaviourImpl::kFlagX, x);

	impl->m_flags.Set(AbstractDragBehaviourImpl::kFlagY, y);
}

Reflex::TRef <Reflex::GLX::ResizeBehaviour> Reflex::GLX::ResizeBehaviour::Create()
{
	return New<ResizeBehaviourImpl>();
}

Reflex::TRef <Reflex::GLX::MoveBehaviour> Reflex::GLX::MoveBehaviour::Create()
{
	return New<MoveBehaviourImpl>();
}

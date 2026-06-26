#include "../../../../include/reflex/glx/behaviours/gestures.h"
#include "../../../../include/reflex/glx/functions/input.h"
#include "../../../../include/reflex/glx/functions/properties.h"
#include "../library.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

enum CaptureFlags : UInt8
{
	kCaptureTarget = MakeBit(0),
	kCaptureActive = MakeBit(1)
};

Reference <GLX::Event> MakePointerDownEvent(UInt8 pointer_slot, UInt8 pointer_flags, Point pointer_position)
{
	auto e = Make<Event>(kPointerDown);

	Data::SetUInt8(e, kpointer_slot, pointer_slot);
	Data::SetUInt8(e, kflags, pointer_flags | kPointerFlagEmulated);
	Data::SetUInt8(e, kmodifiers, System::GetModifierKeys());
	SetPoint(e, kposition, pointer_position);

	return e;
}

struct AbstractSwipeGestureRecognizer : public Detail::AbstractGestureRecognizer
{
	AbstractSwipeGestureRecognizer(bool emulate_pointer_down, Float32 threshold)
		: AbstractGestureRecognizer(emulate_pointer_down)
		, m_threshold(threshold)
	{
	}

	bool OnTransactionTender(UInt8 pointer_flags, Float32 time_delta, Point position_delta) override
	{
		auto x = Abs(position_delta.x);
		auto y = Abs(position_delta.y);

		m_yaxis = y > x;

		return y > m_threshold || x > m_threshold;
	}

	const Float m_threshold;
	
	bool m_yaxis;
};

struct SwipeGestureRecognizer : public AbstractSwipeGestureRecognizer
{
	REFLEX_OBJECT_EX(SwipeGestureRecognizer, AbstractSwipeGestureRecognizer, "GLX::SwipeGestureRecognizer");

	using AbstractSwipeGestureRecognizer::AbstractSwipeGestureRecognizer;

	bool EmitGestureTransaction(Key32 id, TransactionStage stage, bool yaxis, Float position)
	{
		auto e = Make<Event>(id);

		Data::SetUInt8(e, kstage, stage);
		Data::SetBool(e, kaxis, yaxis);
		Data::SetFloat32(e, kvalue, Detail::SnapToPixels(position));

		Core::WeakReference weakref(object);

		return Detail::EmitRequest(weakref, e);
	}

	bool OnTransactionStage(TransactionStage stage, Float32 time, Point position_delta) override
	{
		constexpr auto ApplyStickyness = [](Float position, Float swipe_abs)
		{
			constexpr Float kStickyDistance = 128.0f;

			auto swipe = Min(swipe_abs / kStickyDistance, 1.0f);

			auto x = 1.0f - Square(1.0f - swipe);

			return (x * 0.125f) * kStickyDistance * Sign(position);
		};

		m_position = Detail::GetPoint(m_yaxis, position_delta);

		auto swipe_abs = Abs(m_position);

		switch (stage)
		{
		case kTransactionStageBegin:
			m_allow = EmitGestureTransaction(kSwipeGesture, kTransactionStageBegin, m_yaxis, m_position);
			return true;

		case kTransactionStagePerform:
			if (!m_allow)	//sticky resistance if cant swipe
			{
				m_position = ApplyStickyness(m_position, swipe_abs);
			}
			else if (swipe_abs > 128.0f)
			{
				EmitGestureTransaction(kSwipeGesture, kTransactionStageEnd, m_yaxis, m_position);
				return false;
			}
			EmitGestureTransaction(kSwipeGesture, kTransactionStagePerform, m_yaxis, m_position);
			return true;

		case kTransactionStageEnd:
			if (m_allow && (time < 0.3f) && (swipe_abs > 32.0f))
			{
				EmitGestureTransaction(kSwipeGesture, kTransactionStageEnd, m_yaxis, m_position);
			}
			else
			{
				if (!m_allow) m_position = ApplyStickyness(m_position, swipe_abs);

				EmitGestureTransaction(kSwipeGesture, kTransactionStageCancel, m_yaxis, m_position);
			}
			return false;
		}

		return false;
	}

	Float m_position;

	bool m_allow;
};

struct PanGestureRecognizer : public AbstractSwipeGestureRecognizer
{
	REFLEX_OBJECT_EX(PanGestureRecognizer, AbstractSwipeGestureRecognizer, "GLX::PanGestureRecognizer");

	using AbstractSwipeGestureRecognizer::AbstractSwipeGestureRecognizer;

	void OnDetachObject() override
	{
		DetachClock(object, "pan_flick");

		AbstractSwipeGestureRecognizer::OnDetachObject();
	}

	Point EmitPanTransaction(TransactionStage stage, bool yaxis, Float elapsed_time, Point position)
	{
		auto position_delta = SetDelta(m_position_delta_z, position);

		if (auto time_inc = SetDelta(m_elapsed_time_z, elapsed_time))
		{
			auto velocity = position_delta / MakeSize(time_inc);

			auto slot = m_velocity_count++ & 15;

			m_velocities[slot] = { velocity, elapsed_time };
		}

		auto e = Make<Event>(kPanGesture);

		Data::SetUInt8(e, kstage, UInt8(stage));
		SetPoint(e, kdelta, position_delta);

		object->Emit(e);

		return position_delta;
	}

	bool OnTransactionTender(UInt8 pointer_flags, Float32 time_delta, Point position_delta) override
	{
		DetachClock(object, "pan_flick");

		return AbstractSwipeGestureRecognizer::OnTransactionTender(pointer_flags, time_delta, position_delta);
	}

	bool OnTransactionStage(TransactionStage stage, Float32 time, Point position_delta) override
	{
		switch (stage)
		{
		case kTransactionStageBegin:
			MemClear(&m_elapsed_time_z, UInt(&m_end - &m_elapsed_time_z) * sizeof(Float32));
			return true;

		case kTransactionStagePerform:
			EmitPanTransaction(stage, m_yaxis, time, position_delta);
			return true;

		case kTransactionStageEnd:
			EmitPanTransaction(kTransactionStageEnd, m_yaxis, time, position_delta);	//call last to save a Reference
			{
				constexpr auto kTimeWindow = 0.1f;
				constexpr auto kWeightFalloff = 0.5f;

				Point flick;

				Float32 time_cutoff = time - kTimeWindow;

				auto weight = 1.0f;
				auto weight_sum = 0.0f;

				if (m_velocity_count)
				{
					Int last_idx = (m_velocity_count - 1) & 15;	//wrap to ring buffer of size 16

					REFLEX_LOOP(idx, Min<UInt>(m_velocity_count, 16))
					{
						auto & i = m_velocities[last_idx-- & 15];

						if (i.b < time_cutoff) break;

						auto v = i.a;

						flick.x += v.x * weight;
						flick.y += v.y * weight;

						weight_sum += weight;

						weight *= kWeightFalloff;
					}
				}

				if (weight_sum)
				{
					flick.x /= weight_sum;
					flick.y /= weight_sum;
				}

				if (m_elapsed_time_z < 0.125f && m_elapsed_time_z > 0.0f)
				{
					auto total = position_delta / MakeSize(m_elapsed_time_z);

					if (Abs(total.x) > Abs(flick.x)) flick.x = total.x;

					if (Abs(total.y) > Abs(flick.y)) flick.y = total.y;
				}

				m_flick_velocity = flick;
			}

			if (Reinterpret<UInt64>(m_flick_velocity))
			{
				GLX::AttachAnimationClock(object, "pan_flick", [this, yaxis = m_yaxis, position_delta](Float time_delta) mutable
				{
					constexpr auto kFlickDecay = MakeSize(0.965f);	//TODO adjust for time_delta which isnt guaranteed to be 60hz

					auto retain = AutoRelease(this);

					m_flick_velocity *= kFlickDecay;

					position_delta += m_flick_velocity * MakeSize(time_delta);

					m_elapsed_time_z += time_delta;

					auto inc = EmitPanTransaction(kTransactionStageEnd, yaxis, m_elapsed_time_z, position_delta);

					if (Abs(inc.x) < 1.0f && Abs(inc.y) < 1.0f)
					{
						DetachClock(object, "pan_flick");
					}
				});
			}
			return false;
		}

		return false;
	}

	Float32 m_elapsed_time_z;

	UInt m_velocity_count;

	Pair <Point,Float32> m_velocities[16];

	Point m_flick_velocity;

	Point m_position_delta_z;

	Float32 m_end;
};

REFLEX_END_INTERNAL

struct Reflex::GLX::Detail::AbstractGestureRecognizer::State : public Reflex::Object
{
	State(AbstractGestureRecognizer & owner, UInt8 slot, UInt8 flags, const Event & e);

	void PointerDrag(AbstractGestureRecognizer & owner, const Event & e);

	void PointerUp(AbstractGestureRecognizer & owner, const Event & e);


	REFLEX_INLINE void RunClock(const Function <void(Float)> & onclock)
	{
		m_clock = CreateAnimationClock(onclock);
	}

	REFLEX_INLINE void StopClock() { m_clock.Clear(); }

	static AbstractDraggable * AcquireTarget(AbstractGestureRecognizer & owner, Point pointer_delta);

	static bool CallTransactionStage(AbstractGestureRecognizer & owner, TransactionStage stage, Point pointer_position);

	static void EmulatePointerDown(AbstractGestureRecognizer & owner);


	Reference <Object> m_clock;


	UInt8 m_pointer_slot;

	UInt8 m_pointer_flags;

	bool m_initial;

	TransactionStage m_stage;


	Float64 m_start_time;

	Float m_elapsed_time;

	Point m_position_origin_window;
};

Reflex::GLX::Detail::AbstractGestureRecognizer::AbstractGestureRecognizer(bool emulate_pointer_down)
	: m_emulate_pointer_down(emulate_pointer_down)
	, m_capture_flags(0)
{
}

Reflex::GLX::Core::Trap Reflex::GLX::Detail::AbstractGestureRecognizer::OnDragTender(UInt8 slot, UInt8 flags)
{ 
	return OnTransactionTender(flags, 0.0f, kOrigin) ? Core::kTrapActive : Core::kTrapThru; 
}

void Reflex::GLX::Detail::AbstractGestureRecognizer::OnDetachObject()
{
	m_state.Clear();

	m_capture_flags = 0;
}

bool Reflex::GLX::Detail::AbstractGestureRecognizer::OnPointerTender(Core::PointerAction action, const Core::Pointer & pointer, UInt8 flags, Core::Trap & trap)
{
	if (trap == Core::kTrapThru)
	{
		//pointer input has been disabled
	}
	else if (action == Core::kPointerActionPress && Not(flags & (kPointerFlagMulti | kClickFlagRmb | kPointerFlagEmulated)))
	{
		//root AbstractGestureRecognizer will trap the event

		trap = Core::kTrapActive;

		return true;
	}

	return false;
}

bool Reflex::GLX::Detail::AbstractGestureRecognizer::OnEvent(GLX::Object & src, Event & e)
{
	switch (e.id.value)
	{
	case kPointerDown:
		if (src == object && !m_capture_flags)		//not a from nested child && not emulated
		{
			auto pointer_flags = GetClickFlags(e);

			if (Not(pointer_flags & (kPointerFlagMulti | kClickFlagRmb | kPointerFlagEmulated)))
			{
				m_capture_flags = kCaptureTarget | kCaptureActive;

				m_state = New<State>(*this, GetPointerSlot(e), pointer_flags, e);

				return true;
			}
		}
		break;

	case kPointerDrag:
		if (m_capture_flags & kCaptureActive)
		{
			REFLEX_ASSERT(m_state)

			Cast<State>(m_state)->PointerDrag(*this, e);
		}
		return True(m_capture_flags & kCaptureTarget);

	case kPointerUp:
		if (m_capture_flags & kCaptureActive)
		{
			REFLEX_ASSERT(m_state)

			Cast<State>(m_state)->PointerUp(*this, e);
		}
		m_state.Clear();
		return True(Poll(m_capture_flags) & kCaptureTarget);
	}

	return false;
}

Reflex::GLX::Detail::AbstractGestureRecognizer::State::State(AbstractGestureRecognizer & owner, UInt8 slot, UInt8 flags, const Event & e)
	: m_pointer_slot(slot)
	, m_pointer_flags(flags)
	, m_initial(true)
	, m_stage(kTransactionStageNull)
	, m_start_time(GetTimestamp(e))
	, m_elapsed_time(0.0f)
{
	//TODO support multi-pointer

	auto object = owner.object;

	m_position_origin_window = GetPosition(e);

	RunClock([this, &owner](Float delta) mutable
	{
		auto retain = AutoRelease(owner);	//we are not inside an Event, so the Delegate could die inside AcquireTarget

		m_elapsed_time += delta;

		if (m_stage == kTransactionStageNull)
		{
			AcquireTarget(owner, {});
		}
	});
}

void Reflex::GLX::Detail::AbstractGestureRecognizer::State::PointerDrag(AbstractGestureRecognizer & owner, const Event & e)
{
	//framework guarantees delegate retained during event, so callbacks cant destroy this

	if (m_stage == kTransactionStageNull)
	{
		AcquireTarget(owner, GetDelta(e));
	}
	else if (m_stage == kTransactionStagePerform)
	{
		CallTransactionStage(owner, kTransactionStagePerform, GetPosition(e));
	}
}

void Reflex::GLX::Detail::AbstractGestureRecognizer::State::PointerUp(AbstractGestureRecognizer & owner, const Event & e)
{
	//framework guarantees delegate retained during event, so callbacks cant destroy owner

	if (m_stage == kTransactionStagePerform)
	{
		m_elapsed_time = Float32(Max(GetTimestamp(e) - m_start_time, 1.0 / 120.0));

		auto pointer_position = GetPosition(e);

		if (CallTransactionStage(owner, kTransactionStagePerform, pointer_position))
		{
			CallTransactionStage(owner, kTransactionStageEnd, pointer_position);
		}
	}
	else
	{
		EmulatePointerDown(owner);
	}

	owner.m_state.Clear();
}

Reflex::GLX::Detail::AbstractDraggable * Reflex::GLX::Detail::AbstractGestureRecognizer::State::AcquireTarget(AbstractGestureRecognizer & owner, Point pointer_delta)
{
	using TenderFn = FunctionPointer <GLX::Object*(AbstractDraggable & dlg, UInt8 slot, UInt8 flags, Float32 time_delta, Point position_delta) > ;

	constexpr TenderFn kTenderFns[2] =
	{
		[](AbstractDraggable & dlg, UInt8 slot, UInt8 flags, Float32 time_delta, Point position_delta) -> GLX::Object *
		{
			if (Cast<AbstractGestureRecognizer>(dlg)->OnTransactionTender(flags, time_delta, position_delta))
			{
				return dlg.object.Adr();
			}
			else
			{
				return nullptr;
			}
		},
		[](AbstractDraggable & dlg, UInt8 slot, UInt8 flags, Float32 time_delta, Point position_delta) -> GLX::Object *
		{
			switch (dlg.OnDragTender(slot, flags))
			{
			case Core::kTrapActive:
				return dlg.object.Adr();

			case Core::kTrapPassive:
				if (auto pointer = Core::desktop->QueryPointer(slot))
				{
					return dlg.object->FindPointerTarget(Core::kPointerActionPress, *pointer, flags, TransformPosition(dlg.object, pointer->position)).Adr();
				}
				return nullptr;

			default:
				return nullptr;
			}
		}
	};

	const Reflex::Detail::DynamicTypeRef g_types[] = { AbstractGestureRecognizer::kDynamicTypeInfo, AbstractDraggable::kDynamicTypeInfo };

	constexpr auto Tender = [](AbstractGestureRecognizer & owner, Reflex::Detail::DynamicTypeRef object_t, TenderFn tender, Point position_delta, GLX::Object & target)
	{
		AbstractDraggable * consumer = nullptr;

		auto state = Cast<State>(owner.m_state);

		target.EnumerateDelegates([&](GLX::Object::Delegate & delegate)
		{
			if (!consumer)
			{
				if (Reflex::Detail::IsOrInheritsFrom(delegate.object_t, object_t))
				{
					auto draggable = Cast<AbstractDraggable>(delegate).Adr();

					auto slot = state->m_pointer_slot;
					auto flags = state->m_pointer_flags;

					if (auto target_object = tender(*draggable, slot, flags, state->m_elapsed_time, position_delta))
					{
						owner.object->SetProperty(Detail::MakePointerDownLinkID(state->m_pointer_slot), New<Detail::LegacyWeakReferenceObject>(*target_object));

						AbstractGestureRecognizer * previous = draggable == &owner ? nullptr : &owner;

						if (Reflex::Detail::CheckObjectType(*draggable, AbstractGestureRecognizer::kDynamicTypeInfo))
						{
							auto receiver = Cast<AbstractGestureRecognizer>(draggable);

							if (previous)
							{
								REFLEX_ASSERT(!receiver->m_capture_flags);

								receiver->m_state = previous->m_state;
								receiver->m_capture_flags = previous->m_capture_flags;

								previous->m_state.Clear();
								previous->m_capture_flags = 0;
							}

							if (State::CallTransactionStage(*receiver, kTransactionStageBegin, Core::desktop->QueryPointer(slot)->position))
							{
								state->m_stage = kTransactionStagePerform;
							}
							else
							{
								State::EmulatePointerDown(*receiver);

								state->m_stage = kTransactionStageCancel;
							}
						}
						else
						{
							REFLEX_ASSERT(previous);	//must be different because this can only occur on gesture

							previous->m_capture_flags = 0;

							target_object->EmitEx(MakePointerDownEvent(slot, flags, state->m_position_origin_window));

							state->m_stage = kTransactionStageCancel;
						}

						state->StopClock();

						consumer = draggable;
					}
				}
			}
		});

		return consumer;
	};

	REFLEX_ASSERT(owner.GetRetainCount() > 1);	//check its not just in the object list, but is held by eg event caller
	REFLEX_ASSERT(owner.m_state);

	auto object = owner.object;

	auto state = Cast<State>(owner.m_state);

	bool init = Poll(state->m_initial);

	auto pointer = Core::desktop->QueryPointer(state->m_pointer_slot);

	auto target = object->FindPointerTarget(Core::kPointerActionPress, *pointer, kPointerFlagEmulated, TransformPosition(object, pointer->position));

	auto object_t = g_types[init];
	auto tender = kTenderFns[init];

	while (target != object)
	{
		if (IsNull(owner.object)) return nullptr;

		if (auto dlg = Tender(owner, object_t, tender, pointer_delta, target)) return dlg;

		target = target->GetParent();
	}

	return Tender(owner, object_t, tender, pointer_delta, object);
}

bool Reflex::GLX::Detail::AbstractGestureRecognizer::State::CallTransactionStage(AbstractGestureRecognizer & owner, TransactionStage stage, Point pointer_position)
{
	REFLEX_ASSERT(owner.GetRetainCount() > 1);	//check its not just in the object list, but is held by eg event caller

	auto object = owner.object;

	auto state = Cast<State>(owner.m_state);

	bool cont = owner.OnTransactionStage(stage, state->m_elapsed_time, pointer_position - state->m_position_origin_window);
		
	if (!cont)
	{
		owner.m_capture_flags &= ~kCaptureActive;
	}

	return cont;
}

void Reflex::GLX::Detail::AbstractGestureRecognizer::State::EmulatePointerDown(AbstractGestureRecognizer & owner)
{
	if (owner.m_emulate_pointer_down && (owner.m_capture_flags & kCaptureActive))
	{
		auto capture_flags = owner.m_capture_flags;

		auto state = Cast<State>(owner.m_state);

		auto object = owner.object;

		auto pointer = *Core::desktop->QueryPointer(state->m_pointer_slot);

		pointer.position = state->m_position_origin_window;

		auto target = object->FindPointerTarget(Core::kPointerActionPress, pointer, kPointerFlagEmulated, TransformPosition(object, pointer.position));

		owner.m_capture_flags &= ~kCaptureActive;

		auto e = MakePointerDownEvent(state->m_pointer_slot, state->m_pointer_flags, pointer.position);

		if (auto dest = target->EmitEx(e))
		{
			if (Data::GetUInt8(dest, kcapture, Core::kTrapPassive) != Core::kTrapThru)
			{
				e->id = kPointerUp;

				dest->ProcessEvent(dest, e);
			}
		}

		owner.m_capture_flags = capture_flags;
	}
}

Reflex::TRef <Reflex::GLX::Object::Delegate> Reflex::GLX::CreateLongTapGestureRecognizer(bool emulate_pointer_down)
{
	struct LongTapGestureRecognizer : public Detail::AbstractGestureRecognizer
	{
		using AbstractGestureRecognizer::AbstractGestureRecognizer;

		bool OnTransactionTender(UInt8 pointer_flags, Float32 time_delta, Point position_delta) override
		{
			return time_delta > 0.5f && !ExceedsDragThreshold(position_delta, 4.0f);
		}

		bool OnTransactionStage(TransactionStage stage, Float32 time, Point position_delta) override
		{
			if (stage == kTransactionStageBegin)
			{
				object->Emit(Make<Event>(kLongTapGesture));
			}

			return false;
		}
	};

	return REFLEX_CREATE(LongTapGestureRecognizer, emulate_pointer_down);
}

Reflex::TRef <Reflex::GLX::Object::Delegate> Reflex::GLX::CreateSwipeGestureRecognizer(bool emulate_pointer_down, Float threshold)
{
	return REFLEX_CREATE(SwipeGestureRecognizer, emulate_pointer_down, threshold);
}

Reflex::TRef <Reflex::GLX::Object::Delegate> Reflex::GLX::CreatePanGestureRecognizer(bool emulate_pointer_down, Float threshold)
{
	return REFLEX_CREATE(PanGestureRecognizer, emulate_pointer_down, threshold);
}

void Reflex::GLX::IgnoreGestures(Object & object, bool include_children)
{
	struct IgnoreGesturesHelper : public Detail::AbstractDraggable
	{
		IgnoreGesturesHelper(bool include_children)
			: m_trap(include_children ? Core::kTrapPassive : Core::kTrapActive)
		{
		}

		Core::Trap OnDragTender(UInt8 slot, UInt8 flags) override
		{
			return m_trap;
		}

		const Core::Trap m_trap;
	};

	object.SetDelegate(MakeKey32("IgnoreGestures"), REFLEX_CREATE(IgnoreGesturesHelper, include_children));
}

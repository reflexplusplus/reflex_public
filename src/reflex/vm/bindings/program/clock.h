#pragma once

#include "reflex/vm.h"




//TODO

REFLEX_NS(Reflex::VM)

//template <class FUNC, class LIST, class ... VARGS> void SafeIterate(LIST & list, VARGS && ... param)
//{
//	if (auto t = list.GetFirst())
//	{
//		Reference <typename LIST::Type> itr = t;
//
//		do
//		{
//			FUNC::Call(itr, std::forward<VARGS>(param)...);
//
//			if (auto next = itr->GetNext())
//			{
//				itr = next;
//			}
//			else
//			{
//				break;
//			}
//		} 
//		while (true);
//	}
//}

struct PeriodicClock;

struct PeriodicClockList;

struct PeriodicClockItem : public Item <PeriodicClockItem,false>
{
	PeriodicClockItem(PeriodicClockList & list, Float32 initial, Float32 interval, VM::Detail::FnObject & callback);
	
	using Item <PeriodicClockItem,false>::Attach;

	void Process(Float32 delta);


	Reference <PeriodicClockList> m_list;

	Reference <VM::Detail::FnObject> m_callback;

	Float32 m_remaining;

	Float32 m_interval;
};

struct PeriodicClockList : 
	public Item <PeriodicClockList,false>,
	public PeriodicClockItem::List
{
	typedef PeriodicClockItem Type;

	PeriodicClockList(PeriodicClock & clock, VM::Context & context);

	virtual void OnBegin()
	{
		context_scope.Init(context);
	}

	virtual void OnEnd()
	{
		context_scope.Deinit();
	}

	Reference <PeriodicClock> m_clock;

	VM::Context & context;

	Reflex::Detail::Initialiser <VM::Context::Scope> context_scope;
};

struct PeriodicClock : 
	public Object,
	public List <PeriodicClockList,false>
{
	PeriodicClock()
		: m_prev(System::GetElapsedTime()),
		m_wrap(0),
		m_clock(System::CreateListener(System::kNotificationClock, this, [](void * pself)
		{
			auto & self = *Cast<PeriodicClock>(pself);

			if ((self.m_wrap++ & 3) == 0)
			{
				auto delta = Float32(SetDelta(self.m_prev, System::GetElapsedTime()));

				SafeIterate(Cast<List<PeriodicClockList,false>>(self), [](PeriodicClockList & list, Float32 delta)
				{
					list.OnBegin();

					SafeIterate(list, [](PeriodicClockItem & item, Float32 delta)
					{
						item.Process(delta);
					}, 
					delta);

					list.OnEnd();
				},
				delta);
			}
		}))
	{
	}

	template <class TYPE = PeriodicClockList> static TRef <PeriodicClockList> AcquireList(VM::Context & context)
	{
		auto self = The<PeriodicClock>::Acquire();

		for (auto & i : self)
		{
			if (&i.context == &context) return i;
		}

		return New<TYPE>(self, context);
	}

	Float64 m_prev;

	Reference <Object> m_clock;

	UInt8 m_wrap;
};

inline PeriodicClockList::PeriodicClockList(PeriodicClock & clock, VM::Context & context)
	: context(context),
	m_clock(clock)
{
	Attach(clock);
}

inline PeriodicClockItem::PeriodicClockItem(PeriodicClockList & list, Float32 initial, Float32 interval, VM::Detail::FnObject & callback)
	: m_list(list),
	m_remaining(Max(initial,0.0f)),
	m_interval(Max(initial, 0.0f)),
	m_callback(callback)
{
	Attach(list);
}

REFLEX_INLINE void PeriodicClockItem::Process(Float32 delta)
{
	m_remaining -= delta;

	if (m_remaining <= 0.0f)
	{
		m_remaining += m_interval;

		(*m_callback)(m_callback->context);
	}
}

REFLEX_END

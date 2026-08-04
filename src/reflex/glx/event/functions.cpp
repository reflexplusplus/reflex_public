#include "reflex/glx/event/functions.h"




//
//message

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct ScopedFlag : public Reflex::Object
{
	ScopedFlag()
	{
		SetOnHeap(*ToPointer<Allocator>(sizeof(void*)));
	}

	void OnDestruct() override 
	{
	}
};

struct EventHandler : public Object::Delegate
{
	REFLEX_OBJECT_EX(EventHandler, Delegate, "GLX::EventHandler");

	EventHandler(GLX::Object & object, Key32 id, const Function <bool(GLX::Object&, Event&)> & onevent)
		: id(id),
		callback(onevent) 
	{
	}

	bool OnEvent(GLX::Object & src, Event & e) override
	{
		return callback(src, e);
	}

	const Key32 id;

	const Function <bool(GLX::Object&, Event&)> callback;
};

struct BindEventVoidHandler : public Object::Delegate
{
	REFLEX_IF_DEBUG(REFLEX_OBJECT_EX(BindEventVoidHandler, Delegate, "GLX::BindEventVoid"));

	BindEventVoidHandler(GLX::Object & object, Key32 id, const Function <void()> & callback) : id(id), callback(callback) {}

	bool OnEvent(GLX::Object & src, Event & e) override
	{
		if (e.id == id)
		{
			callback();

			return true;
		}

		return false;
	}

	const Key32 id;

	const Function <void()> callback;
};

struct EventForwarder : public Object::Delegate
{
	REFLEX_OBJECT_EX(EventForwarder, Delegate, "GLX::EventForwarder");

	EventForwarder(Key32 event_id, GLX::Object & to)
		: m_id(event_id),
		m_to(to)
	{
	}

	bool OnEvent(GLX::Object & src, Event & e) override
	{
		if (e.id == m_id && m_to)
		{
			ScopedFlag flag;

			auto clone = AutoRelease(e.Clone());

			Core::WeakReference itr(m_to);

			while (IsValid(*itr))
			{
				auto & level = *itr;

				if (!level.QueryProperty<ScopedFlag>(m_id))
				{
					level.SetProperty(m_id, flag);

					bool rtn = level.ProcessEvent(src, clone);

					itr->UnsetProperty<ScopedFlag>(m_id);

					if (rtn) return rtn;

					itr = itr->GetParent();
				}
				else
				{
					return false;
				}
			}
		}

		return false;
	}

	const Key32 m_id;

	const Core::WeakReference m_to;
};

REFLEX_END_INTERNAL

REFLEX_SET_TRAIT(Reflex::GLX::ScopedFlag, IsSingleThreadExclusive);

bool Reflex::GLX::EmitCloseRequest(Object & object)
{
	return object.Emit(Make<Event>(kRequestClose));
}

const Reflex::GLX::Event * Reflex::GLX::QueryAntecedent(const Event & e, Key32 id, const Event * fallback)
{
	auto itr = e.GetPrev();

	while (itr)
	{
		if (itr->id == id) return itr;

		itr = itr->GetPrev();
	}

	return fallback;
}

Reflex::WChar Reflex::GLX::GetKeyCharacter(const Event & e)
{
	if (auto p = e.QueryProperty<ObjectOf<WChar>>(K32("character")))
	{
		return p->value;
	}

	return WChar(0);
}

void Reflex::GLX::EmitTransaction(Object & object, TransactionStage stage, UInt idx, Float32 value)
{
	auto e = Make<Event>(kTransaction);

	Data::SetUInt8(e, kstage, UInt8(stage));

	Data::SetUInt32(e, kindex, idx);

	Data::SetFloat32(e, kvalue, value);

	Data::SetUInt8(e, kmodifiers, System::GetModifierKeys());

	object.Emit(e);
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::GetItem(Event & e)
{
	return e.QueryProperty<Object>(kitem, &Object::null);
}

bool Reflex::GLX::Detail::EmitRequest(Core::WeakReference & src, Event & e, bool allow_default)
{
	src->Emit(e);

	return Data::GetBool(e, kallow, allow_default) && src;
}

void Reflex::GLX::SetEventDelegate(Object & object, Key32 id, const Function <bool(Object&,Event&)> & onevent)
{
	object.SetDelegate(id, New<EventHandler>(object, id, onevent));
}

void Reflex::GLX::BindEvent(Object & object, Key32 id, const Function <bool(Object&, Event&)> & onevent)
{
	struct BindEventHandler : public EventHandler
	{
		using EventHandler::EventHandler;

		bool OnEvent(GLX::Object & src, Event & e) override
		{
			if (e.id == id)
			{
				return callback(src, e);
			}
			else
			{
				return false;
			}
		}
	};

	object.SetDelegate(id, New<BindEventHandler>(object, id, onevent));
}

void Reflex::GLX::BindEventVoid(Object & object, Key32 id, const Function <void()> & callback)
{
	object.SetDelegate(id, New<BindEventVoidHandler>(object, id, callback));
}

void Reflex::GLX::BeginEventForwarding(Object & from, Key32 event_id, Object & to)
{
	from.SetDelegate(event_id, New<EventForwarder>(event_id, to));
}

bool Reflex::GLX::Emit(Object & src, Key32 id, Data::PropertySet && params)
{
	REFLEX_ASSERT(Core::Context::IsActive());

	GLX::Core::Context ctx;

	auto e = Make<Event>(id, std::move(params));

	Data::SetUInt8(e, kmodifiers, System::GetModifierKeys());

	return src.Emit(e);
}

bool Reflex::GLX::Send(Object & dest, Key32 id, Data::PropertySet && params)
{
	REFLEX_ASSERT(Core::Context::IsActive());

	GLX::Core::Context ctx;

	auto e = Make<Event>(id, std::move(params));

	Data::SetUInt8(e, kmodifiers, System::GetModifierKeys());

	return dest.ProcessEvent(dest, e);
}

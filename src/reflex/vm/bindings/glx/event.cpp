#include "global.h"
#include "../program/clock.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::GLXVM)

REFLEX_DECLARE_KEY32(repeat);
REFLEX_DECLARE_KEY32(modifiers);
REFLEX_DECLARE_KEY32(character);
REFLEX_DECLARE_KEY32(state);
REFLEX_DECLARE_KEY32(allow);

typedef ObjectOf <GLX::Point> ObjectOfPoint;

template <class TYPE>
struct NullValueOf
{
	inline static ObjectOf <TYPE> st_null;
};

template <class TYPE> REFLEX_INLINE const TYPE & CheckValue(const Data::PropertySet & data, Key32 id, TYPE def = TYPE())
{
	auto & null = NullValueOf<TYPE>::st_null;

	null.value = def;

	return data.QueryProperty<ObjectOf<TYPE>>(id, &null)->value;
}

REFLEX_INLINE bool CheckType(const Reflex::Object & object, TypeID type_id)
{
	auto itr = object.object_t;

	if (itr->type_id == type_id) return true;

	while ((itr = itr->base))
	{
		if (itr->type_id == type_id) return true;
	}

	return false;
}

template <class FROM, class TO> void TranslateParameters(GLX::Event & e)
{
	if (auto range = e.Iterate< ObjectOf <FROM> >())
	{
		Array < Pair <Key32, TO> > temp;

		for (auto & i : range)
		{
			temp.Push({ i.key.id, TO(i.value->value) });
		}

		for (auto & i : temp)
		{
			e.SetProperty(i.a, REFLEX_CREATE(ObjectOf<TO>, i.b));
		}
	}
}

void TranslateEventDefault(VM::Context & context, GLX::Event & e) 
{
	TranslateParameters<UInt32,Int32>(e);

	TranslateParameters<bool,UInt8>(e);
}

void ApplyEventNull(GLX::Object & src, GLX::Event & e) {}

void TranslateKeyDown(VM::Context & context, GLX::Event & e)
{
	e.SetProperty(krepeat, REFLEX_CREATE(Data::UInt8Property, CheckValue(e, krepeat, false)));
}

void TranslateKeyCharacter(VM::Context & context, GLX::Event & e)
{
	TranslateEventDefault(context, e);

	auto character = CheckValue<WChar>(e, kcharacter);

	e.SetProperty(kcharacter, REFLEX_CREATE(Data::Int32Property, character));

	e.SetProperty(kcharacter, New<VM::String>(ToView(character)));
}

//void TranslateDragOver(VM::Context & context, GLX::Event & e)
//{
//	TranslateEventDefault(context, e);
//
//	auto source = Data::Detail::AcquireProperty<GLX::Object>(e, GLX::kdrag_source);
//
//	if (auto pdragdata = source->QueryProperty<Reflex::Object>(GLX::kdata))
//	{
//		for (auto & i : DragHandler::range)
//		{
//			if (CheckType(*pdragdata, i.apptype))
//			{
//				i.fn(context, GLX::kdata, *pdragdata, e);
//			}
//		}
//	}
//}
//
//void ApplyDragOver(GLX::Object & src, GLX::Event & e)
//{
//	e.SetProperty(K32("accept"), New<Data::UInt8Property>(CheckValue<UInt8>(e, K32("accept"), true)));
//}

void TranslateDropExternal(VM::Context & context, GLX::Event & e)
{
	TranslateEventDefault(context, e);

	typedef ObjectOf < Array<WString> > Strings;

	if (auto pfiles = DynamicCast<Strings>(*e.QueryProperty<Reflex::Object>(GLX::kdata)))
	{
		auto bindings = context.program->bindings;

		auto string_array_t = bindings->GetTypeByRTTID(VM::ArrayOfStringTarg::kRTTID);

		auto strings = New<VM::ObjectArray>(context, string_array_t, bindings->string_t);

		auto & files = pfiles->value;

		auto ptr = strings->Extend<VM::String>(context, files.GetSize());

		for (auto & i : files) *ptr++ = New<VM::String>(i);

		e.SetProperty({ GLX::kdata, string_array_t->type_id }, strings);
	}
}

typedef Pair < FunctionPointer<void(VM::Context&,GLX::Event&)>,FunctionPointer<void(GLX::Object&,GLX::Event&)> > EventConverter;

struct AbstractEventHandler : public GLX::Object::Delegate
{
	static EventConverter GetEventConverterByID(UInt32 id)
	{
		switch (id)
		{
		case GLX::kKeyDown:
			return { &TranslateKeyDown, &ApplyEventNull };

		case GLX::kCharacter:
			return { &TranslateKeyCharacter, &ApplyEventNull };

		//case GLX::kDragOver:
		//	return { &TranslateDragOver, &ApplyDragOver};

		//case GLX::kDragDrop:
		//case GLX::kDragEnter:
		//case GLX::kDragLeave:
		//	return { &TranslateDragOver, &ApplyEventNull };

		case GLX::kDragDropReceiveExternal:
			return { &TranslateDropExternal, &ApplyEventNull };

		//case GLX::AbstractList::kListSelect:
		//	return { &TranslateListSelect, &ApplyListSelect };

		//case GLX::List::kListReorder:
		//	return { &TranslateEventDefault, &ApplyListSelect };

		default:
			return { &TranslateEventDefault, &ApplyEventNull };
		}
	}

	AbstractEventHandler(VM::Context & context, GLX::Object & target, UInt32 id, VM::Detail::FnObject & fn)
		: context(context),
		m_id(id),
		m_converters(GetEventConverterByID(id)),
		m_fn(fn)
	{
		target.SetDelegate(id, this);
	}

	VM::Context & context;

	const UInt32 m_id;

	const EventConverter m_converters;

	const Reference <VM::Detail::FnObject> m_fn;
};

struct EventHandlerVoid : public AbstractEventHandler
{
	using AbstractEventHandler::AbstractEventHandler;

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == m_id)
		{
			m_converters.a(context, e);

			VM::CallReturningVoid(context, m_fn);

			m_converters.b(src, e);

			return true;
		}

		return false;
	}
};

struct EventHandlerWithEventParam : public AbstractEventHandler
{
	using AbstractEventHandler::AbstractEventHandler;

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == m_id)
		{
			Reference retain(src);

			m_converters.a(context, e);

			VM::CallReturningVoid(context, m_fn, e);

			m_converters.b(src, e);

			return true;
		}

		return false;
	}
};

struct EventHandlerFullSignature : public AbstractEventHandler
{
	using AbstractEventHandler::AbstractEventHandler;

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		if (e.id == m_id)
		{
			Reference retain(src);

			m_converters.a(context, e);

			bool rtn = VM::CallReturningValue<bool>(context, m_fn, MakeTuple(&src, &e));

			m_converters.b(src, e);

			return rtn;
		}

		return false;
	}
};

struct EventHandlerGeneric : public AbstractEventHandler
{
	using AbstractEventHandler::AbstractEventHandler;

	virtual bool OnEvent(GLX::Object & src, GLX::Event & e) override
	{
		auto converters = GetEventConverterByID(e.id.value);

		Reference retain(src);

		converters.a(context, e);

		bool rtn = VM::CallReturningValue<bool>(context, m_fn, MakeTuple(&src, &e));

		converters.b(src, e);

		return rtn;
	}
};

REFLEX_END_INTERNAL

void Reflex::GLXVM::BindEvent(VM::Compiler::State & cstate, Key32 glx, VM::TypeRef dynamic_t, VM::TypeRef object_t, VM::TypeRef point_t)
{
	auto bindings = cstate.bindings;

	auto void_t = bindings->void_t;
	auto bool_t = bindings->bool_t;
	auto uint8_t = bindings->uint8_t;
	auto float32_t = bindings->float32_t;
	auto key32_t = bindings->key32_t;
	auto callback_t = bindings->callback_t;

	VM_BEGIN_ENUM(cstate, glx, "Transaction")
	BIND_ENUM(kTransactionStageBegin),
	BIND_ENUM(kTransactionStagePerform),
	BIND_ENUM(kTransactionStageEnd),
	BIND_ENUM(kTransactionStageCancel),
	VM_END_ENUM;


	VM::AddFunction(bindings, glx, "Emit", bool_t, { object_t, key32_t, dynamic_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,Key32,Data::PropertySet&);

		auto e = AutoRelease(REFLEX_CREATE(GLX::Event, args.b));

		for (auto & i : args.c.Iterate())
		{
			e->SetProperty(i.key, i.value);
		}

		VM_RTN(args.a.Emit(e));
	});


	auto event_t = VM::RegisterObject<GLX::Event>(bindings, glx, "Event");

	event_t->members = { VM_BIND_MEMBER(GLX::Event,id,cstate,key32_t) };

	VM::Detail::BindObjectMethod<&GLX::Object::Emit>(bindings, object_t, "Emit");

	VM::Detail::BindObjectMethod<&GLX::Object::ProcessEvent>(bindings, object_t, "ProcessEvent");


	AddMethod(bindings, "EnablePointerCapture", void_t, { event_t, bool_t, bool_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Event&, bool, bool);

		GLX::EnablePointerCapture(args.a, args.b, args.c);
	});

	AddFunction(bindings, kGLX, "GetPointerPosition", point_t, { object_t, event_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,GLX::Event&);

		VM_RTN(GLX::GetPointerPosition(args.a, args.b));
	});


	auto sender_event_callback_t = cstate.InstantiateTemplateType(VM::kFn, { uint8_t, object_t, event_t });

	auto event_callback_t = cstate.InstantiateTemplateType(VM::kFn, { void_t, event_t });

	AddMethod(bindings, VM::Compiler::opSet, void_t, { object_t, key32_t, callback_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,UInt32,VM::Detail::FnObject&);

		New<EventHandlerVoid>(context, args.a, args.b, args.c);

		args.a.SetProperty({ args.b, K32("Fn@void") }, args.c);
	});

	AddMethod(bindings, VM::Compiler::opSet, void_t, { object_t, key32_t, event_callback_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,UInt32,VM::Detail::FnObject&);

		New<EventHandlerWithEventParam>(context, args.a, args.b, args.c);

		args.a.SetProperty({ args.b, K32("Fn@(void,GLX::Event)") }, args.c);
	});

	AddMethod(bindings, VM::Compiler::opSet, void_t, { object_t, key32_t, sender_event_callback_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&,UInt32,VM::Detail::FnObject&);

		New<EventHandlerFullSignature>(context, args.a, args.b, args.c);

		args.a.SetProperty({ args.b, K32("Fn@(UInt8,GLX::Object,GLX::Event)") }, args.c);
	});

	AddMethod(bindings, "SetDelegate", void_t, {object_t, key32_t, sender_event_callback_t}, [](VM::Context & context)
	{
		VM_POP(GLX::Object&, UInt32, VM::Detail::FnObject&);

		New<EventHandlerGeneric>(context, args.a, args.b, args.c);
	});

	AddMethod(bindings, "ClearDelegate", void_t, { object_t, key32_t, sender_event_callback_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Object&, UInt32);

		args.a.ClearDelegate(args.b);
	});


	auto CreateListenerCallback = [](VM::Context & context)
	{
		const auto GetDragDropData = reinterpret_cast<decltype(&GLX::Core::Desktop::GetFocus)>(&GLX::Core::Desktop::GetDragDropData);

		const Tuple <Key32, TypeID, decltype(&GLX::Core::Desktop::GetFocus)> kNotifications[] =
		{
			{ K32("MouseOver"), K32("Fn@(void,GLX::Object"), &GLX::Core::Desktop::GetMouseOver },
			{ K32("Focus"), K32("Fn@(void,GLX::Object"), &GLX::Core::Desktop::GetFocus },
			{ K32("DragDropBegin"), K32("Fn@(void,Object"), GetDragDropData },
			{ K32("DragDropEnd"), K32("Fn@(void,Object"), GetDragDropData },
			{ K32("DragDropTarget"), K32("Fn@(void,GLX::Object"), &GLX::Core::Desktop::GetDragDropTarget }
		};

		VM_POP(Key32, VM::Detail::FnObject&);

		if (auto idx = Search<KeyCompare>(kNotifications, args.a))
		{
			auto [id, type_id, get_object] = kNotifications[idx.value];

			if (args.b.object_t->type_id == type_id)
			{
				VM_RTN(GLX::Core::desktop->CreateListener(GLX::Core::Desktop::Notification(idx.value), [&context, get_object, fn = AutoRelease(args.b)]()
				{
					REFLEX_ASSERT(Reflex::Detail::ContextScope::GetCurrent() == GLX::Core::desktop->GetContextID());

					context.Call(fn, ((*GLX::Core::desktop).*get_object)());
				}));

				return;
			}
		}

		VM_RTN(REFLEX_NULL(Reflex::Object));
	};


	auto object_callback_t = cstate.InstantiateTemplateType(VM::kFn, { void_t, object_t });

	AddFunction(bindings, glx, "CreateListener", bindings->object_t, { key32_t, object_callback_t }, CreateListenerCallback);


	auto property_callback_t = cstate.InstantiateTemplateType(VM::kFn, { void_t, bindings->object_t });

	AddFunction(bindings, glx, "CreateListener", bindings->object_t, { key32_t, property_callback_t }, CreateListenerCallback);


	auto animationclock_t = cstate.InstantiateTemplateType(VM::kFn, { void_t, float32_t });

	AddFunction(bindings, glx, "CreateAnimationClock", bindings->object_t, { animationclock_t }, [](VM::Context & context)
	{
		VM_POP1(VM::Detail::FnObject&);

		VM_RTN(GLX::Core::desktop->CreateAnimationClock([&context, fn = AutoRelease(arg)](Float32 delta)
		{
			REFLEX_ASSERT(Reflex::Detail::ContextScope::GetCurrent() == GLX::Core::desktop->GetContextID());

			context.Call(fn, delta);
		}));
	});

	AddFunction(bindings, VM::kGlobal, "CreatePeriodicClock", bindings->object_t, { float32_t, callback_t }, [](VM::Context & context)
	{
		struct UiClock : public VM::PeriodicClockList
		{
			using VM::PeriodicClockList::PeriodicClockList;

			virtual void OnBegin() override
			{
				renderer = GLX::Core::Context::IsActive() ? Null<System::Renderer>() : GLX::Core::g_renderer;	//GLX Dialog functions may be holding context alive
				
				renderer->BeginAccess();

				VM::PeriodicClockList::OnBegin();
			}

			virtual void OnEnd() override
			{
				VM::PeriodicClockList::OnEnd();

				renderer->EndAccess();
			}

			TRef <System::Renderer> renderer;
		};

		VM_POP(Float32,VM::Detail::FnObject&);

		auto list = VM::PeriodicClock::AcquireList<UiClock>(context);

		VM_RTN(REFLEX_CREATE(VM::PeriodicClockItem, list, args.a, args.a, args.b));
	});
}

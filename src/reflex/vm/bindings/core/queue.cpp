#include "../[require].h"




REFLEX_BEGIN_INTERNAL(Reflex::VM)

inline const Symbol kMap = { kGlobal, K32("Queue") };

template <class TYPE>
struct AbstractQueue :
	public Object,
	public Queue <TYPE,1>
{
	using Queue<TYPE,1>::m_has_client;


protected:

	AbstractQueue(VM::TypeRef queue_t)
	{
		Detail::FinaliseObject(*this, queue_t);
	}
};

typedef AbstractQueue <void*> AnyQueue;

constexpr CString::View kQueueReader = "QueueReader";
constexpr CString::View kQueueWriter = "QueueWriter";

struct AbstractQueueAccessor : public Object
{
	AbstractQueueAccessor(AnyQueue & queue, bool writer)
		: queue(queue),
		writer(writer)
	{
	}

	~AbstractQueueAccessor()
	{
		REFLEX_ATOMIC_WRITE(Cast<AnyQueue>(queue)->m_has_client[writer], false);
	}

	Reference <Object> queue;

	const bool writer;
};

template <class TYPE>
struct ValueQueue : public AbstractQueue <Data::Archive>
{
	static TypeRef Bind(Compiler::State & cstate, const CString::View & name, TypeRef value_t);

	ValueQueue(Context & context, VM::TypeRef queue_t)
		: AbstractMap<TYPE,Data::Archive,false>(context, queue_t),
		m_null(queue_t->params)
	{
	}

	Data::Archive m_null;
};

struct ObjectQueue : public AbstractQueue <Object*>
{
	static TypeRef Bind(Compiler::State & cstate, const CString::View & name, TypeRef value_t);

	ObjectQueue(Context & context, TypeRef queue_t)
		: AbstractQueue<Object*>(queue_t)
	{
	}

	~ObjectQueue()
	{
		Object * remaining;

		while (Pop(remaining)) Release(remaining);
	}

	static TRef <ObjectQueue> Create(Context & context, VM::TypeRef queue_t, UInt size)
	{
		auto bytes = UInt(size * sizeof(void*));

		auto t = Reflex::Detail::Constructor<ObjectQueue>::CreateVariableSize(g_default_allocator, bytes, context, queue_t);

		t->SetSize(size);

		MemClear(t->GetData().data, bytes);

		return t;
	}
};

//template <class TYPE> TypeRef ValueQueue<TYPE>::Bind(Compiler::State & cstate, const CString::View & name, TypeRef value_t)
//{
//	auto bindings = cstate.bindings;
//
//	auto bool_t = bindings->bool_t;
//
//	auto archive_t = bindings->archive_t;
//
//
//	UInt32 size = value_t->size;
//
//	auto queue_t = Detail::CreateObjectType(bindings, Detail::AcquireObjectType(cstate, REFLEX_OBJECT_TYPE(Object), kGlobal, name), kGlobal, name, true, true);
//
//
//	Detail::SetTypeCtr(queue_t, value_t->params, [](VM_CTR_PARAMS) -> Object &
//	{
//		return *REFLEX_CREATE(ValueQueue, context, type);
//	});
//
//	Data::Archive map_param = Data::Pack(queue_t);
//
//
//	return queue_t;
//}

TypeRef ObjectQueue::Bind(Compiler::State & cstate, const CString::View & name, TypeRef value_t)
{
	auto bindings = cstate.bindings;

	auto bool_t = bindings->bool_t;


	auto queue_t = Detail::CreateObjectType(bindings, Detail::AcquireObjectType(cstate, REFLEX_OBJECT_TYPE(Object), kGlobal, name), kGlobal, name, true, true);


	
	Data::Archive queue_param = Data::Pack(queue_t);

	AddConstructor(bindings, queue_t, { bindings->int32_t }, queue_param, [](Context & context)
	{
		auto queue_t = VM::ReadFunctionData<TypeRef>(context);

		VM_POP1(UInt32);

		auto t = ObjectQueue::Create(context, queue_t, RoundUpPow2<UInt>(arg, 4));

		VM_RTN(t);
	});

	Detail::SetTypeCtr(queue_t, Data::Pack(value_t), [](VM_CTR_PARAMS) -> Object &
	{
		auto t = ObjectQueue::Create(context, type, 4);

		t->m_has_client[false] = true;

		t->m_has_client[true] = true;

		return *t;
	});



	auto writer_name = AcquireStaticString(cstate, Detail::MakeTemplateName(cstate, kQueueWriter, { value_t }));

	auto queue_writer_t = Detail::CreateObjectType(bindings, Detail::AcquireObjectType(cstate, REFLEX_OBJECT_TYPE(Object), kGlobal, writer_name), kGlobal, writer_name, true, false);

	cstate.RegisterTemplateType(kGlobal, kQueueWriter, 1, false, {}, 0);

	Detail::SetTypeCtr(queue_writer_t, queue_param, [](VM_CTR_PARAMS) -> Object &
	{
		auto queue = ObjectQueue::Create(context, Data::Unpack<TypeRef>(type->params), 1);

		Retain(REFLEX_NULL(Object));

		queue->Push(&REFLEX_NULL(Object));

		return Detail::FinaliseObject(*REFLEX_CREATE(AbstractQueueAccessor, *Reinterpret<AnyQueue>(queue.Adr()), true), type);
	});

	AddConstructor(bindings, queue_writer_t, { queue_t }, Data::Pack(queue_writer_t), [](Context & context)
	{
		auto queue_writer_t = ReadFunctionData<TypeRef>(context);

		VM_POP1(ObjectQueue*);

		if (!REFLEX_ATOMIC_SET_FILTERED(arg->m_has_client[true], true))
		{
			arg = Cast<ObjectQueue>(Cast<AbstractQueueAccessor>(context.GetNullByRTTID(queue_writer_t->type_id, 0))->queue.Adr());
		}

		auto & itr = Detail::FinaliseObject(*REFLEX_CREATE(AbstractQueueAccessor, *Reinterpret<AnyQueue>(arg), false), queue_writer_t);

		VM_RTN(itr);
	});

	AddMethod(bindings, "Push", bool_t, { queue_writer_t, value_t }, [](Context & context)
	{
		VM_POP(AbstractQueueAccessor&,Object*);

		TRef queue = Cast<ObjectQueue>(args.a.queue);

		Retain(args.b);

		bool c = queue->Push(args.b);

		if (!c) Release(args.b);

		VM_RTN(c);
	});


	auto reader_name = AcquireStaticString(cstate, Detail::MakeTemplateName(cstate, kQueueReader, { value_t }));

	auto queue_reader_t = Detail::CreateObjectType(bindings, Detail::AcquireObjectType(cstate, REFLEX_OBJECT_TYPE(Object), kGlobal, reader_name), kGlobal, reader_name, true, false);

	cstate.RegisterTemplateType(kGlobal, kQueueReader, 1, false, {}, 0);

	Detail::SetTypeCtr(queue_reader_t, queue_param, [](VM_CTR_PARAMS) -> Object &
	{
		auto queue = ObjectQueue::Create(context, Data::Unpack<TypeRef>(type->params), 1);

		return Detail::FinaliseObject(*REFLEX_CREATE(AbstractQueueAccessor, *Reinterpret<AnyQueue>(queue.Adr()), false), type);
	});

	AddConstructor(bindings, queue_reader_t, { queue_t }, Data::Pack(MakeTuple(UIntNative(queue_t->type_id), queue_reader_t)), [](Context & context)
	{
		auto [queue_rttid,queue_reader_t] = ReadFunctionData<Pair<UIntNative,TypeRef>>(context);

		VM_POP1(ObjectQueue*);

		if (!REFLEX_ATOMIC_SET_FILTERED(arg->m_has_client[false], true))
		{
			arg = Cast<ObjectQueue>(context.GetNullByRTTID(TypeID(queue_rttid), 0));
		}

		auto & itr = Detail::FinaliseObject(*REFLEX_CREATE(AbstractQueueAccessor, *Reinterpret<AnyQueue>(arg), false), queue_reader_t);

		VM_RTN(itr);
	});

	AddMethod(bindings, "Pop", bool_t, { queue_reader_t, ByRef(value_t) }, [](Context & context)
	{
		VM_POP(AbstractQueueAccessor&,Object*&);

		TRef queue = Cast<ObjectQueue>(args.a.queue);

		Object * fetch;
		
		bool c = queue->Pop(fetch);

		if (c) Reflex::Detail::SetReferenceCountedPointer(args.b, fetch);

		VM_RTN(c);
	});

	AddMethod(bindings, "Pop", value_t, { queue_reader_t }, {}, Data::Pack(value_t->type_id), [](Context & context)
	{
		auto value_rttid = ReadFunctionData<TypeID>(context);

		VM_POP1(AbstractQueueAccessor&);

		TRef queue = Cast<ObjectQueue>(arg.queue);

		Object * fetch;

		if (queue->Pop(fetch))
		{
			Release(fetch);
		}
		else
		{
			fetch = context.GetNullByRTTID(value_rttid, 0);
		}

		VM_RTN(fetch);
	});

	return queue_t;
}

REFLEX_END_INTERNAL

const Reflex::VM::Module Reflex::VM::gCoreQueue("Core > Queue", { &gCore }, kMaxUInt8, [](Compiler::State & cstate, UInt8 context, Object & clientdata)
{
	cstate.RegisterTemplateType(kGlobal, "Queue", 1, false, {}, [](Compiler::State & cstate, const Compiler::State::ClientData clientdata, Key32 ns, const CString::View & name, const ArrayView <TypeRef> & targs)
	{
		auto value_t = targs[0];

		if (value_t->IsObject())
		{
			bool ok = value_t->flags.GetWord() & (MakeBit(Type::kFlagNonCircular) | MakeBit(Type::kFlagThreadsafe));

			if (ok)
			{
				return ObjectQueue::Bind(cstate, name, value_t);
			}
		}

		return TypeRef(0);
	});
});

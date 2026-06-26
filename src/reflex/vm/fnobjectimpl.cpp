#include "fnobjectimpl.h"
#include "compiler/compilerimpl.h"




//
//types

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

template <ContainerType TYPE> using ContainerCircularType = typename Selector<(TYPE > kContainerTypeNonCircularObjects),Circular,Dummy>::type;

template <ContainerType TYPE>
struct FnObjectImpl::Captures
{
	struct Retainer
	{
		static void Callback(UInt16 contextid, Object * object) { Retain(object); }
	};

	Captures()
		: n(0)
	{
		if constexpr (TYPE > kContainerTypeNull) layout.Append(Data::Pack(UInt32(0)));
	}

	Captures(UInt n, const LayoutTemplate & layouttemplate)
		: n(UInt16(n))
	{
		if constexpr (TYPE > kContainerTypeNull)
		{
			layouttemplate.PackLayout(layout);

			Enumerate<Retainer>();
		}
	}

	Captures(const Captures & captures)
		: n(captures.n),
		layout(captures.layout)
	{
		Enumerate<Retainer>();
	}

	Captures(Captures && rhs) = delete;

	Captures(const Captures & captures, Context & to, bool & ok)
		: Captures(captures)
	{
		ok = true;

		if constexpr (TYPE == kContainerTypeThreadSafeObjects)
		{
			Enumerate<Retainer>();
		}
		else if constexpr (TYPE > kContainerTypeThreadSafeObjects)
		{
			auto & layout = *Reinterpret<Layout>(Captures::layout.GetData());

			auto base = Reinterpret<UInt8>(&layout) + 4;

			auto objects = Reinterpret<LayoutTemplate::ObjectInfo>(base + layout.size);

			REFLEX_LOOP_PTR(objects, itr, layout.num_object)
			{
				auto & pobject = *Reinterpret<Pointer>(base + itr->a);

				if (auto copy = CrossContextCopy(*pobject, to, false))
				{
					pobject = copy;
				}
				else
				{
					ok = false;
				}

				Retain(*pobject);
			}
		}
	}

	~Captures()
	{
		struct Releaser
		{
			static void Callback(UInt16 contextid, Object * object) { Release(object); }
		};

		Enumerate<Releaser>();
	}

	REFLEX_INLINE void Load(Stack & stack) const
	{
		if constexpr (TYPE > kContainerTypeNull)
		{
			auto & layout = *Reinterpret<Layout>(this->layout.GetData());

			auto pcaptures = Extend<kAllocateNone>(stack, layout.size).data;

			auto base = Reinterpret<UInt8>(&layout) + 4;

			MemCopy(base, pcaptures, layout.size);
		}
	}

	void ReleaseData(UInt16 contextid)
	{
		if constexpr (TYPE > kContainerTypeNonCircularObjects)
		{
			struct DataReleaser
			{
				static void Callback(UInt16 contextid, Object * & object)
				{
					ReleaseContextCirculars(contextid, *object);

					Reflex::Detail::SetReferenceCountedPointer(object, &REFLEX_NULL(Object));
				}
			};

			Enumerate<DataReleaser>(contextid);
		}
	}

	UInt16 GetByteSize() const
	{
		if constexpr (TYPE > kContainerTypeNull)
		{
			return Reinterpret<Layout>(Captures::layout.GetData())->size;
		}
		else
		{
			return {};
		}
	}

	const UInt16 n;



private:

	template <class ACTION> REFLEX_INLINE void Enumerate(UInt16 contextid = 0)
	{
		if constexpr (TYPE > kContainerTypeValues)
		{
			auto & layout = *Reinterpret<Layout>(this->layout.GetData());

			auto base = Reinterpret<UInt8>(&layout) + 4;

			auto objects = Reinterpret<LayoutTemplate::ObjectInfo>(base + layout.size);

			REFLEX_LOOP_PTR(objects, itr, layout.num_object)
			{
				ACTION::Callback(contextid, *Reinterpret<Pointer>(base + itr->a));
			}
		}
	}

	typename Selector<(TYPE > kContainerTypeNull),Data::Archive,NullType>::type layout;
};

struct NullFnObject : public FnObjectImpl
{
	NullFnObject(Context & context, const Type & fnobject_t);

	virtual void CallInternal(ContextImpl & context) const override;

	virtual void Invoke(Context & context) const override;

	virtual FnObjectImpl * CrossContextClone(Context & to, const Type & type, bool mt) override { return this; }

	template <bool INTERNAL> void Call(ContextImpl & context) const;
};

template <ContainerType TYPE>
struct ScriptFnObject :
	public FnObjectImpl,
	public ContainerCircularType<TYPE>
{
	ScriptFnObject(Context & context, const Type & fnobject_t, const ScriptFunction & scriptfunction, const Pair <UInt,LayoutTemplate> & captures);

	virtual void CallInternal(ContextImpl & context) const override;

	virtual void Invoke(Context & context) const override;

	virtual FnObjectImpl * CrossContextClone(Context & to, const Type & type, bool mt) override;

	virtual void OnReleaseData() override;

	Captures <TYPE> captures;
};

template <ContainerType TYPE>
struct ContextSafeScriptFnObject :
	public FnObjectImpl,
	public ContainerCircularType<TYPE>
{
	ContextSafeScriptFnObject(Context & context, Context & fncontext, const Type & fnobject_t, const ScriptFunction & scriptfunction, const Captures <TYPE> & captures = {});

	virtual void CallInternal(ContextImpl & context) const override;

	virtual void Invoke(Context & context) const override;

	virtual FnObjectImpl * CrossContextClone(Context & to, const Type & type, bool mt) override
	{
		return this;
	}

	virtual void OnReleaseData() override;

	Reference <Context> m_fncontext;

	Captures <TYPE> captures;
};

//struct ScriptExternalFnObject :
//	public FnObjectImpl,
//	public Circular
//{
//	ScriptExternalFnObject(Context & context, const Type & fnobject_t, const ExternalFunction & externalfunction, const Pair <UInt,LayoutTemplate> & captures)
//		: FnObjectImpl(context, fnobject_t, &externalfunction),
//		Circular(context, this),
//		captures(captures.a, captures.b)
//	{
//	}
//
//	virtual void CallInternal(ContextImpl & context) const override
//	{
//		captures.Load(context.stack);
//
//		auto & externalfn = *Cast<ExternalFunction>(this->pfunction);
//
//		if (externalfn.fastcall)
//		{
//			auto fn = reinterpret_cast<const ExternalFunctionPtr&>(externalfn.externalfnptr);
//
//			(fn)(context);
//		}
//		else
//		{
//			reinterpret_cast<const ExternalFunctionPtr&>(externalfn.externalfnptr)(context, externalfn);
//		}
//	}
//
//	virtual void Invoke(Context & context) const override
//	{
//		captures.Load(context.stack);
//
//		auto & externalfn = *Cast<ExternalFunction>(this->pfunction);
//
//		if (externalfn.fastcall)
//		{
//			auto fn = reinterpret_cast<const ExternalFunctionPtr&>(externalfn.externalfnptr);
//
//			(fn)(context);
//		}
//		else
//		{
//			reinterpret_cast<const ExternalFunctionPtr&>(externalfn.externalfnptr)(context, externalfn);
//		}
//	}
//
//	virtual FnObjectImpl * CrossContextClone(Context & to, const Type & type, bool mt) override
//	{
//		return 0;
//	}
//
//	virtual void OnReleaseData() override
//	{
//		captures.ReleaseData(GetContextID());
//	}
//
//	Captures <kContainerTypeCircularObjects> captures;
//};

struct ExternalFnObject :
	public FnObjectImpl,	//this is for c++ defined function objects, so can not have captures
	public Circular
{
	ExternalFnObject(Context & context, const Type & fnobject_t, TypeID object_t, Object & object, const FunctionPointer <void(Object&,Context&)> & function);

	virtual void CallInternal(ContextImpl & context) const override;

	virtual void Invoke(Context & context) const override;

	virtual FnObjectImpl * CrossContextClone(Context & to, const Type & type, bool mt) override;

	virtual void OnReleaseData() override;


	TypeID m_object_t;

	Reference <Object> m_object;

	FunctionPointer <void(Object&,Context&)> m_function;
};

REFLEX_END_INTERNAL




//
//defs

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

template <bool MT> Object * FnObjectImpl::ContextCopyFnObject(Context & to, Object & fnobject, TypeRef type, bool mt)
{
	auto self = Cast<FnObjectImpl>(fnobject);

	if constexpr (MT)
	{
		if (!(self->pfunction->flags2 & Function::kFlagsMt)) return 0;
	}

	if (!(self->context.program->bindings->context_flags & kContextFlagUi))
	{
		if (to.program->bindings->GetTypeByRTTID(type->type_id))
		{
			return self->CrossContextClone(to, self->type, MT);
		}
	}

	return 0;
}

FnObjectImpl::Type::Type(Bindings & bindings, CString::View name, bool noncircular, TypeRef rtn_t, const ArrayView <TypeRef> & argsv)
	: VM::Type(bindings, VM::AcquireObjectType(REFLEX_OBJECT_TYPE(Object), name, {}), kGlobal, name, noncircular, false),
	rtn_t(rtn_t),
	args(argsv)
{
	SetTypeCtr(this, {}, [](VM_CTR_PARAMS) -> Object&
	{
		return FinaliseObject(*REFLEX_CREATE(NullFnObject, context, *Cast<FnObjectImpl::Type>(type)), type);
	});

	Type::contextcopyfn[false] = &FnObjectImpl::ContextCopyFnObject<false>;
	Type::contextcopyfn[true] = &FnObjectImpl::ContextCopyFnObject<true>;

	Array <Argument> args;

	args.Allocate(argsv.size + 1);

	args.Push(this);

	UInt16 size = 0;

	for (auto & i : argsv)
	{
		size += i->size;

		args.Push(i);
	}

	Type::arguments_size = size;

	Tuple <UInt8,UInt8,UInt8,UInt8> param32 = { 0, rtn_t->size, 0, 0 };

	bindings.RegisterIntrinsic(kGlobal, Compiler::opInvoke, OPCODE(CallFnObject), size + sizeof(Object*), Reinterpret<UInt32>(param32), rtn_t, args);
}

REFLEX_INLINE FnObjectImpl::FnObjectImpl(Context & context, const Type & type, const Function * pfunction)
	: FnObject(context),
	type(type),
	pfunction(pfunction)
{
	FinaliseObject(*this, &type);
}

template <ContainerType TYPE> REFLEX_INLINE ScriptFnObject<TYPE>::ScriptFnObject(Context & context, const Type & fnobject_t, const ScriptFunction & scriptfunction, const Pair <UInt,LayoutTemplate> & captures)
	: FnObjectImpl(context, fnobject_t, &scriptfunction),
	ContainerCircularType<TYPE>(context, *this),
	captures(captures.a, captures.b)
{
}

template <ContainerType TYPE> void ScriptFnObject<TYPE>::CallInternal(ContextImpl & context) const
{
	auto & current = context.instructionptr;

	auto & fn = Cast<ScriptFunction>(*pfunction);

	REFLEX_ASSERT(&this->context == &context);

	captures.Load(context.stack);

	Push(context.stack, CurrentPositionStore({ current, sizeof(void *), UInt16(context.stack.GetSize()) }));	//disabled CONTEXT SWITCHING

	ProgramImpl::PushStackFrame<true>(context, *fn.layout);

	current = fn.instructions.GetData() - 1;

	REFLEX_ASSERT(GetRetainCount());
}

template <ContainerType TYPE> void ScriptFnObject<TYPE>::Invoke(Context & context) const
{
	auto & fn = Cast<ScriptFunction>(*pfunction);

	REFLEX_ASSERT(&this->context == &context);

	REFLEX_ASSERT(!DataReleased());

	captures.Load(context.stack);

	Cast<ContextImpl>(context)->DoCall(fn);
}

template <ContainerType TYPE> void ScriptFnObject<TYPE>::OnReleaseData()
{
	captures.ReleaseData(GetContextID());
}

template <ContainerType TYPE> FnObjectImpl * ScriptFnObject<TYPE>::CrossContextClone(Context & to, const Type & type, bool mt)
{
	return REFLEX_CREATE(ContextSafeScriptFnObject<TYPE>, to, context, type, *Cast<ScriptFunction>(this->pfunction), captures);
}

template <ContainerType TYPE> REFLEX_INLINE ContextSafeScriptFnObject<TYPE>::ContextSafeScriptFnObject(Context & context, Context & fncontext, const Type & type, const ScriptFunction & scriptfunction, const Captures <TYPE> & captures)
	: FnObjectImpl(fncontext, type, &scriptfunction),
	ContainerCircularType<TYPE>(context, *this),
	m_fncontext(fncontext),
	captures(captures)
{
}

template <ContainerType TYPE> REFLEX_INLINE void ContextSafeScriptFnObject<TYPE>::CallInternal(ContextImpl & context) const
{
	REFLEX_ASSERT(!DataReleased());

	auto & current = context.instructionptr;

	auto & fn = Cast<ScriptFunction>(*pfunction);

	auto & fncontext = FnObject::context;

	if (&fncontext == &context)
	{
		captures.Load(context.stack);

		Push(context.stack, CurrentPositionStore({ current, sizeof(void *), UInt16(context.stack.GetSize()) }));	//disabled CONTEXT SWITCHING

		ProgramImpl::PushStackFrame<true>(context, *fn.layout);

		current = fn.instructions.GetData() - 1;
	}
	else
	{
		CrossContextCall<true>(context, *this, captures);
	}

	REFLEX_ASSERT(GetRetainCount());
}

template <ContainerType TYPE> void ContextSafeScriptFnObject<TYPE>::Invoke(Context & context) const
{
	REFLEX_ASSERT(!DataReleased());

	auto & fn = Cast<ScriptFunction>(*pfunction);

	auto & fncontext = FnObject::context;

	if (&fncontext == &context)
	{
		captures.Load(context.stack);

		Cast<ContextImpl>(context)->DoCall(fn);
	}
	else
	{
		CrossContextCall<false>(context, *this, captures);
	}

	REFLEX_ASSERT(GetRetainCount());
}

template <ContainerType TYPE> void ContextSafeScriptFnObject<TYPE>::OnReleaseData()
{
	captures.ReleaseData(GetContextID());
}

REFLEX_INLINE NullFnObject::NullFnObject(Context & context, const Type & fnobject_t)
	: FnObjectImpl(context, fnobject_t, 0)
{
}

REFLEX_INLINE ExternalFnObject::ExternalFnObject(Context & context, const Type & fnobject_t, TypeID object_t, Object & object, const FunctionPointer <void(Object&,Context&)> & function)
	: FnObjectImpl(context, fnobject_t, 0),
	Circular(context, this),
	m_object_t(object_t),
	m_object(object),
	m_function(function)
{
}

void ExternalFnObject::CallInternal(ContextImpl & context) const
{
	REFLEX_ASSERT(!DataReleased());

	REFLEX_ASSERT(&this->context == &context);

	m_function(m_object, FnObject::context);

	auto rtn_size = type.rtn_t->size;

	auto & stack = context.stack;

	auto top = stack.GetData() + stack.GetSize();

	auto prtn = top - rtn_size;

	MemCopy(prtn, prtn - sizeof(FnObject*), rtn_size);

	stack.Shrink(sizeof(FnObject *));
}

void ExternalFnObject::Invoke(Context & context) const
{
	REFLEX_ASSERT(!DataReleased());

	REFLEX_ASSERT(&this->context == &context);

	m_function(m_object, FnObject::context);
}

FnObjectImpl * ExternalFnObject::CrossContextClone(Context & to, const Type & type, bool mt)
{
	REFLEX_ASSERT(!DataReleased());

	if (auto to_t = to.program->bindings->GetTypeByRTTID(m_object_t))
	{
		if (auto copy = to_t->contextcopyfn[false](to, m_object, to_t, false))
		{
			return REFLEX_CREATE(ExternalFnObject, to, type, m_object_t, *copy, m_function);
		}
	}

	return 0;
}

void ExternalFnObject::OnReleaseData()
{
	m_object = RemoveConst(type);

	m_function = [](Object & object, Context & context)
	{
		auto & stack = context.stack;

		auto type = Cast<Type>(object);

		stack.Shrink(type->arguments_size);

		auto rtn_t = type->rtn_t;

		if (rtn_t->IsObject())
		{
			VM_RTN(VM::Detail::GetNull(context, rtn_t));
		}
		else
		{
			stack.Append(rtn_t->params);
		}
	};
}

void NullFnObject::CallInternal(ContextImpl & context) const
{
	Call<true>(Cast<ContextImpl>(context));
}

void NullFnObject::Invoke(Context & context) const
{
	Call<false>(Cast<ContextImpl>(context));
}

template <bool INTERNAL> REFLEX_INLINE void NullFnObject::Call(ContextImpl & context) const
{
	auto & stack = context.stack;

	auto args = Detail::Pop(stack, type.arguments_size);

	for (auto & i : type.args)
	{
		args += i->size;
	}

	if (INTERNAL) stack.Shrink(sizeof(FnObject *));	//this woul normally be done by program OPCODE(CallReturn), but we are skipping that

	if (type.rtn_t->IsObject())
	{
		Push(stack, VM::Detail::GetNull(context, type.rtn_t));
	}
	else
	{
		stack.Append(type.rtn_t->params);
	}
}

TRef <FnObjectImpl> FnObjectImpl::Create(Context & context, const Type & type, ContainerType container, const Function & function, const Pair <UInt,LayoutTemplate> & captures)
{
	if (function.type == Function::kTypeScript)
	{
		switch (container)
		{
		case kContainerTypeValues:
			return REFLEX_CREATE(ScriptFnObject<kContainerTypeValues>, context, type, Cast<ScriptFunction>(function), captures);

		case kContainerTypeThreadSafeObjects:
			return REFLEX_CREATE(ScriptFnObject<kContainerTypeThreadSafeObjects>, context, type, Cast<ScriptFunction>(function), captures);

		case kContainerTypeNonCircularObjects:
			return REFLEX_CREATE(ScriptFnObject<kContainerTypeNonCircularObjects>, context, type, Cast<ScriptFunction>(function), captures);

		case kContainerTypeCircularObjects:
			return REFLEX_CREATE(ScriptFnObject<kContainerTypeCircularObjects>, context, type, Cast<ScriptFunction>(function), captures);

		default:
			return REFLEX_CREATE(ScriptFnObject<kContainerTypeNull>, context, type, Cast<ScriptFunction>(function), captures);
		}
	}
	//else if (function.type == Function::kTypeExternal)
	//{
	//	return REFLEX_CREATE(ScriptExternalFnObject, context, type, Cast<ExternalFunction>(function), captures);
	//}
	else
	{
		return REFLEX_CREATE(NullFnObject, context, type);
	}
}

TRef <FnObjectImpl> FnObjectImpl::CreateContextSafe(Context & ownercontext, const Type & type, Context & fncontext, const ScriptFunction & function)
{
	return REFLEX_CREATE(ContextSafeScriptFnObject<kContainerTypeNull>, ownercontext, fncontext, type, function);
}

template <bool INTERNAL, ContainerType TYPE> void FnObjectImpl::CrossContextCall(Context & context, const FnObjectImpl & fnobject, const Captures <TYPE> & captures)
{
	REFLEX_ASSERT(bool(TYPE) == bool(captures.n));

	auto & fncontext = fnobject.context;

	auto & stack = fncontext.stack;

	auto & fn = Cast<ScriptFunction>(*fnobject.pfunction);

	auto & rtn_t = *fn.rtn.type;

	auto stack_args_size = fnobject.type.arguments_size;

	REFLEX_ASSERT(stack_args_size + captures.GetByteSize() == fn.arguments_size);

	auto args = Detail::Pop(context.stack, stack_args_size);

	if (INTERNAL) context.stack.Shrink(sizeof(FnObject *));

	if (!fnobject.DataReleased())
	{
		//TODO captures must be previously MT copied

		//if (false && K32("argsarevalues"))
		//{
		//	//fncontext.stack.Append(Data::Archive::View(args, argssize));
		//}
		//else
		{
			for (auto & i : ReverseSplice<false>(fn.args, (TYPE ? captures.n : 0)).a)
			{
				auto & type = *i.type;

				if (type.IsObject())
				{
					auto & source = **Reinterpret<Object *>(args);

					if (auto copy = CrossContextCopy(source, fncontext, false))
					{
						Push(stack, copy);
					}
					else if (auto null = context.GetNullByRTTID(type.type_id, 0))
					{
						Push(stack, null);
					}
					else
					{
						REFLEX_ASSERT(false);
					}
				}
				else
				{
					stack.Append({ args, type.size });
				}

				args += type.size;
			}
		}

		fnobject(fncontext);

		if (rtn_t.IsObject())
		{
			auto & rtn = Pop<Object&>(stack);

			if (auto copy = CrossContextCopy(rtn, context, false))
			{
				VM_RTN(copy);
			}
			else
			{
				auto null = context.GetNullByRTTID(rtn_t.type_id, &REFLEX_NULL(Object));

				VM_RTN(null);
			}
		}
		else
		{
			auto rtnsize = rtn_t.size;

			context.stack.Append({ VM::Detail::Pop(fncontext.stack, rtnsize), rtnsize });
		}
	}
	else
	{
		stack.Shrink(fn.arguments_size);

		if (rtn_t.IsObject())
		{
			auto null = context.GetNullByRTTID(rtn_t.type_id, &REFLEX_NULL(Object));

			VM_RTN(null);
		}
		else
		{
			context.stack.Append(rtn_t.params);
		}
	}
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::VM::Detail::FnObject> Reflex::VM::Detail::FnObject::CreateExternalMethod(Context & context, TypeRef fnobject_t, TypeID object_t, TRef <Object> object, const FunctionPointer <void(Object&,Context&)> & function)
{
	return REFLEX_CREATE(ExternalFnObject, context, *Cast<FnObjectImpl::Type>(fnobject_t), object_t, object, function);
}

Reflex::VM::TypeRef Reflex::VM::Detail::FnObject::RegisterType(Bindings & bindings, CString::View name, const Array<TypeRef>::View & targs)
{
	auto types = Splice(targs, 1);

	auto void_t = bindings.void_t;

	for (auto & i : types.b)
	{
		if (i == void_t)
		{
			return 0;
		}
	}

	return REFLEX_CREATE(FnObjectImpl::Type, bindings, name, false, types.a.GetFirst(), types.b);
}

Reflex::VM::Detail::FnObject::FnObject(Context & context)
	: context(context)
{
}

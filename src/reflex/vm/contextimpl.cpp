#include "programimpl.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

constexpr UInt64 kEmptyStackMarker = K64("EmptyStack");

UInt16 st_contextthreadcounter = 1;

ContextImpl::~ContextImpl()
{
	Scope scope(*this);

	REFLEX_ASSERT(m_initalised.GetWord() == 3);

	REFLEX_ASSERT(m_global_stackframe == m_current_stackframe);

	ProgramImpl::PopStackFrame<true,false>(*this, *Cast<ProgramImpl>(program)->global_layout);

	if (auto n = GetNumItem())
	{
		auto contextid = GetContextID();
	
		UInt nstep = 0;

		while (auto trackingtoken = GetFirst())
		{
			auto & owner = trackingtoken->circular.object;

			Reference <Object> retain(owner);

			trackingtoken->Detach();

			REFLEX_ASSERT(contextid == owner.GetContextID());	//if its a circular it should belong to this context

			if (contextid == owner.GetContextID())
			{
				owner.ReleaseData();
			}

			nstep++;
		}

		if constexpr (REFLEX_DEBUG) output.Log(Join("Cleaned up ", ToCString(n), " remaining objects in ", ToCString(nstep), " steps"));
	}

	if constexpr (REFLEX_DEBUG)
	{
		REFLEX_ASSERT(!m_stackframes);

		REFLEX_ASSERT(!stack);

		UInt maxsize = 0;

		if (auto ptr = Reinterpret<UInt64>(stack.GetData()))
		{
			auto size = 8192 / 8;

			REFLEX_LOOP_PTR(ptr, p, size)
			{
				if (*p == kEmptyStackMarker)
				{
					maxsize = UInt((p - ptr) * 8);

					break;
				}
			}
		}

		m_stackframes.SetSize(m_stackframes.GetCapacity());

		auto maxframes = Search(m_stackframes, ToPointer<UInt8>(Reinterpret<UIntNative>(kEmptyStackMarker))).value;

		output.Log("max stack:", maxsize, maxframes);
	}

	PropertySet::Clear();

	Release(program);
}

Object * ContextImpl::GetNullByRTTID(UInt32 type_id, Object * fallback) const
{
	typedef Sequence < TypeID, Object* > Opaque;

	return *Reinterpret<Opaque>(m_tid2null).SearchValue(type_id, &fallback);
}

#define OP(CODE) case OPCODE(CODE): ProgramImpl::CODE(*this, current); break;

REFLEX_INLINE bool ContextImpl::Run(UInt stackframidx, const ScriptFunction * fn)
{
	auto & current = Context::instructionptr;

	//auto stackframidx = m_stackframes.GetSize();

	while (true)
	{
		switch (current->opcode)
		{
		#include "../../../include/reflex/vm/detail/[opcodes].h"

		case OPCODE(Abort):
			{
				auto stackframes = Splice(m_stackframes, stackframidx);

				REFLEX_RLOOP_PTR(stackframes.b.data, itr, stackframes.b.size)
				{
					auto stackframe = *itr;

					auto & layout = **Reinterpret<Layout*>(stackframe - sizeof(void*));

					ProgramImpl::PopStackFrame<true, true>(*this, layout);
				}

				if (fn && stackframes.b)
				{
					auto caller = Pop<CurrentPositionStore>(stack);

					auto size = caller.b;

					size += fn->arguments_size;

					stack.Shrink(size);	//pop[call_instruction.opcode == OPCODE(CallFnObject)]);

					auto rtn_t = fn->rtn.type;

					if (rtn_t->IsObject())
					{
						auto & context = *this;

						VM_RTN(VM::Detail::GetNull(context, rtn_t));
					}
					else
					{
						stack.Append(rtn_t->params);
					}
				}
			}
			return true;

		case OPCODE(End):
			REFLEX_ASSERT(m_stackframes.GetSize() == stackframidx);
			return true;

		case kNumOpcode:
			REFLEX_ASSERT(false);
			return false;
		};

		current++;
	}

	return false;
}

#undef OP

void ContextImpl::InitialiseNulls(const Program & exe)
{
	REFLEX_ASSERT(!m_initalised);

	REFLEX_ASSERT(!program);

	REFLEX_ASSERT(Scope::GetCurrent() == this);


	Reflex::Detail::SetReferenceCountedPointer(const_cast<ConstTRef<Program>&>(program), exe);

	auto executableimpl = Cast<ProgramImpl>(exe);

	m_executable = executableimpl;

	Context::instructionptr = executableimpl->global_instructions.GetData();

	
	//Scope scope(*this);

	stack.SetSize(16384 * 1);		//WORKAROUND -> Can crash if resizing, see PushLocalValue

	auto ptr = Reinterpret<UInt64>(stack.GetData());

	auto size = stack.GetSize() / 8;

	REFLEX_LOOP_PTR(ptr, p, size) *p = kEmptyStackMarker;

	stack.Clear();

	m_return_buffer.Allocate(256);


	m_stackframes.SetSize(256);

	m_stackframes.Fill(ToPointer<UInt8>(Reinterpret<UIntNative>(kEmptyStackMarker)));

	m_stackframes.Clear();



	auto alltypes = executableimpl->bindings->GetTypes(Reinterpret<Symbol>(UInt64(0)), kMaxUInt64);

	Sequence <UInt,TypeRef> sorted;

	sorted.Allocate(alltypes.GetSize());

	for (auto & i : alltypes)
	{
		if (i.value->object_t) sorted.Insert(i.value->tidx, i.value);
	}

	m_tid2null.Allocate(alltypes.GetSize());

	for (auto & i : sorted)
	{
		auto & type = *i.value;

		REFLEX_ASSERT(type.null);

		auto type_id = type.type_id;

		if (type.null != &VM::Type::GetContextNull)
		{
			auto & null = type.null(*this, &type);

			m_tid2null.Acquire(type_id) = null;
		}
		else if (auto ctr = type.ctr)
		{
			auto & null = ctr(*this, &type);

			REFLEX_ASSERT(null.object_t->type_id == type_id);

			REFLEX_ASSERT(null.GetContextID() == GetContextID());

			m_tid2null.Acquire(type_id) = null;
		}
		else
		{
			REFLEX_ASSERT(false);
		}

		if (type.flags.Check(VM::Type::kFlagThreadsafe))
		{
			REFLEX_ASSERT(type.flags.Check(VM::Type::kFlagNonCircular));
		}
	}

	m_initalised.Set(0);
}

void ContextImpl::InitialiseGlobals(Object & clientdata)
{
	REFLEX_ASSERT(m_initalised.GetWord() == 1);

	REFLEX_ASSERT(Scope::GetCurrent() == this);

	RemoveConst(Context::clientdata) = clientdata;

	auto program = Cast<ProgramImpl>(m_executable);

	auto & global_layout = *program->global_layout;

	ProgramImpl::PushStackFrame<true>(*this, global_layout);

	m_global_stackframe = m_current_stackframe;

	Context::instructionptr = program->global_instructions.GetData();

	m_initalised.Set(1);	//because Invoke debug code detects this during static init phase, but its valid as stackframe was pushed

	m_initalised.Set(1, Run(1, 0));
}

bool ContextImpl::DoCall(const ScriptFunction & fn)
{
	REFLEX_ASSERT(m_initalised.GetWord() == 3);

	Scope scope(*this);

	decltype (CurrentPositionStore::a) store = Context::instructionptr;							//exe DISABLED context switching

	Pair <Instruction> last =
	{
		{ kMaxUInt16, 0, OPCODE(Data), Reinterpret<UInt16>(MakeTuple(UInt8(0), UInt8(fn.rtn.type->size))), 0 },
		{ kMaxUInt16, 0, OPCODE(End), 0, 0 }
	};

	Push(stack, CurrentPositionStore({&last.a, 0, UInt16(stack.GetSize()) }));		//exe DISABLED context switching

	auto top = m_stackframes.GetSize();

	ProgramImpl::PushStackFrame<true>(*this, *fn.layout);

	Context::instructionptr = fn.instructions.GetData();

	Run(top, &fn);

	Context::instructionptr = store;

	return true;
}

void ContextImpl::AdoptCirculars(Context & context, Reflex::Detail::DynamicTypeRef object_t)
{
	Array < Pair <Circular::TrackingToken*, Reference <Object>> > tokens;

	tokens.Allocate(GetNumItem());

	for (auto & i : *this) tokens.Push<kAllocateNone>({ &i, i.circular.object });

	for (auto & i : tokens)
	{
		if (Reflex::Detail::IsOrInheritsFrom(i.b->object_t, object_t)) i.a->Attach(*this);
	}
}

void ContextImpl::Abort()
{
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::VM::Context> Reflex::VM::Context::Create(UInt16 contextid)
{
	return REFLEX_CREATE(Detail::ContextImpl, contextid);
}

Reflex::VM::Detail::Circular::Circular(Context & context, TRef <Object> object)
	: object(object),
	trackingtoken(*this)
{
	trackingtoken.Attach(Cast<ContextImpl>(context));
}

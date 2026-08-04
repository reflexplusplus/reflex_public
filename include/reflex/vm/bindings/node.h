#pragma once

#include "core.h"




REFLEX_NS(Reflex::VM::Detail)

template <class TYPE> void BindIterable(Compiler::State & bindings, Key32 ns, TypeRef object_t, Key32 tag);

REFLEX_END




//
//

REFLEX_NS(Reflex::VM::Detail)

template <class TYPE>
struct ItemIterator : public Object
{
	REFLEX_OBJECT(ItemIterator,Object);

	Reference <TYPE> m_next;

	UInt8 m_valid = 0;
};

template <class TYPE> inline void BindIterable(Compiler::State & compilestate, Key32 ns, TypeRef object_t)
{
	REFLEX_USE(VM::Detail);

	auto bindings = compilestate.bindings;

	auto int32_t = bindings->int32_t;

	AddMethod(bindings, "GetNumChild", int32_t, { object_t }, [](Context & context)
	{
		VM_RTN(Pop<TYPE&>(context.stack).GetNumItem());
	});

	AddMethod(bindings, "GetFirstChild", object_t, { object_t }, [](Context & context)
	{
		if (auto itr = Pop<TYPE&>(context.stack).GetFirst())
		{
			VM_RTN(itr);
		}
		else
		{
			VM_RTN(&Reflex::Detail::GetNullInstance<TYPE>());
		}
	});

	AddMethod(bindings, "GetLastChild", object_t, { object_t }, [](Context & context)
	{
		if (auto itr = Pop<TYPE&>(context.stack).GetLast())
		{
			VM_RTN(itr);
		}
		else
		{
			VM_RTN(&Reflex::Detail::GetNullInstance<TYPE>());
		}
	});

	AddMethod(bindings, "GetPrev", object_t, { object_t }, [](Context & context)
	{
		if (auto itr = Pop<TYPE&>(context.stack).GetPrev())
		{
			VM_RTN(itr);
		}
		else
		{
			VM_RTN(&Reflex::Detail::GetNullInstance<TYPE>());
		}
	});

	AddMethod(bindings, "GetNext", object_t, { object_t }, [](Context & context)
	{
		if (auto itr = Pop<TYPE&>(context.stack).GetNext())
		{
			VM_RTN(itr);
		}
		else
		{
			VM_RTN(&Reflex::Detail::GetNullInstance<TYPE>());
		}
	});

	AddFunction(bindings, ns, "LookupChildAtIndex", object_t, { object_t, int32_t }, [](Context & context)
	{
		VM_POP(TYPE&,UInt32);

		if (args.b < args.a.GetNumItem())
		{
			VM_RTN(LookupItemAtIndex(args.a, args.b));
		}
		else
		{
			VM_RTN(&Reflex::Detail::GetNullInstance<TYPE>());
		}
	});


	StaticString itrname = AcquireStaticString(compilestate, Detail::MakeTemplateName(compilestate, "ItemIterator", { object_t }));

	auto iterator_t = RegisterObject< ItemIterator<TYPE> >(bindings, kGlobal, itrname);

	AddFunction(bindings, kGlobal, Compiler::opBegin, iterator_t, { object_t }, [](Context & context)
	{
		auto self = REFLEX_CREATE(ItemIterator<TYPE>);

		if (auto itr = Pop<TYPE&>(context.stack).GetFirst())
		{
			self->m_next = *itr;

			self->m_valid = 1;
		}

		VM_RTN(self);
	});

	AddFunction(bindings, kGlobal, Compiler::opNext, bindings->bool_t, { iterator_t, VM::ByRef(object_t) }, [](Context & context)
	{
		VM_POP(ItemIterator<TYPE>&,TYPE*&);

		auto & self = args.a;
		auto & itr = args.b;

		VM_RTN(self.m_valid);

		Reflex::Detail::SetReferenceCountedPointer(itr, self.m_next.Adr());

		if (auto next = itr->GetNext())
		{
			self.m_next = next;

			self.m_valid = 1;
		}
		else
		{
			self.m_next.Clear();

			self.m_valid = 0;
		}
	});
}

REFLEX_END

namespace Reflex { template <class TYPE> struct IsSingleThreadExclusive < typename VM::Detail::ItemIterator <TYPE> > { static constexpr bool value = true; }; }

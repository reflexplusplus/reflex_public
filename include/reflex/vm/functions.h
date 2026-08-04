#pragma once

#include "bind.h"
#include "compiler.h"
#include "context.h"




//
//Experimental API

REFLEX_NS(Reflex::VM)

const ScriptFunction * QueryFunction(const Program & program, Symbol symbol, Argument rtn, const ArrayView <Argument> & args);

template <class RTN, class ... ARGS> const ScriptFunction * QueryFunction(const Program & program, Symbol symbol);


template <class FN, class ... ARGS> void CallReturningVoid(Context & context, const FN & fn, const ARGS & ... args);

template <class RTN, class FN, class ... ARGS> RTN CallReturningValue(Context & context, const FN & fn, const ARGS & ... args);

template <class RTN, class FN, class ... ARGS> TRef <RTN> CallReturningObject(Context & context, const FN & fn, const ARGS & ... args);

REFLEX_END

#define VM_SAFE_ENCODE(TYPE,UID,object,stream) VM::SafeEncode<K32(REFLEX_STRINGIFY(UID))>(stream, Cast<TYPE>(object));

#define VM_SAFE_DECODE(TYPE,UID,object,stream) VM::SafeDecode<K32(REFLEX_STRINGIFY(UID))>(stream, Cast<TYPE>(object));




//
//impl

REFLEX_NS(Reflex::VM::Detail)

template <class ARG> inline Argument MakeArgument(const Bindings & bindings)
{
	using Base = NonRefT<ARG>;

	return { Detail::TypeGetter<Base>::Get(bindings), kIsReference<ARG>, kNullKey, kIsConst<Base> };
}

void PublishError(File::ResourcePool & resourcepool, const WString & path, UInt line, CString && stage, CString && error);

REFLEX_END

template <class RTN, class ... VARGS> inline const Reflex::VM::ScriptFunction * Reflex::VM::QueryFunction(const Program & program, Symbol symbol)
{
	auto bindings = program.bindings;

	if constexpr (sizeof...(VARGS))
	{
		Argument args[sizeof...(VARGS)];

		Reflex::Detail::EnumerateTupleTypes<Tuple <VARGS...>>([&args, bindings]<UInt IDX, class TYPE>()
		{
			args[IDX] = Detail::MakeArgument<TYPE>(bindings);
		});

		return QueryFunction(program, symbol, Detail::MakeArgument<RTN>(bindings), args);
	}
	else
	{
		return QueryFunction(program, symbol, Detail::MakeArgument<RTN>(bindings), {});
	}
}

template <class FN, class ... ARGS> REFLEX_INLINE void Reflex::VM::CallReturningVoid(Context & context, const FN & fn, const ARGS & ... args)
{
#if (REFLEX_DEBUG)
	UInt start = context.stack.GetSize();
	
	context.Call(fn, args...);
	
	REFLEX_ASSERT(context.stack.GetSize() == start);
#else
	context.Call(fn, args...);
#endif
}

template <class RTN, class FN, class ... ARGS> REFLEX_INLINE RTN Reflex::VM::CallReturningValue(Context & context, const FN & fn, const ARGS & ... args)
{
#if (REFLEX_DEBUG)
		UInt start = context.stack.GetSize();

		context.Call(fn, args...);

		REFLEX_ASSERT(context.stack.GetSize() == start + kSizeOf<RTN>);
#else
		context.Call(fn, args...);
#endif

	return Detail::Pop<RTN>(context.stack);
}

template <class RTN, class FN, class ... ARGS> REFLEX_INLINE Reflex::TRef <RTN> Reflex::VM::CallReturningObject(Context & context, const FN & fn, const ARGS & ... args)
{
	return CallReturningValue<RTN*>(context, fn, args...);
}

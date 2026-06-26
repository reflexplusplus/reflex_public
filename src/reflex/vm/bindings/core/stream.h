#pragma once

#include "../[require].h"




REFLEX_NS(Reflex::VM)

template <class VALUE> REFLEX_INLINE void StoreValue(ValueArray & archive, const VALUE & value)
{
	MemCopy(&value, archive.Extend<UInt8>(sizeof(VALUE)), sizeof(VALUE));
}

template <bool ASSUME, class VALUE> REFLEX_INLINE void RestoreValue(ValueArray & stream, VALUE & value)
{
	if (Copy(ASSUME) || stream.m_size >= sizeof(VALUE))
	{
		MemCopy(stream.m_ptr, &value, sizeof(VALUE));

		stream.Nudge(sizeof(VALUE));
	}
}

template <bool ASSUME, class VALUE> REFLEX_INLINE VALUE RestoreValue(ValueArray & stream)
{
	VALUE value = {};

	RestoreValue<ASSUME,VALUE>(stream, value);

	return value;
}

template <bool ASSUME> REFLEX_INLINE void RestoreBytes(ValueArray & stream, void * dest, UInt bytes)
{
	if (Copy(ASSUME) || stream.m_size >= bytes)
	{
		MemCopy(stream.m_ptr, dest, bytes);

		stream.Nudge(bytes);
	}
}

const Function * GetStore(Compiler::State & tbindings, TypeRef type);

const Function * GetRestore(Compiler::State & tbindings, TypeRef type);

void Dispatch(Context & context, const Function & fn, Pair <Object*> && args);

REFLEX_END

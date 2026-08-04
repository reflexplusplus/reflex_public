#pragma once

#include "../[require].h"




//
//

REFLEX_NS(Reflex::VM::Detail)

struct ObjectST : public Object {};

struct IntegralArrayIterator : public Object
{
	IntegralArrayIterator()
		: m_current(0),
		m_end(0)
	{
	}

	IntegralArrayIterator(Object & allocation, void * start, void * end)
		: m_allocation(allocation),
		m_current(start),
		m_end(end)
	{
	}

	REFLEX_INLINE UInt8 Next(UInt8 & value, UIntNative stride)
	{
		if (m_current < m_end)
		{
			MemCopy(m_current, &value, stride);

			reinterpret_cast<UInt8*&>(m_current) += stride;

			return true;
		}
		else
		{
			return false;
		}
	}

	template <class TYPE> REFLEX_INLINE UInt8 optimisedNext(TYPE & value)
	{
		if (m_current < m_end)
		{
			value = *Reinterpret<TYPE>(m_current);

			reinterpret_cast<TYPE*&>(m_current)++;

			return true;
		}
		else
		{
			return false;
		}
	}



private:

	Reference <Object> m_allocation;

	void * m_current, * m_end;
};

struct ObjectArrayIterator : public Object
{
	typedef Reference <Object> * ElementPointer;

	ObjectArrayIterator()
		: m_null(REFLEX_NULL(Object)),
		m_current(0),
		m_end(0)
	{
	}

	ObjectArrayIterator(Object & allocation, Object & null_item, ElementPointer start, ElementPointer end)
		: m_null(null_item),
		m_allocation(allocation),
		m_current(start),
		m_end(end)
	{
	}

	template <class OBJECT> REFLEX_INLINE UInt8 Next(OBJECT * & itr)
	{
		if (m_current < m_end)
		{
			Reflex::Detail::SetReferenceCountedPointer(itr, Cast<OBJECT>(m_current->Adr()));

			m_current++;

			return true;
		}
		else
		{
			Reflex::Detail::SetReferenceCountedPointer(itr, Cast<OBJECT>(m_null.Adr()));

			return false;
		}
	}



private:

	Reference <Object> m_null;

	Reference <Object> m_allocation;

	ElementPointer m_current, m_end;
};

REFLEX_END

REFLEX_SET_TRAIT(VM::Detail::ObjectST,IsSingleThreadExclusive);

REFLEX_SET_TRAIT(VM::Detail::IntegralArrayIterator,IsSingleThreadExclusive)

REFLEX_SET_TRAIT(VM::Detail::ObjectArrayIterator,IsSingleThreadExclusive)

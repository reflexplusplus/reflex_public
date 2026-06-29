#pragma once

#include "detail/functions/allocate.h"
#include "detail/initialiser.h"
#include "array/view.h"




//
//Experimental API

namespace Reflex
{

	template <class TYPE> class Allocation;


	template <class TYPE> inline ArrayRegion <TYPE> ToRegion(Allocation <TYPE> & allocation) { return { allocation.begin(), allocation.size }; }

	template <class TYPE> inline ArrayView <TYPE> ToView(const Allocation <TYPE> & allocation) { return { allocation.begin(), allocation.size }; }

}




//
//Allocation

template <class TYPE> 
class Reflex::Allocation : public Object
{
public:

	REFLEX_OBJECT(Allocation, Object);


	
	//lifetime
	
	static TRef <Allocation> Create(UInt size = 0);

	static TRef <Allocation> Create(ArrayView <TYPE> data);


	
	//length

	void Shrink(UInt n);



	//access
	
	TYPE & operator[](UInt idx) { REFLEX_ASSERT(idx < size); return m_first.Adr()[idx]; }

	const TYPE & operator[](UInt idx) const { REFLEX_ASSERT(idx < size); return m_first.Adr()[idx]; }


	TYPE * GetData() { return m_first.Adr(); }

	const TYPE * GetData() const { return m_first.Adr(); }



	//iterate

	TYPE * begin() { return m_first.Adr(); }

	TYPE * end() { return m_first.Adr() + size; }

	const TYPE * begin() const { return m_first.Adr(); }

	const TYPE * end() const { return m_first.Adr() + size; }



	//data
	
	const UInt size;



protected:

	friend Detail::Constructor <Allocation>;

	Allocation(UInt size) : size(size) {}


	Detail::Initialiser <TYPE> m_first;

};




//
//impl

REFLEX_NS(Reflex::Detail)

template <class TYPE>
struct AllocationWithDestructor : public Allocation <TYPE>
{
	friend Detail::Constructor <AllocationWithDestructor>;

	using Allocation = Allocation <TYPE>;

	AllocationWithDestructor(UInt size)
		: Allocation(size)
	{
		REFLEX_LOOP_PTR(Allocation::m_first.Adr(), ptr, size) Detail::Constructor<TYPE>::Construct(ptr);
	}

	AllocationWithDestructor(const ArrayView <TYPE> & view)
		: Allocation(view.size)
	{
		auto src = view.data;

		REFLEX_LOOP_PTR(Allocation::m_first.Adr(), ptr, view.size) Detail::Constructor<TYPE>::Construct(ptr, *src++);
	}

	~AllocationWithDestructor()
	{
		REFLEX_RLOOP_PTR(Allocation::m_first.Adr(), ptr, Allocation::size)
		{
			Detail::Constructor<TYPE>::Destruct(ptr);
		}
	}
};

REFLEX_END

template <class TYPE> struct Reflex::IsAbstract < Reflex::Allocation <TYPE> > { static constexpr bool value = true; };

template <class TYPE> Reflex::TRef < Reflex::Allocation <TYPE> > inline Reflex::Allocation<TYPE>::Create(UInt size)
{
	if constexpr (kIsRawConstructible<TYPE>)
	{
		return Detail::Constructor<Allocation>::CreateVariableSize(g_default_allocator, kSizeOf<TYPE> * (size ? size - 1 : 0), size);
	}
	else
	{
		return Detail::Constructor< Detail::AllocationWithDestructor <TYPE> >::CreateVariableSize(g_default_allocator, kSizeOf<TYPE> * (size ? size - 1 : 0), size);
	}
}

template <class TYPE> Reflex::TRef < Reflex::Allocation <TYPE> > inline Reflex::Allocation<TYPE>::Create(ArrayView <TYPE> view)
{
	auto num_bytes = kSizeOf<TYPE> * view.size;

	auto extra = view ? num_bytes : 0;

	if constexpr (kIsRawConstructible<TYPE>)
	{
		auto allocation = Detail::Constructor<Allocation>::CreateVariableSize(g_default_allocator, extra, view.size);

		MemCopy(view.data, allocation->m_first.Adr(), num_bytes);

		return allocation;
	}
	else
	{
		return Detail::Constructor< Detail::AllocationWithDestructor <TYPE> >::CreateVariableSize(g_default_allocator, extra, view);
	}
}

template <class TYPE> inline void Reflex::Allocation<TYPE>::Shrink(UInt n)
{
	REFLEX_ASSERT(n <= size);

	if constexpr (kIsRawConstructible<TYPE>)
	{
		RemoveConst(size) -= n;
	}
	else
	{
		auto data = GetData();

		while (n--)
		{
			Detail::Constructor<TYPE>::Destruct(data[--RemoveConst(size)]);
		}
	}
}

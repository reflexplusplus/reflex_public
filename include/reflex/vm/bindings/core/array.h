#pragma once

#include "core.h"




//
//

REFLEX_NS(Reflex::VM)

inline const Symbol kArray = { kGlobal, K32("Array") };

struct Allocation : public Object
{
public:

	REFLEX_OBJECT(Allocation, Object);

	static TRef <Allocation> Create(Context & context, TypeRef value_t, UInt size);


	const UInt size;
	
	void * const reserved;
	
	UInt8 data[1];



protected:

	Allocation(UInt size);

}; 

struct AbstractArray : public Object
{
	AbstractArray(TypeRef type, UInt value_size, Object & allocator, void * data);

	void SetDataExternal(Object & allocation, void * ptr, UInt n);

	virtual bool SetData(Allocation & allocation) = 0;

	virtual TRef <Allocation> CreateAllocation(Context & context, UInt size) = 0;

	virtual void * GetNull() = 0;

	Data::Archive::Region GetRawRegion()
	{
		return { Reinterpret<UInt8>(m_ptr), m_size * m_stride };
	}


	Reference <Object> m_allocation;

	void * m_ptr;

	const UInt m_stride;

	UInt m_capacity;

	UInt m_size;

	UInt m_wrap;
};

struct ValueArray : public AbstractArray
{
	ValueArray(TypeRef array_t, TypeRef value_t);

	virtual TRef <Allocation> CreateAllocation(Context & context, UInt size) override;

	virtual bool SetData(Allocation & allocation) override;

	virtual void * GetNull() override;


	//c++ interface

	void Clear()
	{
		m_size = 0;

		m_wrap = 1;
	}

	void Nudge(UInt n)	//for streaming
	{
		REFLEX_ASSERT(n <= m_size);

		reinterpret_cast<UInt8*&>(m_ptr) += n;

		m_capacity -= 4;

		m_size -= n;

		m_wrap = m_size ? m_size : 1;
	}

	template <class TYPE> ArrayView <TYPE> GetView() const
	{
		REFLEX_ASSERT(sizeof(TYPE) == m_stride);

		return { Reinterpret<TYPE>(m_ptr), m_size };
	}

	template <class TYPE> ArrayRegion <TYPE> GetRegion()
	{
		REFLEX_ASSERT(sizeof(TYPE) == m_stride);

		return { Reinterpret<TYPE>(m_ptr), m_size };
	}

	void * Extend(UInt n);

	template <class TYPE> inline TYPE * Extend(UInt n)
	{
		REFLEX_ASSERT(sizeof(TYPE) == m_stride);

		return Reinterpret<TYPE>(ValueArray::Extend(n));
	}

	template <class TYPE> inline void SetData(Object & allocation, const ArrayRegion <TYPE> & region)
	{
		REFLEX_ASSERT(sizeof(TYPE) == m_stride);

		AbstractArray::SetDataExternal(allocation, region.data, region.size);
	}


	UInt8 null[1];
};

struct ObjectArray : public AbstractArray
{
	ObjectArray(Context & context, TypeRef array_t, TypeRef value_t);

	virtual TRef <Allocation> CreateAllocation(Context & context, UInt size) override;

	virtual bool SetData(Allocation & allocation) override;

	virtual void * GetNull() override;

	virtual void OnReleaseData() override;

	Data::Archive::View GetRawView() const
	{
		return { Cast<UInt8>(m_ptr), UInt(m_size * sizeof(Detail::Pointer)) };
	}


	//C++ interface

	Object ** Extend(Context & context, UInt n);

	template <class TYPE> Reference <TYPE> * Extend(Context & context, UInt n)
	{
		return Reinterpret<Reference<TYPE>>(ObjectArray::Extend(context, n));
	}

	template <class TYPE> ArrayView < Reference <TYPE> > GetView() const
	{
		return { Cast<Reference<TYPE>>(m_ptr), m_size };
	}

	template <class TYPE> ArrayRegion < Reference <TYPE> > GetRegion() const
	{
		return { Cast<Reference<TYPE>>(m_ptr), m_size };
	}


	const Reference <Object> null;
};


using ArrayOfCircularObjects = ObjectArray;
using ArrayOfNonCircularObjects = ObjectArray;

using ArrayOfUInt8Targ = VM_TEMPLATE_TARG("Array@UInt8", ValueArray);
using ArrayOfStringTarg = VM_TEMPLATE_TARG("Array@String", ArrayOfNonCircularObjects);

REFLEX_END

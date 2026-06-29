#pragma once

#include "../allocator/allocator.h"
#include "../detail/functions.h"
#include "../functions/cast.h"
#include "../functions/logic.h"
#include "../functions/memory.h"
#include "defines.h"
#include "traits.h"
#include "functions/region.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> class Array;		//dynamically-sized contiguous container

}




//
//Array

template <class TYPE>
class Reflex::Array
{
public:

	//value

	static constexpr bool kIsNullTerminated = IsNullTerminated<TYPE>::value;



	//declarations

	using Type = TYPE;

	using Itr = TYPE *;

	using Region = ArrayRegion <TYPE>;

	using View = ArrayView <const TYPE>;



	//lifetime

	Array();

	explicit Array(UInt length);

	Array(const View & view);

	Array(const Array & value);

	Array(const std::initializer_list <TYPE> & list);

	Array(const TYPE * nullterminated);	//for string

	Array(Array && rhs);

	Array(nullptr_t) = delete;


	
	//VC workaround (ObjectOf needs default constructor)

	Array(Allocator & allocator);

	explicit Array(UInt length, Allocator & allocator);

	Array(const View & view, Allocator & allocator);


	~Array();



	//capacity

	void Compact();

	template <AllocatePolicy ALLOC = kAllocateExact> bool Allocate(UInt capacity);

	template <AllocatePolicy ALLOC = kAllocateExact> bool Reserve(UInt capacity) { return Allocate<ALLOC>(capacity); }

	UInt GetCapacity() const;



	//size

	template <AllocatePolicy ALLOC = kAllocateOver> void Expand(UInt n);

	void Shrink(UInt n);

	template <AllocatePolicy ALLOC = kAllocateExact, class ... VARGS> void SetSize(UInt length, VARGS && ... vargs);

	UInt GetSize() const;



	//write access

	void Clear();


	template <AllocatePolicy ALLOC = kAllocateOver> TYPE & Push();

	template <AllocatePolicy ALLOC = kAllocateOver> TYPE & Push(const TYPE & value);

	template <AllocatePolicy ALLOC = kAllocateOver> TYPE & Push(TYPE && value);


	template <AllocatePolicy ALLOC = kAllocateOver> void Append(const View & value);


	void Pop();


	TYPE & Insert(UInt idx);

	TYPE & Insert(UInt idx, const TYPE & copy);

	TYPE & Insert(UInt idx, TYPE && value);


	void Remove(UInt idx);

	void Remove(UInt idx, UInt n);


	void Wipe();

	void Fill(const TYPE & value);

	
	void Swap(Array & value);



	//read access

	bool Empty() const;


	TYPE & GetFirst();

	const TYPE & GetFirst() const;


	TYPE & GetLast();

	const TYPE & GetLast() const;


	TYPE & operator[](UInt idx);

	const TYPE & operator[](UInt idx) const;


	TYPE * GetData();

	const TYPE * GetData() const;



	//operators

	Array & operator=(const Array & value);

	Array & operator=(Array && value);


	Array & operator=(const View & value);

	Array & operator=(const std::initializer_list <TYPE> & list);

	template <UInt SIZE> Array & operator=(const TYPE(&data)[SIZE]);

	Array & operator=(const TYPE * nullterminated);

	Array & operator=(nullptr_t) = delete;


	explicit operator bool() const;

	bool operator==(const View & value) const;

	bool operator!=(const View & value) const;

	bool operator<(const View & value) const;



	//iterate

	TYPE * begin() { return m_ptr; }

	TYPE * end() { return m_ptr + m_size; }

	const TYPE * begin() const { return m_ptr; }

	const TYPE * end() const { return m_ptr + m_size; }


	auto rbegin() { return Detail::ReverseItr(end()); }

	auto rend() { return Detail::ReverseItr(begin()); }

	auto rbegin() const { return Detail::ReverseItr(end()); }

	auto rend() const { return Detail::ReverseItr(begin()); }



	const TRef <Allocator> allocator;



protected:

	template <bool OVERALLOC> bool DoAllocate(UInt capacity);

	void NullTerminate();


	TYPE * m_ptr;

	UInt32 m_capacity;

	UInt32 m_size;

};

REFLEX_SET_TRAIT_TEMPLATED(Array, IsBoolCastable);




//
//impl

REFLEX_STATIC_ASSERT(!std::is_trivially_copyable<Reflex::Array<char>>::value);
REFLEX_STATIC_ASSERT(!Reflex::kIsRawCopyable<Reflex::Array<char>>);

REFLEX_NS(Reflex::Detail)

REFLEX_INLINE UInt CalculateExpandedCapacity(UInt capacity)
{
	return (capacity | 2) << 1;
}

template <class TYPE>
struct ArrayItemTypeImpl < Array <TYPE> >
{
	using Type = TYPE;
};

template <class TYPE>
struct ArrayItemTypeImpl < const Array <TYPE> >
{
	using Type = const TYPE;
};

template <class TYPE, bool VALUE>
struct NullTerminator
{
	REFLEX_INLINE static void Validate(const TYPE & value) {}

	REFLEX_INLINE static TYPE * Allocate(Allocator & allocator, UInt request, const AllocInfo & info)
	{
		return Detail::Allocate<TYPE>(allocator, request, info);
	}

	REFLEX_INLINE static void Apply(TYPE&) {}
};

template <class TYPE>
struct NullTerminator <TYPE,true>
{
	REFLEX_INLINE static void Validate(const TYPE & value)
	{
		REFLEX_ASSERT(value);
	}

	REFLEX_INLINE static TYPE * Allocate(Allocator & allocator, UInt request, const AllocInfo & info)
	{
		return Detail::Allocate<TYPE>(allocator, request + 1, info);
	}

	REFLEX_INLINE static void Apply(TYPE & value) { value = 0; }
};

template <class TYPE> REFLEX_INLINE const char * GetDebugTypeName()
{
#if REFLEX_DEBUG && defined(REFLEX_RTTI_ENABLED)
	return typeid(TYPE).name();
#else
	return "unavailable";
#endif
}

REFLEX_END

template <class TYPE> REFLEX_INLINE Reflex::Array<TYPE>::Array() : Array(g_default_allocator) {}

template <class TYPE> inline Reflex::Array<TYPE>::Array(UInt length) : Array(length, g_default_allocator) {}

template <class TYPE> inline Reflex::Array<TYPE>::Array(const Array & value) : Array(ToView(value), g_default_allocator) {}

template <class TYPE> inline Reflex::Array<TYPE>::Array(const View & view) : Array(view, g_default_allocator) {}

template <class TYPE> inline Reflex::Array<TYPE>::Array(const std::initializer_list <TYPE> & list) : Array(View(list.begin(), UInt(list.size())), g_default_allocator) {}

template <class TYPE> inline Reflex::Array<TYPE>::Array(Allocator & allocator)
	: allocator(allocator)
	, m_ptr(nullptr)
	, m_capacity(0)
	, m_size(0)
{
	if (kIsNullTerminated)
	{
		DoAllocate<false>(16);

		NullTerminate();
	}
}

template <class TYPE> inline Reflex::Array<TYPE>::Array(UInt length, Allocator & allocator)
	: allocator(allocator)
	, m_ptr(nullptr)
	, m_capacity(0)
	, m_size(0)
{
	if (kIsNullTerminated)
	{
		if (!length)
		{
			DoAllocate<false>(1);
		}
	}

	SetSize(length);
}

template <class TYPE> inline Reflex::Array<TYPE>::Array(const View & view, Allocator & allocator)
	: allocator(allocator)
	, m_ptr(Detail::NullTerminator<TYPE,kIsNullTerminated>::Allocate(allocator, view.size, AllocInfo(Detail::GetDebugTypeName<TYPE>(), "Array(const View & view)")))
	, m_capacity(view.size)
	, m_size(view.size)
{
	if constexpr (kIsRawCopyable<TYPE>)
	{
		MemCopy(view.data, m_ptr, sizeof(TYPE) * m_size);
	}
	else
	{
		auto src = view.data;

		REFLEX_LOOP_PTR(m_ptr, ptr, m_size) Detail::Constructor<TYPE>::Construct(ptr, *src++);
	}

	NullTerminate();
}

template <class TYPE> inline Reflex::Array<TYPE>::Array(Array && value)
	: allocator(value.allocator)
{
	if (kIsNullTerminated)
	{
		m_ptr = Detail::NullTerminator<TYPE, true>::Allocate(allocator, 0, AllocInfo(Detail::GetDebugTypeName<TYPE>(), "Array(Array &&)"));
		m_capacity = 0;
		m_size = 0;

		NullTerminate();

		Swap(value);
	}
	else
	{
		m_ptr = value.m_ptr;
		m_capacity = value.m_capacity;
		m_size = value.m_size;

		value.m_ptr = nullptr;
		value.m_capacity = 0;
		value.m_size = 0;
	}
}

template <class TYPE> inline Reflex::Array<TYPE>::~Array()
{
	Clear();

	allocator->Free(m_ptr);
}

template <class TYPE> REFLEX_INLINE void Reflex::Array<TYPE>::Compact()
{
	DoAllocate<false>(m_size);
}

template <class TYPE> template <Reflex::AllocatePolicy ALLOC> REFLEX_INLINE bool Reflex::Array<TYPE>::Allocate(UInt capacity)
{
	if constexpr (ALLOC == kAllocateNone) return true;

	if (capacity > m_capacity)
	{
		return DoAllocate<ALLOC == kAllocateOver>(capacity);
	}
	else
	{
		return true;
	}
}

template <class TYPE> REFLEX_INLINE Reflex::UInt Reflex::Array<TYPE>::GetCapacity() const
{
	return m_capacity;
}

template <class TYPE> inline void Reflex::Array<TYPE>::Clear()
{
	if constexpr (!kIsRawConstructible<TYPE>)
	{
		auto ptr = m_ptr + m_size;

		while (ptr != m_ptr)
		{
			Detail::Constructor<TYPE>::Destruct(*--ptr);
		}
	}

	m_size = 0;

	NullTerminate();
}

template <class TYPE> inline void Reflex::Array<TYPE>::Shrink(UInt n)
{
	REFLEX_ASSERT(n <= m_size);

	if constexpr (kIsRawConstructible<TYPE>)
	{
		m_size -= n;
	}
	else
	{
		while (n--)
		{
			Detail::Constructor<TYPE>::Destruct(m_ptr[--m_size]);
		}
	}

	NullTerminate();
}

template <class TYPE> template <Reflex::AllocatePolicy ALLOC> REFLEX_INLINE void Reflex::Array<TYPE>::Expand(UInt n)
{
	UInt length = m_size + n;

	Allocate<ALLOC>(length);

	if constexpr (!kIsRawConstructible<TYPE>)
	{
		REFLEX_LOOP_PTR(m_ptr + m_size, ptr, n) Detail::Constructor<TYPE>::Construct(ptr);
	}

	m_size = length;

	NullTerminate();
}

template <class TYPE> template <Reflex::AllocatePolicy ALLOC, class ... VARGS> inline void Reflex::Array<TYPE>::SetSize(UInt length, VARGS && ... vargs)
{
	if constexpr (kIsRawConstructible<TYPE> && ALLOC == kAllocateNone)
	{
		m_size = length;
	}
	else
	{
		Int change = length - m_size;

		if (change > 0)
		{
			Allocate<ALLOC>(length);

			if constexpr (kIsRawConstructible<TYPE>)
			{
				m_size = length;
			}
			else
			{
				auto ptr = m_ptr + m_size;

				auto end = ptr + change;

				while (ptr < end) Detail::Constructor<TYPE>::Construct(ptr++, std::forward<VARGS>(vargs)...);

				m_size = length;
			}
		}
		else
		{
			if constexpr (kIsRawConstructible<TYPE>)
			{
				m_size = length;
			}
			else
			{
				while (change++ != 0)
				{
					Detail::Constructor<TYPE>::Destruct(m_ptr[--m_size]);
				}
			}
		}
	}

	if constexpr (ALLOC == kAllocateNone)
	{
		REFLEX_ASSERT(m_size <= m_capacity);
	}

	NullTerminate();
}

template <class TYPE> REFLEX_INLINE Reflex::UInt Reflex::Array<TYPE>::GetSize() const
{
	return m_size;
}

template <class TYPE> template <Reflex::AllocatePolicy ALLOC> REFLEX_INLINE TYPE & Reflex::Array<TYPE>::Push()
{
	REFLEX_ASSERT(!kIsNullTerminated);

	if constexpr (ALLOC == kAllocateNone)
	{
		REFLEX_ASSERT(m_size < m_capacity);
	}

	Allocate<ALLOC>(m_size + 1);

	REFLEX_ASSERT(m_ptr);

	auto ptr = m_ptr + m_size;

	Detail::Constructor<TYPE>::Construct(ptr);

	++m_size;

	NullTerminate();

	return *ptr;
}

template <class TYPE> template <Reflex::AllocatePolicy ALLOC> REFLEX_INLINE TYPE & Reflex::Array<TYPE>::Push(const TYPE & value)
{
	REFLEX_ASSERT(!Inside(ToUIntNative(GetAdr(value)), ToUIntNative(m_ptr), UIntNative(m_capacity * sizeof(TYPE))));

	if constexpr (ALLOC == kAllocateNone)
	{
		REFLEX_ASSERT(m_size < m_capacity);
	}

	Detail::NullTerminator<TYPE,kIsNullTerminated>::Validate(value);

	Allocate<ALLOC>(m_size + 1);

	REFLEX_ASSERT(m_ptr);

	auto ptr = m_ptr + m_size;

	Detail::Constructor<TYPE>::Construct(ptr, value);

	++m_size;

	NullTerminate();

	return *ptr;
}

template <class TYPE> template <Reflex::AllocatePolicy ALLOC> REFLEX_INLINE TYPE & Reflex::Array<TYPE>::Push(TYPE && value)
{
	REFLEX_ASSERT(!Inside(ToUIntNative(GetAdr(value)), ToUIntNative(m_ptr), UIntNative(m_capacity * sizeof(TYPE))));

	if constexpr (ALLOC == kAllocateNone)
	{
		REFLEX_ASSERT(m_size < m_capacity);
	}

	Detail::NullTerminator<TYPE,kIsNullTerminated>::Validate(value);

	Allocate<ALLOC>(m_size + 1);

	REFLEX_ASSERT(m_ptr);

	auto ptr = m_ptr + m_size;

	Detail::Constructor<TYPE>::Construct(ptr, std::move(value));

	++m_size;

	NullTerminate();

	return *ptr;
}

template <class TYPE> inline void Reflex::Array<TYPE>::Pop()
{
	REFLEX_ASSERT(m_size > 0);

	--m_size;

	if constexpr (!kIsRawConstructible<TYPE>) Detail::Constructor<TYPE>::Destruct(m_ptr[m_size]);

	NullTerminate();
}

template <class TYPE> REFLEX_INLINE TYPE & Reflex::Array<TYPE>::Insert(UInt idx)
{
	return Insert(idx, TYPE());
}

template <class TYPE> inline TYPE & Reflex::Array<TYPE>::Insert(UInt idx, const TYPE & value)
{
	REFLEX_ASSERT(!Inside(ToUIntNative(&value), ToUIntNative(m_ptr), UIntNative(m_capacity * sizeof(TYPE))));

	return Insert(idx, Copy(value));
}

template <class TYPE> inline TYPE & Reflex::Array<TYPE>::Insert(UInt idx, TYPE && value)
{
	++m_size;

	REFLEX_ASSERT(idx < m_size);

	Detail::NullTerminator<TYPE,kIsNullTerminated>::Validate(value);

	if constexpr (kIsRawConstructible<TYPE>)
	{
		if (m_size > m_capacity)
		{
			m_capacity = Detail::CalculateExpandedCapacity(m_size);

			auto ptr = Detail::NullTerminator<TYPE,kIsNullTerminated>::Allocate(allocator, m_capacity, AllocInfo(Detail::GetDebugTypeName<TYPE>(), "Array::Insert"));

			MemCopy(m_ptr, ptr, sizeof(TYPE) * idx);

			MemCopy(m_ptr + idx, ptr + idx + 1, sizeof(TYPE) * ((m_size - 1) - idx));

			allocator->Free(m_ptr);

			m_ptr = ptr;
		}
		else
		{
			MemMove(m_ptr + idx, m_ptr + idx + 1, sizeof(TYPE) * ((m_size - 1) - idx));
		}

		auto & item = m_ptr[idx];

		item = std::move(value);

		NullTerminate();

		return item;
	}
	else
	{
		if (m_size > m_capacity)
		{
			m_capacity = Detail::CalculateExpandedCapacity(m_size);

			auto ptr = Detail::NullTerminator<TYPE,kIsNullTerminated>::Allocate(allocator, m_capacity, AllocInfo(Detail::GetDebugTypeName<TYPE>(), "Array::Insert"));

			{
				UInt pos = idx;

				while (pos-- != 0)
				{
					auto & current = m_ptr[pos];

					Detail::Constructor<TYPE>::Construct(ptr + pos, std::move(current));

					Detail::Constructor<TYPE>::Destruct(current);
				}
			}

			{
				Int start = idx;

				Int ipos = m_size - 1;

				while (ipos-- > start)
				{
					auto & current = m_ptr[ipos];

					Detail::Constructor<TYPE>::Construct(ptr + ipos + 1, std::move(current));

					Detail::Constructor<TYPE>::Destruct(current);
				}
			}

			Detail::Constructor<TYPE>::Construct(ptr + idx, std::move(value));

			allocator->Free(m_ptr);

			m_ptr = ptr;

			NullTerminate();

			return m_ptr[idx];
		}
		else if (idx < (m_size - 1))
		{
			auto item = m_ptr + idx;

			auto itr = m_ptr + m_size - 1;

			Detail::Constructor<TYPE>::Construct(itr);

			while (itr-- > item)
			{
				itr[1] = std::move(*itr);
			}

			*item = std::move(value);

			NullTerminate();

			return *item;
		}
		else
		{
			auto ptr = m_ptr + idx;

			Detail::Constructor<TYPE>::Construct(ptr, std::move(value));

			NullTerminate();

			return *ptr;
		}
	}
}

template <class TYPE> template <Reflex::AllocatePolicy ALLOC> REFLEX_INLINE void Reflex::Array<TYPE>::Append(const View & view)
{
	UInt length = m_size + view.size;

#if REFLEX_DEBUG
	const bool from_self = Inside(ToUIntNative(view.data), ToUIntNative(m_ptr), UIntNative(m_capacity * sizeof(TYPE)));
	const bool safe = from_self ? ((view.data + view.size) <= (m_ptr + m_size)) && (length <= m_capacity) : true;
	REFLEX_ASSERT(Copy(ALLOC == kAllocateNone) || safe);
#endif

	Allocate<ALLOC>(length);

	if constexpr (kIsRawConstructible<TYPE>)
	{
		MemCopy(view.data, m_ptr + m_size, sizeof(TYPE) * view.size);

		m_size = length;
	}
	else
	{
		auto ptr = m_ptr + m_size;

		REFLEX_LOOP_PTR(view.data, itr, view.size) Detail::Constructor<TYPE>::Construct(ptr++, *itr);

		m_size = length;
	}

	if constexpr (ALLOC == kAllocateNone)
	{
		REFLEX_ASSERT(m_size <= m_capacity);
	}

	NullTerminate();
}

template <class TYPE> inline void Reflex::Array<TYPE>::Remove(UInt idx)
{
	REFLEX_ASSERT(idx < m_size);

	--m_size;

	if constexpr (kIsRawConstructible<TYPE>)
	{
		MemMove(m_ptr + idx + 1, m_ptr + idx, sizeof(TYPE) * (m_size - idx));
	}
	else
	{
		REFLEX_LOOP_PTR(m_ptr + idx, ptr, m_size - idx)
		{
			*ptr = std::move(ptr[1]);
		}

		Detail::Constructor<TYPE>::Destruct(m_ptr[m_size]);
	}

	NullTerminate();
}

template <class TYPE> inline void Reflex::Array<TYPE>::Remove(UInt idx, UInt n)
{
	REFLEX_ASSERT((idx + n) <= m_size);

	if constexpr (kIsRawConstructible<TYPE>)
	{
		m_size -= n;	//BUG fix 26/5/2021: was after MemMove!!!

		UInt from = idx + n;

		MemMove(m_ptr + from, m_ptr + idx, sizeof(TYPE) * (m_size - idx));
	}
	else
	{
		while (n--) Remove(idx);
	}

	NullTerminate();
}

template <class TYPE> inline void Reflex::Array<TYPE>::Wipe()
{
	Reflex::Wipe<TYPE>(ArrayRegion<TYPE>(m_ptr, m_size));
}

template <class TYPE> inline void Reflex::Array<TYPE>::Fill(const TYPE & value)
{
	Reflex::Fill<TYPE>(ArrayRegion<TYPE>(m_ptr, m_size), value);
}

template <class TYPE> REFLEX_INLINE TYPE & Reflex::Array<TYPE>::operator[](UInt idx)
{
	REFLEX_ASSERT(idx < m_size);

	return m_ptr[idx];
}

template <class TYPE> REFLEX_INLINE const TYPE & Reflex::Array<TYPE>::operator[](UInt idx) const
{
	REFLEX_ASSERT(idx < m_size);

	return m_ptr[idx];
}

template <class TYPE> REFLEX_INLINE TYPE * Reflex::Array<TYPE>::GetData()
{
	return m_ptr;
}

template <class TYPE> REFLEX_INLINE const TYPE * Reflex::Array<TYPE>::GetData() const
{
	return m_ptr;
}

template <class TYPE> REFLEX_INLINE bool Reflex::Array<TYPE>::Empty() const
{
	return m_size == 0;
}

template <class TYPE> REFLEX_INLINE TYPE & Reflex::Array<TYPE>::GetFirst()
{
	REFLEX_ASSERT(m_size > 0);

	return *m_ptr;
}

template <class TYPE> REFLEX_INLINE const TYPE & Reflex::Array<TYPE>::GetFirst() const
{
	REFLEX_ASSERT(m_size > 0);

	return *m_ptr;
}

template <class TYPE> REFLEX_INLINE TYPE & Reflex::Array<TYPE>::GetLast()
{
	REFLEX_ASSERT(m_size > 0);

	return m_ptr[m_size - 1];
}

template <class TYPE> REFLEX_INLINE const TYPE & Reflex::Array<TYPE>::GetLast() const
{
	REFLEX_ASSERT(m_size > 0);

	return m_ptr[m_size - 1];
}

template <class TYPE> REFLEX_INLINE void Reflex::Array<TYPE>::Swap(Array & value)
{
	Reflex::Swap(RemoveConst(allocator), RemoveConst(value.allocator));
	Reflex::Swap(m_ptr, value.m_ptr);
	Reflex::Swap(m_capacity, value.m_capacity);
	Reflex::Swap(m_size, value.m_size);
}

template <class TYPE> REFLEX_INLINE Reflex::Array <TYPE> & Reflex::Array<TYPE>::operator=(const Array & value)
{
	return operator=(ToView(value));
}

template <class TYPE> REFLEX_INLINE Reflex::Array <TYPE> & Reflex::Array<TYPE>::operator=(Array && temp)
{
	Swap(temp);

	return *this;
}

template <class TYPE> inline Reflex::Array <TYPE> & Reflex::Array<TYPE>::operator=(const View & value)
{
	REFLEX_ASSERT(!Inside(ToUIntNative(value.data), ToUIntNative(m_ptr), UIntNative(m_capacity * sizeof(TYPE))));

	UInt length = value.size;

	if constexpr (kIsRawCopyable<TYPE>)
	{
		if (length > m_capacity)
		{
			allocator->Free(m_ptr);

			m_capacity = length;

			m_ptr = Detail::NullTerminator<TYPE,kIsNullTerminated>::Allocate(allocator, m_capacity, AllocInfo(Detail::GetDebugTypeName<TYPE>(), "Array::operator="));
		}

		m_size = length;

		MemCopy(value.data, m_ptr, sizeof(TYPE) * m_size);
	}
	else if (length > m_size)	//is bigger than this
	{
		if (length > m_capacity)			//inplace
		{
			UInt idx = m_size;

			while (idx-- != 0) Detail::Constructor<TYPE>::Destruct(m_ptr[idx]);

			allocator->Free(m_ptr);

			m_capacity = length;

			m_ptr = Detail::NullTerminator<TYPE,kIsNullTerminated>::Allocate(allocator, m_capacity, AllocInfo(Detail::GetDebugTypeName<TYPE>(), "Array::operator="));

			idx = length;

			while (idx-- != 0) Detail::Constructor<TYPE>::Construct(m_ptr + idx, value.data[idx]);

			m_size = length;
		}
		else 		//in place
		{
			for (UInt idx = m_size; idx < length; ++idx)
			{
				Detail::Constructor<TYPE>::Construct(m_ptr + idx, value.data[idx]);
			}

			UInt idx = m_size;

			while (idx-- != 0) m_ptr[idx] = value.data[idx];

			m_size = length;
		}
	}
	else		//is equal/smaller than this
	{
		UInt idx = m_size;

		while (idx-- != length) Detail::Constructor<TYPE>::Destruct(m_ptr[idx]);

		idx = length;

		while (idx-- != 0) m_ptr[idx] = value.data[idx];

		m_size = length;
	}

	NullTerminate();

	return *this;
}

template <class TYPE> REFLEX_INLINE Reflex::Array <TYPE> & Reflex::Array<TYPE>::operator=(const std::initializer_list <TYPE> & list)
{
	return operator=(View(list.begin(), UInt(list.size())));
}

template <class TYPE> template <Reflex::UInt SIZE> REFLEX_INLINE Reflex::Array <TYPE> & Reflex::Array<TYPE>::operator=(const TYPE(&data)[SIZE])
{
	return operator=(View(data));
}

template <class TYPE> REFLEX_INLINE Reflex::Array <TYPE> & Reflex::Array<TYPE>::operator=(const TYPE * nullterminated)
{
	return operator=(View(nullterminated));
}

template <class TYPE> REFLEX_INLINE Reflex::Array<TYPE>::operator bool() const
{
	return m_size != 0;
}

template <class TYPE> REFLEX_INLINE bool Reflex::Array<TYPE>::operator==(const View & value) const
{
	return ToView(*this) == value;
}

template <class TYPE> REFLEX_INLINE bool Reflex::Array<TYPE>::operator<(const View & value) const
{
	return ToView(*this) < value;
}

template <class TYPE> REFLEX_INLINE bool Reflex::Array<TYPE>::operator!=(const View & value) const
{
	return !operator==(value);
}

template <class TYPE> template <bool OVER> inline bool Reflex::Array<TYPE>::DoAllocate(UInt capacity)
{
	REFLEX_ASSERT(capacity > m_capacity);

	m_capacity = OVER ? Detail::CalculateExpandedCapacity(capacity) : capacity;

	if (auto ptr = Detail::NullTerminator<TYPE,kIsNullTerminated>::Allocate(allocator, m_capacity, AllocInfo(Detail::GetDebugTypeName<TYPE>(), "Array::DoAllocate")))
	{
		if constexpr (kIsRawConstructible<TYPE>)
		{
			MemCopy(m_ptr, ptr, sizeof(TYPE) * m_size);
		}
		else
		{
			UInt idx = m_size;

			while (idx-- != 0)
			{
				TYPE & current = m_ptr[idx];

				Detail::Constructor<TYPE>::Construct(ptr + idx, std::move(current));

				Detail::Constructor<TYPE>::Destruct(current);
			}
		}

		allocator->Free(m_ptr);

		m_ptr = ptr;

		return true;
	}
	else
	{
		m_capacity = 0;

		allocator->Free(m_ptr);

		m_ptr = 0;

		return false;
	}
}

template <class TYPE> REFLEX_INLINE void Reflex::Array<TYPE>::NullTerminate()
{
	if constexpr (kIsNullTerminated)
	{
		REFLEX_ASSERT(m_ptr && m_size < (m_capacity + 1));

		Detail::NullTerminator<TYPE, kIsNullTerminated>::Apply(m_ptr[m_size]);
	}
}

template <class TYPE> REFLEX_INLINE Reflex::ArrayRegion<TYPE>::ArrayRegion(ConditionalType < kIsConst<TYPE>, const Array <NonConstT<TYPE>>, Array <TYPE> > & array)
	: data(array.GetData())
	, size(array.GetSize())
{
}

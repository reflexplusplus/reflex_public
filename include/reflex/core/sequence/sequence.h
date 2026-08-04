#pragma once

#include "../detail/functions.h"
#include "../compare_policy.h"
#include "../array.h"




//
//Primary API

namespace Reflex
{

	template <class KEY, class VALUE = NullType, class COMPARE = StandardCompare, bool CONTIGUOUS = false> class Sequence;

}




//
//Sequence

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS>
class Reflex::Sequence
{
public:

	//declarations

	class Item;


	template <bool CONST> class ItrImpl;

	using Itr = ItrImpl <false>;

	using ConstItr = ItrImpl <true>;


	template <bool CONST> struct RangeImpl;

	using Range = RangeImpl <false>;

	using ConstRange = RangeImpl <true>;



	//lifetime

	Sequence();	//VC workaround

	Sequence(Allocator & allocator);

	Sequence(const Sequence & sequence, Allocator & allocator = g_default_allocator);

	Sequence(Sequence && temp);

	Sequence(std::initializer_list <Item> && list, Allocator & allocator = g_default_allocator);

	~Sequence();



	//size

	void Compact();

	void Allocate(UInt capacity);



	//size

	UInt GetSize() const;



	//content

	void Clear();


	VALUE & InsertItem(Item && item);

	VALUE & Insert(const KEY & key);

	VALUE & Insert(const KEY & key, VALUE && temp);

	VALUE & Insert(const KEY & key, const VALUE & value);


	VALUE & SetItem(Item && item);

	VALUE & Set(const KEY & key);

	VALUE & Set(const KEY & key, const VALUE & value);

	VALUE & Set(const KEY & key, VALUE && value);


	template <class ... ARGS> VALUE & Acquire(const KEY & key, ARGS && ... args);	//Returns the value matching key, if does not exist, Inserts with args


	void Remove(UInt idx);

	void Remove(UInt idx, UInt n);



	//lookup

	Idx Search(const KEY & key) const;									//Exact key match, returns invalid Idx if key not present


	VALUE * SearchValue(const KEY & key, VALUE * fallback = nullptr);	//Exact key match, returns pointer to value if found, otherwise fallback

	const VALUE * SearchValue(const KEY & key, const VALUE * fallback = nullptr) const;


	Idx SearchGTE(const KEY & key) const;			//Returns Idx of the first element with key >= input key

	Idx SearchLT(const KEY & key) const;			//Returns Idx of the last element with key < input key



	//access

	bool Empty() const;


	Item & GetFirst();

	const Item & GetFirst() const;


	Item & GetLast();

	const Item & GetLast() const;


	Item & operator[](UInt idx);

	const Item & operator[](UInt idx) const;


	ConditionalType <CONTIGUOUS, Item*, Item * const *> GetData() { return Reinterpret< Array <ItemType> >(m_data).GetData(); }

	ConditionalType <CONTIGUOUS, const Item*, const Item * const *> GetData() const { return Reinterpret< Array <ItemType> >(m_data).GetData();	}



	//assignment

	void Swap(Sequence & sequence);

	Sequence & operator=(const Sequence &) = default;

	Sequence & operator=(Sequence && value);



	//logic

	explicit operator bool() const;

	bool operator==(const Sequence & value) const;

	bool operator!=(const Sequence & value) const;



	//iterate

	auto begin() { return Itr(*this, 0); }

	auto end() { return Itr(*this, GetSize()); }

	auto begin() const { return ConstItr(*this, 0); }

	auto end() const { return ConstItr(*this, GetSize()); }


	auto rbegin() { return Detail::ReverseItr(end()); }

	auto rend() { return Detail::ReverseItr(begin()); }

	auto rbegin() const { return Detail::ReverseItr(end()); }

	auto rend() const { return Detail::ReverseItr(begin()); }



protected:

	VALUE & Push(const KEY & key, VALUE && value);



private:

	using ItemType = ConditionalType <CONTIGUOUS, Item, Item*>;

	struct ItemImpl
	{
		struct Compare;
		bool operator==(const Item & item) const { return COMPARE::eq(key, item.key) && value == item.value; }
		bool operator!=(const Item & item) const { return (!COMPARE::eq(key, item.key)) || value != item.value; }
		operator Item&() { return Reinterpret<Item>(*this); }
		operator const Item&() const { return Reinterpret<Item>(*this); }

		KEY key;

		VALUE value;
	};

	struct DfArray : public Array <ItemImpl*>
	{
		using Base = Array <ItemImpl*>;

		DfArray(Allocator & allocator);

		DfArray(const DfArray & pool, Allocator & allocator);

		void Clear();

		void Remove(UInt idx);

		void Remove(UInt idx, UInt n);

		ItemImpl & Insert(UInt idx, ItemImpl && rhs);

		ItemImpl & Push(ItemImpl && rhs);

		ItemImpl & operator[](UInt idx);

		const ItemImpl & operator[](UInt idx) const;

		void operator=(const DfArray & pool);
	};

	using ArrayBaseType = ConditionalType <CONTIGUOUS, Array<ItemImpl>, Array <ItemImpl*>>;

	using ArrayImpl = ConditionalType <CONTIGUOUS,Array<ItemImpl>,DfArray>;

	void Modify();


	ArrayImpl m_data;

	REFLEX_IF_DEBUG(UInt m_modification_count = 0);
};

namespace Reflex { template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> struct IsBoolCastable < Sequence <KEY,VALUE,COMPARE, CONTIGUOUS> > { static constexpr bool value = true; }; }




//
//Sequence::Item

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS>
class Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Item
{
public:

	//types

	using Key = KEY;

	using Value = VALUE;



	//compare

	bool operator==(const Item & item) const = delete;

	bool operator!=(const Item & item) const = delete;



	//data

	const KEY key;

	VALUE value;

};




//
//Sequence::ItrImpl

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS>
template <bool CONST>
class Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::ItrImpl
{
public:

	//types

	using SequenceType = ConditionalType <CONST,const Sequence,Sequence>;

	using Type = ConditionalType <CONST,const Item,Item>;



	//lifetime

	ItrImpl(SequenceType & sequence, UInt idx);



	//operators

	auto & operator*() const;

	void operator++() { ++m_ptr; };

	void operator--() { --m_ptr; };

	bool operator!=(const ItrImpl & itr) const { return m_ptr != itr.m_ptr; }

	UInt operator-(const ItrImpl & itr) const { return UInt(m_ptr - itr.m_ptr); };



private:

	ConditionalType <CONTIGUOUS,Type*,Type*const*> m_ptr;

	REFLEX_IF_DEBUG(const UInt & modification_count_ref);
	REFLEX_IF_DEBUG(UInt m_modification_count);
};




//
//impl

REFLEX_NS(Reflex)

template <class TYPE> REFLEX_INLINE TYPE & Deref(TYPE ** type);	//no longer used, checming for use

REFLEX_END

REFLEX_NS(Reflex::Detail)

template <class COMPARE, class TYPE, class VALUE> REFLEX_INLINE TYPE * SearchSortedGTE_Ptr(const ArrayRegion <TYPE> & values, VALUE && value, TYPE * empty_fallback)
{
	if (values)
	{
		auto left = values.data;

		auto right = left + values.size - 1;

		auto result = left - 1;

		while (left <= right)
		{
			auto mid = left + (right - left) / 2;

			if (COMPARE::lt(*mid, std::forward<VALUE>(value)))
			{
				left = mid + 1;
			}
			else
			{
				result = mid;

				right = mid - 1;
			}
		}

		return result;
	}
	else
	{
		return empty_fallback;
	}
}

template <class COMPARE, class ITEM, class VALUE> REFLEX_INLINE Int32 SearchSortedGTE(const ArrayView <ITEM> & values, VALUE && value, Int32 empty_fallback = -1)
{
	return Int32(SearchSortedGTE_Ptr<COMPARE>(values, std::forward<VALUE>(value), values.data + empty_fallback) - values.data);
}

template <class COMPARE, class ITEM, class VALUE> REFLEX_INLINE Int32 SearchSortedLT(const ArrayView <ITEM> & values, VALUE && value)
{
	Int32 left = 0;

	Int32 right = values.size - 1;

	Int32 result = -1;

	while (left <= right)
	{
		Int32 mid = left + (right - left) / 2;

		if (COMPARE::lt(values[mid], std::forward<VALUE>(value)))
		{
			result = mid;

			left = mid + 1;
		}
		else
		{
			right = mid - 1;
		}
	}

	return result;
}

REFLEX_END

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Sequence()
	: Sequence(g_default_allocator)
{
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Sequence(Allocator & allocator)
	: m_data(allocator)
{
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Sequence(const Sequence & sequence, Allocator & allocator)
	: m_data(sequence.m_data, allocator)
{
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Sequence(Sequence && temp)
	: m_data(temp.m_data.allocator)
{
	Swap(temp);
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Sequence(std::initializer_list <Item> && list, Allocator & allocator)
	: Sequence(allocator)
{
	Allocate(UInt32(list.size()));

	for (auto & i : list) InsertItem(Copy(i));
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::~Sequence()
{
	Clear();
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Compact()
{
	Modify();

	m_data.Compact();
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Allocate(UInt capacity)
{
	Modify();

	m_data.Allocate(capacity);
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline Reflex::UInt Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::GetSize() const
{
	return m_data.GetSize();
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Clear()
{
	Modify();

	m_data.Clear();
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline VALUE & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Push(const KEY & key, VALUE && value)
{
	Modify();

	return m_data.Push({ key, std::move(value) }).value;
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline VALUE & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::InsertItem(Item && t)
{
	Modify();

	if (Idx result = Detail::SearchSortedGTE<typename ItemImpl::Compare>(ToView(static_cast<ArrayBaseType&>(m_data)), t.key, 0))
	{
		return m_data.Insert(result.value, std::move(Reinterpret<ItemImpl>(t))).value;
	}
	else
	{
		return m_data.Push(std::move(Reinterpret<ItemImpl>(t))).value;
	}
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline VALUE & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Insert(const KEY & key)
{
	return InsertItem({ key, {} });
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline VALUE & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Insert(const KEY & key, VALUE && rhs)
{
	return InsertItem({ key, std::move(rhs) });
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline VALUE & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Insert(const KEY & key, const VALUE & value)
{
	return InsertItem({ key, Copy(value) });
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline VALUE & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::SetItem(Item && t)
{
	if (Idx idx = Detail::SearchSortedGTE<typename ItemImpl::Compare>(ToView(static_cast<ArrayBaseType&>(m_data)), t.key))
	{
		auto & item = this->operator[](idx.value);

		if (COMPARE::eq(item.key, t.key))
		{
			item.value = std::move(t.value);

			return item.value;
		}
		else
		{
			Modify();

			return m_data.Insert(idx.value, std::move(Reinterpret<ItemImpl>(t))).value;
		}
	}
	else
	{
		Modify();

		return m_data.Push(std::move(Reinterpret<ItemImpl>(t))).value;
	}
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline VALUE & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Set(const KEY & key)
{
	return SetItem({ key, {} });
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline VALUE & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Set(const KEY & key, VALUE && value)
{
	return SetItem({ key, std::move(value) });
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline VALUE & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Set(const KEY & key, const VALUE & value)
{
	return SetItem({ key, value });
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> template <class ... ARGS> inline VALUE & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Acquire(const KEY & key, ARGS && ... args)
{
	if (Idx idx = Detail::SearchSortedGTE<typename ItemImpl::Compare>(ToView(static_cast<ArrayBaseType&>(m_data)), key))
	{
		auto & item = this->operator[](idx.value);

		if (COMPARE::eq(item.key, key))
		{
			return item.value;
		}
		else
		{
			Modify();

			return m_data.Insert(idx.value, { key, VALUE(std::forward<ARGS>(args)...) }).value;
		}
	}
	else
	{
		Modify();

		return m_data.Push({ key, VALUE(std::forward<ARGS>(args)...) }).value;
	}
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Remove(UInt idx)
{
	Modify();

	m_data.Remove(idx);
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Remove(UInt idx, UInt n)
{
	Modify();

	m_data.Remove(idx, n);
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE Reflex::Idx Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::SearchGTE(const KEY & key) const
{
	return Detail::SearchSortedGTE<typename ItemImpl::Compare>(ToView(static_cast<const ArrayBaseType&>(m_data)), key);
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE Reflex::Idx Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::SearchLT(const KEY & key) const
{
	return Detail::SearchSortedLT<typename ItemImpl::Compare>(ToView(static_cast<const ArrayBaseType&>(m_data)), key);
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline Reflex::Idx Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Search(const KEY & key) const
{
	if (Idx idx = Detail::SearchSortedGTE<typename ItemImpl::Compare>(ToView(static_cast<const ArrayBaseType&>(m_data)), key))
	{
		auto & item = this->operator[](idx.value);

		if (COMPARE::eq(item.key, key)) return idx;
	}

	return {};
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline VALUE * Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::SearchValue(const KEY & key, VALUE * fallback)
{
	if (Idx idx = Detail::SearchSortedGTE<typename ItemImpl::Compare>(ToView(static_cast<ArrayBaseType&>(m_data)), key))
	{
		auto & item = this->operator[](idx.value);

		if (COMPARE::eq(item.key, key)) return GetAdr(item.value);
	}

	return fallback;
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline const VALUE * Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::SearchValue(const KEY & key, const VALUE * fallback) const
{
	if (Idx idx = Detail::SearchSortedGTE<typename ItemImpl::Compare>(ToView(static_cast<const ArrayBaseType&>(m_data)), key))
	{
		auto & item = this->operator[](idx.value);

		if (COMPARE::eq(item.key, key)) return GetAdr(item.value);
	}

	return fallback;
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE bool Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Empty() const
{
	return m_data.Empty();
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE typename Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Item & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::GetFirst()
{
	return *m_data.GetFirst();
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE const typename Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Item & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::GetFirst() const
{
	return *m_data.GetFirst();
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE typename Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Item & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::GetLast()
{
	return *m_data.GetLast();
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE const typename Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Item & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::GetLast() const
{
	return *m_data.GetLast();
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE typename Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Item & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::operator[](UInt idx)
{
	return m_data[idx];
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE const typename Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Item & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::operator[](UInt idx) const
{
	return m_data[idx];
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::operator bool() const
{
	return m_data.GetSize();
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE void Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Swap(Sequence & sequence)
{
	m_data.Swap(sequence.m_data);

	Modify();
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE Reflex::Sequence <KEY,VALUE,COMPARE,CONTIGUOUS> & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::operator=(Sequence && temp)
{
	Swap(temp);

	return *this;
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline bool Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::operator==(const Sequence & value) const
{
	return m_data == value.m_data;
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline bool Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::operator!=(const Sequence & value) const
{
	return m_data != value.m_data;
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::Modify()
{
	REFLEX_IF_DEBUG(m_modification_count++);
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS>
struct Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::ItemImpl::Compare
{
	static REFLEX_INLINE bool lt(const ConditionalType<CONTIGUOUS,ItemImpl,ItemImpl*> & a, const KEY & b)
	{
		if constexpr (CONTIGUOUS)
		{
			return COMPARE::lt(a.key, b);
		}
		else
		{
			return COMPARE::lt(a->key, b);
		}
	}
};

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::DfArray::DfArray(Allocator & allocator)
	: Array<ItemImpl*>(allocator)
{
	REFLEX_STATIC_ASSERT(!CONTIGUOUS);
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::DfArray::DfArray(const DfArray & source, Allocator & allocator)
	: Array<ItemImpl*>(allocator)
{
	REFLEX_STATIC_ASSERT(!CONTIGUOUS);

	Base::SetSize(source.GetSize());

	auto psrc = source.GetData();

	for (auto & item : *this)
	{
		item = Detail::Allocate<ItemImpl>(allocator, AllocInfo(Detail::GetDebugTypeName<Sequence>(), "DfArray(const DfArray&)"));

		Detail::Constructor<ItemImpl>::Construct(item, **psrc++);
	}
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::DfArray::Clear()
{
	for (auto & item : *this)
	{
		Detail::Constructor<ItemImpl>::Destruct(*item);

		Base::allocator->Free(item);
	}

	Base::Clear();
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE void Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::DfArray::Remove(UInt idx)
{
	auto item = Base::operator[](idx);

	Detail::Constructor<ItemImpl>::Destruct(*item);

	Base::allocator->Free(item);

	Base::Remove(idx);
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE void Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::DfArray::Remove(UInt idx, UInt n)
{
	REFLEX_LOOP_PTR(Base::GetData() + idx, pitem, n)
	{
		auto item = *pitem;

		Detail::Constructor<ItemImpl>::Destruct(*item);

		Base::allocator->Free(item);
	}

	Base::Remove(idx, n);
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE typename Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::ItemImpl & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::DfArray::Insert(UInt idx, ItemImpl && rhs)
{
	auto item = Detail::Allocate<ItemImpl>(Base::allocator, AllocInfo(Detail::GetDebugTypeName<Sequence>(), "Insert"));

	Detail::Constructor<ItemImpl>::Construct(item, std::move(rhs));

	Base::Insert(idx, item);

	return *item;
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE typename Reflex::Sequence<KEY, VALUE, COMPARE, CONTIGUOUS>::ItemImpl & Reflex::Sequence<KEY, VALUE, COMPARE, CONTIGUOUS>::DfArray::Push(ItemImpl && rhs)
{
	auto item = Detail::Allocate<ItemImpl>(Base::allocator, AllocInfo(Detail::GetDebugTypeName<Sequence>(), "Push"));

	Detail::Constructor<ItemImpl>::Construct(item, std::move(rhs));

	Base::Push(item);

	return *item;
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE typename Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::ItemImpl & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::DfArray::operator[](UInt idx)
{
	return *this->Base::operator[](idx);
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> REFLEX_INLINE const typename Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::ItemImpl & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::DfArray::operator[](UInt idx) const
{
	return *this->Base::operator[](idx);
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::DfArray::operator=(const DfArray & source)
{
	auto & allocator = *Base::allocator;

	Clear();

	Base::SetSize(source.GetSize());

	auto psrc = source.GetData();

	for (auto & item : *this)
	{
		item = Detail::Allocate<ItemImpl>(allocator, AllocInfo(Detail::GetDebugTypeName<Sequence>(), "operator="));

		Detail::Constructor<ItemImpl>::Construct(item, **psrc++);
	}
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> template <bool CONST> inline Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::ItrImpl<CONST>::ItrImpl(SequenceType & sequence, UInt idx)
	: m_ptr(sequence.GetData() + idx)
#if REFLEX_DEBUG
	, modification_count_ref(sequence.m_modification_count)
	, m_modification_count(sequence.m_modification_count)
#endif
{
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> template <bool CONST> inline auto & Reflex::Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>::ItrImpl<CONST>::operator*() const
{
	REFLEX_IF_DEBUG(REFLEX_ASSERT(m_modification_count == modification_count_ref));

	if constexpr (CONTIGUOUS)
	{
		return *m_ptr;
	}
	else
	{
		return **m_ptr;
	}
}

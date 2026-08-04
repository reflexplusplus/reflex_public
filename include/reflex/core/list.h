#pragma once

#include "object/object.h"
#include "idx.h"
#include "meta/auxtypes.h"
#include "function/type.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE, bool RETAIN = true, class BASE = Object> class List;

	template <class TYPE, bool RETAIN = true, class BASE = Object> class Item;


	template <class auto_1> Idx LookupIndex(auto_1 && item, Idx fallback = {});


	template <bool BOUNDS_CHECK = false, class TYPE, bool RETAIN, class BASE> TYPE * LookupItemAtIndex(List <TYPE,RETAIN,BASE> & list, UInt idx);

	template <bool BOUNDS_CHECK = false, class TYPE, bool RETAIN, class BASE> const TYPE * LookupItemAtIndex(const List <TYPE,RETAIN,BASE> & list, UInt idx);


	template <class CALLBACK, class TYPE, bool RETAIN, class BASE, class... VARGS> auto SafeIterate(List <TYPE,RETAIN,BASE> & list, CALLBACK callback, VARGS && ... args);

	template <class CALLBACK, class TYPE, bool RETAIN, class BASE, class... VARGS> auto SafeReverseIterate(List <TYPE, RETAIN, BASE> & list, CALLBACK callback, VARGS && ... args);

}




//
//List

template <class TYPE, bool RETAIN, class BASE>
class Reflex::List
{
public:

	REFLEX_NONCOPYABLE(List);



	//types

	using Type = TYPE;

	using ItemType = Reflex::Item <TYPE,RETAIN,BASE>;


	template <bool CONST, bool REVERSE> class ItrImpl;

	using Itr = ItrImpl <false,false>;

	using ConstItr = ItrImpl <true,false>;

	using ReverseItr = ItrImpl <false,true>;

	using ConstReverseItr = ItrImpl <true,true>;


	template <bool CONST, bool REVERSE> class RangeImpl;

	using ItemRange = RangeImpl <false,false>;

	using ConstItemRange = RangeImpl <true,false>;



	//lifetime

	List();

	~List();



	//info

	UInt GetNumItem() const;

	bool Empty() const;

	explicit operator bool() const;



	//access

	TYPE * GetFirst();

	const TYPE * GetFirst() const;


	TYPE * GetLast();

	const TYPE * GetLast() const;



	//iterate

	Itr begin() { return Cast<TYPE>(Cast<ItemType>(m_first)); }

	Itr end() { return {}; }

	ConstItr begin() const { return Cast<TYPE>(Cast<ItemType>(m_first)); }

	ConstItr end() const { return {}; }


	ReverseItr rbegin() { return Cast<TYPE>(Cast<ItemType>(m_last)); }

	ReverseItr rend() { return {}; }

	ConstReverseItr rbegin() const { return Cast<TYPE>(Cast<ItemType>(m_last)); }

	ConstReverseItr rend() const { return {}; }



protected:

	//content

	void Clear();



private:

	friend ItemType;

	
	void * m_first;

	void * m_last;

	UInt m_nitem;

	REFLEX_IF_DEBUG(public: UInt m_modification_count);
};

namespace Reflex { template <class TYPE, bool RETAIN, class BASE> struct IsBoolCastable < List <TYPE,RETAIN,BASE> > { static constexpr bool value = true; }; }




//
//Item

template <class TYPE, bool RETAIN, class BASE>
class Reflex::Item : public BASE
{
public:

	//types

	using Type = TYPE;

	using List = Reflex::List <TYPE,RETAIN,BASE>;



	//lifetime

	~Item();



	//list

	List * GetList(bool debug_ignore_recursion_guard_TEMP = false);

	const List * GetList(bool debug_ignore_recursion_guard_TEMP = false) const;



	//siblings

	TYPE * GetPrev();

	const TYPE * GetPrev() const;


	TYPE * GetNext();

	const TYPE * GetNext() const;



	//special (for generic implementations)

	static consteval UInt GetOffset() { return REFLEX_OFFSETOF(Item, m_list); }



protected:

	//lifetime

	Item();

	template <typename ... VARGS> Item(VARGS && ... v);



	//location

	void Attach(List & list);

	void InsertBefore(TYPE & item);

	void InsertAfter(TYPE & item);

	bool SendBottom();

	bool SendTop();

	void SetIndex(UInt idx);

	bool Detach();



	//special templated callback

	inline void OnAttach(List & list) {}

	inline void OnDetach(List & list) {}



private:

	friend Reflex::List <TYPE,RETAIN,BASE>;


	List * m_list;

	Item * m_prev;

	Item * m_next;

	REFLEX_IF_DEBUG(UInt m_detaching_guard = 0);

};




//
//ItemIterator

template <class TYPE, bool RETAIN, class BASE>
template <bool CONST, bool REVERSE>
class Reflex::List<TYPE,RETAIN,BASE>::RangeImpl
{
public:

	//types

	using List = ConditionalType <CONST,const typename TYPE::List,typename TYPE::List>;

	using Type = ConditionalType <CONST,const TYPE,TYPE>;



	//lifetime

	RangeImpl(List & list)
		: m_begin(REVERSE ? list.GetLast() : list.GetFirst())
		, m_end(nullptr)
	{
	}

	RangeImpl(Type & first, Type & last)
		: m_begin(REVERSE ? &last : &first)
		, m_end(REVERSE ? first.GetPrev() : last.GetNext())
	{
	}



	//valid

	operator bool() const { return m_begin != m_end; }



	//iterate

	ItrImpl <CONST,REVERSE> begin() { return m_begin; }

	ItrImpl <CONST,REVERSE> end() { return m_end; }



private:

	Type * m_begin;

	Type * m_end;

};




//
//ItemIterator

template <class TYPE, bool RETAIN, class BASE>
template <bool CONST, bool REVERSE>
class Reflex::List<TYPE,RETAIN,BASE>::ItrImpl
{
public:

	//types

	using Type = ConditionalType <CONST,const TYPE,TYPE>;



	//lifetime

	ItrImpl();

	ItrImpl(Type * item);



	//operators

	Type & operator*() const { Validate(); return *m_itr; }

	bool operator!=(const ItrImpl & itr) const;

	void operator++();



private:

	void Validate() const;


	Type * m_itr;

	REFLEX_IF_DEBUG(UInt m_modification_count);

};




//
//impl

REFLEX_NS(Reflex::Detail)

template <bool REVERSE, class TYPE> inline TRef <TYPE> Traverse(TYPE * & itr)
{
	auto & current = *itr;

	itr = REVERSE ? itr->GetPrev() : itr->GetNext();

	return current;
}

REFLEX_END

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE Reflex::List<TYPE,RETAIN,BASE>::List()
	: m_first(nullptr)
	, m_last(nullptr)
	, m_nitem(0)
	REFLEX_IF_DEBUG(, m_modification_count(0))
{
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE Reflex::List<TYPE,RETAIN,BASE>::~List()
{
	Clear();
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE void Reflex::List<TYPE,RETAIN,BASE>::Clear()
{
	while (auto entry = Cast<ItemType>(m_last))
	{
		entry->Detach();
	}
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE Reflex::UInt Reflex::List<TYPE,RETAIN,BASE>::GetNumItem() const
{
	return m_nitem;
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE bool Reflex::List<TYPE,RETAIN,BASE>::Empty() const
{
	return m_last == nullptr;
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE Reflex::List<TYPE,RETAIN,BASE>::operator bool() const
{
	return m_last;
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE TYPE * Reflex::List<TYPE,RETAIN,BASE>::GetFirst()
{
	return Cast<TYPE>(Cast<ItemType>(m_first));
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE const TYPE * Reflex::List<TYPE,RETAIN,BASE>::GetFirst() const
{
	return Cast<TYPE>(Cast<ItemType>(m_first));
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE TYPE * Reflex::List<TYPE,RETAIN,BASE>::GetLast()
{
	return Cast<TYPE>(Cast<ItemType>(m_last));
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE const TYPE * Reflex::List<TYPE,RETAIN,BASE>::GetLast() const
{
	return Cast<TYPE>(Cast<ItemType>(m_last));
}




//
//

REFLEX_NS(Reflex::Detail)

template <class BASE>	//this was the quickest way to implement SendTop/SendBottom/SetIndex so that they dont retain/release and dont notify
struct ItemWithoutCallbackHack : public Item <ItemWithoutCallbackHack<BASE>, false, BASE>
{
	using Item<ItemWithoutCallbackHack, false, BASE>::Attach;
	using Item<ItemWithoutCallbackHack, false, BASE>::Detach;
	using Item<ItemWithoutCallbackHack, false, BASE>::InsertBefore;
	using Item<ItemWithoutCallbackHack, false, BASE>::InsertAfter;

	void OnAttach(typename Item<ItemWithoutCallbackHack, false, BASE>::List & list) {}

	void OnDetach(typename Item<ItemWithoutCallbackHack, false, BASE>::List & list) {}
};

REFLEX_END

template <class TYPE, bool RETAIN, class BASE> Reflex::Item<TYPE,RETAIN,BASE>::Item()
	: m_list(nullptr)
	, m_prev(nullptr)
	, m_next(nullptr)
{
}

template <class TYPE, bool RETAIN, class BASE> template <typename ... VARGS> Reflex::Item<TYPE,RETAIN,BASE>::Item(VARGS && ... v)
	: BASE(std::forward<VARGS>(v)...)
	, m_list(nullptr)
	, m_prev(nullptr)
	, m_next(nullptr)
{
}

template <class TYPE, bool RETAIN, class BASE> inline Reflex::Item<TYPE,RETAIN,BASE>::~Item()
{
	if (m_list)
	{
		auto & list = *m_list;

		if (m_prev)
		{
			m_prev->m_next = m_next;
		}
		else
		{
			list.m_first = m_next;
		}

		if (m_next)
		{
			m_next->m_prev = m_prev;
		}
		else
		{
			list.m_last = m_prev;
		}

		list.m_nitem--;

		REFLEX_IF_DEBUG(list.m_modification_count++);
	}
}

template <class TYPE, bool RETAIN, class BASE> inline void Reflex::Item<TYPE,RETAIN,BASE>::Attach(List & list)
{
	auto self = Cast<TYPE>(this);

	if constexpr (RETAIN) Retain(self);

	Detach();

	m_list = &list;

	m_prev = Cast<Item>(list.m_last);

	m_next = nullptr;

	if (m_prev)
	{
		m_prev->m_next = this;
	}
	else
	{
		list.m_first = this;
	}

	list.m_last = this;

	list.m_nitem++;

	REFLEX_IF_DEBUG(list.m_modification_count++);

	self->OnAttach(list);
}

template <class TYPE, bool RETAIN, class BASE> inline void Reflex::Item<TYPE,RETAIN,BASE>::InsertBefore(TYPE & item)
{
	REFLEX_ASSERT(this != &item);

	auto target_list = item.GetList();

	if (m_list != target_list)	//from no list, or to different list
	{
		auto self = Cast<TYPE>(this);

		if constexpr (RETAIN) Retain(self);

		Detach();

		m_list = target_list;

		m_prev = item.m_prev;

		m_next = &item;

		if (item.m_prev)
		{
			item.m_prev->m_next = this;
		}
		else
		{
			target_list->m_first = this;
		}

		item.m_prev = this;

		target_list->m_nitem++;

		REFLEX_IF_DEBUG(target_list->m_modification_count++);

		self->OnAttach(*target_list);
	}
	else if (target_list)	//same list
	{
		using Accessor = Detail::ItemWithoutCallbackHack <BASE>;

		auto self = Reinterpret<Accessor>(this);

		self->Detach();

		m_list = target_list;

		m_prev = item.m_prev;

		m_next = &item;

		if (item.m_prev)
		{
			item.m_prev->m_next = this;
		}
		else
		{
			target_list->m_first = this;
		}

		item.m_prev = this;

		target_list->m_nitem++;

		REFLEX_IF_DEBUG(target_list->m_modification_count++);

		//self->OnAttach(*target_list);
	}
	else
	{
		Detach();
	}
}

template <class TYPE, bool RETAIN, class BASE> inline void Reflex::Item<TYPE,RETAIN,BASE>::InsertAfter(TYPE & item)
{
	REFLEX_ASSERT(this != &item);

	auto target_list = item.GetList();

	if (m_list != target_list)	//from no list, or to different list
	{
		auto self = Cast<TYPE>(this);

		if constexpr (RETAIN) Retain(self);

		Detach();

		m_list = target_list;

		m_prev = &item;

		m_next = item.m_next;

		if (item.m_next)
		{
			item.m_next->m_prev = this;
		}
		else
		{
			target_list->m_last = this;
		}

		item.m_next = this;

		target_list->m_nitem++;

		REFLEX_IF_DEBUG(target_list->m_modification_count++);

		self->OnAttach(*target_list);
	}
	else if (target_list)	//same list
	{
		using Accessor = Detail::ItemWithoutCallbackHack <BASE>;

		auto self = Reinterpret<Accessor>(this);

		self->Detach();

		m_list = target_list;

		m_prev = &item;

		m_next = item.m_next;

		if (item.m_next)
		{
			item.m_next->m_prev = this;
		}
		else
		{
			target_list->m_last = this;
		}

		item.m_next = this;

		target_list->m_nitem++;

		REFLEX_IF_DEBUG(target_list->m_modification_count++);

		//self->OnAttach(*target_list);
	}
	else
	{
		Detach();
	}
}

template <class TYPE, bool RETAIN, class BASE> inline bool Reflex::Item<TYPE,RETAIN,BASE>::SendBottom()
{
	using Accessor = Detail::ItemWithoutCallbackHack <BASE>;

	auto pitem = Reinterpret<Accessor>(this);

	if (pitem->GetPrev())
	{
		auto list = pitem->GetList();

		pitem->InsertBefore(*list->GetFirst());

		return true;
	}

	return false;
}

template <class TYPE, bool RETAIN, class BASE> inline bool Reflex::Item<TYPE,RETAIN,BASE>::SendTop()
{
	using Accessor = Detail::ItemWithoutCallbackHack <BASE>;

	auto pitem = Reinterpret<Accessor>(this);

	if (pitem->GetNext())
	{
		auto list = pitem->GetList();

		pitem->InsertAfter(*list->GetLast());

		return true;
	}

	return false;
}

template <class TYPE, bool RETAIN, class BASE> inline void Reflex::Item<TYPE,RETAIN,BASE>::SetIndex(UInt idx)
{
	using Accessor = Detail::ItemWithoutCallbackHack <BASE>;

	auto pitem = Reinterpret<Accessor>(this);

	if (auto list = pitem->GetList())
	{
		pitem->Detach();

		if (idx < list->GetNumItem())
		{
			auto prev = list->GetFirst();

			while (idx--) prev = prev->GetNext();

			pitem->InsertBefore(*prev);
		}
		else
		{
			pitem->Attach(*list);
		}
	}
}

template <class TYPE, bool RETAIN, class BASE> inline bool Reflex::Item<TYPE,RETAIN,BASE>::Detach()
{
	if (m_list)
	{
		REFLEX_ASSERT(!m_detaching_guard);

		auto self = Cast<TYPE>(this);

		auto & list = *m_list;

		REFLEX_IF_DEBUG(m_detaching_guard++);
		self->OnDetach(list);
		REFLEX_IF_DEBUG(m_detaching_guard--);

		if (m_prev)
		{
			m_prev->m_next = m_next;
		}
		else
		{
			list.m_first = m_next;
		}

		if (m_next)
		{
			m_next->m_prev = m_prev;
		}
		else
		{
			list.m_last = m_prev;
		}

		m_list = nullptr;
		m_prev = nullptr;
		m_next = nullptr;

		if constexpr (RETAIN) Release(self);

		list.m_nitem--;

		REFLEX_IF_DEBUG(list.m_modification_count++);

		return true;
	}

	return false;
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE typename Reflex::Item<TYPE,RETAIN,BASE>::List * Reflex::Item<TYPE,RETAIN,BASE>::GetList(bool debug_ignore_recursion_guard_TEMP)
{
	REFLEX_ASSERT(!m_detaching_guard || debug_ignore_recursion_guard_TEMP);
	return m_list;
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE const typename Reflex::Item<TYPE,RETAIN,BASE>::List * Reflex::Item<TYPE,RETAIN,BASE>::GetList(bool debug_ignore_recursion_guard_TEMP) const
{
	REFLEX_ASSERT(!m_detaching_guard || debug_ignore_recursion_guard_TEMP);
	return m_list;
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE TYPE * Reflex::Item<TYPE,RETAIN,BASE>::GetPrev()
{
	return Cast<TYPE>(m_prev);
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE const TYPE * Reflex::Item<TYPE,RETAIN,BASE>::GetPrev() const
{
	return Cast<TYPE>(m_prev);
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE TYPE * Reflex::Item<TYPE,RETAIN,BASE>::GetNext()
{
	return Cast<TYPE>(m_next);
}

template <class TYPE, bool RETAIN, class BASE> REFLEX_INLINE const TYPE * Reflex::Item<TYPE,RETAIN,BASE>::GetNext() const
{
	return Cast<TYPE>(m_next);
}




//
//itr

template <class TYPE, bool RETAIN, class BASE> template <bool CONST, bool REVERSE> REFLEX_INLINE Reflex::List<TYPE,RETAIN,BASE>::ItrImpl<CONST,REVERSE>::ItrImpl()
	: m_itr(nullptr)
{
	REFLEX_IF_DEBUG(m_modification_count = 0);
}

template <class TYPE, bool RETAIN, class BASE> template <bool CONST, bool REVERSE> REFLEX_INLINE Reflex::List<TYPE,RETAIN,BASE>::ItrImpl<CONST,REVERSE>::ItrImpl(Type * item)
	: m_itr(item)
{
	REFLEX_IF_DEBUG(m_modification_count = item ? Cast<ItemType>(item)->GetList()->m_modification_count : 0);
}

template <class TYPE, bool RETAIN, class BASE> template <bool CONST, bool REVERSE> REFLEX_INLINE bool Reflex::List<TYPE,RETAIN,BASE>::ItrImpl<CONST,REVERSE>::operator!=(const ItrImpl & itr) const 
{
	return m_itr != itr.m_itr; 
}

template <class TYPE, bool RETAIN, class BASE> template <bool CONST, bool REVERSE> REFLEX_INLINE void Reflex::List<TYPE,RETAIN,BASE>::ItrImpl<CONST,REVERSE>::operator++() 
{
	if constexpr (REVERSE) 
	{ 
		m_itr = m_itr->GetPrev();
	} 
	else 
	{ 
		m_itr = m_itr->GetNext(); 
	}
}

template <class TYPE, bool RETAIN, class BASE> template <bool CONST, bool REVERSE> inline void Reflex::List<TYPE,RETAIN,BASE>::ItrImpl<CONST,REVERSE>::Validate() const
{
	REFLEX_IF_DEBUG(REFLEX_ASSERT(m_modification_count == Cast<ItemType>(m_itr)->GetList()->m_modification_count));
}




//
//helpers

REFLEX_NS(Reflex::Detail)

template <class TYPE> struct TempToRef
{
	using type = TYPE &;
};

template <class TYPE> struct TempToRef <TYPE&&>
{
	using type = TYPE &;
};

template <class TYPE> using TempToRefT = TempToRef<TYPE>::type;

template <class T> constexpr bool kIsCopyableArg = kIsRawCopyable<NonRefT <T>> && (sizeof(NonRefT <T>) <= sizeof(void*));


template <class T> using ArgStorageType = ConditionalType < kIsCopyableArg <T>, NonRefT <T>, TempToRefT <T> >;

template <class T> using ArgPassType = ConditionalType < kIsCopyableArg <T>, NonRefT <T>, ConditionalType < kIsReference <T>, T, const T &> >;


void SafeIterate(List <Object> & list, UInt item_offset, bool st, void * client, FunctionPointer <void(void*,Object&)> callback);

bool SafeIterate(List <Object> & list, UInt item_offset, bool st, void * client, FunctionPointer <bool(void*,Object&)> callback);

void SafeReverseIterate(List <Object> & list, UInt item_offset, bool st, void * client, FunctionPointer <void(void*, Object&)> callback);

bool SafeReverseIterate(List <Object> & list, UInt item_offset, bool st, void * client, FunctionPointer <bool(void*, Object&)> callback);

REFLEX_END

template <bool BOUNDS_CHECK, class TYPE, bool RETAIN, class BASE> REFLEX_INLINE TYPE * Reflex::LookupItemAtIndex(List <TYPE,RETAIN,BASE> & list, UInt idx)
{
	if constexpr (BOUNDS_CHECK) if (idx >= list.GetNumItem()) return nullptr;

	REFLEX_ASSERT(idx < list.GetNumItem());

	auto itr = list.GetFirst();

	while (idx--) itr = itr->GetNext();

	return itr;
}

template <bool BOUNDS_CHECK, class TYPE, bool RETAIN, class BASE> REFLEX_INLINE const TYPE * Reflex::LookupItemAtIndex(const List <TYPE,RETAIN,BASE> & list, UInt idx)
{
	return LookupItemAtIndex<BOUNDS_CHECK>(RemoveConst(list), idx);
}

template <class auto_1> inline Reflex::Idx Reflex::LookupIndex(auto_1 && ref, Idx fallback)
{
	auto & item = Deref(ref);

	if (auto list = item.GetList())
	{
		UInt idx = 0;

		for (auto itr = list->GetFirst(); itr; itr = itr->GetNext())
		{
			if (itr == &item) return idx;

			idx++;
		}
	}

	return fallback;
}

template <class CALLBACK, class TYPE, bool RETAIN, class BASE, class... VARGS> REFLEX_INLINE auto Reflex::SafeIterate(List <TYPE,RETAIN,BASE> & list, CALLBACK callback, VARGS && ... args)
{
	struct Context
	{
		CALLBACK & callback;

		Tuple < Detail::ArgStorageType <VARGS>... > args;
	};

	using ListType = List <TYPE,RETAIN,BASE>;

	using ItemType = typename ListType::Type;

	REFLEX_STATIC_ASSERT(kIsObject<BASE>);

	Context ctx = { callback, std::forward<VARGS>(args)... };

	return Detail::SafeIterate(Reinterpret<Reflex::List<Object>>(list), ItemType::GetOffset(), kIsSingleThreadExclusive<ItemType>, &ctx, [](void * data, Object & object)
	{
		auto & ctx = *Reinterpret<Context>(data);

		if constexpr (sizeof...(VARGS) == 0)
		{
			return ctx.callback(Cast<ItemType>(object));
		}
		else if constexpr (sizeof...(VARGS) == 1)
		{
			return ctx.callback(Cast<ItemType>(object), ctx.args.a);
		}
		else if constexpr (sizeof...(VARGS) == 2)
		{
			return ctx.callback(Cast<ItemType>(object), ctx.args.a, ctx.args.b);
		}
		else if constexpr (sizeof...(VARGS) == 3)
		{
			return ctx.callback(Cast<ItemType>(object), ctx.args.a, ctx.args.b, ctx.args.c);
		}
		else if constexpr (sizeof...(VARGS) == 4)
		{
			return ctx.callback(Cast<ItemType>(object), ctx.args.a, ctx.args.b, ctx.args.c, ctx.args.d);
		}

		REFLEX_STATIC_ASSERT(sizeof...(VARGS) <= 4);
	});
}

template <class CALLBACK, class TYPE, bool RETAIN, class BASE, class... VARGS> REFLEX_INLINE auto Reflex::SafeReverseIterate(List <TYPE, RETAIN, BASE> & list, CALLBACK callback, VARGS && ... args)
{
	struct Context
	{
		CALLBACK & callback;

		Tuple < Detail::ArgStorageType <VARGS>... > args;
	};

	using ListType = List <TYPE, RETAIN, BASE>;

	using ItemType = typename ListType::Type;

	REFLEX_STATIC_ASSERT(kIsObject<BASE>);

	Context ctx = { callback, std::forward<VARGS>(args)... };

	return Detail::SafeReverseIterate(Reinterpret<Reflex::List<Object>>(list), ItemType::GetOffset(), kIsSingleThreadExclusive<ItemType>, &ctx, [](void * data, Object & object)
	{
		auto & ctx = *Reinterpret<Context>(data);

		if constexpr (sizeof...(VARGS) == 0)
		{
			return ctx.callback(Cast<ItemType>(object));
		}
		else if constexpr (sizeof...(VARGS) == 1)
		{
			return ctx.callback(Cast<ItemType>(object), ctx.args.a);
		}
		else if constexpr (sizeof...(VARGS) == 2)
		{
			return ctx.callback(Cast<ItemType>(object), ctx.args.a, ctx.args.b);
		}
		else if constexpr (sizeof...(VARGS) == 3)
		{
			return ctx.callback(Cast<ItemType>(object), ctx.args.a, ctx.args.b, ctx.args.c);
		}
		else if constexpr (sizeof...(VARGS) == 4)
		{
			return ctx.callback(Cast<ItemType>(object), ctx.args.a, ctx.args.b, ctx.args.c, ctx.args.d);
		}

		REFLEX_STATIC_ASSERT(sizeof...(VARGS) <= 4);
	});
}

#define REFLEX_UPCAST_ITEM(BASE, TYPE) using Type = TYPE; REFLEX_INLINE Type * GetPrev() { return Cast<Type>(BASE::GetPrev()); } REFLEX_INLINE const Type * GetPrev() const { return Cast<Type>(BASE::GetPrev()); } REFLEX_INLINE TYPE * GetNext() { return Cast<TYPE>(BASE::GetNext()); } REFLEX_INLINE const Type * GetNext() const { return Cast<Type>(BASE::GetNext()); }

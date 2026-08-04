#pragma once

#include "list.h"
#include "array.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE, class BASE = Object, bool RETAIN = true> class Node;


	template <class TYPE> Idx LookupBranchIndex(const TYPE & root, const TYPE & node);

	template <class TYPE> bool BranchContains(const TYPE & parent, const TYPE & child);

}




//
//Detail

namespace Reflex::Detail
{

	template <class TYPE, class BASE, bool RETAIN> class NodeBranchIterator;

	template <class TYPE, class BASE, bool RETAIN> class NodeParentIterator;

}




//
//decl

template <class TYPE, class BASE, bool RETAIN>
class Reflex::Node :
	public Item <TYPE,RETAIN,BASE>,
	public List <TYPE,RETAIN,BASE>
{
public:

	//declarations

	using BaseType = BASE;

	using NodeType = Reflex::Node <TYPE,BASE,RETAIN>;

	using Type = TYPE;

	using Item = Reflex::Item <TYPE,RETAIN,BASE>;

	using List = Reflex::List <TYPE,RETAIN,BASE>;


	using BranchIterator = Detail::NodeBranchIterator <TYPE,BASE,RETAIN>;

	using ConstBranchIterator = Detail::NodeBranchIterator <const TYPE,BASE,RETAIN>;


	using ParentRange = Detail::RangeHolder < Detail::NodeParentIterator <TYPE,BASE,RETAIN> >;

	using ConstParentRange = Detail::RangeHolder < Detail::NodeParentIterator <const TYPE,BASE,RETAIN> >;



	//access

	TYPE * GetParent();

	const TYPE * GetParent() const;



	//disambig

	using List::Empty;

	using List::operator bool;



protected:

	using Item::Item;



	//location

	void Attach(Node & node);



	//content

	using List::Clear;

};




//
//NodeParentIterator

template <class TYPE, class BASE, bool RETAIN>
class Reflex::Detail::NodeParentIterator
{
public:

	//types

	using Type = TYPE;



	//lifetime

	NodeParentIterator();

	NodeParentIterator(TYPE & start);



	//current

	bool operator!=(const NodeParentIterator & item) const;

	TYPE & operator*() const;

	void operator++();



private:

	TYPE * m_current;

};




//
//NodeBranchIterator

template <class TYPE, class BASE, bool RETAIN>
class Reflex::Detail::NodeBranchIterator
{
public:

	//

	struct Itr
	{
		Itr(NodeBranchIterator & range) : range(range) { }

		bool operator!=(const NodeBranchIterator * ref) const;

		TYPE & operator*() const;

		void operator++();

		NodeBranchIterator & range;
	};



	//lifetime

	NodeBranchIterator();

	NodeBranchIterator(TYPE & start);



	//range

	explicit operator bool() const { return True(m_stack); }

	TYPE & operator()();



	//iterate

	NodeBranchIterator::Itr begin() { return *this; }

	NodeBranchIterator * end() { return this; }



private:

	Array <TYPE*> m_stack;

};




//
//impl

REFLEX_NS(Reflex)

template <class TYPE, class BASE, bool RETAIN> REFLEX_INLINE TYPE * Node<TYPE,BASE,RETAIN>::GetParent()
{
	return Cast<TYPE>(Item::GetList());
}

template <class TYPE, class BASE, bool RETAIN> REFLEX_INLINE const TYPE * Node<TYPE,BASE,RETAIN>::GetParent() const
{
	return RemoveConst(this)->GetParent();
}

template <class TYPE, class BASE, bool RETAIN> REFLEX_INLINE void Node<TYPE,BASE,RETAIN>::Attach(Node & node)
{
	REFLEX_ASSERT(this != &node);

	Item::Attach(node);
}

REFLEX_END




//
//

REFLEX_NS(Reflex::Detail)

template <class TYPE, class BASE, bool RETAIN> REFLEX_INLINE NodeParentIterator<TYPE,BASE,RETAIN>::NodeParentIterator()
	: m_current(0)
{
}

template <class TYPE, class BASE, bool RETAIN> REFLEX_INLINE NodeParentIterator<TYPE,BASE,RETAIN>::NodeParentIterator(TYPE & start)
	: m_current(&start)
{
}

template <class TYPE, class BASE, bool RETAIN> REFLEX_INLINE bool NodeParentIterator<TYPE,BASE,RETAIN>::operator!=(const NodeParentIterator & item) const
{
	return m_current != item.m_current;
}

template <class TYPE, class BASE, bool RETAIN> REFLEX_INLINE TYPE & NodeParentIterator<TYPE,BASE,RETAIN>::operator*() const
{
	return *Cast<TYPE>(m_current);
}

template <class TYPE, class BASE, bool RETAIN> REFLEX_INLINE void NodeParentIterator<TYPE,BASE,RETAIN>::operator++()
{
	m_current = m_current->Node<NonConstT<TYPE>,BASE>::GetParent();
}

REFLEX_END




//
//

REFLEX_NS(Reflex::Detail)

template <class TYPE, class BASE, bool RETAIN> inline NodeBranchIterator<TYPE,BASE,RETAIN>::NodeBranchIterator()
{
}

template <class TYPE, class BASE, bool RETAIN> inline NodeBranchIterator<TYPE,BASE,RETAIN>::NodeBranchIterator(TYPE & start)
{
	m_stack.Allocate(8);

	if (auto first = start.GetFirst()) m_stack.Push(first);
}

template <class TYPE, class BASE, bool RETAIN> REFLEX_INLINE TYPE & NodeBranchIterator<TYPE,BASE,RETAIN>::operator()()
{
	auto & itr = m_stack.GetLast();

	auto & current = *itr;	//this store is important, if do after if, stack could realloc

	Traverse<false>(itr);

	if (auto child = current.GetFirst())
	{
		m_stack.Push(child);
	}

	while (!m_stack.GetLast())
	{
		m_stack.Pop();

		if (!m_stack) break;
	}

	return current;
}

template <class TYPE, class BASE, bool RETAIN> REFLEX_INLINE TYPE & NodeBranchIterator<TYPE,BASE,RETAIN>::Itr::operator*() const
{
	return *range.m_stack.GetLast();
}

template <class TYPE, class BASE, bool RETAIN> REFLEX_INLINE bool NodeBranchIterator<TYPE,BASE,RETAIN>::Itr::operator!=(const NodeBranchIterator * end) const
{
	REFLEX_ASSERT(&range == end);

	return True(range.m_stack);
}

template <class TYPE, class BASE, bool RETAIN> REFLEX_INLINE void NodeBranchIterator<TYPE,BASE,RETAIN>::Itr::operator++()
{
	range();
}

REFLEX_END




//
//

template <class TYPE> inline Reflex::Idx Reflex::LookupBranchIndex(const TYPE & root, const TYPE & node)
{
	using Node = typename TYPE::NodeType;

	const Node * itr = &node;

	while (auto parent = itr->GetParent())
	{
		if (parent == &root) return LookupIndex(itr);

		itr = parent;
	}

	return {};
}

template <class TYPE> inline bool Reflex::BranchContains(const TYPE & parent, const TYPE & child)
{
	for (auto & i : typename TYPE::ConstParentRange(child))
	{
		if (&i == &parent) return true;
	}

	return false;
}

#define REFLEX_UPCAST_NODE(BASE, TYPE) REFLEX_UPCAST_ITEM(BASE, TYPE); typedef TYPE List; REFLEX_INLINE UInt GetNumItem() const { return BASE::GetNumItem(); } REFLEX_INLINE TYPE * GetFirst() { return Cast<TYPE>(BASE::GetFirst()); } REFLEX_INLINE const TYPE * GetFirst() const { return Cast<TYPE>(BASE::GetFirst()); } REFLEX_INLINE TYPE * GetLast() { return Cast<TYPE>(BASE::GetLast()); } REFLEX_INLINE const TYPE * GetLast() const { return Cast<TYPE>(BASE::GetLast()); } typedef Reflex::Detail::NodeBranchIterator <TYPE,typename TYPE::BaseType,true> BranchIterator; typedef Reflex::Detail::NodeBranchIterator <const TYPE,typename TYPE::BaseType,true> ConstBranchIterator; typedef Reflex::Detail::NodeParentIterator <TYPE,typename TYPE::BaseType,true> ParentRange; typedef Reflex::Detail::NodeParentIterator <const TYPE,typename TYPE::BaseType,true> ConstParentRange; ItemItr <TYPE,false> begin() { return Cast<TYPE>(this->GetFirst()); } ItemItr <TYPE,false> end() { return {}; } ItemItr <const TYPE,false> begin() const { return Cast<TYPE>(this->GetFirst()); } ItemItr <const TYPE,false> end() const { return {}; }


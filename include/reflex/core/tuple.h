#pragma once

#include "detail/functions/initalise.h"
#include "meta/multitraits.h"
#include "meta/functions.h"




//
//Primary API

namespace Reflex
{

	template <class ... ARGS> struct Tuple;


	template <class A, class B = A> using Pair = Tuple <A,B>;

	template <class TYPE> using Triad = Tuple <TYPE,TYPE,TYPE>;

	template <class TYPE> using Quad = Tuple <TYPE,TYPE,TYPE,TYPE>;


	template <class ... ARGS> constexpr auto MakeTuple(ARGS && ... args);


	template <class ... ARGS> bool operator==(const Tuple <ARGS...> & a, const Tuple <ARGS...> & b);

	template <class ... ARGS> bool operator<(const Tuple <ARGS...> & a, const Tuple <ARGS...> & b);

}




//
//Tuple

template <class ... VARGS> 
struct Reflex::Tuple 
{
	//A a = {};
	//B b = {};
	//etc ...;
};




//
//impl

REFLEX_NS(Reflex)

template <class A> struct Tuple <A> { A a = {}; };
template <class A, class B> struct Tuple <A, B> { A a = {}; B b = {}; };
template <class A, class B, class C> struct Tuple <A, B, C> { A a = {}; B b = {}; C c = {}; };
template <class A, class B, class C, class D> struct Tuple <A, B, C, D> { A a = {}; B b = {}; C c = {}; D d = {}; };
template <class A, class B, class C, class D, class E> struct Tuple <A, B, C, D, E> { A a = {}; B b = {}; C c = {}; D d = {}; E e = {}; };
template <class A, class B, class C, class D, class E, class F> struct Tuple <A, B, C, D, E, F> { A a = {}; B b = {}; C c = {}; D d = {}; E e = {}; F f = {}; };


template <class ... VARGS> struct TupleSize {};

template <class ... VARGS> struct TupleSize < Tuple <VARGS...> > { static constexpr UInt value = sizeof...(VARGS); };

REFLEX_PUBLISH_TRAIT_VALUE(TupleSize);


template <class TUPLE, UInt IDX> struct TupleElement { };

template <class TUPLE> struct TupleElement <TUPLE,0> { using Type = decltype(TUPLE::a); static inline auto & Get(TUPLE & t) { return t.a; } };
template <class TUPLE> struct TupleElement <TUPLE,1> { using Type = decltype(TUPLE::b); static inline auto & Get(TUPLE & t) { return t.b; } };
template <class TUPLE> struct TupleElement <TUPLE,2> { using Type = decltype(TUPLE::c); static inline auto & Get(TUPLE & t) { return t.c; } };
template <class TUPLE> struct TupleElement <TUPLE,3> { using Type = decltype(TUPLE::d); static inline auto & Get(TUPLE & t) { return t.d; } };
template <class TUPLE> struct TupleElement <TUPLE,4> { using Type = decltype(TUPLE::e); static inline auto & Get(TUPLE & t) { return t.e; } };
template <class TUPLE> struct TupleElement <TUPLE,5> { using Type = decltype(TUPLE::f); static inline auto & Get(TUPLE & t) { return t.f; } };

REFLEX_END

REFLEX_NS(Reflex::Detail)

template <class TUPLE, class FN, UInt IDX = 0> static inline void EnumerateTupleTypes(FN && fn)
{
	if constexpr (IDX < kTupleSize<TUPLE>)
	{
		using Element = TupleElement<TUPLE, IDX>;

		fn.template operator() < IDX, typename Element::Type > ();

		EnumerateTupleTypes<TUPLE, FN, IDX + 1>(std::forward<FN>(fn));
	}
}

template <class TUPLE, class FN, UInt IDX = 0> static inline void EnumerateTuple(TUPLE & tuple, FN && fn)
{
	if constexpr (IDX < kTupleSize<TUPLE>)
	{
		using Element = TupleElement<TUPLE, IDX>;

		fn.template operator() < IDX, typename Element::Type > (Element::Get(tuple));

		EnumerateTuple<TUPLE, FN, IDX + 1>(tuple, std::forward<FN>(fn));
	}
}

template <class TUPLE, UInt IDX> static inline bool TupleEqualsRecursive(const TUPLE & a, const TUPLE & b)
{
	using Element = TupleElement<const TUPLE, IDX>;

	if (Element::Get(a) != Element::Get(b))
	{
		return false;
	}
	else
	{
		if constexpr (IDX == 0)
		{
			return true;
		}
		else
		{
			return TupleEqualsRecursive<TUPLE, IDX - 1>(a, b);
		}
	}
}

template <class TUPLE, UInt IDX> static inline bool TupleLessThanRecursive(const TUPLE & a, const TUPLE & b)
{
	using Element = TupleElement<const TUPLE, IDX>;

	if (Element::Get(a) < Element::Get(b))
	{
		return true;
	}
	else if (Element::Get(b) < Element::Get(a))
	{
		return false;
	}
	else
	{
		if constexpr (IDX == 0)
		{
			return false;
		}
		else
		{
			return TupleLessThanRecursive<TUPLE, IDX - 1>(a, b);
		}
	}
}

REFLEX_END

template <class ... VARGS> struct Reflex::IsRawCopyable < Reflex::Tuple <VARGS...> >
{
	static constexpr bool raw_copyable = AndTraits<IsRawCopyable, VARGS...>::value;
	static constexpr bool no_padding = AddTraits<SizeOf, VARGS...>::value == sizeof(Tuple <VARGS...>);

	static constexpr bool value = raw_copyable && no_padding;
};

template <class ... ARGS> REFLEX_INLINE constexpr auto Reflex::MakeTuple(ARGS && ... args)
{
	return Tuple < NonConstT< NonRefT <ARGS> > ... > { std::forward<ARGS>(args)... };
}

template <class ... VARGS> REFLEX_INLINE bool Reflex::operator==(const Tuple <VARGS...> & a, const Tuple <VARGS...> & b)
{
	using Tuple = Tuple <VARGS...>;

	if constexpr (kIsRawCopyable<Tuple>)
	{
		return MemCompare(&a, &b, sizeof(Tuple));
	}
	else
	{
		return Detail::TupleEqualsRecursive<Tuple, kTupleSize<Tuple> -1>(a, b);
	}
}

template <class ... VARGS> REFLEX_INLINE bool Reflex::operator<(const Tuple <VARGS...> & a, const Tuple <VARGS...> & b)
{
	using Tuple = Tuple <VARGS...>;

	return Detail::TupleLessThanRecursive<Tuple, kTupleSize<Tuple> -1>(a, b);
}
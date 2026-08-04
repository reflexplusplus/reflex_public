#pragma once

#include "detail/store.h"
#include "detail/restore.h"




//
//Primary API

namespace Reflex::Data
{

	template <class TYPE> void Serialize(Data::Archive & stream, const TYPE & type);

	template <class FIRST, class ... REST> void Serialize(Data::Archive & stream, const FIRST & t, const REST & ... args);


	template <class FIRST, class ... REST> void Deserialize(Data::Archive::View & stream, FIRST & t, REST & ... args);

	template <class TYPE> void Deserialize(Data::Archive::View & stream, TYPE & t);


	template <class ... VARGS> auto Deserialize(Data::Archive::View & stream);

}




//
//impl

template <class TYPE> inline void Reflex::Data::Serialize(Archive & archive, const TYPE & p1)
{
	Detail::StoreImpl(archive, p1);
}

template <class FIRST, class ... REST> inline void Reflex::Data::Serialize(Archive & archive, const FIRST & p1, const REST & ... args)
{
	Detail::StoreImpl(archive, p1);

	Serialize(archive, args...);
}

template <class P1, class ... PX> inline void Reflex::Data::Deserialize(Archive::View & archive, P1 & p1, PX & ... args)
{
	Detail::RestoreImpl(archive, p1);

	Deserialize(archive, args...);
}

template <class TYPE> inline void Reflex::Data::Deserialize(Archive::View & view, TYPE & t)
{
	Detail::RestoreImpl(view, t);
}

template <class ... VARGS> inline auto Reflex::Data::Deserialize(Archive::View & view)
{
	using TupleType = Tuple <VARGS...>;

	if constexpr (sizeof...(VARGS) == 1)
	{
		typename TupleElement<TupleType,0>::Type t;

		Deserialize(view, t);

		return std::move(t);
	}
	else
	{
		TupleType t;

		Deserialize(view, t);

		return std::move(t);
	}
}

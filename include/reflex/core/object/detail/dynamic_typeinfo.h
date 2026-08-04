#pragma once

#include "../../typeid.h"




//
//macros

#define REFLEX_OBJECT_TYPE(TYPE) TYPE::kDynamicTypeInfo




//
//Detail

namespace Reflex::Detail
{

	struct DynamicTypeInfo;

	using DynamicTypeRef = const DynamicTypeInfo *;

	struct DynamicCastable;

}




//
//DynamicTypeInfo

struct Reflex::Detail::DynamicTypeInfo
{
	DynamicTypeInfo(const DynamicTypeInfo * base, const TypeID * ptype_id, const char * tname);

	DynamicTypeInfo(const DynamicTypeInfo &) = delete;

	DynamicTypeInfo & operator=(const DynamicTypeInfo &) = delete;

	operator DynamicTypeRef() const { return this; }


	const DynamicTypeRef base;

	const TypeID & type_id;

	const char * tname;
};

struct Reflex::Detail::DynamicCastable
{
	DynamicCastable(const DynamicTypeInfo & class_t) : object_t(class_t) {}

	DynamicCastable(const DynamicCastable & b) = delete;

	DynamicCastable & operator=(const DynamicCastable & b) = delete;

	const DynamicTypeRef object_t = 0;
};




//
//impl

REFLEX_NS(Reflex::Detail)

REFLEX_INLINE void SetDynamicCastableTypeInfo(DynamicCastable * object, const DynamicTypeInfo & objectinfo)
{
	const_cast<const DynamicTypeInfo *&>(object->object_t) = &objectinfo;
}

template <class TYPE>
struct DynamicCastableTypeSetter
{
	REFLEX_INLINE DynamicCastable * GetInstance()
	{
		constexpr UIntNative koffset = REFLEX_OFFSETOF(TYPE, typesetter);

		return Reinterpret<typename TYPE::Base>(Reinterpret<UInt8>(this) - koffset)->GetBase().Adr();
	}

	REFLEX_INLINE DynamicCastableTypeSetter()
	{
		SetDynamicCastableTypeInfo(GetInstance(), TYPE::kDynamicTypeInfo);
	}

	REFLEX_INLINE DynamicCastableTypeSetter(const DynamicCastableTypeSetter & self)
	{
		SetDynamicCastableTypeInfo(GetInstance(), TYPE::kDynamicTypeInfo);
	}

	REFLEX_INLINE ~DynamicCastableTypeSetter()
	{
		SetDynamicCastableTypeInfo(GetInstance(), TYPE::kDynamicTypeInfo);
	}

	inline void operator=(const DynamicCastableTypeSetter &) const {}
};

REFLEX_END

inline Reflex::Detail::DynamicTypeInfo::DynamicTypeInfo(const DynamicTypeInfo * base, const UInt32 * ptype_id, const char * tname)
	: base(base)
	, type_id(*ptype_id)
	, tname(tname)
{
	REFLEX_ASSERT(this != base);
}

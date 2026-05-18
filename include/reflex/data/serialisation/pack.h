#pragma once

#include "detail/packer.h"




//
//Primary API

namespace Reflex::Data
{

	template <class TYPE> Archive::View Pack(const TYPE & value);

	template <class TYPE> void Unpack(const Archive::View & ref, TYPE & value);

	template <class TYPE> TYPE Unpack(const Archive::View & ref);

}




//
//impl

template <class TYPE> REFLEX_INLINE Reflex::Data::Archive::View Reflex::Data::Pack(const TYPE & value)
{
	return Detail::Packer<TYPE>::Pack(value);
}

template <class TYPE> REFLEX_INLINE void Reflex::Data::Unpack(const Archive::View & archive, TYPE & value)
{
	Detail::Packer<TYPE>::Unpack(archive, value);
}

template <class TYPE> inline TYPE Reflex::Data::Unpack(const Archive::View & archive)
{
	TYPE rtn;

	Unpack(archive, rtn);

	return rtn;
}

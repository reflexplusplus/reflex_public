#pragma once

#include "[require].h"




REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

ContainerType GetContainerType2(bool values, bool objects, bool threadsafe, bool noncircular);

REFLEX_END_INTERNAL




//
//

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

template <bool VALUES, bool OBJECTS, bool THREADSAFE, bool NONCIRCULAR>
struct ContainerTypeLogic
{
};

template <bool THREADSAFE, bool NONCIRCULAR>
struct ContainerTypeLogic <false,false,THREADSAFE,NONCIRCULAR>
{
	static const ContainerType value = kContainerTypeNull;
};

template <bool THREADSAFE, bool NONCIRCULAR>
struct ContainerTypeLogic <true,false,THREADSAFE,NONCIRCULAR>
{
	static const ContainerType value = kContainerTypeValues;
};

template <bool VALUES, bool THREADSAFE, bool NONCIRCULAR>
struct ContainerTypeLogic <VALUES, true, THREADSAFE, NONCIRCULAR>
{
	static const ContainerType value = (NONCIRCULAR ? (THREADSAFE ? kContainerTypeThreadSafeObjects : kContainerTypeNonCircularObjects) : kContainerTypeCircularObjects);
};

//template <bool VALUES>
//struct ContainerTypeLogic <VALUES, true, true, true>
//{
//	static const ContainerType value = kContainerTypeThreadSafeObjects;
//};

inline const ContainerType kContainerType[16] =
{
	ContainerTypeLogic<false,false,false,false>::value,
	ContainerTypeLogic<true,false,false,false>::value,
	ContainerTypeLogic<false,true,false,false>::value,
	ContainerTypeLogic<true,true,false,false>::value,
	ContainerTypeLogic<false,false,true,false>::value,
	ContainerTypeLogic<true,false,true,false>::value,
	ContainerTypeLogic<false,true,true,false>::value,
	ContainerTypeLogic<true,true,true,false>::value,

	ContainerTypeLogic<false,false,false,true>::value,
	ContainerTypeLogic<true,false,false,true>::value,
	ContainerTypeLogic<false,true,false,true>::value,
	ContainerTypeLogic<true,true,false,true>::value,
	ContainerTypeLogic<false,false,true,true>::value,
	ContainerTypeLogic<true,false,true,true>::value,
	ContainerTypeLogic<false,true,true,true>::value,
	ContainerTypeLogic<true,true,true,true>::value,
};

inline ContainerType GetContainerType2(bool values, bool objects, bool threadsafe, bool noncircular)
{
	return kContainerType[MakeBits(values, objects, threadsafe, noncircular)];
}

REFLEX_END_INTERNAL


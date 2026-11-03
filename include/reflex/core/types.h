#pragma once

#include "detail/integer.h"




//
//Primary API

namespace Reflex
{

	using UInt8 = unsigned char;

	using UInt16 = unsigned short;

	using UInt32 = typename Detail::UIntOfSize<4>::type;

	using UInt64 = typename Detail::UIntOfSize<8>::type;

	using UIntNative = typename Detail::UIntOfSize<sizeof(void*)>::type;

	using UInt = UInt32;


	using Int8 = signed char;

	using Int16 = signed short;

	using Int32 = typename Detail::IntOfSize<4>::type;

	using Int64 = typename Detail::IntOfSize<8>::type;

	using IntNative = typename Detail::IntOfSize<sizeof(void*)>::type;

	using Int = Int32;


	using Float64 = double;

	using Float32 = float;

	using Float = float;


	using WChar = wchar_t;


	template <class TYPE> using Atomic = std::atomic <TYPE>;

	using AtomicUInt8 = Atomic <UInt8>;

	using AtomicUInt32 = Atomic <UInt32>;

	using AtomicUInt64 = Atomic <UInt64>;

	using AtomicFloat32 = Atomic <Float32>;

	using AtomicPointer = Atomic <void*>;


	class Object;

	template <class TYPE> class TRef;

}

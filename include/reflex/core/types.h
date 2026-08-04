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


	using AtomicUInt8 = std::atomic <UInt8>;

	using AtomicUInt32 = std::atomic <UInt32>;

	using AtomicFloat32 = std::atomic <Float32>;

	using AtomicPointer = std::atomic <void*>;


	class Object;

	template <class TYPE> class TRef;

}

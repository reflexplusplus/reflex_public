#include "reflex/core/module.h"
#include "reflex/core/allocation.h"




//
//library

Reflex::Detail::Module Reflex::root_module = { "Reflex" };

REFLEX_BEGIN_INTERNAL(Reflex)

template <class TYPE>
struct NullAllocation : public Allocation <TYPE>
{
	NullAllocation() : Allocation<TYPE>(0) {}
};

struct Library
{
	Library();

	Detail::StaticObject < ObjectOf <CString> > cstring;
	Detail::StaticObject < ObjectOf <WString> > wstring;

	Detail::StaticObject < NullAllocation <UInt8> > allocation_uint8;
	Detail::StaticObject < NullAllocation <UInt32> > allocation_uint32;
};

Library::Library()
	: cstring(g_default_allocator),
	wstring(g_default_allocator)
{
	RemoveConst(kMainThreadID) = System::GetThreadID();

	RemoveConst(ObjectOf<bool>::kDynamicTypeInfo).tname = "ObjectOf@bool";

	RemoveConst(ObjectOf<UInt8>::kDynamicTypeInfo).tname = "ObjectOf@UInt8";
	RemoveConst(ObjectOf<UInt16>::kDynamicTypeInfo).tname = "ObjectOf@UInt16";
	RemoveConst(ObjectOf<UInt32>::kDynamicTypeInfo).tname = "ObjectOf@UInt32";
	RemoveConst(ObjectOf<UInt64>::kDynamicTypeInfo).tname = "ObjectOf@UInt64";

	RemoveConst(ObjectOf<Int8>::kDynamicTypeInfo).tname = "ObjectOf@Int8";
	RemoveConst(ObjectOf<Int16>::kDynamicTypeInfo).tname = "ObjectOf@Int16";
	RemoveConst(ObjectOf<Int32>::kDynamicTypeInfo).tname = "ObjectOf@Int32";
	RemoveConst(ObjectOf<Int64>::kDynamicTypeInfo).tname = "ObjectOf@Int64";

	RemoveConst(ObjectOf<Float32>::kDynamicTypeInfo).tname = "ObjectOf@Float32";
	RemoveConst(ObjectOf<Float64>::kDynamicTypeInfo).tname = "ObjectOf@Float64";
	RemoveConst(ObjectOf<Key32>::kDynamicTypeInfo).tname = "ObjectOf@Key32";

	RemoveConst(ObjectOf<CString>::kDynamicTypeInfo).tname = "ObjectOf@CString";
	RemoveConst(ObjectOf<WString>::kDynamicTypeInfo).tname = "ObjectOf@WString";

	RemoveConst(Allocation<UInt8>::kDynamicTypeInfo).tname = "Allocation@UInt8";
	RemoveConst(Allocation<UInt32>::kDynamicTypeInfo).tname = "Allocation@UInt32";
}

REFLEX_STATIC_ASSERT(sizeof(void *) == (REFLEX_64BIT ? 8 : 4));
REFLEX_STATIC_ASSERT(sizeof(UInt8) == 1);
REFLEX_STATIC_ASSERT(sizeof(UInt16) == 2);
REFLEX_STATIC_ASSERT(sizeof(UInt32) == 4);
REFLEX_STATIC_ASSERT(sizeof(UInt64) == 8);
REFLEX_STATIC_ASSERT(sizeof(Int8) == 1);
REFLEX_STATIC_ASSERT(sizeof(Int16) == 2);
REFLEX_STATIC_ASSERT(sizeof(Int32) == 4);
REFLEX_STATIC_ASSERT(sizeof(Int64) == 8);
REFLEX_STATIC_ASSERT(sizeof(UIntNative) == sizeof(void *));
REFLEX_STATIC_ASSERT(sizeof(Key32) == 4);
REFLEX_STATIC_ASSERT(sizeof(Key64) == 8);

Detail::Module::Member <Library> g_library(root_module);

REFLEX_END_INTERNAL

REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::CString, Reflex::g_library->cstring.value);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::WString, Reflex::g_library->wstring.value);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::ObjectOf<Reflex::CString>, Reflex::g_library->cstring);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::ObjectOf<Reflex::WString>, Reflex::g_library->wstring);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Allocation<Reflex::UInt8>, Reflex::g_library->allocation_uint8);
REFLEX_INSTANTIATE_EXTERN_NULL(Reflex::Allocation<Reflex::UInt32>, Reflex::g_library->allocation_uint32);

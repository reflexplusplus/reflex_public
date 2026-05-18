#pragma once

#include "allocator.h"
#include "../detail/initialiser.h"
#include "../object/tref.h"
#include "../key.h"
#include "../function.h"




//
//Secondary API

#define REFLEX_INSTANTIATE_DEFAULT_ALLOCATOR volatile static bool g_reflex_allocator_instantiated = []() { return Reflex::True(Reflex::GetDefaultAllocator()); }()

namespace Reflex
{

	class StandardAllocator;

	class NonAllocator;	//used in null-instances, allows debug of invalid null instance write


	TRef <Allocator> CreateAllocator(Key32 type, const Object & params);

	extern const Key32 kRecycleAllocID;

	extern const Key32 kBlockAllocID;

}


	
	
//
//StandardAllocator 

class Reflex::StandardAllocator final : public Allocator
{
public:

	REFLEX_OBJECT(StandardAllocator, Allocator);



	//types
	
	struct Lock;
		
		

	//lifetime

	StandardAllocator();

	~StandardAllocator();



	//info

	UInt64 GetNumBytes() const;

	UInt GetNumAllocation() const;



protected:

	virtual void * OnAlloc(UIntNative n, const AllocInfo & info) override;

	virtual void OnFree(void * ptr) override;

	virtual void OnSetProperty(Address address, Object & object) override;

	

private:

	UInt8 m_impl[REFLEX_DEBUG ? 128 : 4];
};




//
//StandardAllocator::Lock (for debug IDE)

struct Reflex::StandardAllocator::Lock
{
	Lock(StandardAllocator & allocator);

	~Lock();

	void Enumerate(UInt start, UInt n, const Function <void(const AllocInfo & info)> & callback) const;

	const TRef <StandardAllocator> allocator;
};




//
//NonAllocator 

class Reflex::NonAllocator : public Allocator
{
public:

	REFLEX_OBJECT(NonAllocator, Allocator);



	virtual void * OnAlloc(UIntNative n, const AllocInfo & info) override { REFLEX_ASSERT(false); return nullptr; }

	virtual void OnFree(void * ptr) override { REFLEX_ASSERT(false); }

};




//
//impl

#define REFLEX_CHECK_DEFAULT_ALLOCATOR static_assert(sizeof(::g_reflex_allocator_instantiated), "REQUIRE_CHECK_DEFAULT_ALLOCATOR")

REFLEX_NS(Reflex)

TRef <StandardAllocator> GetDefaultAllocator();

inline TRef <StandardAllocator> g_default_allocator = GetDefaultAllocator();

inline NonAllocator g_non_allocator;

REFLEX_END

#if REFLEX_DEBUG
#else
inline void * Reflex::StandardAllocator::OnAlloc(UIntNative n, const AllocInfo & info)
{
	return REFLEX_ALLOC16(n);
}

inline void Reflex::StandardAllocator::OnFree(void * ptr)
{
	REFLEX_FREE16(RemoveConst(ptr));
}
#endif

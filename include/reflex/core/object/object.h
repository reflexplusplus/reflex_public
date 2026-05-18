#pragma once

#include "../detail/allocinfo.h"
#include "../detail/scopeof.h"
#include "../meta/auxtypes.h"
#include "detail/dynamic_typeinfo.h"
#include "address.h"
#include "forward.h"




//
//Primary API

#define REFLEX_OBJECT(OBJECT, BASE) REFLEX_OBJECT_EX(OBJECT, BASE, REFLEX_STRINGIFY(OBJECT))

namespace Reflex
{

	class Object;

}




//
//Object

class Reflex::Object : public Detail::DynamicCastable	//Object must be first base class, single inheritance chains only
{
public:
	
	//dynamic type info
	
	static const Detail::DynamicTypeInfo kDynamicTypeInfo;

	static Object & null;



	//construction 
	
	//typically use Make<TYPE>(...), New<TYPE>(...) or REFLEX_CREATE(TYPE,...) for auto-delete and debug tracking

	//if using operator new, manually call SetOnHeap(allocator) after new to enable auto-delete when ref-count == 0
	
	static void * operator new(size_t size, Allocator & allocator, const AllocInfo & info);
	static void operator delete(void *, Allocator & allocator, const AllocInfo & info);

	static void * operator new(size_t size, Allocator & allocator);
	static void operator delete(void *, Allocator & allocator);

	static void * operator new(size_t size);							//uses default allocator, see new.h
	static void operator delete(void * ptr);



	//lifetime

	virtual ~Object();



	//ownership (avoid calling these manually, use Reference)

	void RetainMt() const;

	void ReleaseMt() const;


	void RetainSt() const;

	void ReleaseSt() const;



	//generic properties

	template <class TYPE> void UnsetProperty(Key32 id);

	template <class auto_1> void SetProperty(Key32 id, auto_1 && data);

	template <class TYPE> TYPE * QueryProperty(Key32 id, TYPE * fallback = nullptr) const;



	//assign

	void operator=(const Object &) {}



	//allocator info (non-null indicates on heap)

	Allocator * GetAllocator() const { return m_allocator; }



	//ADVANCED (not needed in general use)

	void UnsetProperty(Address key);

	void SetProperty(Address key, TRef <Object> data);

	Object * QueryProperty(Address key, Object * fallback = nullptr) const;


	UInt32 GetRetainCount() const;

	AtomicUInt32 & GetActualRetainCount() const;


	const UInt16 & GetContextID() const { return m_contextid; }

	void ReleaseData();														//only call if you own this object and control its dependencies

	bool DataReleased() const;


	TRef <Object> GetBase();												//for REFLEX_OBJECT_EX (workaround for Android studio, using BASE::SetOnHeap doesnt work)


	
	//placement new (for REFLEX_CREATE and Detail::Constructor)

	static void * operator new(size_t, void * ptr) { return ptr; }			

	static void operator delete(void *, void *) {}



	//ADVANCED

	void SetOnHeap(Allocator & allocator);	//called automatically by Reflex::New

	void SetOnStack();



protected:

	//lifetime

	Object(UInt16 contextid);

	Object();

	Object(const Object & object);



	//callbacks

	virtual void OnUnsetProperty(Address address) {}

	virtual void OnSetProperty(Address address, Object & value)					//default implementation simply discards the property
	{
		value.RetainMt();

		value.ReleaseMt();
	}

	virtual void OnQueryProperty(Address address, Object * & pointer_out) const {}	//only return owned/retained properties, not created ones



	//ADVANCED

	virtual void OnReleaseData() {}												//for clear up of circular references (primarily used by VM)

	virtual void OnDestruct();


	
private:

	//prohibit arrays

	static void * operator new[](size_t num_bytes);								

	static void operator delete[](void * ptr);

	virtual void OnQueryProperty(Address address, Object * & pointer_out) final {}	//legacy


	Allocator * m_allocator;

	mutable AtomicUInt32 m_retain_count;

	UInt16 m_contextid;

	UInt8 m_released;

	UInt8 m_contextflags;

};




//
//impl

#define REFLEX_OBJECT_EX(CLASS, BASE, TNAME) using Base = BASE; static inline const Reflex::Detail::DynamicTypeInfo kDynamicTypeInfo = { &BASE::kDynamicTypeInfo, &Reflex::Detail::TypeIndex<CLASS>::value, TNAME }; const Reflex::Detail::DynamicCastableTypeSetter <CLASS> typesetter; using BASE::object_t; using BASE::operator new; using BASE::RetainSt; using BASE::ReleaseSt; using BASE::RetainMt; using BASE::ReleaseMt; using BASE::GetAllocator; using BASE::UnsetProperty; using BASE::SetProperty; using BASE::QueryProperty; using BASE::SetOnHeap; using BASE::SetOnStack; using BASE::GetBase

#define REFLEX_NULL(TYPE) Reflex::Detail::GetNullInstance<TYPE>()

REFLEX_NS(Reflex::Detail)

extern UInt16 st_contextidcounter;

using ContextScope = ScopeOf <UInt16, true, Object>;

inline UInt16 AcquireContextID() { return ++st_contextidcounter; }	//TODO move to VM

template <class TYPE> inline TYPE * SetOnHeap(TYPE * t, Allocator & allocator)
{
	t->GetBase()->SetOnHeap(allocator);

	return t;
}

template <class TYPE> REFLEX_INLINE auto & Spawn(const Object & owner, TYPE && ref)
{
	auto & object = Deref(ref);

	REFLEX_ASSERT(object.GetContextID() == owner.GetContextID());

	return object;
}

constexpr UInt32 kCounterMask = 0xFFFFFFUL;

constexpr UInt32 kInverseCounterMask = ~kCounterMask;

REFLEX_END

REFLEX_INLINE Reflex::Object::Object()
	: Object(Detail::ContextScope::GetCurrent())
{
}

REFLEX_INLINE Reflex::Object::Object(const Object & object)
	: Object(Detail::ContextScope::GetCurrent())
{
}

#if REFLEX_DEBUG
#else
inline Reflex::Object::~Object()
{
}
#endif

REFLEX_INLINE void Reflex::Object::RetainMt() const
{
	REFLEX_ATOMIC_INC(GetActualRetainCount(), 1);
}

REFLEX_INLINE void Reflex::Object::ReleaseMt() const
{
	REFLEX_ASSERT(GetActualRetainCount() != 0);

	if (REFLEX_ATOMIC_DEC(GetActualRetainCount(), 1) == 1)
	{
		RemoveConst(this)->OnDestruct();
	}
}

REFLEX_INLINE void Reflex::Object::RetainSt() const
{
	GetActualRetainCount()++;
}

REFLEX_INLINE void Reflex::Object::ReleaseSt() const
{
	REFLEX_ASSERT(GetActualRetainCount() != 0);

	if (--GetActualRetainCount() == 0)
	{
		RemoveConst(this)->OnDestruct();
	}
}

template <class TYPE> REFLEX_INLINE void Reflex::Object::UnsetProperty(Key32 id)
{
	REFLEX_STATIC_ASSERT_OBJECT_TYPE(TYPE);

	OnUnsetProperty(MakeAddress<TYPE>(id));
}

template <class auto_1> REFLEX_INLINE void Reflex::Object::SetProperty(Key32 id, auto_1 && data)
{
	auto & property = Deref(data);

	using TYPE = NonConstT < NonRefT < decltype(property) > >;

	REFLEX_STATIC_ASSERT_OBJECT_TYPE(TYPE);

	OnSetProperty(MakeAddress<TYPE>(id), property);
}

template <class TYPE> REFLEX_INLINE TYPE * Reflex::Object::QueryProperty(Key32 id, TYPE * fallback) const
{
	REFLEX_STATIC_ASSERT_OBJECT_TYPE(TYPE);

	return Cast<TYPE>(QueryProperty(MakeAddress<TYPE>(id), fallback));
}

REFLEX_INLINE void Reflex::Object::SetOnHeap(Allocator & allocator)
{
	m_allocator = &allocator;

	Reinterpret<volatile UInt8>(&GetActualRetainCount())[3] = 0;
}

REFLEX_INLINE void Reflex::Object::UnsetProperty(Address address)
{
	OnUnsetProperty(address);
}

REFLEX_INLINE Reflex::Object * Reflex::Object::QueryProperty(Address address, Object * pobject) const
{
	OnQueryProperty(address, pobject);

	return pobject;
}

REFLEX_INLINE Reflex::AtomicUInt32 & Reflex::Object::GetActualRetainCount() const 
{ 
	return m_retain_count; 
}

#if (!REFLEX_DEBUG)
REFLEX_INLINE void Reflex::Object::ReleaseData()
{
	if (!DataReleased())
	{
		m_released = 1;

		OnReleaseData();
	}
}
#endif

REFLEX_INLINE bool Reflex::Object::DataReleased() const 
{
#if REFLEX_DEBUG
	return m_released & 1; 
#else
	return m_released; 
#endif
}

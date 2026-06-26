#pragma once

#include "[require].h"

#include <Foundation/Foundation.h>


// MEMO: the "new" semantic (either just new or newSomething) is equivalent to [[alloc] init]
// to the contrary of every other return value which comes autoreleased / unowned.
// To avoid issues though, all my methods will return autoreleased / unowned objects.
// But if assigning an owned object to an existing property, I'll use this macro to denote it.

#if (__has_feature(objc_arc))
	#define REFLEX_OBJC_RETAIN(obj) obj
	#define REFLEX_OBJC_RELEASE(obj)

	#define REFLEX_DISPATCH_RETAIN(obj)
	#define REFLEX_DISPATCH_RELEASE(obj)
#else
	#define REFLEX_OBJC_RETAIN(obj) [obj retain]
	#define REFLEX_OBJC_RELEASE(obj) [obj release]

	#define REFLEX_DISPATCH_RETAIN(obj) dispatch_retain(obj)
	#define REFLEX_DISPATCH_RELEASE(obj) dispatch_release(obj)
#endif





//
//functions

REFLEX_NS(Reflex::System)

template <class TYPE>
class ObjCRef
{
public:

	REFLEX_STATIC_ASSERT(kIsPointer<TYPE>);

	static constexpr bool kManualReferenceCounting = !__has_feature(objc_arc);

	
	ObjCRef();

	ObjCRef(nullptr_t) : ObjCRef() {}

	explicit ObjCRef(TYPE ptr, bool add_ref);

	ObjCRef(const ObjCRef & ref);

	ObjCRef(ObjCRef && ref);

	~ObjCRef();


	void Clear();


	ObjCRef & operator=(const ObjCRef & ref);

	ObjCRef & operator=(ObjCRef && ref);


	explicit operator bool() const { return m_ptr; }


	operator TYPE() { return m_ptr; }

	operator const TYPE() const { return m_ptr; }

	TYPE operator->() { return m_ptr; }

	const TYPE operator->() const { return m_ptr; }

	TYPE Get() { return m_ptr; }

	const TYPE Get() const { return m_ptr; }



private:

	TYPE m_ptr;

};

// Assigns to a strong (retained) variable. Will be release'd when replaced, cleared or destroyed.
template <class TYPE> ObjCRef <TYPE> MakeObjCRef(TYPE ptr) { return ObjCRef<TYPE>(ptr, true); }

// Assigns to a non-retained variable (i.e. does not increase the refcount); this is not the same as a weak reference,
// or a transitive pointer, since it will call [ptr release] upon destruction.
// Use it when taking ownership of a variable that's been initialised with [[alloc] init] or [new] (retainCount = 1).
template <class TYPE> ObjCRef <TYPE> MakeOwnedObjCRef(TYPE ptr) { return ObjCRef<TYPE>(ptr, false); }

REFLEX_END




//
//impl

template <class TYPE> inline Reflex::System::ObjCRef<TYPE>::ObjCRef()
	: m_ptr(0)
{
}

template <class TYPE> inline Reflex::System::ObjCRef<TYPE>::ObjCRef(TYPE ptr, bool add_ref)
	: m_ptr(ptr)
{
	if constexpr (kManualReferenceCounting)
	{
		if (add_ref) [m_ptr retain];
	}
}

template <class TYPE> inline Reflex::System::ObjCRef<TYPE>::ObjCRef(const ObjCRef & ref)
	: m_ptr(ref.m_ptr)
{
	if constexpr (kManualReferenceCounting) [m_ptr retain];
}

template <class TYPE> inline Reflex::System::ObjCRef<TYPE>::ObjCRef(ObjCRef && ref)
	: m_ptr(ref.m_ptr)
{
	ref.m_ptr = nullptr;
}

template <class TYPE> inline Reflex::System::ObjCRef<TYPE>::~ObjCRef()
{
	if constexpr (kManualReferenceCounting) [m_ptr release];
}

template <class TYPE> inline void Reflex::System::ObjCRef<TYPE>::Clear()
{
	if constexpr (kManualReferenceCounting) [m_ptr release];

	m_ptr = nullptr;
}

template <class TYPE> inline Reflex::System::ObjCRef <TYPE> & Reflex::System::ObjCRef<TYPE>::operator=(const ObjCRef & ref)
{
	if constexpr (kManualReferenceCounting)
	{
		[ref.m_ptr retain];
		
		[m_ptr release];
	}

	m_ptr = ref.m_ptr;

	return *this;
}

template <class TYPE> inline Reflex::System::ObjCRef <TYPE> & Reflex::System::ObjCRef<TYPE>::operator=(ObjCRef && ref)
{
	if (m_ptr != ref.m_ptr)
	{
		if constexpr (kManualReferenceCounting) [m_ptr release];

		m_ptr = ref.m_ptr;

		ref.m_ptr = nullptr;
	}

	return *this;
}


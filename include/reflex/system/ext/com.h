#pragma once

#include "../[require].h"




//
//Internal

namespace Reflex::COM
{

	struct MicrosoftAccessor;

	struct ThirdPartyAccessor;

	template <class TYPE, class IFACE = MicrosoftAccessor> struct Reference;

}




//
//Reference

template <class TYPE, class IFACE>
struct Reflex::COM::Reference
{
	Reference()
		: m_ptr(nullptr)
	{
	}

	Reference(TYPE * ptr)
		: m_ptr(ptr)
	{
	}

	Reference(const Reference & ref)
		: m_ptr(nullptr)
	{
		if (ref) IFACE::AddRef(RemoveConst(ref.m_ptr));

		Set(ref.m_ptr);
	}

	~Reference()
	{
		Clear();
	}


	void Clear()
	{
		Set(nullptr);
	}

	Reference & operator=(TYPE * ptr)
	{
		Set(ptr);

		return *this;
	}

	void operator=(const Reference & ref);


	TYPE * Get() { return m_ptr; }

	const TYPE * Get() const { return m_ptr; }


	operator TYPE*() { return m_ptr; }

	operator const TYPE*() const { return m_ptr; }

	TYPE * operator->() { return m_ptr; }

	TYPE & operator*() { return *m_ptr; }


	explicit operator bool() const { return True(m_ptr); }


	TYPE ** WriteAdr()
	{
		REFLEX_ASSERT(!m_ptr);

		return &this->m_ptr;
	}

	TYPE * const * ReadAdr() const
	{
		REFLEX_ASSERT(m_ptr);

		return &this->m_ptr;
	}


	bool operator==(TYPE * b) const { return m_ptr == b; }

	bool operator==(const TYPE * b) const { return m_ptr == b; }

	bool operator!=(TYPE * b) const { return m_ptr != b; }

	bool operator!=(const TYPE * b) const { return m_ptr != b; }



private:

	void operator&();



protected:

	void Set(TYPE * ptr)
	{
		if (m_ptr) IFACE::Release(m_ptr);

		m_ptr = ptr;
	}


	TYPE * m_ptr;
};




//
//MicrosoftAccessor

struct Reflex::COM::MicrosoftAccessor
{
	template <class TYPE> static void AddRef(TYPE * & ptr) { ptr->AddRef(); }
	template <class TYPE> static void Release(TYPE * & ptr) { ptr->Release(); }
};




//
//ThirdPartyAccessor

struct Reflex::COM::ThirdPartyAccessor
{
	template <class TYPE> static void AddRef(TYPE * & ptr) { ptr->addRef(); }
	template <class TYPE> static void Release(TYPE * & ptr) { ptr->release(); }
};




//
//impl

REFLEX_NS(Reflex)
template <class TYPE, class IFACE> TYPE & Deref(COM::Reference <TYPE, IFACE> & ref) { return *ref; }
REFLEX_END

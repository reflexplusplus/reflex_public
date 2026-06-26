#include "reflex/core/object.h"
#include "reflex/core/allocator.h"
#include "reflex/core/functions/bit.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex)

struct NullObject : public Detail::StaticObject <Object>
{
} g_null_object;

#ifdef REFLEX_AUTO_SET_ON_HEAP
thread_local UInt g_num_allocating = 0;
thread_local Pair <Object*,Allocator*> g_allocating[16];
#endif

REFLEX_END_INTERNAL

Reflex::UInt16 Reflex::Detail::st_contextidcounter = 0;

const Reflex::Detail::DynamicTypeInfo Reflex::Object::kDynamicTypeInfo(0, &REFLEX_TYPEID(Object), "Reflex::Object");

Reflex::Object & Reflex::Object::null = Reflex::g_null_object;

#undef new

void * Reflex::Object::operator new(size_t size, Allocator & allocator, const AllocInfo & debug_info)
{
	auto mem = g_default_allocator->Allocate(size, debug_info);

#ifdef REFLEX_AUTO_SET_ON_HEAP
	auto num_allocating = g_num_allocating;

	REFLEX_ASSERT(num_allocating < 16);

	g_allocating[num_allocating++] = { static_cast<Object*>(mem), &allocator };

	g_num_allocating = num_allocating;
#endif

	return mem;
}

void Reflex::Object::operator delete(void * adr, Allocator &, const AllocInfo &)
{
	auto obj = static_cast<Object*>(adr);

	REFLEX_ASSERT(obj->m_allocator);

	obj->m_allocator->Free<true>(adr);	//true -> assume not null
}

Reflex::Object::Object(UInt16 contextid)
	: DynamicCastable(kDynamicTypeInfo)
	, m_allocator(nullptr)
	, m_retain_count(Detail::kInverseCounterMask)
	, m_contextid(contextid)
	, m_released(0)
	, m_contextflags(0)
{
#ifdef REFLEX_AUTO_SET_ON_HEAP
	auto num_allocating = g_num_allocating;

	auto allocating = g_allocating;

	REFLEX_LOOP(idx, num_allocating)
	{
		auto item = allocating[idx];

		if (item.a == this)
		{
			SetOnHeap(*item.b);

			--num_allocating;

			allocating[idx] = allocating[num_allocating];

			allocating[num_allocating] = {};

			g_num_allocating = num_allocating;

			break;
		}
	}
#endif
}

#if REFLEX_DEBUG
Reflex::Object::~Object()
{
	if (GetAllocator())
	{
		auto retain_count = REFLEX_ATOMIC_READ(GetActualRetainCount());

		REFLEX_ASSERT(!retain_count);
	}
}
#endif

Reflex::UInt32 Reflex::Object::GetRetainCount() const
{
	return GetActualRetainCount() & Detail::kCounterMask;
}

void Reflex::Object::SetOnStack()
{
	m_allocator = 0;

	Reinterpret<volatile UInt8>(&GetActualRetainCount())[3] = kMaxUInt8;
}

void Reflex::Object::OnDestruct()
{
	REFLEX_ASSERT(GetAllocator());

	m_released = BitSet(m_released, 7);

	delete this;
}

#if REFLEX_DEBUG
void Reflex::Object::ReleaseData()
{
	REFLEX_ASSERT(!BitCheck(m_released, 7));

	if (!DataReleased())
	{
		m_released = BitSet(m_released, 0);

		OnReleaseData();
	}
}
#endif

void * Reflex::Object::operator new[](size_t num_bytes)
{
	REFLEX_ASSERT(false);

	void * t = 0;	//this is never called, but shut up clang compiler error

	return t;
}

void Reflex::Object::operator delete[](void * ptr)
{
}

void Reflex::UnsetAbstractProperty(Object & owner, Key32 id)
{
	owner.UnsetProperty<Object>(id);
}

void Reflex::SetAbstractProperty(Object & owner, Key32 id, TRef <Object> property)
{
	owner.SetProperty(id, property);
}

Reflex::Object * Reflex::QueryAbstractProperty(Object & owner, Key32 id, Object * fallback)
{
	return owner.QueryProperty<Object>(id, fallback);
}

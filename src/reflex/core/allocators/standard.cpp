#include "system.h"
#include "../../../../include/reflex/core/debug.h"
#include "reflex/core/tuple.h"




REFLEX_BEGIN_INTERNAL(Reflex)

#if (REFLEX_DEBUG)

struct DebugAllocatorImpl
{
	typedef Pair <UInt64> MemGuard;

	struct Item : public Reflex::Item <Item, false, NullType>
	{
		Item(List & list, const AllocInfo & allocinfo, UInt capacity, UInt idx);

		MemGuard & GetUpperGuard() { return *Reinterpret<MemGuard>(client_mem + capacity); }

		const AllocInfo allocinfo;

		const UInt capacity;

		const UInt idx;

		const MemGuard lower_guard;

		alignas(16) UInt8 client_mem[1];
	};


	DebugAllocatorImpl();

	~DebugAllocatorImpl();


	REFLEX_INLINE void * Alloc(UIntNative n, const AllocInfo & info);

	REFLEX_INLINE void Free(void * ptr);


	SystemAllocator m_system_allocator;

	Reference <System::CriticalSection> m_cs;

	Item::List m_list;

	UInt m_count, m_break;

	UInt64 m_bytes;


	static constexpr MemGuard kMemGuard = { ID64("[ReflexM"), ID64("emGuard]") };
};

DebugAllocatorImpl::Item::Item(List & list, const AllocInfo & allocinfo, UInt capacity, UInt idx)
	: allocinfo(allocinfo)
	, capacity(capacity)
	, idx(idx)
	, lower_guard(kMemGuard)
{
	GetUpperGuard() = kMemGuard;

	//mark memory with nans, helps to find bugs in DSP code

	REFLEX_LOOP_PTR(client_mem, ptr, capacity) (*ptr) = kMaxUInt8;

	Attach(list);
}

DebugAllocatorImpl::DebugAllocatorImpl()
	: m_cs(System::CriticalSection::Create(true, m_system_allocator))
	, m_count(0)
	, m_break(kMaxUInt32)
	, m_bytes(0)
{
}

DebugAllocatorImpl::~DebugAllocatorImpl()
{
	if (m_list.GetNumItem())
	{
		System::DebugLog(false, "*** DebugAllocator LEAKS ***");

		for (auto & i : m_list)
		{
			auto & allocinfo = i.allocinfo;

			//!cant allocate as allocator is shutting down, Reflex::DebugLog doesnt allocate

			auto buffer = Detail::DebugJoin(kSpace, i.idx, allocinfo.type);

			DebugLog(false, buffer.data, allocinfo.file, allocinfo.line);
		}
	}
}

REFLEX_INLINE void * DebugAllocatorImpl::Alloc(UIntNative capacity, const AllocInfo & info)
{
	if (m_count == m_break)
	{
		DebugLog(true, "DebugAllocator break", info.file, info.line);
	}

	System::CriticalSection::Lock lock(*m_cs);

	if (void * ptr = REFLEX_ALLOC16(sizeof(Item) + capacity + sizeof(MemGuard)))
	{
		auto pitem = Detail::Constructor<Item>::Construct(ptr, m_list, info, UInt(capacity), m_count++);

		REFLEX_ASSERT(pitem->lower_guard == kMemGuard);
		REFLEX_ASSERT(pitem->GetUpperGuard() == kMemGuard);

		m_bytes += capacity;

		return pitem->client_mem;
	}
	else
	{
		REFLEX_ASSERT(false);

		return nullptr;
	}
}

REFLEX_INLINE void DebugAllocatorImpl::Free(void * client_mem)
{
	constexpr UInt kOffset = REFLEX_OFFSETOF(Item, client_mem);

	System::CriticalSection::Lock lock(*m_cs);

	auto pitem = Reinterpret<Item>(Reinterpret<UInt8>(client_mem) - kOffset);

	REFLEX_ASSERT(pitem->lower_guard == kMemGuard);
	REFLEX_ASSERT(pitem->GetUpperGuard() == kMemGuard);

	m_bytes -= pitem->capacity;

	Detail::Constructor<Item>::Destruct(*pitem);

	REFLEX_FREE16(pitem);
}

#endif

REFLEX_END_INTERNAL

Reflex::StandardAllocator::StandardAllocator()
{
#if	(REFLEX_DEBUG)
	REFLEX_STATIC_ASSERT(sizeof(m_impl) >= sizeof(DebugAllocatorImpl));

	MemClear(m_impl, sizeof(DebugAllocatorImpl));

	Detail::Constructor<DebugAllocatorImpl>::Construct(m_impl);
#else
	*Reinterpret<UInt32>(m_impl) = 0;
#endif
}

Reflex::StandardAllocator::~StandardAllocator()
{
#if	(REFLEX_DEBUG)
	Detail::Constructor<DebugAllocatorImpl>::Destruct(Reinterpret<DebugAllocatorImpl>(m_impl));
#else
	if (auto nleak = *Reinterpret<UInt32>(m_impl))
	{
		DebugLog(nleak, "memory leaks detected", "StandardAllocator", 0);
	}
#endif
}

Reflex::UInt64 Reflex::StandardAllocator::GetNumBytes() const
{
#if	(REFLEX_DEBUG)
	return Reinterpret<DebugAllocatorImpl>(m_impl)->m_bytes;
#else
	return 0;
#endif
}

Reflex::UInt Reflex::StandardAllocator::GetNumAllocation() const
{
#if	(REFLEX_DEBUG)
	return Reinterpret<DebugAllocatorImpl>(m_impl)->m_list.GetNumItem();
#else
	return *Reinterpret<UInt32>(m_impl);
#endif
}

Reflex::StandardAllocator::Lock::Lock(StandardAllocator & allocator)
	: allocator(allocator)
{
#if	(REFLEX_DEBUG)
	Reinterpret<DebugAllocatorImpl>(allocator.m_impl)->m_cs->Enter();
#endif
}

Reflex::StandardAllocator::Lock::~Lock()
{
#if	(REFLEX_DEBUG)
	Reinterpret<DebugAllocatorImpl>(allocator->m_impl)->m_cs->Leave();
#endif
}

void Reflex::StandardAllocator::Lock::Enumerate(UInt start, UInt n, const Function <void(const AllocInfo & info)> & callback) const
{
#if	(REFLEX_DEBUG)
	auto & self = *Reinterpret<DebugAllocatorImpl>(allocator->m_impl);

	Array <AllocInfo> temp(n, self.m_system_allocator);

	auto pdest = temp.GetData();

	auto psrc = self.m_list.GetLast();

	while (n--)
	{
		*pdest++ = psrc->allocinfo;

		psrc = psrc->GetPrev();
	}

	for (auto & i : temp) callback(i);
#else
	AllocInfo info;
	REFLEX_LOOP(i, n) callback(info);
#endif
}

#if	(REFLEX_DEBUG)

void * Reflex::StandardAllocator::OnAlloc(UIntNative n, const AllocInfo & info)
{
	return Reinterpret<DebugAllocatorImpl>(m_impl)->Alloc(n, info);
}

void Reflex::StandardAllocator::OnFree(void * ptr)
{
	REFLEX_ASSERT(ptr);

	Reinterpret<DebugAllocatorImpl>(m_impl)->Free(ptr);
}

#endif

void Reflex::StandardAllocator::OnSetProperty(Address address, Object & object)
{
	auto t = AutoRelease(object);

#if	(REFLEX_DEBUG)
	auto self = Reinterpret<DebugAllocatorImpl>(m_impl);

	typedef ObjectOf <UInt32> UInt32Property;

	if (address == MakeAddress<UInt32Property>(K32("break")))
	{
		self->m_break = Cast<UInt32Property>(object)->value;
	}
#endif
}

#pragma once

#include "../../include/detail/allocator.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::Detail)

template <bool MT>
class FastAllocator : public Allocator
{
public:

	FastAllocator();



private:

	struct Header;

	struct Bank;

	struct Pool;


	struct Header : public Item <Header,ListPolicy<false>>
	{
		static const UInt st_size = sizeof(Reflex::Object) + (sizeof(void*) * 4);


		typedef Reflex::Item <Header,ListPolicy<false>> Item;

		using Item::Attach;

		using Item::Detach;


		Bank * bank;
	};

	struct Bank : public Item <Bank,ListPolicy<false>>
	{
		Bank(Pool & pool);

		~Bank();


		using Item<Bank,ListPolicy<false>>::Attach;


		Header & Acquire();

		void Release(Header & block);


		Pool & pool;

		UInt8 * m_blocks;

		UInt m_nfree;

		Reflex::List <Header,ListPolicy<false>> m_free;
	};

	struct Pool
	{
		Pool(UInt blocksize, UInt nblock);

		~Pool();


		UInt GetBlockSize() const {return m_blocksize;}

		UInt GetNumBlock() const {return m_nblock;}


		Header & Acquire();

		void Release(Header & block);


		Bank * CreateBank();


		const UInt m_blocksize;

		const UInt m_nblock;


		List <Bank,ListPolicy<false>> m_banks;

	};


	virtual void * Allocate(UIntNative n, const AllocInfo & info) override;

	virtual void Release(const void * ptr) override;


	Header & PointerToHeader(void * ptr);

	void * HeaderToPointer(Header & blockheader);


	typedef ConditionalType<MT,System::CriticalSection::Lock,Null> Lock;

	Reference <System::CriticalSection> m_cs;

	Pool m_size16, m_size32, m_size64, m_size128, m_size256;

	Pool * m_size2pool[257];
};

template <bool MT> inline FastAllocator<MT>::FastAllocator()
	: m_cs(MT ? System::CriticalSection::Create(false) ? Null<System::CriticalSection>())
	, m_size16(16, 1024)
	, m_size32(32, 512)
	, m_size64(64, 256)
	, m_size128(128, 128)
	, m_size256(256, 64)
{
	for (UInt idx = 0; idx < 16; ++idx) m_size2pool[idx] = &m_size16;
	for (UInt idx = 16; idx < 32; ++idx) m_size2pool[idx] = &m_size32;
	for (UInt idx = 32; idx < 64; ++idx) m_size2pool[idx] = &m_size64;
	for (UInt idx = 64; idx < 128; ++idx) m_size2pool[idx] = &m_size128;
	for (UInt idx = 128; idx < 257; ++idx) m_size2pool[idx] = &m_size256;

	REFLEX_ASSERT(Header::st_size >= sizeof(Header));
}

template <bool MT> inline void * FastAllocator<MT>::Allocate(UIntNative size, const AllocInfo & info)
{
	Lock lock(*m_cs);

	if (size < 257)
	{
		auto & pool = *m_size2pool[size];

		return HeaderToPointer(pool.Acquire());
	}
	else
	{
		auto & block = *reinterpret_cast<Header*>(g_default_allocator->Alloc(Header::st_size + size, info));

		block.bank = 0;

		return HeaderToPointer(block);
	}
}

template <bool MT> inline void FastAllocator<MT>::Release(const void * ptr)
{
	if (ptr)
	{
		Lock lock(*m_cs);

		auto & header = PointerToHeader(RemoveConst(ptr));

		if (header.bank)
		{
			header.bank->pool.Release(header);
		}
		else
		{
			g_default_allocator->Free(&header);
		}
	}
	else
	{
	}
}

template <bool MT> REFLEX_INLINE typename FastAllocator<MT>::Header & FastAllocator<MT>::PointerToHeader(void * ptr)
{
	auto pblock = Cast<UInt8>(ptr) - Header::st_size;

	return *reinterpret_cast<Header*>(pblock);
}

template <bool MT> REFLEX_INLINE void * FastAllocator<MT>::HeaderToPointer(Header & blockheader)
{
	UInt8 * pblock = reinterpret_cast<UInt8*>(&blockheader);

	return pblock + Header::st_size;
}


template <bool MT> REFLEX_INLINE FastAllocator<MT>::Bank::Bank(Pool & pool)
	: pool(pool)
{
	UInt blocksize = Header::st_size + pool.GetBlockSize();

	UInt nblock = pool.GetNumBlock();

	m_blocks = static_cast<UInt8*>(g_default_allocator->Alloc(blocksize * nblock, "FastAllocator"));

	m_nfree = nblock;


	UInt8 * pblock = m_blocks;

	REFLEX_LOOP(idx, nblock)
	{
		Header * header = Detail::Constructor<Header>::Construct(pblock);

		header->bank = this;

		header->Attach(m_free);

		pblock += blocksize;
	}
}

template <bool MT> REFLEX_INLINE FastAllocator<MT>::Bank::~Bank()
{
	UInt blocksize = Header::st_size + pool.GetBlockSize();

	UInt nblock = pool.GetNumBlock();

	REFLEX_ASSERT(nblock == m_free.GetNumItem());


	UInt8 * pblock = m_blocks;

	REFLEX_LOOP(idx, nblock)
	{
		auto header = reinterpret_cast<Header*>(pblock);

		Detail::Constructor<Header>::Deconstruct2(*header);

		pblock += blocksize;
	}

	g_default_allocator->Free(m_blocks);
}

template <bool MT> REFLEX_INLINE void FastAllocator<MT>::Bank::Release(Header & block)
{
	block.Attach(m_free);

	++m_nfree;
}

template <bool MT> REFLEX_INLINE typename FastAllocator<MT>::Header & FastAllocator<MT>::Bank::Acquire()
{
	Header & block = *m_free.GetLast();

	block.Detach();

	--m_nfree;

	return block;
}

template <bool MT> REFLEX_INLINE FastAllocator<MT>::Pool::Pool(UInt blocksize, UInt nblock)
	: m_blocksize(blocksize)
	, m_nblock(nblock)
{
	CreateBank();
}

template <bool MT> REFLEX_INLINE FastAllocator<MT>::Pool::~Pool()
{
	for (auto itr = m_banks.GetLast(); itr;)
	{
		auto & bank = Traverse<true>(itr);

		Detail::Constructor<Bank>::Destruct(bank);

		g_default_allocator->Free(&bank);
	}
}

template <bool MT> REFLEX_INLINE typename FastAllocator<MT>::Header & FastAllocator<MT>::Pool::Acquire()
{
	for (auto itr = m_banks.GetLast(); itr;)
	{
		auto & bank = Traverse<true>(itr);

		if (!bank.m_free.Empty())
		{
			return bank.Acquire();
		}
	}

	CreateBank();

	return m_banks.GetLast()->Acquire();
}

template <bool MT> REFLEX_INLINE void FastAllocator<MT>::Pool::Release(Header & header)
{
	Bank * bank = header.bank;

	bank->Release(header);

	if (bank->m_nfree == m_nblock)
	{
		Detail::Constructor<Bank>::Deconstruct2(*bank);

		g_default_allocator->Free(bank);
	}
}

template <bool MT> REFLEX_INLINE typename FastAllocator<MT>::Bank * FastAllocator<MT>::Pool::CreateBank()
{
	auto bank = Detail::Constructor<Bank>::Construct(g_default_allocator->Alloc(sizeof(Bank)), *this);

	bank->Attach(m_banks);

	return bank;
}

REFLEX_END_INTERNAL

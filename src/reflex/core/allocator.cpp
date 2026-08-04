#include "reflex/core/allocator/allocators.h"

#include "allocators/standard.cpp"

#include <map>




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex)

Detail::Initialiser <StandardAllocator> g_default_allocator_bytes;

AtomicUInt8 g_default_allocator_init_guard = 0;

REFLEX_END_INTERNAL

template class Reflex::Array <char>;

template class Reflex::Array <Reflex::WChar>;

const Reflex::Key32 Reflex::kRecycleAllocID = K32("Recycle");
const Reflex::Key32 Reflex::kBlockAllocID = K32("Block");

Reflex::TRef <Reflex::Allocator> Reflex::CreateAllocator(Key32 type, const Object &)
{
	struct RecycleAlloc : public Allocator
	{
		struct Memory : public Item <Memory, false>
		{
			Memory(List & list, UIntNative size)
				: size(size)
			{
				Attach(list);
			}

			using Item <Memory, false>::Attach;
			using Item <Memory, false>::Detach;

			const UInt32 magic = 1234;
			const UIntNative size;
			UInt32 start;
		};

		~RecycleAlloc()
		{
			UInt n = 0;

			for (auto & i : map)
			{
				REFLEX_ASSERT(!i.second.a);

				for (auto itr = i.second.b.GetFirst(); itr;)
				{
					auto memory = Reflex::Detail::Traverse<false>(itr);

					memory->Memory::~Memory();

					g_default_allocator->Free(memory.Adr());

					n++;
				}
			}
		}

		virtual void * OnAlloc(UIntNative n, const AllocInfo & info) override
		{
			Pair <Memory::List> & lists = map[n];

			if (auto last = lists.b.GetLast())
			{
				last->Attach(lists.a);

				return &last->start;
			}

			auto memory = Cast<Memory>(g_default_allocator->Allocate(sizeof(Memory) + n, info));

			Detail::Constructor<Memory>::Construct(memory, lists.a, n);

			return &memory->start;
		}

		virtual void OnFree(void * ptr) override
		{
			REFLEX_ASSERT(ptr);

			if (ptr)
			{
				auto & memory = *Reinterpret<Memory>(Cast<UInt8>(RemoveConst(ptr)) - REFLEX_OFFSETOF(Memory, start));

				if (memory.size > kMaxUInt16)
				{
					memory.Memory::~Memory();

					g_default_allocator->Free(&memory);
				}
				else
				{
					auto & current = map[memory.size];

					memory.Attach(current.b);
				}
			}
		}

		std::map < UIntNative, Pair <Memory::List> > map;
	};

	struct FastBlockAllocator : public Allocator
	{
		FastBlockAllocator(UInt item_size, UInt block_size = 256)
			: item_size(item_size)
			, block_size(item_size * block_size)
			, m_current(0)
			, m_end(0)
		{
		}

		void AquireBlock()
		{
			auto & block = m_blocks.Push();

			block.SetSize(block_size);

			m_current = block.GetData();

			m_end = m_current + block.GetSize();
		}

		virtual void * OnAlloc(UIntNative n, const AllocInfo & info) override
		{
			REFLEX_ASSERT(n <= item_size);

			if (m_current == m_end) AquireBlock();

			auto rtn = m_current;

			m_current += item_size;

			return rtn;
		}

		virtual void OnFree(void * ptr) override {}


		const UInt item_size, block_size;

		Array < Array <UInt8> > m_blocks;

		UInt8 * m_current, * m_end;
	};

	struct BlockAlloc : public Allocator
	{
		struct Block : public Item <Block>
		{
			using Item <Block>::Attach;
			using Item <Block>::Detach;

			UInt8 m_memory[1024 * 1024 * 1];
			UInt8 * m_top;
		};

		BlockAlloc()
		{
			auto first = REFLEX_CREATE(Block);

			first->Attach(m_blocks);

			first->m_top = first->m_memory;
		}

		virtual void * OnAlloc(UIntNative n, const AllocInfo & info) override
		{
			REFLEX_ASSERT(n <= sizeof(Block::m_memory));

			auto & last = *m_blocks.GetLast();

			auto remainder = UInt(last.m_memory + sizeof(last.m_memory) - last.m_top);

			if (n > remainder)
			{
				auto next = REFLEX_CREATE(Block);

				next->Attach(m_blocks);

				auto rtn = next->m_memory;

				next->m_top = rtn + n;

				return rtn;
			}
			else
			{
				auto rtn = last.m_top;

				last.m_top += n;

				return rtn;
			}
		}

		virtual void OnFree(void * ptr) override { }

		Block::List m_blocks;
	};

	switch (type.value)
	{
	case K32("Recycle"):
		//REFLEX_ASSERT(false);
		return REFLEX_CREATE(RecycleAlloc);

	case K32("Block"):
		//REFLEX_ASSERT(false);
		return REFLEX_CREATE(BlockAlloc);

	case K32("FastBlock128"):
		return REFLEX_CREATE(FastBlockAllocator, 128);

	default:
		return g_default_allocator;
	}
}

Reflex::TRef <Reflex::StandardAllocator> Reflex::GetDefaultAllocator()
{
	if (REFLEX_ATOMIC_SET_FILTERED(g_default_allocator_init_guard, UInt8(1)))
	{
		g_default_allocator_bytes.Init();

#if REFLEX_DEBUG
		atexit([]()
		{
			g_default_allocator_bytes.Deinit();
		});
#endif
	}

	g_default_allocator = g_default_allocator_bytes.Adr();

	return g_default_allocator_bytes.Adr();
}

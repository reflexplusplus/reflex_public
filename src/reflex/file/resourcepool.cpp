#include "resourcepool.h"
#include "../../../include/reflex/file/functions/path.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::File)

ResourcePoolImpl::ResourcePoolImpl(VirtualFileSystem & filesystem)
	: ResourcePool(filesystem),
	m_flushing(false)
{
	REFLEX_ASSERT(module.IsInitalised() || IsNull(filesystem));

	Retain(filesystem);

	m_nitems[Attributes::kStatusMissing] = 0;
	m_nitems[Attributes::kStatusStreaming] = 0;
	m_nitems[Attributes::kStatusReady] = 0;
}

ResourcePoolImpl::~ResourcePoolImpl()
{
	{
		Lock lock(*this);

		lock.Flush();
	}

	if (REFLEX_DEBUG)
	{
		UInt n = 0;

		for (auto & i : m_items)
		{
			auto & token = i.value;

			if (token.object->GetAllocator())
			{
				output.Log(token.path);

				n++;
			}
		}
	}

	Release(filesystem);
}

Reflex::UInt ResourcePoolImpl::GetTotalNumItem() const
{
	return m_items.GetSize();
}

const UInt & ResourcePoolImpl::GetNumItem(Attributes::Status status) const
{
	return m_nitems[status];
}

bool ResourcePoolImpl::Flush(UInt cycles)
{
	if (m_flushing) output.Log("! Recursive Flush !");

	m_flushing++;

	bool flushed = false;

	while (True(cycles) && m_items.GetSize())
	{
		m_flush_position = Modulo(m_flush_position + 1, m_items.GetSize());

		auto & token = m_items[m_flush_position].value;

		auto & object = *token.object;

		if (object.GetRetainCount() == 1)
		{
			RemoveItem(m_flush_position);

			flushed = true;

			StateMt::Notify();
		}

		cycles--;
	}

	m_flushing--;

	return flushed;
}

void ResourcePoolImpl::RemoveItem(UInt idx)
{
	m_nitems[m_items[idx].value.attributes.status]--;

	m_items.Remove(idx);

	StateMt::Notify();
}

REFLEX_END_INTERNAL

Reflex::File::ResourcePool::ResourcePool(VirtualFileSystem & filesystem)
	: filesystem(filesystem)
{
}

Reflex::TRef <Reflex::File::ResourcePool> Reflex::File::ResourcePool::Create(VirtualFileSystem & filesystem)
{
	return REFLEX_CREATE(ResourcePoolImpl, filesystem);
}

void Reflex::File::ResourcePool::Lock::Clear(TypeID type_id)
{
	auto impl = Cast<ResourcePoolImpl>(resourcepool);

	auto & items = impl->m_items;

	UInt32 begin = items.SearchGTE({ kZeroKey, type_id }).value;

	UInt32 end = Max<UInt32>(begin, items.SearchLT({ kZeroKey, type_id + 1 }).value + 1);

	auto n = end - begin;

	REFLEX_LOOP_PTR(items.GetData() + begin, ptr, n) impl->m_nitems[(*ptr)->value.attributes.status]--;

	items.Remove(begin, n);

	impl->StateMt::Notify();
}

Reflex::File::ResourcePool::Token & Reflex::File::ResourcePool::Lock::Insert(const WString::View & path, const Attributes & attributes, TypeID type_id, TRef <Object> object)
{
	auto impl = Cast<ResourcePoolImpl>(resourcepool);

	Address address = { path, type_id };

	if (auto current = impl->m_items.SearchValue(address))
	{
		impl->m_nitems[current->attributes.status]--;
	}

	auto & token = impl->m_items.Acquire(address);

	token.address = address;

	token.path = path;

	token.attributes = attributes;

	token.object = object;

	impl->m_nitems[attributes.status]++;

	impl->StateMt::Notify();

	return token;
}

bool Reflex::File::ResourcePool::Lock::Remove(Address adr)
{
	auto impl = Cast<ResourcePoolImpl>(resourcepool);

	if (auto idx = impl->m_items.Search(adr))
	{
		impl->RemoveItem(idx.value);

		return true;
	}

	return false;
}

const Reflex::File::ResourcePool::Token * Reflex::File::ResourcePool::Lock::Query(Address adr) const
{
	auto impl = Cast<ResourcePoolImpl>(resourcepool);

	return impl->m_items.SearchValue(adr);
}

void Reflex::File::ResourcePool::Lock::Enumerate(const Function <void(const Token &)> & callback) const
{
	auto impl = Cast<ResourcePoolImpl>(resourcepool);

	for (auto & i : impl->m_items) callback(i.value);
}

void Reflex::File::ResourcePool::Lock::Enumerate(TypeID type_id, const Function <void(const Token&)> & callback) const
{
	typedef typename decltype(ResourcePoolImpl::m_items)::ConstItr Itr;

	auto & items = Cast<ResourcePoolImpl>(resourcepool)->m_items;

	auto range = Reverse(MakeRange(items, { kZeroKey, type_id }, { kZeroKey, type_id + 1 }));

	for (auto & i : range)
	{
		callback(i.value);
	}
}

bool Reflex::File::ResourcePool::Lock::PurgeIncremental()
{
	auto impl = Cast<ResourcePoolImpl>(resourcepool);

	return impl->Flush(8);
}

bool Reflex::File::ResourcePool::Lock::Flush()
{
	auto impl = Cast<ResourcePoolImpl>(resourcepool);

	bool flushed = false;

	while (impl->Flush(impl->m_items.GetSize())) flushed = true;

	return flushed;
}

const Reflex::File::ResourcePool::Token & Reflex::File::ResourcePool::Lock::RetrieveToken(TypeID type_id, const WString::View & path, const Data::PropertySet & options, Ctr ctr)
{
	auto impl = Cast<ResourcePoolImpl>(resourcepool);

	Address address = { path, type_id };

	REFLEX_ASSERT(!And(type_id == REFLEX_TYPEID(Data::ArchiveObject), !path));

	if (auto ptoken = impl->m_items.SearchValue(address))
	{
		return *ptoken;
	}
	else
	{
		auto & token = impl->m_items.Insert(address);

		token.address = address;

		token.path = path;

		if (auto instream = AutoRelease(lock.Read(path, token.attributes)))
		{
			token.object = (*ctr)({ *this, options, path, token.attributes.status }, instream);
		}
		else
		{
			token.attributes.status = Attributes::kStatusMissing;

			REFLEX_ASSERT(!And(type_id == REFLEX_TYPEID(Data::ArchiveObject), !token.attributes.resolved_path));

			token.attributes.resolved_path = path;

			token.object = (*ctr)({ *this, options, path, Attributes::kStatusMissing }, System::FileHandle::null);

			output.Warn("Missing Resource:", path);
		}

		impl->m_nitems[token.attributes.status]++;

		impl->StateMt::Notify();

		return token;
	}
}

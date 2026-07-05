#include "globalimpl.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

REFLEX_INLINE UInt64 PackTimeSizeStatus(const File::Attributes & attributes)
{
	UInt64 time = attributes.size_time.b & kMaxUInt32;
	UInt64 size = attributes.size_time.a & 0x3FFFFFFFull;	// 30 bits
	UInt64 status = attributes.status & 0x3ull;				// 2 bits

	return (status << 62) | (size << 32) | time;
}

GlobalImpl::GlobalImpl(Reflex::File::ResourcePool & resourcepool, Data::PropertySet & prefs)
	: m_resourcepool(resourcepool)
	, m_prefs(prefs)
	, m_monitor(*this)
	, m_counter(0)
	, m_clock(System::CreateListener(System::kNotificationClock, this, [](void * pself)
	{
		auto & self = *Cast<GlobalImpl>(pself);

		if (GLX::module.IsInitalised() && ((self.m_counter++ & 31) == 0 || self.m_monitor.Poll()))
		{
			GLX::Core::Context context;

			//TODO support multiple resourcepools

			File::ResourcePool::Lock lock(self.m_resourcepool);

			auto & clients = self.m_clients;

			for (auto & i : clients)
			{
				REFLEX_ASSERT(i.value.b->m_resourcepool == self.m_resourcepool);

				i.value.b->Refresh(lock);
			}

			while (auto token = self.m_pending_rebuilds.GetFirst())
			{
				auto retain = AutoRelease(token->self);

				retain->ForceRebuild(lock);

				token->Detach();
			}
		}
	}))
{
	REFLEX_ASSERT(System::module.IsInitalised());

	REFLEX_ASSERT_MAINTHREAD("IDE::GlobalImpl::GlobalImpl");

	File::output.Log("IDE");
}

GlobalImpl::~GlobalImpl()
{
	REFLEX_ASSERT_MAINTHREAD("IDE::GlobalImpl::~GlobalImpl");

	File::output.Log("/IDE");
}

ResourceGroupImpl::ResourceGroupImpl(File::ResourcePool & resourcepool, Key32 uid, WString::View desc, const Function <void(ResourceGroup&)> & onreload)
	: m_resourcepool(resourcepool)
	, m_uid(uid)
	, m_onreload(onreload)
{
	m_pending_rebuild_token.self = this;

	auto global = TheGlobal::Get();

	global->m_clients.Set(uid, { desc, this });

	global->Notify();
}

ResourceGroupImpl::~ResourceGroupImpl()
{
	if (kIsAwake)
	{
		auto global = TheGlobal::Get();

		auto & clients = global->m_clients;

		if (auto idx = clients.Search(m_uid))
		{
			if (clients[idx.value].value.b.Adr() == this)
			{
				clients.Remove(idx.value);

				global->Notify();
			}
		}
	}
}

void ResourceGroupImpl::Refresh(File::ResourcePool::Lock & lock)
{
	for (auto & i : m_items)
	{
		if (auto current = lock.Query(i.adr))
		{
			File::Attributes attributes = current->attributes;

			auto file = AutoRelease(lock.lock.Read(current->attributes.resolved_path, attributes));

			if ((i.time_size_status != PackTimeSizeStatus(attributes)) && IsValid(file))
			{
				m_pending_rebuild_token.Attach(TheGlobal::Get()->m_pending_rebuilds);

				break;
			}
		}
		else
		{
			//this case covers that 2 ResourceGroupImpl are using same item, and it already got removed in another Refresh

			m_pending_rebuild_token.Attach(TheGlobal::Get()->m_pending_rebuilds);

			break;
		}
	}
}

void ResourceGroupImpl::Clear()
{
	m_items.Clear();

	TheGlobal::Get<true>()->Notify();
}

void ResourceGroupImpl::AddItem(Address adr, ConstTRef <Object> object)
{
	File::ResourcePool::Lock lock(m_resourcepool);

	UInt64 time = 0;

	if (auto token = lock.Query(adr))
	{
		time = PackTimeSizeStatus(token->attributes);
	}

	m_items.Push({ adr, object.RemoveConst(), time });

	TheGlobal::Get<true>()->Notify();
}

void ResourceGroupImpl::ForceRebuild(File::ResourcePool::Lock & lock)
{
	if (m_items)
	{
		for (auto & i : m_items)
		{
			lock.Remove(i.adr);
		}

		m_items.Clear();

		m_onreload(*this);

		REFLEX_ASSERT_EX(m_items || GetRetainCount() == 1, "ResouceGroup is not tracking anything after refresh");
	}
}

REFLEX_END_INTERNAL

const bool & Reflex::IDE::kIsAwake = Reflex::IDE::TheGlobal::IsAwake();

Reflex::TRef <Reflex::Object> Reflex::IDE::Start(File::ResourcePool & resourcepool, Data::PropertySet & prefs)
{
	REFLEX_ASSERT(File::module.IsInitalised());
	
	return TheGlobal::Acquire(resourcepool, prefs);
}

Reflex::TRef <Reflex::IDE::ResourceGroup> Reflex::IDE::ResourceGroup::Create(File::ResourcePool & resourcepool, Key32 uid, WString::View desc, const Function <void(ResourceGroup&)> & onreload)
{
	if (kIsAwake)
	{
		return REFLEX_CREATE(ResourceGroupImpl, resourcepool, uid, desc, onreload);
	}
	else
	{
		return REFLEX_CREATE(PlaceholderResourceGroup);
	}
}

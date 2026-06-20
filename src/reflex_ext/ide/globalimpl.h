#pragma once

#include "placeholder.h"




//
//decl

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

struct ResourceGroupImpl : public ResourceGroup
{
	struct RebuildToken : public Reflex::Item <RebuildToken, false>
	{
		using Item::Attach;
		using Item::Detach;

		ResourceGroupImpl * self;
	};

	struct ResourceRef
	{
		Address adr;
		Reference <Object> object;
		UInt64 time_size_status;
	};

	ResourceGroupImpl(File::ResourcePool & resourcepool, Key32 uid, const WString::View & desc, const Function <void(ResourceGroup&)> & onreload);

	~ResourceGroupImpl();

	void Refresh(File::ResourcePool::Lock & lock);

	void Clear() override;

	void AddItem(Address adr, ConstTRef <Object> object) override;

	void ForceRebuild(File::ResourcePool::Lock & lock) override;


	const Reference <File::ResourcePool> m_resourcepool;

	Array <ResourceRef> m_items;

	const Key32 m_uid;

	const Function <void(ResourceGroup&)> m_onreload;

	RebuildToken m_pending_rebuild_token;
};

struct GlobalImpl : public Global
{
	static constexpr Key32 kLogFile = K32("IDE/LogFile");


	GlobalImpl(File::ResourcePool & resourcepool, Data::PropertySet & prefs, const WString::View & logfile);

	~GlobalImpl();

	using State::Notify;


	Reference <File::ResourcePool> m_resourcepool;

	Reference <Data::PropertySet> m_prefs;


	Sequence <Key32, Pair < WString, TRef <ResourceGroupImpl> > > m_clients;


	State::Monitor m_monitor;

	UInt32 m_counter;

	ResourceGroupImpl::RebuildToken::List m_pending_rebuilds;
	
	Reference <Object> m_clock;
};

typedef Reflex::The <GlobalImpl> TheGlobal;

REFLEX_END_INTERNAL

#pragma once

#include "placeholder.h"




//
//decl

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

struct ResourceGroupImpl : public ResourceGroup
{
	struct Item
	{
		Address adr;
		Reference <Object> object;
		UInt64 time_size_status;
	};

	ResourceGroupImpl(File::ResourcePool & resourcepool, Key32 uid, const WString::View & desc, const Function <void(ResourceGroup&)> & onreload);

	~ResourceGroupImpl();

	void Refresh(File::ResourcePool::Lock & lock);

	virtual void Clear() override;

	virtual void AddItem(Address adr, ConstTRef <Object> object) override;

	virtual void ForceRebuild(File::ResourcePool::Lock & lock) override;


	const Reference <File::ResourcePool> m_resourcepool;

	Array <Item> m_items;

	const Key32 m_uid;

	const Function <void(ResourceGroup&)> m_onreload;
};

struct GlobalImpl : public Global
{
	static inline const Key32 kLogFile = K32("IDE/LogFile");


	GlobalImpl(File::ResourcePool & resourcepool, Data::PropertySet & prefs, const WString::View & logfile);

	~GlobalImpl();

	using State::Notify;


	Reference <File::ResourcePool> m_resourcepool;

	Reference <Data::PropertySet> m_prefs;


	Sequence <Key32, Pair < WString, TRef <ResourceGroupImpl> > > m_clients;


	State::Monitor m_monitor;

	UInt32 m_counter;

	Reference <Object> m_clock;
};

typedef Reflex::The <GlobalImpl> TheGlobal;

REFLEX_END_INTERNAL

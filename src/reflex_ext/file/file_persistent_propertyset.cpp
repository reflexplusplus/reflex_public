#include "reflex_ext/file/persistent_propertyset.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::File)

struct NullPersistentPropertySet : public PersistentPropertySet
{
	NullPersistentPropertySet()
		: PersistentPropertySet(Null<Data::SerializableFormat>())
	{
	}

	void OnUnsetProperty(Address address) override {}

	void OnSetProperty(Address adr, Object & object) override
	{
		auto a = AutoRelease(object);

		File::output.Error("File::PersistentPropertySet::OnSetProperty on null discarded");
	}

	void OnQueryProperty(Address address, Object * & value) const override
	{
	}
};

Reflex::Detail::Module g_module("Reflex::File.ext", File::module);

Reflex::Detail::Module::Member <NullPersistentPropertySet> g_null_persistentpropertyset(g_module);

REFLEX_END_INTERNAL

Reflex::File::PersistentPropertySet::PersistentPropertySet(const Data::SerializableFormat & format, Allocator & allocator)
	: Data::PropertySet(allocator),
	format(format)
{
}

Reflex::File::PersistentPropertySet::PersistentPropertySet(PersistentPropertySet && value)
	: Data::PropertySet(std::move(value)),
	format(value.format)
{
}

Reflex::TRef <Reflex::Object> Reflex::File::PersistentPropertySet::CreateListener(const Function <void(Notification, Key32)> & callback)
{
	REFLEX_STATIC_ASSERT(sizeof(UInt32) == sizeof(Notification));

	return m_signal.CreateListener(Reinterpret<Function<void(UInt32,Key32)>>(callback));
}

bool Reflex::File::PersistentPropertySet::Open(const WString::View & path, Key32 context)
{
	auto archive = File::Open(path);

	bool ok;

	{
		SignalComponent<Notification,Key32>::Mute mute(m_signal);

		format->Reset(*this);

		ok = format->Decode(*this, archive);
	}

	State::Notify();

	m_signal.Notify(kNotificationRestore, context);

	return ok;
}

void Reflex::File::PersistentPropertySet::Save(const WString::View & path)
{
	Data::Archive archive;

	format->Encode(archive, *this);

	File::Save(path, archive);
}

void Reflex::File::PersistentPropertySet::Reset(Key32 context)
{
	{
		SignalComponent<Notification,Key32>::Mute mute(m_signal);

		format->Reset(*this);
	}

	State::Notify();

	m_signal.Notify(kNotificationReset, context);
}

void Reflex::File::PersistentPropertySet::Deserialize(Data::Archive::View & stream, Key32 context)
{
	{
		SignalComponent<Notification,Key32>::Mute mute(m_signal);

		format->Deserialize(stream, *this);
	}

	State::Notify();

	m_signal.Notify(kNotificationRestore, context);
}

void Reflex::File::PersistentPropertySet::Serialize(Data::Archive & stream) const
{
	m_signal.Notify(kNotificationStore, kNullKey);

	format->Serialize(stream, *this);
}

void Reflex::File::PersistentPropertySet::OnUnsetProperty(Address address)
{
	Data::PropertySet::OnUnsetProperty(address);

	if (format->SupportsType(address.type_id))
	{
		State::Notify();

		m_signal.Notify(kNotificationUpdate, kNullKey);
	}
}

void Reflex::File::PersistentPropertySet::OnSetProperty(Address address, Object & object)
{
	if (format->SupportsType(address.type_id))
	{
		Data::PropertySet::OnSetProperty(address, object);

		State::Notify();

		m_signal.Notify(kNotificationUpdate, kNullKey);
	}
	else
	{
		REFLEX_ASSERT(false);

		File::output.Error("non-persistent type discarded", object.object_t->tname);

		AutoRelease(object);
	}
}

Reflex::File::PersistentPropertySet & Reflex::File::PersistentPropertySet::null = Reflex::File::g_null_persistentpropertyset;

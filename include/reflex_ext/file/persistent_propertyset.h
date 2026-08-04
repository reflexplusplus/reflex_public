#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::File
{

	class PersistentPropertySet;

}




//
//PersistentPropertySet

class Reflex::File::PersistentPropertySet :
	public Data::PropertySet,
	public State
{
public:

	REFLEX_OBJECT(PersistentPropertySet, Data::PropertySet);

	static PersistentPropertySet & null;

	using Data::PropertySet::Iterate;


	enum Notification : UInt32
	{
		kNotificationUpdate,
		kNotificationReset,		//called after Reset
		kNotificationRestore,	//called after Deserialize
		kNotificationStore,		//called before Serialize

		kNumNotification
	};

	static constexpr auto kContextSession = kNullKey;

	static constexpr auto kContextPreset = MakeKey32("Preset");



	//lifetime

	PersistentPropertySet(const Data::SerializableFormat & format, Allocator & allocator = g_default_allocator);

	PersistentPropertySet(const PersistentPropertySet & propertyset) = delete;

	PersistentPropertySet(PersistentPropertySet && rhs);



	//subscribe
	
	TRef <Object> CreateListener(const Function <void(Notification notification, Key32 context)> & callback);



	//io

	void Reset(Key32 context = kNullKey);

	void Deserialize(Data::Archive::View & stream, Key32 context = kContextSession);

	void Serialize(Data::Archive & stream) const;


	bool Open(const WString::View & path, Key32 context = kContextPreset);

	void Save(const WString::View & path);



	//assign

	PersistentPropertySet & operator=(const PersistentPropertySet & propertyset) = delete;

	PersistentPropertySet & operator=(PersistentPropertySet && rhs);



	//compare

	using Data::PropertySet::operator bool;

	bool operator==(const PersistentPropertySet & propertyset) const;

	bool operator!=(const PersistentPropertySet & propertyset) const;



	//links

	const ConstTRef <Data::SerializableFormat> format;



private:

	void OnUnsetProperty(Address address) override;

	void OnSetProperty(Address adr, Object & object) override;


	SignalComponent <Notification,Key32> m_signal;
};




//
//impl

REFLEX_INLINE Reflex::File::PersistentPropertySet & Reflex::File::PersistentPropertySet::operator=(PersistentPropertySet && rhs)
{
	if (rhs.format == format)
	{
		Data::PropertySet::operator=(std::move(rhs));
	}
	else
	{
		format->Reset(*this);

		format->Decode(*this, EncodePropertySet(format, rhs));
	}

	return *this;
}

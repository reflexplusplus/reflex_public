#include "../../../../include/reflex_ext/bootstrap/common/persistent_state.h"




//
//impl

Reflex::Bootstrap::PersistentState::PersistentState(File::PersistentPropertySet & propertyset, Key32 chunkid, UInt16 chunkversion)
	: Data::iSerializable(chunkversion),
	propertyset(propertyset),
	chunkid(chunkid),
	m_listener(propertyset.CreateListener([this](File::PersistentPropertySet::Notification n, Key32 context)
{
	switch (n)
	{
	case File::PersistentPropertySet::kNotificationReset:
		Data::iSerializable::Reset(context);
		break;

	case File::PersistentPropertySet::kNotificationRestore:
		RestoreState(context);
		break;

	case File::PersistentPropertySet::kNotificationStore:
		StoreState();
		break;

	default:	//passify android studio
		break;
	}
}))
{
}

void Reflex::Bootstrap::PersistentState::RestoreState(Key32 context)
{
	if (auto chunk = Data::GetBinary(propertyset, chunkid))
	{
		Data::iSerializable::Deserialize(chunk, context);
	}
	else
	{
		Data::iSerializable::Reset();
	}
}

void Reflex::Bootstrap::PersistentState::StoreState()
{
	if (Data::iSerializable::version)
	{
		Data::Archive stream;

		Data::iSerializable::Serialize(stream);

		Data::SetBinary(propertyset, chunkid, stream);
	}
	else if constexpr (REFLEX_DEBUG)
	{
		File::output.Warn("Bootstrap::PersistentState not stored as chunkversion is 0");
	}
}

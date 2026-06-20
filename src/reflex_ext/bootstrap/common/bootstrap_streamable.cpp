#include "../../../../include/reflex_ext/bootstrap/common/streamable.h"




//
//impl

Reflex::Bootstrap::Streamable::Streamable(File::PersistentPropertySet & propertyset, Key32 chunkid, UInt16 chunkversion)
	: Data::iStreamable(chunkversion),
	propertyset(propertyset),
	chunkid(chunkid),
	m_listener(propertyset.CreateListener([this](File::PersistentPropertySet::Notification n, Key32 context)
{
	switch (n)
	{
	case File::PersistentPropertySet::kNotificationReset:
		Data::iStreamable::Reset(context);
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

void Reflex::Bootstrap::Streamable::RestoreState(Key32 context)
{
	if (auto chunk = Data::GetBinary(propertyset, chunkid))
	{
		Data::iStreamable::Deserialize(chunk, context);
	}
	else
	{
		Data::iStreamable::Reset();
	}
}

void Reflex::Bootstrap::Streamable::StoreState()
{
	if (Data::iStreamable::version)
	{
		Data::Archive stream;

		Data::iStreamable::Serialize(stream);

		Data::SetBinary(propertyset, chunkid, stream);
	}
	else if constexpr (REFLEX_DEBUG)
	{
		File::output.Warn("Bootstrap::Streamable not stored as chunkversion is 0");
	}
}

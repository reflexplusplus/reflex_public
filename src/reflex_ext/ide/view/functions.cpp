#include "../globalimpl.h"




void Reflex::IDE::Detail::ResetSerializable(Key32 context, GLX::Object & object)
{
	if (auto iface = QueryInterface<Data::iSerializable>(object))
	{
		iface->Reset(context);
	}
}

void Reflex::IDE::Detail::RestoreSerializable(const Data::PropertySet & propertyset, Key32 context, GLX::Object & object)
{
	if (auto iface = QueryInterface<Data::iSerializable>(object))
	{
		if (auto stream = Data::GetBinary(propertyset, object.id))
		{
			iface->Deserialize(stream, context);
		}
		else
		{
			iface->Reset(context);
		}
	}
}

void Reflex::IDE::Detail::StoreSerializable(Data::PropertySet & propertyset, const GLX::Object & object)
{
	if (object.id != kNullKey)
	{
		if (auto iface = QueryInterface<Data::iSerializable>(object))
		{
			//REFLEX_ASSERT(iface->version);

			auto chunk = Data::Detail::AcquireProperty<Data::ArchiveObject>(propertyset, object.id);

			chunk->value.Clear();

			iface->Serialize(chunk->value);
		}
	}
}

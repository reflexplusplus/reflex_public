#include "../../../../include/reflex/data/interfaces/streamable.h"
#include "../../../../include/reflex/data/serialisation.h"




//
//declarations

void Reflex::Data::iStreamable::Serialize(Archive & stream) const
{
	Marker <UInt32> size(stream);

	Data::Serialize(stream, version);

#if REFLEX_DEBUG
	auto pos = stream.GetSize();
#endif

	OnStore(stream);

#if REFLEX_DEBUG
	if (stream.GetSize() != pos && !version)
	{
		System::DebugLog(false, "iStreamable::Store with version == 0");
	}
#endif

	size.Set(size.GetDelta());
}

void Reflex::Data::iStreamable::Deserialize(Archive::View & stream, Key32 context)
{
	auto size = Data::Deserialize<UInt32>(stream);
	
	auto bytes = ReadBytes(stream, size);

	auto version = *Reinterpret<UInt16>(bytes.data);

	auto chunk = Nudge(bytes, 2);

	if (version == iStreamable::version)
	{
		OnRestore(chunk, context);

		return;
	}
	else if (version < iStreamable::version)
	{
		if (OnImport(version, chunk, context))
		{
			return;
		}
	}

	OnReset(context);
}

void Reflex::Data::iStreamable::Skip(Archive::View & stream)
{
	auto size = Data::Deserialize<UInt32>(stream);

	stream = Nudge(stream, size);
}

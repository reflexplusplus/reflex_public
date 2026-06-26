#include "riff.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::Data)

UInt32 EnumerateRIFF(Archive::View & instream, PropertySet & output)
{
	struct Header
	{
		UInt32 riffid;
		UInt32 size;
		UInt32 headerid;
	};

	REFLEX_STATIC_ASSERT(sizeof(Header) == 12);

	if (instream.size < sizeof(Header)) return 0;

	bool bitreverse = false;

	auto header = Reinterpret<Header>(instream.data);

	instream = Nudge(instream, sizeof(Header));

	switch (header->riffid)
	{
	case ID32("RIFF"):
		break;

	case ID32("FORM"):
		bitreverse = true;
		break;

	default:
		return 0;
	};

	while (instream.size > 8)
	{
		auto chunkheader = Deserialize<UInt32,UInt32>(instream);

		if (bitreverse) chunkheader.b = Reflex::BitReverse(chunkheader.b);

		auto remainder = instream.size;

		if (remainder < chunkheader.b)
		{
			auto chunk = Inc(instream, remainder);

			AcquirePropertySet(output, K32("streaming"))->SetProperty(chunkheader.a, REFLEX_CREATE(ArchiveObject, chunk));
		}
		else
		{
			auto chunk = Inc(instream, chunkheader.b);

			SetBinary(output, chunkheader.a, chunk);

			auto skip = chunkheader.b & 1;

			if (skip && instream) Inc(instream, skip);
		}
	}

	return header->headerid;
}

bool RIFF::SupportsType(UInt32 tid) const
{
	return tid == REFLEX_TYPEID(ArchiveObject) || tid == REFLEX_TYPEID(Key32Property);
}

void RIFF::OnReset(PropertySet & dynamic) const
{
	UnsetUInt32(dynamic, kid);

	UnsetAll<ArchiveObject>(dynamic);
}

SerializableFormat::DeserializeError RIFF::OnDeserialize(Archive::View & instream, PropertySet & data, UInt32 options) const
{
	auto headerid = EnumerateRIFF(instream, data);

	SetUInt32(data, kid, headerid);

	return kDeserializeErrorNone;	//TODO
}

void RIFF::OnSerialize(Archive & out, const PropertySet & in) const
{
	struct Chunk
	{
		Chunk(Archive & stream, UInt32 id)
			: stream(stream)
		{
			Data::Serialize(stream, id);

			m_sizepos = stream.GetSize();

			Data::Serialize(stream, UInt32(0));
		}

		~Chunk()
		{
			auto size = stream.GetSize() - (m_sizepos + 4);

			if (size & 1)
			{
				Data::Serialize(stream, UInt8(0));

				size++;
			}

			*Reinterpret<UInt32>(stream.GetData() + m_sizepos) = size;
		}

		Archive & stream;

		UInt m_sizepos;
	};

	Chunk outer(out, ID32("RIFF"));

	auto id = GetUInt32(in, kid, GetKey32(in, kid).value);	//fallback FOR REFLEX VM -> need remapping or UInt32 support

	REFLEX_ASSERT(id);

	Data::Serialize(out, id);

	if (auto pindex = in.QueryProperty<ArrayOfKey32Property>(K32("index")))
	{
		for (auto & i : pindex->value)
		{
			Chunk chunk(out, i.value);

			auto data = GetProperty<ArchiveObject>(in, i);

			out.Append(data->value);
		}
	}
	else
	{
		for (auto & i : in.Iterate<ArchiveObject>())
		{
			Chunk chunk(out, i.key.id.value);

			out.Append(i.value->value);
		}
	}
}

REFLEX_END_INTERNAL

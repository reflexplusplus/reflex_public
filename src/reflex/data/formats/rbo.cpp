#include "rbo.h"




//
//impl

Reflex::Data::PropertySetFormatImpl::PropertySetFormatImpl(UInt32 magic)
	: m_magic(magic)
{
}

bool Reflex::Data::PropertySetFormatImpl::SupportsType(TypeID type_id) const
{
	return True(m_types.Search(type_id));
}

Reflex::Array <Reflex::Address> Reflex::Data::PropertySetFormatImpl::Compare(const PropertySet & a, const PropertySet & b) const
{
	Array <Address> matches;

	TypeID type_id = 0;

	TypeHandler null = { 0,0,0,0,0,[](const PropertySetFormat & format, const Object & a, const Object & b){ return true; } };

	const TypeHandler * ptype = &null;

	for (auto & i : a.Iterate())
	{
		if (SetFiltered(type_id, i.key.type_id))
		{
			ptype = m_types.Search(type_id, &null);
		}
		
		if (auto p = RemoveConst(b).QueryProperty(i.key))
		{
			if (ptype->compare(*this, i.value, *p)) matches.Push(i.key);
		}
	}

	return matches;
}

void Reflex::Data::PropertySetFormatImpl::OnReset(PropertySet & data) const
{
	for (auto & i : m_types)
	{
		data.UnsetAll(i.value.type);
	}
}

void Reflex::Data::PropertySetFormatImpl::OnSerialize(Archive & stream, const PropertySet & data) const
{
	Data::Serialize(stream, m_magic);

	SerializePropertySetImpl(stream, data);
}

Reflex::Data::SerializableFormat::DeserializeError Reflex::Data::PropertySetFormatImpl::OnDeserialize(Archive::View & stream, PropertySet & data, UInt32 options) const
{
	if (stream.size < 5) return kDeserializeErrorInvalidHeader;

	auto header = Data::Deserialize<UInt32>(stream);

	if (header == m_magic)
	{
		try
		{
			DeserializePropertySetImpl(stream, data);
		}
		catch (DeserializeError e)
		{
			return e;
		}

		return kDeserializeErrorNone;
	}
	else
	{
		//support legacy versions

		stream = Nudge(stream, -4);

		return Cast<BinaryFormat>(kBinaryFormat)->OnDeserialize(stream, data, options);
	}
}

REFLEX_NOINLINE void Reflex::Data::PropertySetFormatImpl::SerializePropertySetImpl(Archive & stream, const PropertySet & data) const
{
	//collect types

	Array <const TypeHandler*> supported;

	TypeID type_id = 0;

	for (auto & i : data.Iterate())
	{
		if (SetFiltered(type_id, i.key.type_id))
		{
			if (auto type = m_types.Search(type_id))
			{
				supported.Push(type);
			}
			else
			{
				auto tname = i.value->object_t->tname;

				File::output.Error(i.key.id.value, tname, "PropertySetFormat unsupported type will not be stored");
			}
		}
	}


	//store

	Data::Serialize(stream, UInt8(supported.GetSize()));

	for (auto ptype_handler : supported)
	{
		Data::Serialize(stream, ptype_handler->persistentid);

		Data::Marker <UInt32> size(stream);

		for (auto & i : data.Iterate(ptype_handler->type))
		{
			auto & key = i.key;

			Data::Serialize(stream, key.id.value - kHashSeed);

			ptype_handler->store(*this, stream, i.value);
		}

		size.Set(stream.GetSize() - (size.GetPosition() + sizeof(UInt32)));
	}
}

REFLEX_NOINLINE void Reflex::Data::PropertySetFormatImpl::DeserializePropertySetImpl(Archive::View & stream, PropertySet & data) const
{
	TypeHandler * null = nullptr;

	auto ntype = Data::Deserialize<UInt8>(stream);

	while (ntype--)
	{
		if (stream.size < 5) throw(kDeserializeErrorInvalidStream);	//UInt8 id + UInt32 size

		auto persistentid = Data::Deserialize<UInt8>(stream);

		if (auto ptype_handler = *m_persistentids.Search(persistentid, &null))
		{
			auto substream_size = Data::Deserialize<UInt32>(stream);

			if (stream.size < substream_size) throw(kDeserializeErrorInvalidStream);

			auto substream = ReadBytes(stream, substream_size);

			while (substream)
			{
				auto key = Data::Deserialize<UInt32>(substream) + kHashSeed;

				data.SetProperty({ key, ptype_handler->type }, ptype_handler->restore(*this, substream));
			}
		}
		else
		{
			throw(kDeserializeErrorUnknownType);
		}
	}
}

bool Reflex::Data::BinaryFormat::SupportsType(TypeID type_id) const
{
	return type_id == REFLEX_TYPEID(ArchiveObject);
}

void Reflex::Data::BinaryFormat::OnReset(PropertySet & node) const
{
	node.UnsetAll(REFLEX_TYPEID(ArchiveObject));
}

Reflex::Data::SerializableFormat::DeserializeError Reflex::Data::BinaryFormat::OnDeserialize(Archive::View & stream, PropertySet & data, UInt32 options) const
{
	if (stream.size < 4) return kDeserializeErrorInvalidHeader;

	auto header = Data::Deserialize<UInt32>(stream);

	constexpr char kHeader[2] = { 'R', 'B' };

	if (Reinterpret<UInt16>(header) == *Reinterpret<UInt16>(kHeader))
	{
		REFLEX_LOOP(idx, Reinterpret<Pair<UInt16>>(header).b)
		{
			if (stream.size < 8) return kDeserializeErrorInvalidStream;

			auto [id,size] = Data::Deserialize<Key32,UInt32>(stream);

			if (stream.size < size) return kDeserializeErrorInvalidStream;

			SetBinary(data, id, ReadBytes(stream, size));
		}

		return kDeserializeErrorNone;
	}
	else if (header == kMagicV3)
	{
		//intentionally not stream checking here as this format isnt in general use

		auto npropertyset = Data::Deserialize<UInt16>(stream);

		while (npropertyset--)
		{
			File::output.Error("Data::BinaryFormat::OnRestore discarding Data::PropertySet");

			Data::Deserialize<UInt32>(stream);

			auto object = Make<PropertySet>();

			OnDeserialize(stream, object, options);
		}

		auto n = Data::Deserialize<UInt16>(stream);

		while (n--)
		{
			auto [key, size] = Data::Deserialize<Pair<UInt32>>(stream);

			SetBinary(data, key, ReadBytes(stream, size));
		}

		return kDeserializeErrorNone;
	}
	else if (header == kLegacyV2)
	{
		struct LegacyHeader
		{
			UInt32 magica, magicb;
			UInt16 version;
			UInt16 flags;
			UInt32 size;
		};

		stream = Nudge(stream, -4);

		auto & legacyheader = *Reinterpret<LegacyHeader>(ReadBytes(stream, 16).data);

		if (ImportLegacy(legacyheader.version, BitCheck(legacyheader.flags, 1), data, stream))
		{
			return kDeserializeErrorNone;
		}
		else
		{
			return kDeserializeErrorInvalidStream;
		}
	}
	else if (header == Reinterpret<UInt32>(kLegacyV1))
	{
		//auto decompressed = Reflex::Data::Decompress(Data::Archive::View(stream.data + 13, stream.b - 13));

		//auto view stream(decompressed);

		//return ImportLegacy<1>(true, data, stream);

		File::output.Error("BinaryFormat kLegacyV1 ignored");

		return kDeserializeErrorUnsupportedVersion;
	}

	return kDeserializeErrorInvalidHeader;
}

void Reflex::Data::BinaryFormat::OnSerialize(Archive & stream, const PropertySet & data) const
{
	constexpr char kHeader[2] = { 'R', 'B' };

	auto range = data.Iterate<ArchiveObject>();

	Data::Serialize(stream, *Reinterpret<UInt16>(kHeader), UInt16(range.GetSize()));

	for (auto & i : range)
	{
		Data::Serialize(stream, i.key.id, i.value->value.GetSize());

		stream.Append(i.value->value);
	}

	if constexpr (REFLEX_DEBUG)
	{
		if (auto range = data.Iterate<PropertySet>())
		{
			File::output.Error("Data::BinaryFormat::OnStore ignoring Data::PropertySet");
		}
	}
}

bool Reflex::Data::BinaryFormat::ImportLegacy(UInt version, bool _64bit, PropertySet & registry, Archive::View & stream) const
{
	if (version == 1)
	{

	}
	else if (version == 0)
	{
		if constexpr (REFLEX_DEBUG)
		{
			REFLEX_ASSERT(Data::Deserialize<UInt16>(stream) == 0);
		}
		else
		{
			stream = Nudge(stream, sizeof(UInt16));
		}
	}
	else
	{
		return false;
	}

	UInt skip = _64bit ? 4 : 0;

	REFLEX_LOOP(idx, Data::Deserialize<UInt16>(stream))
	{
		Pair <UInt32> keys;

		Data::Deserialize(stream, keys.a);

		stream = Nudge(stream, skip);

		Data::Deserialize(stream, *Detail::AcquireProperty<ArchiveObject>(registry, keys.a));
	}

	return true;
}

Reflex::TRef <Reflex::Data::Detail::PropertySetFormat> Reflex::Data::Detail::PropertySetFormat::Create(UInt32 magic)
{
	return REFLEX_CREATE(PropertySetFormatImpl, magic);
}

bool Reflex::Data::PropertySetFormatImpl::CompareDynamic(const PropertySetFormat & format, const PropertySet & a, const PropertySet & b)
{
	auto matches = format.Compare(a, b);

	auto asize = a.Iterate().GetSize();

	auto bsize = b.Iterate().GetSize();

	return matches.GetSize() == asize && asize == bsize;
}

bool Reflex::Data::PropertySetFormatImpl::CompareArraysOfDynamics(const PropertySetFormat & format, const PropertySetArray & a, const PropertySetArray & b)
{
	if (a.value.GetSize() == b.value.GetSize())
	{
		auto pb = b.value.GetData();

		REFLEX_LOOP_PTR(a.value.GetData(), pa, a.value.GetSize())
		{
			if (!CompareDynamic(format, *pa, *pb++)) return false;
		}

		return true;
	}

	return false;
}

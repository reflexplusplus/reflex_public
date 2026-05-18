#include "../../../include/reflex_ext/file/embedded_resource.h"




//
//implementation

REFLEX_BEGIN_INTERNAL(Reflex::File)

template <bool COMPRESSED> Data::Archive UnpackResourceImpl(const EmbeddedResource & item)
{
	if constexpr (COMPRESSED)
	{
		return Decompress(Data::kLZ4, item.data);
	}
	else
	{
		return item.data;
	}
}

constexpr FunctionPointer <Data::Archive(const EmbeddedResource&)> kUnpackFns[2] =
{
	&UnpackResourceImpl<false>,
	&UnpackResourceImpl<true>,
};

struct ResourceDecoder : public File::Detail::MemoryReader
{
	template <bool COMPRESSED> static TRef <System::FileHandle> Open(const EnumerableEmbeddedResource & item);

	static const FunctionPointer <TRef<System::FileHandle>(const EnumerableEmbeddedResource&)> st_openfns[4];

	ResourceDecoder(const EnumerableEmbeddedResource & item)
		: m_data(Extract(item))
	{
		m_start = m_data.GetData();

		m_end = m_start + m_data.GetSize();

		m_ptr = m_start;
	}

	Data::Archive m_data;
};

template <bool COMPRESSED> TRef <System::FileHandle> ResourceDecoder::Open(const EnumerableEmbeddedResource & item)
{
	if constexpr (COMPRESSED)
	{
		return REFLEX_CREATE(ResourceDecoder, item);
	}
	else
	{
		return File::Detail::CreateMemoryReader(item.data);
	}
}

constexpr FunctionPointer <TRef<System::FileHandle>(const EnumerableEmbeddedResource&)> ResourceDecoder::st_openfns[4] =
{
	&ResourceDecoder::Open<false>,
	&ResourceDecoder::Open<true>,
};

REFLEX_END_INTERNAL

Reflex::File::EnumerableEmbeddedResource::EnumerableEmbeddedResource(Key32 group, Key32 id, const UInt8 * data, UInt32 size, UInt32 uncompressed_size)
	: EmbeddedResource({ { data, size }, uncompressed_size }),
	group(group),
	id(id)
{
	REFLEX_ASSERT(Retrieve(MakeTuple(group, id)) == this);
}

const Reflex::File::EnumerableEmbeddedResource * Reflex::File::EnumerableEmbeddedResource::Retrieve(Pair <Key32> id)
{
	if (IsValidKey(id.b))
	{
		auto id64 = Reinterpret<UInt64>(id);

		for (auto & i : range)
		{
			if (*Reinterpret<UInt64>(&i.group) == id64)
			{
				return &i;
			}
		}
	}

	return 0;
}

Reflex::Data::Archive Reflex::File::Extract(const EmbeddedResource & item)
{
	return (*kUnpackFns[True(item.uncompressed_size)])(item);
}

Reflex::File::EnumerableEmbeddedResource::Locator::Locator()
	: File::VirtualFileSystem::Locator(K32("res"), { 1, 1 }) 
{
}

Reflex::TRef <Reflex::System::FileHandle> Reflex::File::EnumerableEmbeddedResource::Locator::OnRead(const ArrayView <WString::View> & subdomain, const WString::View & path, File::Attributes & attributes) const
{
	REFLEX_ASSERT(subdomain);

	if (auto item = EnumerableEmbeddedResource::Retrieve({ subdomain.GetFirst(), path }))
	{
		auto rtn = (*ResourceDecoder::st_openfns[True(item->uncompressed_size)])(*item);

		attributes.size_time = { item->data.size, 0 };	//TODO correct size

		return rtn;
	}

	auto ptr = subdomain.GetFirst().data;

	WString::View fullpath = { ptr, UInt32((path.data + path.size) - ptr) };

	File::output.Warn("missing resource", fullpath);

	return {};
}


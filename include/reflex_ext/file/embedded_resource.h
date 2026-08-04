#pragma once

#include "[require].h"




//
//Secondary API

#define REFLEX_FORCE_INSTANTIATE(ITEM) namespace REFLEX_CONCATENATE(InstantiateResourceAt,__LINE__){ const auto volatile * t = &ITEM; }

namespace Reflex::File
{

	struct EmbeddedResource;

	class EnumerableEmbeddedResource;


	Data::Archive Extract(const EmbeddedResource & item);

	template <class TYPE> inline TYPE UnpackResource(Key32 group, Key32 id, const TYPE & fallback);

}




//
//EmbeddedResource

struct Reflex::File::EmbeddedResource
{
	Data::Archive::View data;

	UInt32 uncompressed_size;
};




//
//EnumerableEmbeddedResource

class Reflex::File::EnumerableEmbeddedResource :
	public EmbeddedResource,
	public Reflex::Detail::StaticItem <EnumerableEmbeddedResource>
{
public:

	//types
	
	class Locator;



	//access

	static const EnumerableEmbeddedResource * Retrieve(Pair <Key32> id);



	//lifetime

	EnumerableEmbeddedResource(Key32 group, Key32 id, const UInt8 * data, UInt32 size, UInt32 compression = 0);

	Key32 group, id;

};



//
//ResourceLocator

class Reflex::File::EnumerableEmbeddedResource::Locator : public File::VirtualFileSystem::Locator
{
public:

	REFLEX_OBJECT(EnumerableEmbeddedResource::Locator, File::VirtualFileSystem::Locator);

	Locator();

	virtual TRef <System::FileHandle> OnRead(const ArrayView <WString::View> & subdomain, const WString::View & path, File::Attributes & attributes) const override;
};




//
//impl

template <class TYPE> inline TYPE Reflex::File::UnpackResource(Key32 group, Key32 id, const TYPE & fallback)
{
	if (auto res = EnumerableEmbeddedResource::Retrieve({ group, id }))
	{
		if constexpr (kIsType<TYPE,bool>)
		{
			return True(Data::Unpack<UInt8>(res->data));
		}
		else
		{
			return Data::Unpack<TYPE>(res->data);
		}
	}

	return fallback;
};

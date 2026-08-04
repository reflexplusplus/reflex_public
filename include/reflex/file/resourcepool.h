#pragma once

#include "virtual_filesystem.h"




//
//Secondary API

namespace Reflex::File
{

	class ResourcePool;

}




//
//ResourcePool

class Reflex::File::ResourcePool :
	public Object,
	public State
{
public:

	REFLEX_OBJECT(File::ResourcePool, Object);

	static ResourcePool & null;



	//types

	class Lock;

	struct StreamContext;



	//declarations

	struct Token;

	using Ctr = FunctionPointer <TRef<Object>(const StreamContext & ctx, System::FileHandle & stream)>;



	//lifetime

	[[nodiscard]] static TRef <ResourcePool> Create(VirtualFileSystem & filesystem);



	//info (lock free)

	virtual UInt GetTotalNumItem() const = 0;

	virtual const UInt & GetNumItem(Attributes::Status status) const = 0;



	//links

	const TRef <VirtualFileSystem> filesystem;



protected:

	ResourcePool(VirtualFileSystem & filesystem);

};




//
//ResourcePool::Lock

class Reflex::File::ResourcePool::Lock
{
public:

	//lifetime

	Lock(ResourcePool & resourcepool);



	//write access

	void Clear(TypeID type_id);

	template <class TYPE> TRef <TYPE> Retrieve(const WString::View & path, const Data::PropertySet & options = Data::PropertySet::null, Ctr ctr = &TYPE::Open);

	const Token & RetrieveToken(TypeID type_id, const WString::View & path, const Data::PropertySet & options, Ctr ctr);



	//read access

	const Token * Query(Address adr) const;

	void Enumerate(const Function <void(const Token &)> & callback) const;

	void Enumerate(TypeID type_id, const Function <void(const Token&)> & callback) const;



	//advanced access

	Token & Insert(const WString::View & path, const Attributes & attributes, TypeID type_id, TRef <Object> object);

	bool Remove(Address address);



	//trash

	bool PurgeIncremental();		//incremental flush, do on each clock

	bool Flush();	//full flush



	VirtualFileSystem::Lock lock;

	const TRef <ResourcePool> resourcepool;
};




//
//ResourcePool::StreamContext

struct Reflex::File::ResourcePool::StreamContext
{
	Lock & lock;

	const Data::PropertySet & options;

	WString::View path;

	Attributes::Status status;
};




//
//ResourcePool::Token

struct Reflex::File::ResourcePool::Token
{
	Address address;

	WString path;

	Attributes attributes;

	Reference <Object> object;
};




//
//impl

inline Reflex::File::ResourcePool::Lock::Lock(ResourcePool & resourcepool)
	: lock(resourcepool.filesystem)
	, resourcepool(resourcepool)
{
}

template <class TYPE> REFLEX_INLINE Reflex::TRef <TYPE> Reflex::File::ResourcePool::Lock::Retrieve(const WString::View & path, const Data::PropertySet & options, Ctr open)
{
	return Cast<TYPE>(RetrieveToken(GetTypeID<TYPE>(), path, options, open).object);
}

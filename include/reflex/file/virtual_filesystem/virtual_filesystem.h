#pragma once

#include "attributes.h"




//
//Secondary API

namespace Reflex::File
{

	class VirtualFileSystem;

	constexpr Key32 kdisk = "disk";

}




//
//VirtualFileSystem

class Reflex::File::VirtualFileSystem : public Object
{
public:

	REFLEX_OBJECT(File::VirtualFileSystem, Object);

	static VirtualFileSystem & null;



	//types

	class Lock;

	class Locator;



	//lifetime

	[[nodiscard]] static TRef <VirtualFileSystem> Create(Key32 default_domain = kdisk, bool mt = true);

};

REFLEX_SET_TRAIT(Reflex::File::VirtualFileSystem, IsAbstract);




//
//VirtualFileSystem::Lock

class Reflex::File::VirtualFileSystem::Lock : public System::CriticalSection::Lock
{
public:

	//lifetime

	Lock(VirtualFileSystem & filesystem);

	Lock(const Lock & lock) = delete;



	//locators

	void Clear();

	void Attach(Locator & locator);

	void Detach(Locator & locator);


	void Enumerate(const Function <void(Locator&)> & visitor);



	//io

	[[nodiscard]] TRef <System::FileHandle> Read(const WString::View & path, Attributes & attributes);

	[[nodiscard]] TRef <System::FileHandle> Write(const WString::View & path, bool append);

	bool Delete(const WString::View & path);



	//links

	TRef <VirtualFileSystem> filesystem;



private:

	using WriteFn = FunctionPointer<UIntNative(const VirtualFileSystem::Locator&, const ArrayView <WString::View>&, const Reflex::WString::View&, bool)>;

	UIntNative PerformWrite(bool write, const WString::View & path, bool write_append, UIntNative fallback, WriteFn fn);

};




//
//VirtualFileSystem::Locator

class Reflex::File::VirtualFileSystem::Locator : public Item <Locator>
{
public:

	REFLEX_OBJECT(File::VirtualFileSystem::Locator, Object);

	static Locator & null;



	//lifetime

	Locator(Key32 domain_id, Pair <UInt> minmax_subdomains = { 0, kMaxUInt8 });

	~Locator();



	//location

	TRef <VirtualFileSystem> GetFileSystem();



	//info

	const Key32 domain_id;



protected:

	//access

	virtual TRef <System::FileHandle> OnRead(const ArrayView <WString::View> & subdomain, const WString::View & path, Attributes & attributes) const { return {}; }

	virtual TRef <System::FileHandle> OnWrite(const ArrayView <WString::View> & subdomain, const WString::View & path, bool append) const { return {}; }

	virtual bool OnDelete(const ArrayView <WString::View> & subdomain, const WString::View & path) const { return false; }



private:

	friend class VirtualFileSystem::Lock;

	using Item::Attach;

	using Item::Detach;


	const Pair <UInt> m_minmax_subdomains;

	TRef <VirtualFileSystem> m_filesystem;

};




//
//impl

REFLEX_NS(Reflex::File::Detail)

Tuple <Key32, ArrayView <WString::View>, WString::View> SplitDomain(const WString::View & path, Array <WString::View> & buffer);

REFLEX_END

REFLEX_INLINE Reflex::TRef <Reflex::File::VirtualFileSystem> Reflex::File::VirtualFileSystem::Locator::GetFileSystem()
{
	return *m_filesystem;
}

inline Reflex::TRef <Reflex::System::FileHandle> Reflex::File::VirtualFileSystem::Lock::Write(const WString::View & path, bool append)
{
	auto rtn = PerformWrite(true, path, append, ToUIntNative(Null<System::FileHandle>().Adr()), [](const VirtualFileSystem::Locator & i, const ArrayView <WString::View> & subdomain, const WString::View & path, bool append)
	{
		if (auto file = i.OnWrite(subdomain, path, append)) return ToUIntNative(file.Adr());

		return UIntNative(0);
	});

	return ToPointer<System::FileHandle>(rtn);
}

inline bool Reflex::File::VirtualFileSystem::Lock::Delete(const WString::View & path)
{
	return True(PerformWrite(false, path, false, UIntNative(0), [](const VirtualFileSystem::Locator & i, const ArrayView <WString::View> & subdomain, const WString::View & path, bool append)
	{
		if (i.OnDelete(subdomain, path)) return UIntNative(true);

		return UIntNative(false);
	}));
}

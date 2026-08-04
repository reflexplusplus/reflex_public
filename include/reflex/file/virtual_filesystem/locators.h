#pragma once

#include "virtual_filesystem.h"




//
//Secondary API

namespace Reflex::File
{

	class FileLocator;

	class SearchPath;

}




//
//FileLocator

class Reflex::File::FileLocator : public VirtualFileSystem::Locator
{
public:

	REFLEX_OBJECT(File::FileLocator, Locator);

	FileLocator();

	TRef <System::FileHandle> OnRead(const ArrayView <WString::View> & subdomain, const WString::View & path, Attributes & attributes) const override;

	TRef <System::FileHandle> OnWrite(const ArrayView <WString::View> & subdomain, const WString::View & path, bool append) const override;

	bool OnDelete(const ArrayView <WString::View> & subdomain, const WString::View & path) const override;
};




//
//SearchPath (TODO -> NSA)

class Reflex::File::SearchPath : public VirtualFileSystem::Locator
{
public:

	REFLEX_OBJECT(File::SearchPath, Locator);

	using VirtualFileSystem::Locator::Locator;

	[[nodiscard]] static TRef <SearchPath> Create(const WString::View & path);
};

REFLEX_SET_TRAIT(File::SearchPath, IsAbstract);




//
//impl

REFLEX_NS(Reflex::File::Detail)

[[nodiscard]] REFLEX_INLINE TRef <System::FileHandle> Open(const WString & path, Key32 domain_id, Attributes & attributes)
{
	if (System::GetFileAttributes(path, attributes.size_time))
	{
		return System::FileHandle::Create(path);
	}

	return {};
}

REFLEX_END

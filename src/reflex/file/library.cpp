#include "../../../include/reflex/file/module.h"
#include "../../../include/reflex/file/defines.h"

#include "filesystem.h"
#include "resourcepool.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::File)

struct Library
{
	template <class TYPE> using NullImpl = Reflex::Detail::StaticObject <TYPE>;

	struct NullLocator : public VirtualFileSystem::Locator
	{
		using VirtualFileSystem::Locator::Locator;
	};


	Library();

	NullImpl <FileSystemImpl> null_filesystem;

	NullImpl <NullLocator> null_locator;

	NullImpl <ResourcePoolImpl> null_resourcepool;
};

Library::Library()
	: null_filesystem(kdisk, false),
	null_locator(K32("null"), MakeTuple(kMaxUInt32, UInt(0))),
	null_resourcepool(null_filesystem)
{
}

Reflex::Detail::Module g_module = { "Reflex::File", Data::module };

Reflex::Detail::Module::Member <Library> g_library(g_module);

REFLEX_END_INTERNAL

const Reflex::Detail::Module & Reflex::File::module = g_module;

Reflex::File::VirtualFileSystem & Reflex::File::VirtualFileSystem::null = Reflex::File::g_library->null_filesystem;
Reflex::File::VirtualFileSystem::Locator & Reflex::File::VirtualFileSystem::Locator::null = Reflex::File::g_library->null_locator;
Reflex::File::ResourcePool & Reflex::File::ResourcePool::null = Reflex::File::g_library->null_resourcepool;

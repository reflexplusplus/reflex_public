#include "../../../../include/reflex/file/virtual_filesystem/locators.h"




//
//implementation

Reflex::File::FileLocator::FileLocator()
	: Locator(kdisk, { 0, 0 })
{
}

Reflex::TRef <Reflex::System::FileHandle> Reflex::File::FileLocator::OnRead(const ArrayView <WString::View> & subdomain, const WString::View & path, Attributes & attributes) const
{
	return Detail::Open(path, kdisk, attributes);
}

Reflex::TRef <Reflex::System::FileHandle> Reflex::File::FileLocator::OnWrite(const ArrayView <WString::View> & subdomain, const WString::View & path, bool append) const
{
	return System::FileHandle::Create(path, append ? System::FileHandle::kModeAppend : System::FileHandle::kModeOverwrite);
}

bool Reflex::File::FileLocator::OnDelete(const ArrayView <WString::View> & subdomain, const WString::View & path) const
{
	return System::Delete(path);
}

#pragma once

#include "../virtual_filesystem.h"




//
//Primary API

namespace Reflex::File
{

	Data::Archive Open(VirtualFileSystem::Lock & lock, const WString::View & filename);

	bool Save(VirtualFileSystem::Lock & lock, const WString::View & filename, const Data::Archive::View & data);

	bool Copy(VirtualFileSystem::Lock & lock, const WString & from, const WString & to);


	inline bool IsMissing(Attributes::Status status) { return status == Attributes::kStatusMissing; }

	inline bool IsStreaming(Attributes::Status status) { return status == Attributes::kStatusStreaming; }

}

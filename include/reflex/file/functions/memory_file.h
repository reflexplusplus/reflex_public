#pragma once

#include "../detail/memory_stream.h"




//
//Primary API

namespace Reflex::File
{

	[[nodiscard]] TRef <System::FileHandle> CreateMemoryReader(ConstTRef <Data::ArchiveObject> data);

	[[nodiscard]] TRef <System::FileHandle> CreateMemoryWriter(TRef <Data::ArchiveObject> data);

}




//
//Detail

REFLEX_NS(Reflex::File::Detail)

[[nodiscard]] TRef <System::FileHandle> CreateMemoryReader(const Data::Archive::View & data);	//!does not retain/copy data

REFLEX_END

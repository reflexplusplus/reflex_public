#pragma once

#include "../detail/memory_stream.h"




//
//Primary API

namespace Reflex::File
{

	[[nodiscard]] Unretained <System::FileHandle> CreateMemoryReader(ConstWillRetain <Data::ArchiveObject> data);

	[[nodiscard]] Unretained <System::FileHandle> CreateMemoryWriter(WillRetain <Data::ArchiveObject> data);

}




//
//Detail

REFLEX_NS(Reflex::File::Detail)

[[nodiscard]] Unretained <System::FileHandle> CreateMemoryReader(const Data::Archive::View & data);	//!does not retain/copy data

REFLEX_END

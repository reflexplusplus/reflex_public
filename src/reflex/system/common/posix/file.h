#pragma once

#include "[require].h"




REFLEX_NS(Reflex::System::Common::POSIX)

int QueryWriteableFileDescriptor(System::FileHandle & handle);

REFLEX_INLINE bool Exists(const Common::UTF8 & path)
{
	return access(path.GetData(), F_OK) == 0;
}

REFLEX_END

#pragma once

#include "../../../include/reflex/file/virtual_filesystem.h"




//
//implementation

REFLEX_BEGIN_INTERNAL(Reflex::File)

struct FileSystemImpl : public VirtualFileSystem
{
	FileSystemImpl(Key32 defaultdomain, bool mt);

	~FileSystemImpl();


	UInt32 m_defaultdomain;

	Reflex::Detail::ThreadValidator <REFLEX_DEBUG> m_threadvalidator;

	Reference <System::CriticalSection> m_cs;

	Locator::List m_list;

	UInt32 m_lockcount;

	Array <WString::View> m_buffer;
};

REFLEX_END_INTERNAL

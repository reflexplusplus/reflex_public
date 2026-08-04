#include "sdk.h"




//
//DirectoryImpl

REFLEX_BEGIN_INTERNAL(Reflex::System::OSX)

struct DirectoryImpl : public Directory
{
	DirectoryImpl(const WString & directory, bool hidden);

	virtual bool GetNext(bool & directory, WString & filename) override;


	ObjCRef <NSArray*> m_nskeys;

	ObjCRef <NSArray<NSURL*>*> m_nsarray;

	UInt32 m_size;

	UInt32 m_position;
};

DirectoryImpl::DirectoryImpl(const WString & path, bool hidden)
	: m_nskeys(MakeObjCRef([NSArray arrayWithObject:NSURLIsDirectoryKey]))
	, m_position(0)
{
	auto manager = [NSFileManager defaultManager];

	auto nspath = MakeNSStringRef(path);

	UInt flags = hidden ? 0 : NSDirectoryEnumerationSkipsHiddenFiles;

	@try
	{
		m_nsarray = MakeObjCRef([manager contentsOfDirectoryAtURL:[NSURL fileURLWithPath:nspath]
						   includingPropertiesForKeys:[NSArray arrayWithObject:NSURLIsDirectoryKey]
											  options:flags
												error:nil]);
	}
	@catch (...)
	{
	}

	m_size = UInt([m_nsarray count]);
}

bool DirectoryImpl::GetNext(bool & directory, WString & m_return)
{
	if (m_position < m_size)
	{
		NSURL * nsurl = [m_nsarray objectAtIndex:m_position++];

		NSString * nsfilename = [nsurl lastPathComponent];

		NSDictionary * keys = [nsurl resourceValuesForKeys:m_nskeys error:nil];

		NSNumber * nsnumber = [keys valueForKey:NSURLIsDirectoryKey];

		directory = [nsnumber boolValue];

		m_return = NSStringToWString(nsfilename);

		return true;
	}
	else
	{
		return false;
	}
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::Directory> Reflex::System::Directory::Create(const WString & path, bool hidden)
{
	return REFLEX_CREATE(OSX::DirectoryImpl, path, hidden);
}

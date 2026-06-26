#include "sdk.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::System::OSX)

struct DiskIteratorImpl : public DiskIterator
{
	DiskIteratorImpl();

	bool GetNext(bool & removable, WString &, WString &) override;


	ObjCRef <NSArray*> m_nskeys;

	ObjCRef <NSArray<NSURL*>*> m_nsarray;

	UInt32 m_size;

	UInt32 m_position;

	Pair <WString> m_values;
};

DiskIteratorImpl::DiskIteratorImpl()
	: m_nskeys(MakeOwnedObjCRef([[NSArray alloc] initWithObjects:NSURLNameKey,NSURLLocalizedNameKey,nil]))
	, m_nsarray(MakeObjCRef([[NSFileManager defaultManager] mountedVolumeURLsIncludingResourceValuesForKeys:m_nskeys options:NSVolumeEnumerationSkipHiddenVolumes | NSVolumeEnumerationProduceFileReferenceURLs]))
	, m_size(UInt32([m_nsarray count]))
	, m_position(0)
{
}

bool DiskIteratorImpl::GetNext(bool & removable, WString & volume, WString & display)
{
	if (m_position < m_size)
	{
		NSURL * nsurl = [m_nsarray objectAtIndex:m_position++];

		NSDictionary * nsdictionary = [nsurl resourceValuesForKeys:m_nskeys error:0];

		removable = false;

		volume = ToWString([nsurl path]);

		volume.Push(L'/');

		display = ToWString([nsdictionary objectForKey:NSURLNameKey]);

		return true;
	}
	else
	{
		return false;
	}
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::DiskIterator> Reflex::System::DiskIterator::Create()
{
	return REFLEX_CREATE(OSX::DiskIteratorImpl);
}

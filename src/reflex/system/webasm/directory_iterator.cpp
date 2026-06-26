#include "sdk.h"




//
//impl (STUB)

REFLEX_BEGIN_INTERNAL(Reflex::System::WebASM)

struct DiskIteratorImpl : public DiskIterator
{
	virtual bool GetNext(bool & removable, WString & filename, WString & displayname) override
	{
		if (SetFiltered(m_called, true))
		{
			removable = false;

			filename.Clear();

			displayname = L"Local Storage";

			return true;
		}

		return false;
	}

	bool m_called = false;
};

struct DirectoryIteratorImpl : public DirectoryIterator
{
	DirectoryIteratorImpl(const WString & directory, bool hidden);

	virtual ~DirectoryIteratorImpl();

	virtual bool GetNext(Item & item) override;
};

DirectoryIteratorImpl::DirectoryIteratorImpl(const WString & path, bool hidden)
{
}

DirectoryIteratorImpl::~DirectoryIteratorImpl()
{
}

bool DirectoryIteratorImpl::GetNext(Item & item)
{
	return false;
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::DiskIterator> Reflex::System::DiskIterator::Create()
{
	return REFLEX_CREATE(WebASM::DiskIteratorImpl);
}

Reflex::TRef <Reflex::System::DirectoryIterator> Reflex::System::DirectoryIterator::Create(const WString & path, bool hidden)
{
	return REFLEX_CREATE(WebASM::DirectoryIteratorImpl, path, hidden);
}

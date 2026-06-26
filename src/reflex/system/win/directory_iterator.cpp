#include "[require].h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

struct DiskIteratorImpl : public DiskIterator
{
	DiskIteratorImpl();

	bool GetNext(bool & removable, WString & name, WString & display);

	UInt32 m_bits;
};

struct DirectoryIteratorImpl : public DirectoryIterator
{
	DirectoryIteratorImpl(const WString & directory, bool hidden);

	virtual ~DirectoryIteratorImpl();

	virtual bool GetNext(Item & item);

	HANDLE m_hfind;

	WIN32_FIND_DATA m_finddata;

	DWORD m_ignore;

	
	static constexpr UInt32 kDot = 46ul;

	static constexpr UInt64 kDots = 3014702ul;
};

DiskIteratorImpl::DiskIteratorImpl()
	: m_bits(UInt32(GetLogicalDrives()))
{
}

bool DiskIteratorImpl::GetNext(bool & removable, WString & path, WString & display)
{
	removable = false;

	if (auto idx = GetFirstBit(m_bits))
	{
		m_bits = BitClear(m_bits, idx.value);

		auto letter = WChar(65 + idx.value);

		display = { letter, ':', 92 };

		path = { letter, ':', kPathDelimiter };

		return true;
	}
	else
	{
		return false;
	}
}

DirectoryIteratorImpl::DirectoryIteratorImpl(const WString & path, bool hidden)
	: m_hfind(INVALID_HANDLE_VALUE)
	, m_ignore(FILE_ATTRIBUTE_SYSTEM | (hidden ? FILE_ATTRIBUTE_HIDDEN : 0))
{
	WString pathstr = Join(path, L'*');

	auto w32filename = m_finddata.cFileName;

	REFLEX_LOOP(idx, 4) w32filename[idx] = 0;

	m_hfind = FindFirstFile(pathstr.GetData(), &m_finddata);
}

DirectoryIteratorImpl::~DirectoryIteratorImpl()
{
	FindClose(m_hfind);
}

bool DirectoryIteratorImpl::GetNext(Item & item)
{
	while (m_finddata.cFileName[0])
	{
		auto w32filename = m_finddata.cFileName;

		auto w32attributes = m_finddata.dwFileAttributes;

		bool ignore = (w32attributes & m_ignore) || (*Reinterpret<UInt64>(w32filename) == kDots) || (*Reinterpret<UInt32>(w32filename) == kDot);


		item.is_directory = (w32attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		item.filename = WString::View(w32filename);


		w32filename[0] = 0;

		w32filename[3] = 0;	//this is for catching double dot case to make sure it compares to dots constant

		auto & first = w32filename[0];

		first = FindNextFile(m_hfind, &m_finddata) ? first : 0;


		if (!ignore) return true;
	}

	return false;
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::DiskIterator> Reflex::System::DiskIterator::Create()
{
	return REFLEX_CREATE(Win::DiskIteratorImpl);
}

Reflex::TRef <Reflex::System::DirectoryIterator> Reflex::System::DirectoryIterator::Create(const WString & path, bool hidden)
{
	return REFLEX_CREATE(Win::DirectoryIteratorImpl, path, hidden);
}

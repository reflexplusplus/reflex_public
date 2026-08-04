#include "[require].h"




REFLEX_BEGIN_INTERNAL(Reflex::System::Common::POSIX)

struct DirectoryIteratorImpl : public DirectoryIterator
{
	DirectoryIteratorImpl(DIR * directory, bool hidden);

	~DirectoryIteratorImpl();

	bool GetNext(Item & item) override;


	DIR * m_dir;

	bool m_ignore_hidden;
};

DirectoryIteratorImpl::DirectoryIteratorImpl(DIR * directory, bool hidden)
	: m_dir(directory)
	, m_ignore_hidden(!hidden)
{
}

DirectoryIteratorImpl::~DirectoryIteratorImpl()
{
	closedir(m_dir);
}

bool DirectoryIteratorImpl::GetNext(Item & item)
{
	struct dirent * entry;

	while ((entry = readdir(m_dir)) != nullptr)
	{
		auto pname = entry->d_name;

		auto length = RawStringLength(pname);

		if (pname[0] == '.')
		{
			switch (length)
			{
			case 1:
				continue;	//ignore "."

			case 2:
				if (pname[1] == '.')
				{
					continue;	//ignore ".."
				}
				else
				{
					//fall thru to next case
				}

			default:	//on unix ".name" means hidden
				if (m_ignore_hidden) continue;
				break;
			}
		}

		item.is_directory = (entry->d_type == DT_DIR);

		item.filename = Common::DecodeUTF8({Reinterpret<UInt8>(pname), length});

		return true;
	}

	return false;
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::DirectoryIterator> Reflex::System::DirectoryIterator::Create(const WString & path, bool hidden)
{
	if (DIR * dir = opendir(Common::ToUTF8(path).GetData()))
	{
		return REFLEX_CREATE(Common::POSIX::DirectoryIteratorImpl, dir, hidden);
	}
	else
	{
		return {};
	}
}

#include "../../../include/reflex/file/functions/path.h"




//
//

Reflex::Array < Reflex::Pair <Reflex::WString> > Reflex::File::GetVolumes()
{
	auto volumes = AutoRelease(System::DiskIterator::Create());

	Array < Pair <WString> > rtn;

	rtn.Allocate(4);

	bool removable;

	Pair <WString> pair;

	while (volumes->GetNext(removable, pair.a, pair.b))
	{
		rtn.Push(std::move(pair));
	}

	return rtn;
}

Reflex::Pair < Reflex::Map <Reflex::WString, Reflex::NullType, Reflex::CaseInsensitive> > Reflex::File::List(const WString & path, bool hidden)
{
	Pair < Map <WString, NullType, CaseInsensitive> > rtn;

	auto directory_itr = Make<System::DirectoryIterator>(path, hidden);

	System::DirectoryIterator::Item item;

	while (directory_itr->GetNext(item))
	{
		if (item.is_directory)
		{
			rtn.a.Set(Join(item.filename, kPathDelimiter));
		}
		else
		{
			rtn.b.Set(item.filename);
		}
	}

	return rtn;
}

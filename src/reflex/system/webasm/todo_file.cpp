#include "sdk.h"




//
//Reflex impl

REFLEX_BEGIN_INTERNAL(Reflex::System::WebASM)

struct FileReaderImpl : public Reflex::File::MemoryReader
{
	using Reflex::File::MemoryReader::MemoryReader;

	~FileReaderImpl()
	{
		free(RemoveConst(m_start));
	}
};

struct FileWriterImpl : public Reflex::File::MemoryWriter
{
	FileWriterImpl(Common::UTF8 && path)
		: Reflex::File::MemoryWriter(m_buffer)
		, m_path(std::move(path))
	{
	}

	~FileWriterImpl()
	{
		Flush(true);
	}

	virtual bool Flush(bool commit) override
	{
		(void)commit;

		jsWriteLocalStorage(m_path.GetData(), m_buffer.GetData(), m_buffer.GetSize());

		return true;
	}

	Common::UTF8 m_path;

	Data::Archive m_buffer;
};

REFLEX_INLINE ArrayView <UInt8> ReadLocalStorage(const Common::UTF8 & path)
{
	ArrayView <UInt8> view;

	jsReadLocalStorage(path.GetData(), view.data, view.size);

	return view;
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::FileHandle> Reflex::System::FileHandle::Create(const WString & path, Mode mode, bool lock)
{
	auto utf8 = Common::ToUTF8(path);

	if (mode == kModeRead)
	{
		if (lock)
		{
			//TODO check lock pool to if shared for read
		}

		auto view = WebASM::ReadLocalStorage(utf8);

		if (view.data)	//!important check data not size, size can be 0 but still malloc'd for zero-size file
		{
			return REFLEX_CREATE(WebASM::FileReaderImpl, view);
		}
	}
	else
	{
		if (lock)
		{
			//TODO check is not in lock pool
		}

		auto self = REFLEX_CREATE(WebASM::FileWriterImpl, std::move(utf8));

		if (mode == kModeAppend)
		{
			auto view = WebASM::ReadLocalStorage(self->m_path);

			if (view.data)	//!important check data not size
			{
				self->m_buffer = view;

				self->m_position = view.size;

				free(RemoveConst(view.data));
			}
		}

		return self;
	}

	return REFLEX_NULL(File);//WebASM::File::Impl::Create(path, mode, lock);
}

bool Reflex::System::MakeDirectory(const WString & path)
{
	DEV_ERROR("System::MakeDirectory not implemented");

	return false;
}

bool Reflex::System::IsDirectory(const WString & path)
{
	DEV_ERROR("System::IsDirectory not implemented");

	return false;
}

bool Reflex::System::Rename(const WString & from, const WString & to)
{
    DEV_ERROR("System::Rename not implemented");

    return false;
}

bool Reflex::System::Delete(const WString & path)
{
	auto utf8 = Common::ToUTF8(path);

	jsDeleteLocalStorage(utf8.GetData());

	return true;
}

bool Reflex::System::Exists(const WString & path)
{
	auto utf8 = Common::ToUTF8(path);

	return True(jsQueryLocalStorage(utf8.GetData()));
}

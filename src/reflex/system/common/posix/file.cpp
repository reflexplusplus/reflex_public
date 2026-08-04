#include "file.h"




REFLEX_BEGIN_INTERNAL(Reflex::System::Common::POSIX)

struct FileImpl : public FileHandle
{
	FileImpl(FILE * file, bool lock);
	~FileImpl() override;

	bool IsWriteable() const override { return false; }
	UInt64 GetSize() const override;
	void SetPosition(UInt64 pos) override;
	UInt64 GetPosition() const override;
	UInt32 Read(void * ptr, UInt32 size) override;
	UInt32 Write(const void * ptr, UInt size) override { REFLEX_ASSERT(false); return 0; }
	bool Flush(bool commit) override { return true; }
	bool Truncate() override { return true; }

	FILE * m_file;
};

struct WriteableFileImpl : public FileImpl
{
	REFLEX_OBJECT(WriteableFileImpl, FileImpl);

	using FileImpl::FileImpl;

	~WriteableFileImpl() override;

	bool IsWriteable() const override { return true; }
	UInt32 Write(const void * ptr, UInt size) override;
	bool Flush(bool commit) override;
	bool Truncate() override;

	bool m_dirty = true;
};

struct StdInWorkaround : public FileImpl
{
	using FileImpl::FileImpl;

	UInt32 Read(void * ptr, UInt32 size) override
	{
		if (fgets(Cast<char>(ptr), size, stdin))
		{
			return RawStringLength(Cast<char>(ptr));
		}

		return 0;
	}
};

FileImpl::FileImpl(FILE *file, bool lock)
	: m_file(file)
{
}

FileImpl::~FileImpl()
{
	fclose(m_file);
}

UInt64 FileImpl::GetSize() const
{
	fflush(m_file);

	auto current = ftell(m_file);

	fseek(m_file, 0L, SEEK_END);

	auto size = ftell(m_file);

	fseek(m_file, current, SEEK_SET);

	return size;
}

void FileImpl::SetPosition(UInt64 pos)
{
	fseek(m_file, pos, SEEK_SET);
}

UInt64 FileImpl::GetPosition() const
{
	return ftell(m_file);
}

UInt32 FileImpl::Read(void * ptr, UInt32 size)
{
	return UInt32(fread(ptr, 1, size, m_file));
}

WriteableFileImpl::~WriteableFileImpl()
{
	Flush(true);
}

UInt32 WriteableFileImpl::Write(const void * ptr, UInt size)
{
	m_dirty = true;

	return UInt32(fwrite(ptr, 1, size, m_file));
}

bool WriteableFileImpl::Flush(bool commit)
{
	if (m_dirty)
	{
		if (fflush(m_file) != 0) return false;

		if (commit && fsync(fileno(m_file)) != 0) return false;

		m_dirty = false;

		return true;
	}
	else
	{
		return true;
	}
}

bool WriteableFileImpl::Truncate()
{
	m_dirty = true;

	if (fflush(m_file) != 0) return false;

	return ftruncate(fileno(m_file), ftello(m_file)) == 0;
}

REFLEX_INLINE bool IsDirectory(const Common::UTF8 & utf8)
{
	struct stat statbuf;

	if (stat(utf8.GetData(), &statbuf) == 0) 
	{
		return S_ISDIR(statbuf.st_mode);
	}
	
	return false;
}

REFLEX_END_INTERNAL

int Reflex::System::Common::POSIX::QueryWriteableFileDescriptor(System::FileHandle & handle)
{
	if (auto impl = DynamicCast<Common::POSIX::WriteableFileImpl>(handle))
	{
		return fileno(impl->m_file);
	}

	return -1;
}

Reflex::TRef <Reflex::System::FileHandle> Reflex::System::FileHandle::Create(const WString & path, Mode mode, bool lock)
{
	//https://stackoverflow.com/questions/21113919/difference-between-r-and-w-in-fopen
	//kModeAppend should create the file, which none of the existing modes do (w+ first empty/truncates, a+ does not allow reading from the existing part of the file)
	
	constexpr auto LockOrFail = [](bool lock, FILE * fp, decltype(LOCK_EX) mode)
	{
		if (lock && flock(fileno(fp), mode | LOCK_NB) != 0)
		{
			fclose(fp);
			
			throw(std::make_error_code(std::errc::resource_unavailable_try_again));
		}
	};
	
	auto utf8s = Common::ToUTF8(path);

	FILE * fp = nullptr;

	try
	{
		switch (mode) 
		{
		case kModeRead:
			if ((fp = fopen(utf8s.GetData(), "r")))
			{
				LockOrFail(lock, fp, LOCK_SH);

				rewind(fp);

				return REFLEX_CREATE(Common::POSIX::FileImpl, fp, lock);
			}
			break;

		case kModeOverwrite:
			if ((fp = fopen(utf8s.GetData(), "w+")))
			{
				LockOrFail(lock, fp, LOCK_EX);

				return REFLEX_CREATE(Common::POSIX::WriteableFileImpl, fp, lock);
			}
			break;

		case kModeAppend:
			if ((fp = fopen(utf8s.GetData(), "r+")))	//r+ only opens if the file exists
			{
				LockOrFail(lock, fp, LOCK_EX);

				fseek(fp, 0, SEEK_END);

				return REFLEX_CREATE(Common::POSIX::WriteableFileImpl, fp, lock);
			}
			else if ((fp = fopen(utf8s.GetData(), "w+")))	//maybe the file doesnt exist so try w+
			{
				LockOrFail(lock, fp, LOCK_EX);

				return REFLEX_CREATE(Common::POSIX::WriteableFileImpl, fp, lock);
			}
			break;
		}
	}
	catch (std::error_code e)
	{
		DEV_WARN("System::FileHandle::Create failed", path, UInt8(mode));
	}

	return FileHandle::null;
}

Reflex::TRef <Reflex::System::FileHandle> Reflex::System::FileHandle::Create(StandardStream stream)
{
	if (stream == kStandardStreamIn)
	{
		return REFLEX_CREATE(Common::POSIX::StdInWorkaround, stdin, 0);
	}
	else //if (stream == kStandardStreamOut)
	{
		return REFLEX_CREATE(Common::POSIX::WriteableFileImpl, stdout, 0);
	}
}

bool Reflex::System::Exists(const WString & path) 
{
	return Common::POSIX::Exists(Common::ToUTF8(path));
}

bool Reflex::System::IsDirectory(const WString & path)
{
	return Common::POSIX::IsDirectory(Common::ToUTF8(path));
}

bool Reflex::System::SetFileTime(const WString & path, UInt64 time)
{
	auto utf8 = Common::ToUTF8(path);

	struct utimbuf buf;

	buf.actime = time;
	buf.modtime = time;

	return utime(utf8.GetData(), &buf) == 0;
}

bool Reflex::System::GetFileAttributesEx(const WString & path, Tuple <UInt64,UInt64,UInt64,UInt64> & size_created_modified_accessed)
{
	auto utf8 = Common::ToUTF8(path);

	struct stat buf;

	if (stat(utf8.GetData(), &buf) == 0)
	{
		size_created_modified_accessed.a = buf.st_size;
		size_created_modified_accessed.b = buf.st_ctime;
		size_created_modified_accessed.c = buf.st_mtime;
		size_created_modified_accessed.d = buf.st_atime;

		return true;
	}
	else
	{
		return false;
	}
}

bool Reflex::System::Delete(const WString & path)
{
	auto utf8 = Common::ToUTF8(path);

	if (Common::POSIX::IsDirectory(utf8))
	{
		return remove(utf8.GetData()) == 0;
	}
	else
	{
		int filehandle = open(utf8.GetData(), O_RDONLY);

		if (filehandle == -1) return false;

		if (flock(filehandle, LOCK_EX | LOCK_NB) == 0)
		{
			return remove(utf8.GetData()) == 0;
		}
	}

	return false;
}

bool Reflex::System::MakeDirectory(const WString & path)
{
	auto utf8 = Common::ToUTF8(path);

	mode_t permissions = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;

	if (int error = mkdir(utf8.GetData(), permissions))
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool Reflex::System::Rename(const WString & from, const WString & to)
{
	if (int error = rename(Common::ToUTF8(from).GetData(), Common::ToUTF8(to).GetData()))
	{
		return false;
	}
	else
	{
		return true;
	}
}

Reflex::WString Reflex::System::GetCurrentDirectory()
{
	char path[PATH_MAX];

	if (getcwd(path, PATH_MAX) == nullptr)
	{
		path[0] = char(0);
	}
	
	ArrayView <UInt8> view = { Reinterpret<UInt8>(path), RawStringLength(path) };

	WString rtn = Common::DecodeUTF8(view);

	File::Detail::CorrectTrailingStroke(rtn);

	return rtn;
}

bool Reflex::System::SetCurrentDirectory(const WString & path)
{
	auto utf8 = Common::ToUTF8(path);

	return chdir(utf8.GetData()) == 0;
}

bool Reflex::System::IsAbsolutePath(const WString::View & path)
{
	return (path && path.GetFirst() == kPathDelimiter);
}

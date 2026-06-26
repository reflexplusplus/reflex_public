#include "file_handle.h"



//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

const Pair <DWORD> kFileAccess[3] =
{
	{ GENERIC_READ, OPEN_EXISTING },					//read
	{ GENERIC_READ | GENERIC_WRITE, OPEN_ALWAYS },		//overwrite
	{ GENERIC_READ | GENERIC_WRITE, OPEN_ALWAYS }		//append
};

const UInt8 kFileShareMode[3][2] =
{
	{
		FILE_SHARE_READ | FILE_SHARE_WRITE,	//unlocked
		FILE_SHARE_READ,					//locked
	},

	{
		FILE_SHARE_READ | FILE_SHARE_WRITE,	//unlocked
		0,									//locked
	},

	{
		FILE_SHARE_READ | FILE_SHARE_WRITE,	//unlocked
		0,									//locked
	},
};

const UInt8 kFileInitpos[3] =
{
	FILE_BEGIN,
	FILE_BEGIN,
	FILE_END
};

template <bool WRITE>
struct FileHandleImpl : public System::FileHandle
{
	REFLEX_OBJECT(FileHandleImpl, System::FileHandle);

	FileHandleImpl(HANDLE handle, DWORD initpos);

	~FileHandleImpl();


	bool IsWriteable() const override { return WRITE; }

	UInt64 GetSize() const override;

	void SetPosition(UInt64 position) override;

	UInt64 GetPosition() const override;

	UInt Read(void * ptr, UInt32 size) override;

	UInt32 Write(const void * ptr, UInt32 size) override;

	bool Flush(bool commit) override;

	bool Truncate() override;


	HANDLE m_handle;

	bool m_dirty = false;
};

template <bool WRITE> FileHandleImpl<WRITE>::FileHandleImpl(HANDLE handle, DWORD initpos)
	: m_handle(handle)
{
	if constexpr (WRITE)
	{
		SetFilePointer(m_handle, 0, 0, initpos);

		SetEndOfFile(m_handle);

		FlushFileBuffers(m_handle);
	}
}

template <bool WRITE> FileHandleImpl<WRITE>::~FileHandleImpl()
{
	if constexpr (WRITE) Flush(true);

	CloseHandle(m_handle);
}

template <bool WRITE> UInt64 FileHandleImpl<WRITE>::GetSize() const
{
	LARGE_INTEGER rtn = { 0,0 };

	GetFileSizeEx(m_handle, &rtn);

	return UInt64(rtn.QuadPart);
}

template <bool WRITE> void FileHandleImpl<WRITE>::SetPosition(UInt64 position)
{
	auto & li = Reinterpret<LARGE_INTEGER>(position);

	SetFilePointer(m_handle, li.LowPart, &li.HighPart, FILE_BEGIN);
}

template <bool WRITE> UInt64 FileHandleImpl<WRITE>::GetPosition() const
{
	REFLEX_STATIC_ASSERT(sizeof(LONGLONG) == sizeof(UInt64));

	LARGE_INTEGER null = { 0,0 };

	LARGE_INTEGER rtn = { 0,0 };

	SetFilePointerEx(m_handle, null, &rtn, FILE_CURRENT);

	return Reinterpret<UInt64>(rtn.QuadPart);
}

template <bool WRITE> UInt FileHandleImpl<WRITE>::Read(void * ptr, UInt32 size)
{
	DWORD len = 0;

	ReadFile(m_handle, ptr, size, &len, 0);

	return Reinterpret<UInt>(len);
}

template <bool WRITE> UInt32 FileHandleImpl<WRITE>::Write(const void * bytes_ptr, UInt32 bytes_size)
{
	REFLEX_ASSERT(WRITE);

	DWORD len = 0;

	if constexpr (WRITE)
	{
		m_dirty = true;

		WriteFile(m_handle, bytes_ptr, bytes_size, &len, 0);
	}

	return Reinterpret<UInt>(len);
}

template <bool WRITE> bool FileHandleImpl<WRITE>::Truncate()
{
	REFLEX_ASSERT(WRITE);

	if constexpr (WRITE)
	{
		m_dirty = true;

		return SetEndOfFile(m_handle) != 0;
	}

	return false;
}

template <bool WRITE> bool FileHandleImpl<WRITE>::Flush(bool commit)
{
	REFLEX_ASSERT(WRITE);

	if constexpr (WRITE)
	{
		if (m_dirty)
		{
			if (FlushFileBuffers(m_handle) != 0)
			{
				m_dirty = false;

				return true;
			}

			return false;
		}
		else
		{
			return true;
		}
	}

	return true;
}

REFLEX_INLINE TRef <FileHandle> CreateFileHandle(const WString::View & path, FileHandle::Mode mode, bool lock)
{
	auto access = kFileAccess[mode];

	auto handle = CreateFileW(path.data, access.a, kFileShareMode[mode][lock], NULL, access.b, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		if (mode)
		{
			DEV_WARN("System::FileHandle::Create failed", path, UInt(mode));
		}

		CloseHandle(handle);

		return FileHandle::null;
	}
	else if (mode)
	{
		return REFLEX_CREATE(FileHandleImpl<true>, handle, kFileInitpos[mode]);
	}
	else
	{
		return REFLEX_CREATE(FileHandleImpl<false>, handle, FILE_BEGIN);
	}
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::FileHandle> Reflex::System::FileHandle::Create(const WString & path, Mode mode, bool lock)
{
	return Win::CreateFileHandle(path, mode, lock);
}

Reflex::TRef <Reflex::System::FileHandle> Reflex::System::FileHandle::Create(StandardStream stream)
{
	constexpr auto get_std_handle = [](DWORD which) -> HANDLE
	{
		HANDLE h = GetStdHandle(which);

//#ifndef REFLEX_SYSTEM_CONSOLE
//		if (h == NULL || h == INVALID_HANDLE_VALUE)
//		{
//			AttachConsole(ATTACH_PARENT_PROCESS);
//			
//			h = GetStdHandle(which);
//		}
//#endif

		return h;
	};

	if (stream == kStandardStreamIn)
	{
		return REFLEX_CREATE(Win::FileHandleImpl<false>, get_std_handle(STD_INPUT_HANDLE), 0);
	}
	else //if (stream == kStandardStreamOut)
	{
		_setmode(_fileno(stdout), _O_BINARY);		// Match Linux-style raw byte output

		return REFLEX_CREATE(Win::FileHandleImpl<true>, get_std_handle(STD_OUTPUT_HANDLE), 0);
	}
}

HANDLE Reflex::System::Win::QueryWriteableFileHandle(FileHandle & file_handle)
{
	if (auto impl = DynamicCast<Win::FileHandleImpl<true>>(file_handle)) return impl->m_handle;
	
	return nullptr;
}

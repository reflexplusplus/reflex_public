#include "[include].h"

REFLEX_BEGIN_INTERNAL(Reflex::System::Android)

struct CloudFile final : public System::FileHandle 
{
	CloudFile(JNIEnv* env_, Jni::JavaRef<jobject>& fileUri_, FileHandle::Mode mode)
		: m_writeable(mode != kModeRead)
		, env(env_)
		, m_fileUri(fileUri_.DetachOwnership(), fileUri_.IsGlobalRef())
	{
		// MEMO: we don't support "smart" writing to files, we use the memory block and then write the whole file upon closing/flushing. Same for reading.
		if (mode != kModeOverwrite) 
		{
			BringFileIntoMemory();
		}
		
		m_position = mode == kModeAppend ? data.GetSize() : 0;
	}

	~CloudFile() final 
	{
		Flush(true);
	}

	void BringFileIntoMemory() 
	{
		REFLEX_USE(Jni)

		try 
		{
			g_reflexActivityInstance->ReadFileFully(env, m_fileUri, data);
		}
		catch (const std::runtime_error& error) {
			DEV_LOG("Failed to read file", g_reflexActivityInstance->uriClass.ToString(env, m_fileUri), error.what());
		}
	}

	bool IsWriteable() const override { return m_writeable; }

	bool Flush(bool commit) override
	{
		REFLEX_USE(Jni)

		if (SetFiltered(m_needs_flush, false))
		{
			try
			{
				g_reflexActivityInstance->WriteFileFully(env, m_fileUri, data);
			}
			catch (const std::runtime_error& error)
			{
				DEV_LOG("Failed to write file", g_reflexActivityInstance->uriClass.ToString(env, m_fileUri), error.what());

				m_needs_flush = true;

				return false;
			}
		}

		return true;
	}

	UInt64 GetSize() const override 
	{
		return data.GetSize() - m_start;
	}

	void SetPosition(UInt64 position) override 
	{
		REFLEX_ASSERT(position <= GetSize());
		m_position = UInt32(position);
	}

	UInt64 GetPosition() const override 
	{
		return m_position; 
	}

	UInt32 Read(void * ptr, UInt32 size) override 
	{
		size = Min(size, data.GetSize() - (m_start + m_position));

		MemCopy(data.GetData() + m_position, ptr, size);
	
		m_position += size;
		
		return size;
	}

	UInt32 Write(const void * bytes_data, UInt bytes_size) override 
	{
		REFLEX_ASSERT(m_writeable);
	
		ArrayView <UInt8> chunk = { Cast<UInt8>(bytes_data), bytes_size };

		//Int32 end = m_start + m_position + chunk.b;

		auto actualposition = m_start + m_position;

		if (actualposition < data.GetSize()) 
		{
			auto ninside = data.GetSize() - actualposition;

			auto overwrite = Min(chunk.size, ninside);

			MemCopy(chunk.data, data.GetData() + actualposition, overwrite);

			chunk.data += overwrite;

			chunk.size -= overwrite;
		}

		data.Append(chunk);

		m_position += bytes_size;

		m_needs_flush = true;

		return bytes_size;
	}

	bool Truncate() override 
	{
		REFLEX_ASSERT(m_writeable);

		m_needs_flush = true;

		data.SetSize(m_start + m_position);

		return true;
	}



protected:

	static constexpr UInt m_start = 0;

	Array <UInt8> data;

	UInt m_position = 0;

	bool m_needs_flush = false;



private:

	const bool m_writeable;

	JNIEnv* env;

	Jni::JavaRef<jobject> m_fileUri;
};

struct ExternalResourceRef : System::ExternalResourceRef {
	explicit ExternalResourceRef(const ArrayView<UInt8>& token)
		: pathUtf8(token)
	{}

	Array<UInt8> GetPersistentToken() override {
		return Reinterpret<Array<UInt8>>(pathUtf8);
	}

	TRef<FileHandle> Open(FileHandle::Mode mode) override {
		REFLEX_USE(Jni)

		AttachedJavaEnv env;
		auto& uriClass = g_reflexActivityInstance->uriClass;
		JavaRef inputStream, fileUri;

		try {
			uriClass.FromUTF8(env, pathUtf8, fileUri);
			return REFLEX_CREATE(CloudFile, env, fileUri, mode);

		} catch (std::runtime_error& error) {
			DEV_LOG("error opening file", fileUri ? uriClass.ToString(env, fileUri) : "(unparsed)", error.what());
		}

		return System::FileHandle::null;
	}

private:
	Array<UInt8> pathUtf8;
};

REFLEX_END_INTERNAL

Reflex::TRef<Reflex::System::ExternalResourceRef> Reflex::System::ExternalResourceRef::Locate(const ArrayView<UInt8>& token) 
{
	return REFLEX_CREATE(Android::ExternalResourceRef, token);
}

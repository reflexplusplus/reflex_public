#include "../../../include/reflex/file/functions.h"
#include "../../../include/reflex/file/functions/resourcepool.h"




//
//functions

REFLEX_BEGIN_INTERNAL(Reflex::File)

constexpr WString::View kParent = L"..";

REFLEX_END_INTERNAL

Reflex::Pair < Reflex::WString, Reflex::Reference <Reflex::System::FileHandle> > Reflex::File::Detail::AcquireTempFile(const WString::View & filename, UInt max_tries)
{
	auto time = UInt32(System::GetTime());

	REFLEX_LOOP(idx, max_tries)
	{
		auto temp = Join(filename, L'.', ToWString(time++));

		auto handle = Make<System::FileHandle>(temp, System::FileHandle::kModeAppend);

		if (IsValid(handle) && handle->GetSize() == 0) return { temp, handle };
	}

	return {};
}

Reflex::Data::Archive Reflex::File::Peek(System::FileHandle & file, UInt bytes)
{
	auto pos = file.GetPosition();

	auto rtn = ReadBytes(file, bytes);

	file.SetPosition(pos);

	return rtn;
}

bool Reflex::File::ReadLine(System::FileHandle & file, CString & buffer)
{
	constexpr UInt kBufferSize = 128;

	buffer.Clear();

	auto start = file.GetPosition();

	auto position = start;

	auto size = file.GetSize();

	bool valid = false;

	while (position < size)
	{
		auto ptr = Extend(buffer, kBufferSize).data;

		auto read = file.Read(ptr, kBufferSize);

		CString::View test = { ptr, read };

		auto sub_eolpos = Data::Detail::ReadLine(test).size;

		auto eolpos = position + sub_eolpos;

		position += read;

		if (eolpos < position)
		{
			char eol[2] = { 0, 0 };

			if ((read - sub_eolpos) >= 2)
			{
				eol[0] = ptr[sub_eolpos];
				eol[1] = ptr[sub_eolpos + 1];
			}
			else
			{
				file.SetPosition(eolpos);

				file.Read(eol, 2);
			}

			buffer.Shrink(kBufferSize - UInt(sub_eolpos));  //buffer is now valid string, but we need to skip eol bytes 

			switch (eol[0])
			{
			case 10:
				file.SetPosition(eolpos + 1);
				break;

			case 13:
				file.SetPosition(eolpos + ((eol[1] == 10) ? 2 : 1));
				break;

			default:
				REFLEX_ASSERT(false);
				break;
			}

			return true;
		}
		else
		{
			buffer.Shrink(kBufferSize - read);
		}

		valid = true;
	}

	return valid;
}

Reflex::Data::Archive Reflex::File::Open(const WString & filename)
{
	auto file = Make<System::FileHandle>(filename);

	Data::Archive bytes(UInt32(file->GetSize()));

	file->Read(bytes.GetData(), bytes.GetSize());

	return bytes;
}

bool Reflex::File::Save(const WString & filename, const Data::Archive::View & data)
{
	auto [temp_filename, temp_file] = Detail::AcquireTempFile(filename, 1024);

	if (WriteBytes(temp_file, data) == data.size)
	{
		if (temp_file->Flush(true))
		{
			temp_file.Clear();	//close os filehandle

			return System::Rename(temp_filename, filename);
		}
	}

	temp_file.Clear();	//close os filehandle

	System::Delete(temp_filename);

	return false;
}

Reflex::Data::Archive Reflex::File::ReadBytes(System::FileHandle & file, UInt bytes)
{
	Data::Archive rtn(bytes);

	rtn.SetSize(file.Read(rtn.GetData(), rtn.GetSize()));

	return rtn;
}

Reflex::Data::Archive Reflex::File::Open(VirtualFileSystem::Lock & lock, const WString::View & filename)
{
	Attributes a;

	auto outstream = AutoRelease(lock.Read(filename, a));

	return ReadBytes(outstream);
}

bool Reflex::File::Save(VirtualFileSystem::Lock & lock, const WString::View & filename, const Data::Archive::View & data)
{
	auto outstream = AutoRelease(lock.Write(filename, false));

	if (WriteBytes(outstream, data) == data.size)
	{
		return outstream->Flush(true);
	}

	return false;
}

bool Reflex::File::Copy(System::FileHandle & instream, System::FileHandle & outstream, UInt chunksize)
{
	if (IsValid(instream) && IsValid(outstream))
	{
		Data::Archive buffer(chunksize);

		UInt64 total_read = 0;
		UInt64 total_written = 0;

		while (auto read = instream.Read(buffer.GetData(), chunksize))
		{
			total_read += read;
			total_written += outstream.Write(buffer.GetData(), read);
		}

		return total_read == total_written;
	}

	return false;
}

bool Reflex::File::Copy(VirtualFileSystem::Lock & lock, const WString & from, const WString & to)
{
	Attributes a;

	auto instream = AutoRelease(lock.Read(from, a));

	auto outstream = AutoRelease(lock.Write(to, false));

	return Copy(instream, outstream, kMaxUInt16);
}

void Reflex::File::MakePath(const WString & path)
{
	if (!System::Exists(path))
	{
		if (!System::MakeDirectory(path))
		{
			WString temp = path;

			UInt32 pos = 0;

			while (auto idx = Search(Mid(temp, pos), System::kPathDelimiter))
			{
				pos += idx.value;

				System::MakeDirectory(Left(temp, pos++));
			}

			System::MakeDirectory(temp);
		}
	}
}

void Reflex::File::DeleteDirectoryContent(const WString & path)
{
	auto dir = Make<System::DirectoryIterator>(path, true);

	System::DirectoryIterator::Item item;

	while (dir->GetNext(item))
	{
		auto full_path = Join(path, item.filename, System::kPathDelimiter);

		if (item.is_directory)
		{
			DeleteDirectoryContent(full_path);
		}
		else
		{
			full_path.Pop();
		}

		System::Delete(full_path);
	}
}

void Reflex::File::Detail::RemoveDuplicateStrokes(WString & path)
{
	constexpr WString::View kDoubleStroke = L"//";

	if (auto idx = Search(path, kDoubleStroke))
	{
		UInt write_pos = idx.value;

		bool previous_was_stroke = false;

		ArrayRegion <WChar> region = { path.GetData() + write_pos, path.GetSize() - write_pos };

		for (auto character : region)
		{
			bool is_stroke = (character == System::kPathDelimiter);

			if (is_stroke)
			{
				if (previous_was_stroke) continue;
			}

			path[write_pos++] = character;

			previous_was_stroke = is_stroke;
		}

		path.SetSize(write_pos);
	}
}

Reflex::WString Reflex::File::MakeRelativePath(const WString::View & base, const WString::View & path)
{
	REFLEX_ASSERT(base.GetLast() == System::kPathDelimiter);

	auto base_folder = RemoveTrailingStroke(base);

	auto base_parts = Split(base_folder, System::kPathDelimiter);
	
	auto path_parts = Split(path, System::kPathDelimiter);

	if (base_parts && path_parts)
	{
		UInt n = Min(base_parts.GetSize(), path_parts.GetSize());

		UInt idx = 0;

		while (idx < n && base_parts[idx] == path_parts[idx]) ++idx;


		Array <WString::View> out;

		for (UInt j = idx; j < base_parts.GetSize(); ++j) out.Push(kParent);

		for (UInt j = idx; j < path_parts.GetSize(); ++j) out.Push(path_parts[j]);

		return Merge(out, System::kPathDelimiter);
	}

	return path;
}

Reflex::WString Reflex::File::ResolveRelativePath(const WString::View & path)
{
	if (Search(path, System::kPathDelimiter))
	{
		Array <WString::View> parts = Split(path, System::kPathDelimiter);

		UInt n = parts.GetSize();

		UInt idx = 0;

		while (idx < n)
		{
			auto & part = parts[idx];

			if (part == kParent)
			{
				if (idx)
				{
					idx--;

					parts.Remove(idx, 2);

					n -= 2;
				}
				else
				{
					parts.Remove(idx);

					n--;
				}
			}
			else
			{
				idx++;
			}
		}

		auto merged = Merge(parts, System::kPathDelimiter);

		if (path.GetLast() == System::kPathDelimiter && merged && merged.GetLast() != System::kPathDelimiter)
		{
			merged.Push(System::kPathDelimiter);
		}

		return merged;
	}
	else
	{
		return path;
	}
}

Reflex::WString Reflex::File::ResolveIncludePath(const WString::View & base, const WString::View & path)
{
	if (path)
	{
		auto first = path.GetFirst();

		if (first == L':')	//reflex virtual path
		{
			return path;
		}
		else if (System::IsAbsolutePath(path))
		{
			return path;
		}
		else if (path.size > 1 && path[0] == kDot && path[1] == kPathDelimiter)		//L"./"
		{
			return Join(base, Nudge(path, 2));
		}
		else
		{
			return ResolveRelativePath(Join(base, path));
		}
	}
	else
	{
		return base;
	}
}

Reflex::Pair <Reflex::WString::View> Reflex::File::SplitFilename(const WString::View & path)
{
	if (auto idx = ReverseSearch(path, System::kPathDelimiter))
	{
		return Splice(path, idx.value + 1);
	}

	return { {}, path };
}

bool Reflex::File::CheckExtension(const WString::View & path, const WString::View & ext)
{
	UInt32 extlen = ext.size;

	if (path.size > extlen)
	{
		auto path_ext = ReverseSplice(path, extlen).b;

		return CaseInsensitive::eq(path_ext, ext) && path_ext.data[-1] == kDot;
	}

	return false;
}

Reflex::WString Reflex::File::CorrectExtension(const WString::View & path, const WString::View & extension)
{
	auto [filepath,filename] = SplitFilename(path);

	if (auto result = ReverseSearch<CaseSensitive>(filename, kDot))
	{
		return Join(filepath, Left(filename, result.value + 1), extension);
	}
	else
	{
		return Join(path, kDot, extension);
	}
}

Reflex::WString::View Reflex::File::ResolveExistingFolder(const WString::View & path)
{
	auto exists = path;

	while (auto result = ReverseSearch(exists, System::kPathDelimiter))
	{
		exists.size = result.value;

		WString t = exists;

		if (System::IsDirectory(t))
		{
			exists.size++;

			return exists;
		}
	}

	return { path.data, 0 };
}

const Reflex::File::ResourcePool::Token * Reflex::File::Query(ResourcePool::Lock & resourcepool, TypeID type_id, const WString::View & path)
{
	const ResourcePool::Token * result = resourcepool.Query({ path, type_id });

	if (!result)
	{
		resourcepool.Enumerate(type_id, [&path, &result](const ResourcePool::Token & token)
		{
			if (token.attributes.resolved_path == path) result = &token;
		});
	}

	return result;
}

Reflex::WString::View Reflex::File::GetPath(const ResourcePool::Lock & lock, Address adr)
{
	if (auto p = lock.Query(adr)) return p->path;

	REFLEX_ASSERT(adr.id == kNullKey);

	return {};
}

Reflex::WString::View Reflex::File::GetResolvedPath(const ResourcePool::Lock & lock, Address adr)
{
	if (auto p = lock.Query(adr)) return p->attributes.resolved_path;

	REFLEX_ASSERT(adr.id == kNullKey);

	return {};
}

#include "tasks.h"




REFLEX_BEGIN_INTERNAL(ReflexCLI)

struct ResourceBuilder
{
	ResourceBuilder();

	void Compile(const WString::View & filename, volatile Float & progress);

	enum Format
	{
		kFormatReflex,
		kFormatReflexMinimal,
		kFormatExternal
	};

	struct Item
	{
		CString id;
		CString dest;
		WString source_path;
		Pair <UInt64> source_attributes;
		Data::Archive data;
		bool compress = false;
		bool enumerable = false;
	};

	struct Namespace : public Node <Namespace>
	{
		using Node<Namespace>::Attach;

		void Reset()
		{
			id.Clear();
			items.Clear();
		}

		CString id;
		Array <ResourceBuilder::Item> items;
	};

	UInt GetLevel(const Namespace & node);

	void Recurse(const WString::View & localpath, Namespace & parent, const Data::PropertySet & attributes);

	void DeclareNamespace(System::FileHandle & output, const Namespace & group);

	static Tuple <UInt,UInt,Data::Archive::View> UnpackData(Item & item, Data::Archive & scratch);

	void PrintGap(System::FileHandle & output);

	void PrintComment(System::FileHandle & output, const CString & text);

	CString MakeHex(const Data::Archive::View & data);

	void WriteItem(System::FileHandle & output, Data::Archive & file_scratch, const Array <CString> & ns, Item & item, const UInt & total, UInt & done, volatile Float & progress);

	UInt64 ComputeHash(const WString::View & xml_path);

	static UInt64 ReadCacheHash(const WString & cache_path);

	static void PopulateSourceInfo(Item & item, const WString & source_path)
	{
		item.source_path = source_path;
		System::GetFileAttributes(source_path, item.source_attributes);
	}

	static void AddFolder(Array <Item> & items, const Data::PropertySet & attributes, const ArrayView <WString> & exclude_folders, const ArrayView <WString> & exclude_types, const WString & root, const WString & folder)
	{
		auto abspath = Join(root, folder);

		auto [folders, files] = File::List(abspath);

		for (auto & i : folders)
		{
			auto foldername = ReverseSplice(i.key, 1).a;

			if (auto idx = Search<CaseInsensitive>(exclude_folders, foldername))
			{
				if (CaseInsensitive::eq(exclude_folders[idx.value], foldername)) continue;
			}

			AddFolder(items, attributes, exclude_folders, exclude_types, root, Join(folder, i.key));
		}

		for (auto & i : files)
		{
			if (Search<CaseInsensitive>(exclude_types, File::GetExtension(i.key))) continue;

			auto item = AddItem(items, attributes, i.key);

			item->id = ToCString(Join(folder, i.key));

			PopulateSourceInfo(*item, Join(abspath, i.key));
		}
	}

	REFLEX_NOINLINE static TRef <Item> AddItem(Array <Item> & items, const Data::PropertySet & attributes, const WString::View & localpath)
	{
		constexpr auto get_bool = [](const Data::PropertySet & attrs, Key32 id)
		{
			return Key32(Data::GetCString(attrs, id)) == MakeKey32("true");
		};

		auto & item = items.Push();

		item.id = Data::GetCString(attributes, Data::kid);
		item.enumerable = get_bool(attributes, MakeKey32("public"));

		switch (MakeKey32(Data::GetCString(attributes, MakeKey32("compress"))))
		{
		case MakeKey32("true"):
			item.compress = true;
			break;

		case MakeKey32("auto"):
			item.compress = !Search(ToView(kCompressedFormats), HashFileExtension(File::GetExtension(localpath)));
			break;

		default:
			item.compress = false;
			break;
		}

		return item;
	}

	static CString MakeVarName(const CString & id)
	{
		CString value = id;

		for (auto & c : value)
		{
			auto lower = Lowercase(c);

			if ((lower >= 'a' && lower <= 'z') || (lower >= '0' && lower <= '9'))
			{
				continue;
			}

			c = '_';
		}

		return value;
	}

	static Key32 HashFileExtension(const WString::View & ext)
	{
		UInt32 hash = kHashSeed;

		for (auto & c : ext) Reflex::Detail::IncrementHash(hash, Lowercase(c));

		return hash;
	}

	static Pair <WString> GetOutputPaths(const WString::View & path, const Data::PropertySet & xml);

	Map < Key32, Function<void(const WString::View &, Array <Item> &, const Data::PropertySet &)> > m_types;
	WString m_outputpath;
	Format m_format = kFormatReflex;
	Namespace m_root;

	static constexpr Key32 kFormats[] = { MakeKey32("reflex"), MakeKey32("reflex_minimal"), MakeKey32("external") };
	static constexpr CString::View kTab = "\t";
	static constexpr CString::View kExtern = "extern";
	static constexpr CString::View kInclude = "#include";
	static constexpr CString::View kUL = "u";
	static constexpr CString::View kULL = "ull";
	static constexpr CString::View kDoubleColon = "::";
	static constexpr char kSemiColon = ';';
	static constexpr CString::View kConst = "const";
	static constexpr CString::View kReflex = "Reflex::";
	static constexpr CString::View kUInt64 = "UInt64";
	static constexpr CString::View kArrayView = "ArrayView <Reflex::UInt8> ";
	static constexpr CString::View kPrivateItem = "File::EmbeddedResource ";
	static constexpr CString::View kPublicItem = "File::EnumerableEmbeddedResource ";
	static constexpr CString::View kComment = "//";
	static constexpr char kComma = ',';
	static constexpr UInt kLineSize = 8;
	static constexpr UInt32 kDependencyCacheVersion = 2;
	static constexpr Key32 kCompressedFormats[] =
	{
		"jpg", "jpeg", "png", "gif", "webp",
		"mp3", "ogg", "flac",
		"mp4", "mkv", "mov", "avi",
		"zip", "rar", "7z", "gz", "bz2", "xz",
		"aar"
	};
};

ResourceBuilder::ResourceBuilder()
{
	auto file = [this](const WString::View & localpath, Array <Item> & items, const Data::PropertySet & attributes)
	{
		auto item = AddItem(items, attributes, localpath);

		auto path = ToWString(Data::GetCString(attributes, MakeKey32("path")));
		auto resolved_path = File::ResolveIncludePath(localpath, path);

		if (!item->id) item->id = ToCString(path);

		PopulateSourceInfo(*item, resolved_path);
	};

	auto folder = [this](const WString::View & localpath, Array <Item> & items, const Data::PropertySet & attributes)
	{
		constexpr auto convert = [](const ArrayView <CString::View> & values)
		{
			Array <WString> result;

			for (auto & value : values) result.Push(ToWString(value));

			return result;
		};

		auto path = File::ResolveIncludePath(localpath, File::CorrectTrailingStroke(ToWString(Data::GetCString(attributes, MakeKey32("path")))));
		auto exclude_types = convert(Split(Data::GetCString(attributes, MakeKey32("exclude-types")), ','));
		auto exclude_folders = convert(Split(Data::GetCString(attributes, MakeKey32("exclude-folders")), ','));

		AddFolder(items, attributes, exclude_folders, exclude_types, path, {});
	};

	static constexpr auto read_value = [](ResourceBuilder * self, const WString::View & localpath, Array <Item> & items, const Data::PropertySet & attributes)
	{
		auto item = self->AddItem(items, attributes, localpath);
		return MakeTuple(item, Data::GetCString(attributes, MakeKey32("value")));
	};

	auto bytes = [this](const WString::View & localpath, Array <Item> & items, const Data::PropertySet & attributes)
	{
		auto [item, value] = read_value(this, localpath, items, attributes);
		item->data = Data::HexToBytes(value);
	};

	auto boolean = [this](const WString::View & localpath, Array <Item> & items, const Data::PropertySet & attributes)
	{
		auto [item, value] = read_value(this, localpath, items, attributes);
		item->compress = false;
		item->data.Push(UInt8(value == Reflex::Detail::kFalseTrue[true]));
	};

	auto key32 = [this](const WString::View & localpath, Array <Item> & items, const Data::PropertySet & attributes)
	{
		auto [item, value] = read_value(this, localpath, items, attributes);
		item->compress = false;
		item->data = Data::Pack(MakeKey32(value));
	};

	auto string = [this](const WString::View & localpath, Array <Item> & items, const Data::PropertySet & attributes)
	{
		auto [item, value] = read_value(this, localpath, items, attributes);
		item->data = Data::Pack(value);
	};

	auto sha1 = [this](const WString::View & localpath, Array <Item> & items, const Data::PropertySet & attributes)
	{
		auto [item, value] = read_value(this, localpath, items, attributes);
		item->data = Data::SHA1(Data::Pack(value));
	};

	auto sha256 = [this](const WString::View & localpath, Array <Item> & items, const Data::PropertySet & attributes)
	{
		auto [item, value] = read_value(this, localpath, items, attributes);
		item->data = Data::SHA256(Data::Pack(value));
	};

	m_types.Set(MakeKey32("File"), file);
	m_types.Set(MakeKey32("file"), file);
	m_types.Set(MakeKey32("Folder"), folder);
	m_types.Set(MakeKey32("folder"), folder);
	m_types.Set(MakeKey32("Bytes"), bytes);
	m_types.Set(MakeKey32("bytes"), bytes);
	m_types.Set(MakeKey32("String"), string);
	m_types.Set(MakeKey32("string"), string);
	m_types.Set(MakeKey32("Bool"), boolean);
	m_types.Set(MakeKey32("bool"), boolean);
	m_types.Set(MakeKey32("Key32"), key32);

	//legacy, remove
	m_types.Set(MakeKey32("sha1"), sha1);
	m_types.Set(MakeKey32("sha256"), sha256);
}

Pair <WString> ResourceBuilder::GetOutputPaths(const WString::View & xml_location, const Data::PropertySet & xml)
{
	constexpr auto make_path = [](const WString::View & location, const CString::View & relative_path, const WString::View & ext) -> WString
	{
		if (relative_path)
		{
			return File::ResolveIncludePath(location, File::CorrectExtension(ToWString(relative_path), ext));
		}

		return {};
	};

	if (auto cpp_path = make_path(xml_location, Data::GetCString(xml, MakeKey32("output")), L"cpp"))
	{
		return { File::CorrectExtension(cpp_path, L"h"), cpp_path };
	}

	return
	{
		make_path(xml_location, Data::GetCString(xml, MakeKey32("header")), L"h"),
		make_path(xml_location, Data::GetCString(xml, MakeKey32("source")), L"cpp")
	};
}

void ResourceBuilder::Compile(const WString::View & path, volatile Float & progress)
{
	progress = 0.0f;

	File::VirtualFileSystem::Lock lock(Bootstrap::global->resourcepool->filesystem);

	auto xml_location = File::SplitFilename(path).a;
	auto cache_path = File::CorrectExtension(path, L"cache");
	auto xml = Data::DecodePropertySet(Data::kReflexXmlFormat, File::Open(path));
	auto [h_path, cpp_path] = GetOutputPaths(xml_location, xml);

	if (!(h_path && cpp_path))
	{
		return;
	}

	auto include_path = File::MakeRelativePath(File::SplitFilename(cpp_path).a, h_path);
	m_root.Reset();
	m_format = kFormatReflex;

	if (auto format = Data::GetCString(xml, MakeKey32("format")))
	{
		if (auto idx = Search(kFormats, Key32(format))) m_format = Format(idx.value);
	}

	UInt total = 0;
	UInt done = 0;

	Recurse(xml_location, m_root, xml);

	auto start_time = System::GetElapsedTime();
	auto dependency_hash = ComputeHash(path);

	if (System::Exists(h_path) && System::Exists(cpp_path) && ReadCacheHash(cache_path) == dependency_hash)
	{
		progress = 1.0f;
		
		output.Log(path, "no changes detected");
		
		return;
	}

	for (auto & i : Namespace::BranchIterator(m_root)) total += i.items.GetSize();

	auto buffer = Make<Data::ArchiveObject>();
	buffer->value.Allocate(1024 * 1024);

	Data::Archive file_scratch(1024 * 256);

	auto h = AutoRelease(File::CreateMemoryWriter(*buffer));

	File::WriteLine(h, "#pragma once");

	if (m_format == kFormatReflex)
	{
		File::WriteLine(h);
		File::WriteLine(h, Join(kInclude, ' ', '"', "reflex_ext/file/embedded_resource.h", '"'));
	}
	else if (m_format == kFormatReflexMinimal)
	{
		File::WriteLine(h);
		File::WriteLine(h, Join(kInclude, ' ', '"', "reflex/core/array/view.h", '"'));
	}

	PrintGap(h);
	PrintComment(h, "resources");

	for (auto & i : m_root) DeclareNamespace(h, i);

		SaveGeneratedFile(h_path, buffer->value);

	buffer->value.Clear();

	auto cpp = AutoRelease(File::CreateMemoryWriter(buffer));

	File::WriteLine(cpp, Join(kInclude, ' ', '"', ToCString(include_path), '"'));

	PrintGap(cpp);

	for (auto & group : Namespace::BranchIterator(m_root))
	{
		Array <CString> ns;

		for (auto & i : Namespace::ParentRange(group)) ns.Push(i.id);

		ns.Pop();
		
		Reverse(ns);

		for (auto & i : group.items) WriteItem(cpp, file_scratch, ns, i, total, done, progress);
	}

		SaveGeneratedFile(cpp_path, buffer->value);
		SaveGeneratedFile(cache_path, Data::Pack(dependency_hash));

	output.LogEx(kLogNormal, {}, path, ' ', total, " files, ", ToCString(Float64(buffer->value.GetSize()) / Float64(1024 * 1024), 2), "mb, ", (System::GetElapsedTime() - start_time) * 1000.0, "ms");

	progress = 1.0f;
}

UInt ResourceBuilder::GetLevel(const Namespace & node)
{
	UInt idx = -1;

	for ([[maybe_unused]] auto & i : Namespace::ConstParentRange(node)) ++idx;

	return idx;
}

void ResourceBuilder::Recurse(const WString::View & localpath, Namespace & parent, const Data::PropertySet & attributes)
{
	for (auto & i : Data::GetXmlNodes(attributes))
	{
		Data::PropertySet merged = attributes;
		Data::Assimilate(merged, i);

		auto tag = Data::GetXmlTag(i);

		if (auto handler = m_types.Search(MakeKey32(tag)))
		{
			(*handler)(localpath, parent.items, merged);
		}
		else
		{
			auto nss = Split(tag, kDoubleColon);
			TRef <Namespace> group = parent;

			for (auto & ns : nss)
			{
				auto parent_group = group;

				group = New<Namespace>();
				group->Attach(parent_group);
				group->id = ns;
			}

			Recurse(localpath, group, merged);
		}
	}
}

void ResourceBuilder::DeclareNamespace(System::FileHandle & outstream, const Namespace & group)
{
	CString indent;

	REFLEX_LOOP(idx, GetLevel(group) - 1) indent.Push(char(9));

	File::WriteLine(outstream, Join(indent, "namespace ", group.id));
	File::WriteLine(outstream, Join(indent, "{"));
	File::WriteLine(outstream);

	if (group.items)
	{
		indent.Push(char(9));

		for (auto & item : group.items)
		{
			auto varname = MakeVarName(item.id);

			if (m_format == kFormatReflex)
			{
				File::WriteLine(outstream, Join(indent, kExtern, ' ', kConst, ' ', kReflex, item.enumerable ? kPublicItem : kPrivateItem, varname, kSemiColon));
			}
			else if (m_format == kFormatReflexMinimal)
			{
				File::WriteLine(outstream, Join(indent, kExtern, ' ', kConst, ' ', kReflex, kArrayView, varname, kSemiColon));
			}

			File::WriteLine(outstream);
		}

		indent.Pop();
	}

	for (auto & i : group) DeclareNamespace(outstream, i);

	File::WriteLine(outstream, Join(indent, '}'));
	File::WriteLine(outstream);
}

Tuple <UInt,UInt,Data::Archive::View> ResourceBuilder::UnpackData(Item & item, Data::Archive & scratch)
{
	if (item.source_path)
	{
		scratch = File::Open(item.source_path);
	}
	else
	{
		scratch = item.data;
	}

	UInt original_size = scratch.GetSize();
	UInt compressed_size = original_size;

	if (item.compress)
	{
		scratch = Data::Compress(Data::kLZ4, scratch);
		compressed_size = scratch.GetSize();
	}

	while (Modulo(scratch.GetSize(), kSizeOf<UInt64>)) scratch.Push(0);

	return { compressed_size, original_size, scratch };
}

void ResourceBuilder::WriteItem(System::FileHandle & file, Data::Archive & file_scratch, const Array <CString> & nspc, ResourceBuilder::Item & item, const UInt & total, UInt & done, volatile Float & progress)
{
	PrintComment(file, item.id);

	auto [size, uncompressed_size, data] = UnpackData(item, file_scratch);
	UInt nblock = data.size / kSizeOf<UInt64>;
	auto data_varname = MakeVarName(item.id);
	auto full_symbol = Join('k', Merge(nspc, '_'), '_');

	File::WriteLine(file, Join(kConst, ' ', kReflex, kUInt64, ' ', full_symbol, data_varname, "[", ToCString(nblock), "] ="));
	File::WriteLine(file, "{");

	UInt nline = nblock / kLineSize;
	const UInt8 * pdata = data.data;
	CString line;
	line.Allocate(200);

	REFLEX_LOOP(lineidx, nline)
	{
		line = kTab;

		REFLEX_LOOP(idx, kLineSize)
		{
			UInt64 value = *Reinterpret<UInt64>(pdata);
			pdata += kSizeOf<UInt64>;
			line = Join(line, MakeHex({ Reinterpret<UInt8>(&value), kSizeOf<UInt64> }), kULL, kComma, ' ');
		}

		File::WriteLine(file, line);
	}

	line = kTab;

	REFLEX_LOOP(idx, nblock - (nline * kLineSize))
	{
		UInt64 value = *Reinterpret<UInt64>(pdata);
		pdata += kSizeOf<UInt64>;
		line = Join(line, MakeHex({ Reinterpret<UInt8>(&value), kSizeOf<UInt64> }), kULL, kComma, ' ');
	}

	File::WriteLine(file, line);
	File::WriteLine(file, Join('}', kSemiColon));
	File::WriteLine(file);

	auto archive = Join("reinterpret_cast<const Reflex::UInt8*>(&", full_symbol, data_varname, "), ", ToCString(size), kUL);
	auto compressed = Join(ToCString(item.compress ? uncompressed_size : UInt(0)), kUL);
	auto full_namespace = Merge(nspc, kDoubleColon);

	if (m_format == kFormatReflex)
	{
		CString string = Join(kConst, ' ');

		if (item.enumerable)
		{
			string = Join(string, kReflex, kPublicItem, full_namespace, kDoubleColon, data_varname, '(');
			string = Join(string, "Reflex::K32(", '"', full_namespace, '"', "), Reflex::K32(", '"', item.id, '"', ')');
			string = Join(string, kComma, ' ', archive, kComma, ' ', compressed, ");");
		}
		else
		{
			string = Join(string, kReflex, kPrivateItem, full_namespace, kDoubleColon, data_varname, " = { { ", archive, " }", kComma, ' ', compressed, " };");
		}

		File::WriteLine(file, string);
	}
	else if (m_format == kFormatReflexMinimal)
	{
		File::WriteLine(file, Join(kConst, ' ', kReflex, kArrayView, full_namespace, kDoubleColon, data_varname, " = { ", archive, " };"));
	}

	PrintGap(file);

	progress = ++done / Float(total);
}

void ResourceBuilder::PrintGap(System::FileHandle & file)
{
	const UInt8 lines[] = { 10,10,10,10 };
	file.Write(lines, sizeof(lines));
}

void ResourceBuilder::PrintComment(System::FileHandle & file, const CString & text)
{
	File::WriteLine(file, kComment);
	File::WriteLine(file, Join(kComment, text));
	File::WriteLine(file);
}

CString ResourceBuilder::MakeHex(const Data::Archive::View & data)
{
	Data::Archive temp = data;
	auto dst = temp.GetData();

	REFLEX_RLOOP_PTR(data.data, p, data.size) (*dst++) = *p;

	return Join("0x", Data::BytesToHex(temp));
}

UInt64 ResourceBuilder::ComputeHash(const WString::View & xml_path)
{
	constexpr auto HashPathAndAttributes = [](UInt64 & hash, const WString::View & path)
	{
		hash = Data::FNV1a64(Data::EncodeUCS2(path), hash);

		Tuple <UInt64, UInt64> attributes;

		if (System::GetFileAttributes(path, attributes))
		{
			hash = Data::FNV1a64(Data::Pack(attributes), hash);
		}
		else
		{
			hash = Data::FNV1a64(Data::Pack(MakeTuple(UInt64(0), UInt64(0))), hash);
		}
	};

	UInt64 hash = Data::FNV1a64(Data::Pack(kDependencyCacheVersion));

	HashPathAndAttributes(hash, xml_path);

	for (auto & group : Namespace::BranchIterator(m_root))
	{
		for (auto & item : group.items)
		{
			if (auto & path = item.source_path)
			{
				HashPathAndAttributes(hash, path);
			}
		}
	}

	return hash;
}

UInt64 ResourceBuilder::ReadCacheHash(const WString & cache_path)
{
	auto handle = Make<System::FileHandle>(cache_path);
	
	UInt64 hash = 0;

	if (handle->Read(&hash, 8) == 8) return hash;

	return 0;
}

REFLEX_END_INTERNAL

void ReflexCLI::BuildResources(const WString::View & filename, Float & progress)
{
	ResourceBuilder parser;
	
	parser.Compile(filename, progress);
}

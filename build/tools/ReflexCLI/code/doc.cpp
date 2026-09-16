#include "tasks.h"
#include "documentation.h"

#include "../../common/docformat/docformat.cpp"




//
//declarations

REFLEX_BEGIN_INTERNAL(ReflexCLI)

REFLEX_DECLARE_KEY32(DocPath);

namespace CLI = Bootstrap::CLI;

using namespace Documentation;

constexpr CString::View kDefaultCodebase = "reflex_cpp";

constexpr CString::View kPathDelimiter = "/";

struct DocContext
{
	CString codebase;
	Reference <TableIndex,kReferenceUnsafe> index;
	Array <CString> path;
};

struct InfoHit
{
	Docformat::Symbol symbol;
	const SymbolInfo * info;
	UInt up = UInt(-1);
	UInt down = UInt(-1);
	Array <CString> matched_path;
};

Array <InfoHit> FindCandidateSymbols(const TableIndex & index, ArrayView <CString> base, ArrayView <CString> query, FunctionPointer <bool(const TableIndex&, const SymbolInfo&)> filter = [](const TableIndex &, const SymbolInfo & info) { return info.indexed; });

Array <InfoHit> FindBestHits(ArrayView <InfoHit> hits);

constexpr CString::View kCategoryTags[] = { "[???]", "[enm]", "[obj]", "[val]", "[tyd]", "[fnc]", "[glo]", "[mth]", "[val]", "[nsp]", "[grp]" };

Docformat::Symbol MakeSymbol(ArrayView <CString> path)
{
	return Documentation::MakeSymbol(CopyUnowned(path));
}

Array <CString> SplitPath(CString::View value)
{
	Array <CString> rtn;

	CString all = Replace(value, Docformat::kNamespaceDelimiter, "/");

	auto parts = Split(all, '/');

	for (auto i : parts)
	{
		rtn.Push(Trim(i));
	}

	return rtn;
}

CString::View RestoreCodebase()
{
	return Data::GetCString(Bootstrap::global->prefs, "codebase", kDefaultCodebase);
}

AlreadyRetained <Data::PropertySet> GetCodebasePrefs(CString::View codebase)
{
	return Data::AcquirePropertySet(Bootstrap::global->prefs, codebase);
}

Array <CString> RestorePath(CString::View codebase)
{
	auto prefs = GetCodebasePrefs(codebase);

	return Data::GetCStringArray(prefs, kDocPath);
}

void SetModulePath(CString::View codebase, ArrayView <CString> state)
{
	auto prefs = GetCodebasePrefs(codebase);

	Data::SetCStringArray(prefs, kDocPath, state);
}

REFLEX_NOINLINE bool PathEquals(ArrayView <CString> a, ArrayView <CString> b)
{
	if (a.size != b.size)
	{
		return false;
	}

	REFLEX_LOOP(idx, a.size)
	{
		if (!CaseInsensitive::eq(a[idx], b[idx]))
		{
			return false;
		}
	}

	return true;
}

REFLEX_NOINLINE bool PathStartsWith(ArrayView <CString> path, ArrayView <CString> prefix)
{
	if (path.size < prefix.size)
	{
		return false;
	}

	return PathEquals(Left(path, prefix.size), prefix);
}

WString GetDocumentationFolder(WString::View repo, CString::View codebase)
{
	auto docs = Join(repo, L"documentation/", ToWString(codebase), File::kStroke);

	if (File::Exists(Join(docs, L"symbols.txt")))
	{
		return docs;
	}

	CLI::ThrowError(Join("could not find documentation symbols for '", codebase, "'"));

	return {};
}

DocContext CreateDocContext(const Data::PropertySet & args)
{
	Data::KeyMap unused;

	DocContext context;

	auto root_codebase = Data::GetCString(args, "root");

	context.codebase = root_codebase ? root_codebase : RestoreCodebase();

	context.index = New<TableIndex>(CreateTable(GetDocumentationFolder(GetReflexPath(), context.codebase), unused));

	if (!root_codebase)
	{
		context.path = RestorePath(context.codebase);
	}

	return context;
}

Array <Pair<CString::View, bool>> BuildChain(const ModuleNode & module, bool include_groups)
{
	Array <Pair<CString::View, bool>> chain;

	for (auto itr = &module; itr && itr->name; itr = itr->GetParent())
	{
		if (include_groups || itr->is_namespace)
		{
			chain.Insert(0, { itr->name, itr->is_namespace });
		}
	}

	return chain;
}

void PrintModulePath(System::FileHandle & out, const ModuleNode & location, const ModuleNode & node, CString::View suffix = {})
{
	auto build_string = [&](const ModuleNode & module)
	{
		auto chain = BuildChain(module, true);

		CString text;

		REFLEX_LOOP(idx, chain.GetSize())
		{
			if (idx)
			{
				text.Append(chain[idx].b ? Docformat::kNamespaceDelimiter : kPathDelimiter);
			}

			text.Append(chain[idx].a);
		}

		return text;
	};

	if (!node.name)
	{
		File::WriteLine(out, Join("<root>", suffix));
		return;
	}

	auto node_text = build_string(node);
	auto plocation = &location;

	while (plocation && plocation->name && !BranchContains(*plocation, node))
	{
		plocation = plocation->GetParent();
	}

	CString line;

	if (plocation && plocation->name)
	{
		auto common_text = build_string(*plocation);

		auto pos = common_text.GetSize();

		if (pos < node_text.GetSize())
		{
			auto end = node_text[pos];

			if (end == Docformat::kNamespaceDelimiter.GetLast())
			{
				pos += Docformat::kNamespaceDelimiter.size;
			}
			else if (end == kPathDelimiter.GetLast())
			{
				pos += kPathDelimiter.size;
			}
		}

		auto [grey, white] = Splice<true>(node_text, pos);

		line.Append(CLI::Detail::kColours[CLI::kColourBrightBlack]);
		line.Append(grey);
		line.Append(CLI::Detail::kColours[CLI::kColourBrightWhite]);
		line.Append(white);
	}
	else
	{
		line.Append(CLI::Detail::kColours[CLI::kColourBrightWhite]);
		line.Append(node_text);
	}

	line.Append(CLI::Detail::kColours[CLI::kColourDefault]);

	File::WriteLine(out, Join(line, suffix));
}

void PrintSymbol(System::FileHandle & out, const TableIndex & index, const ModuleNode & location, const SymbolInfo & symbol, CString::View suffix = {})
{
	if (symbol.category_ex >= kCategoryExNamespace)
	{
		PrintModulePath(out, location, index.GetModule(symbol.symbol), suffix);
	}
	else
	{
		CString current_ns;

		if (auto chain = BuildChain(location, false))
		{
			for (auto [name, is_namespace] : BuildChain(location, false))
			{
				current_ns.Append(name);

				current_ns.Append(Docformat::kNamespaceDelimiter);
			}

			current_ns.Shrink(Docformat::kNamespaceDelimiter.size);
		}

		auto display_name = MakeNamespacedSymbol(index, current_ns, symbol);

		CString line;

		if (current_ns)
		{
			line.Append(current_ns);
			line.Append(Docformat::kNamespaceDelimiter);
		}

		line.Append(CLI::Detail::kColours[CLI::kColourBrightWhite]);
		line.Append(display_name);
		line.Append(suffix);

		CLI::Print(out, CLI::kColourBrightBlack, line);
	}
}

CString StripNavigationScope(CString scope, ArrayView <CString> navigation_path)
{
	for (UInt size = navigation_path.size; size > 0; --size)
	{
		auto prefix = MergeNamespace(CopyUnowned(Left(navigation_path, size)));

		auto scope_view = ToView(scope);

		if (CaseInsensitive::eq(scope_view, prefix))
		{
			return {};
		}

		auto delimiter = CString::View(Docformat::kNamespaceDelimiter);

		if (scope_view.size > prefix.GetSize() + delimiter.size && CaseInsensitive::eq(Left<true>(scope_view, prefix.GetSize()), prefix) && Mid<true>(scope_view, prefix.GetSize(), delimiter.size) == delimiter)
		{
			return Mid<true>(scope_view, prefix.GetSize() + delimiter.size);
		}
	}

	return scope;
}

CString FormatSymbolChain(const TableIndex & index, Docformat::Symbol symbol, ArrayView <CString> navigation_path)
{
	if (auto pinfo = index.QuerySymbolInfo(symbol))
	{
		auto [scope, name] = BuildFullSymbolName(index, *pinfo);

		scope = StripNavigationScope(scope, navigation_path);

		return Docformat::PrependNamespace(scope, name);
	}

	return {};
}

CString FormatFieldType(const TableIndex & index, const Docformat::Field & field, ArrayView <CString> navigation_path, UInt16 usage)
{
	CString out;

	if (field.is_const) out.Append("const ");

	auto type = index.ResolveTypedef(field.type, usage);

	if (auto name = FormatSymbolChain(index, type, navigation_path))
	{
		out.Append(name);
	}
	else if (!type)
	{
		out.Append("void");
	}
	else
	{
		out.Append("?");
	}

	if (field.is_pointer) out.Push('*');

	if (field.is_ref) out.Push('&');

	return out;
}

void WriteLabel(System::FileHandle & out, CString::View label, CString::View value)
{
	if (value)
	{
		File::WriteLine(out, Join(CLI::Detail::kColours[CLI::kColourBrightBlack], label, CLI::Detail::kColours[CLI::kColourDefault], value));
	}
}

void WriteSignatures(System::FileHandle & out, const TableIndex & index, const Data::PropertySet & record, ArrayView <CString> navigation_path)
{
	auto RenderSignature = [](const TableIndex & index, const Data::PropertySet & record, const Docformat::FunctionSignature & signature, ArrayView <CString> navigation_path, UInt16 usage)
	{
		CString out;

		out.Append(FormatFieldType(index, signature.rtn, navigation_path, usage));
		out.Push(' ');
		out.Append(Docformat::GetName(record));
		out.Push('(');

		REFLEX_LOOP(idx, signature.arguments.GetSize())
		{
			if (idx)
			{
				out.Append(", ");
			}

			auto & arg = signature.arguments[idx];

			out.Append(FormatFieldType(index, arg, navigation_path, usage));

			if (arg.name)
			{
				out.Push(' ');
				out.Append(arg.name);
			}
		}

		out.Push(')');

		if (signature.is_const)
		{
			out.Append(" const");
		}

		return out;
	};

	if (auto signatures = Docformat::UnpackFunctionSignatures(record))
	{
		for (auto & signature : signatures)
		{
			File::WriteLine(out, RenderSignature(index, record, signature, navigation_path, Docformat::GetIndex(record)));
		}

		File::WriteLine(out);
	}
}

Array <const SymbolInfo*> GetChildren(const TableIndex & index, Docformat::Symbol parent, CategoryEx category)
{
	Array <const SymbolInfo*> children;

	for (auto & entry : index.symbols)
	{
		auto & info = entry.value;

		if (info.indexed && info.parent_symbol == parent && info.category_ex == category)
		{
			children.Push(&info);
		}
	}

	Sort(children, [](const SymbolInfo * a, const SymbolInfo * b)
	{
		return a->index < b->index;
	});

	return children;
}

void WriteTypeChildren(System::FileHandle & out, const TableIndex & index, const ModuleNode & location, const SymbolInfo & parent, ArrayView <CString> navigation_path)
{
	Array <const SymbolInfo*> nested_types;

	for (auto category : { kCategoryExEnum, kCategoryExObject, kCategoryExValue })
	{
		for (auto info : GetChildren(index, parent.symbol, category))
		{
			nested_types.Push(info);
		}
	}

	Sort(nested_types, [](const SymbolInfo * a, const SymbolInfo * b)
	{
		return a->index < b->index;
	});

	if (nested_types)
	{
		CLI::Print(out, CLI::kColourBrightBlack, "nested types:");

		for (auto info : nested_types)
		{
			PrintSymbol(out, index, location, *info, {});
		}
	}

	if (auto methods = GetChildren(index, parent.symbol, kCategoryExMethod))
	{
		CLI::Print(out, CLI::kColourBrightBlack, "methods:");

		for (auto info : methods)
		{
			WriteSignatures(out, index, *info->record, navigation_path);
		}
	}

	if (auto members = GetChildren(index, parent.symbol, kCategoryExMember))
	{
		CLI::Print(out, CLI::kColourBrightBlack, "members:");

		for (auto info : members)
		{
			auto field = Docformat::RestoreField(*info->record);
			File::WriteLine(out, Join(FormatFieldType(index, field, navigation_path, info->index), ' ', field.name));
		}

		File::WriteLine(out);
	}
}

void Codebase(CString::View codebase, System::FileHandle & out)
{
	if (codebase && GetDocumentationFolder(GetReflexPath(), codebase))
	{
		Data::SetCString(Bootstrap::global->prefs, "codebase", codebase);
	}

	File::WriteLine(out, RestoreCodebase());
}

void Codebases(System::FileHandle & out)
{
	auto codebase = RestoreCodebase();

	auto docs = Join(GetReflexPath(), L"documentation/");

	auto [folders, files] = File::List(docs, false);

	for (auto & [i, unused] : folders)
	{
		if (File::Exists(Join(docs, i, L"/symbols.txt")))
		{
			auto name = ToCString(File::RemoveTrailingStroke(i));

			if (name == codebase)
			{
				File::WriteLine(out, name);
			}
			else
			{
				CLI::Print(out, CLI::kColourBrightBlack, name);
			}
		}
	}
}

void Where(const Data::PropertySet & args, System::FileHandle & out)
{
	auto context = CreateDocContext(args);

	auto location = context.index->GetModule(MakeSymbol(context.path));

	auto parent = location->GetParent();

	PrintModulePath(out, parent ? *parent : *context.index->GetRootModule(), location);
}

void Push(const Data::PropertySet & args, CString::View id, System::FileHandle & out)
{
	auto context = CreateDocContext(args);

	if (id)
	{
		auto location = context.index->GetModule(MakeSymbol(context.path));

		auto parts = SplitPath(id);

		for (auto & i : parts)
		{
			auto module = context.index->GetModule(MakeSymbol(context.path));

			bool found = false;

			for (auto & child : *module)
			{
				if (CaseInsensitive::eq(child.name, i))
				{
					context.path.Push(child.name);

					found = true;

					break;
				}
			}

			if (!found) CLI::ThrowError("module not found");
		}

		SetModulePath(context.codebase, context.path);

		PrintModulePath(out, location, context.index->GetModule(MakeSymbol(context.path)));
	}
	else
	{
		CLI::ThrowError("module not found");
	}
}

void Pop(const Data::PropertySet & args, System::FileHandle & out)
{
	auto context = CreateDocContext(args);

	if (context.path)
	{
		context.path.Pop();

		SetModulePath(context.codebase, context.path);
	}

	Where(args, out);
}

void Tree(const Data::PropertySet & args, CString::View base, System::FileHandle & out)
{
	auto label = [](const ModuleNode & node)
	{
		auto kColourBrightWhite = CLI::Detail::kColours[CLI::kColourBrightYellow];

		auto ns = node.is_namespace;

		return Join(kColourDim, ns ? "[nsp]" : "[grp]", ns ? kColourBrightWhite : kColourDefault, ' ', node.name, kColourDefault);
	};

	auto AppendDimCount = [](CString & line, UInt count)
	{
		if (count)
		{
			line.Push(' ');
			line.Append(CLI::Detail::kColours[CLI::kColourBrightBlack]);
			line.Push('(');
			line.Append(ToCString(count));
			line.Push(')');
			line.Append(CLI::Detail::kColours[CLI::kColourDefault]);
		}
	};

	auto context = CreateDocContext(args);

	auto base_node = context.index->GetModule(MakeSymbol(context.path));

	if (base)
	{
		auto hits = FindBestHits(FindCandidateSymbols(*context.index, context.path, SplitPath(base), [](const TableIndex & index, const SymbolInfo & info)
		{
			return info.indexed && index.QueryModule(info.symbol, nullptr);
		}));

		if (hits.Empty())
		{
			CLI::ThrowError("base module not found");
		}

		base_node = context.index->GetModule(hits.GetFirst().symbol);
	}

	if (!base_node)
	{
		CLI::ThrowError("base module not found");
	}

	if (base_node->name)
	{
		File::WriteLine(out, label(*base_node));
	}
	else
	{
		CLI::Print(out, CLI::kColourBrightBlack, "<root>");
	}

	CString::View prefix = CString::View("      ");

	if (auto children = SortChildModules(*base_node, true))
	{
		for (auto child : children)
		{
			CString line = prefix;

			line.Append(label(*child));

			AppendDimCount(line, child->GetNumItem());

			File::WriteLine(out, line);
		}
	}
	else
	{
		CLI::Print(out, CLI::kColourBrightBlack, Join(prefix, "[no children]"));
	}
}

Array <InfoHit> FindCandidateSymbols(const TableIndex & index, ArrayView <CString> base, ArrayView <CString> query, FunctionPointer <bool(const TableIndex&, const SymbolInfo&)> filter)
{
	constexpr auto calculate_direction = [](ArrayView <CString> base_path, ArrayView <CString> candidate_path)
	{
		UInt common = 0;
		UInt size = Min(base_path.size, candidate_path.size);

		while (common < size)
		{
			if (!CaseInsensitive::eq(base_path[common], candidate_path[common]))
			{
				break;
			}

			common++;
		}

		return Pair<UInt> { base_path.size - common, candidate_path.size - common };
	};

	Array <InfoHit> hits;

	if (query)
	{
		for (auto & [symbol, info] : index.symbols)
		{
			auto full_symbol_path = CopyOwned(BuildScope(index, info, true));

			full_symbol_path.Push(Docformat::GetName(*info.record));

			if (Search<CaseInsensitive>(full_symbol_path, query) && filter(index, info))
			{
				auto [up, down] = calculate_direction(base, full_symbol_path);

				hits.Push({ symbol, &info, up, down, full_symbol_path });
			}
		}

		Sort(hits, [](const InfoHit & a, const InfoHit & b)
		{
			if (a.up != b.up)
			{
				return a.up < b.up;
			}

			if (a.down != b.down)
			{
				return a.down < b.down;
			}

			if (a.matched_path.GetSize() != b.matched_path.GetSize())
			{
				return a.matched_path.GetSize() < b.matched_path.GetSize();
			}

			return Merge(a.matched_path, '/') < Merge(b.matched_path, '/');
		});
	}

	return hits;
}

Array <InfoHit> FindBestHits(ArrayView <InfoHit> hits)
{
	Array <InfoHit> best;

	if (hits)
	{
		auto up = hits.GetFirst().up;
		auto down = hits.GetFirst().down;

		for (auto & hit : hits)
		{
			if (hit.up != up || hit.down != down)
			{
				break;
			}

			best.Push(hit);
		}
	}

	return best;
}

void List(const Data::PropertySet & args, CString::View value, System::FileHandle & out)
{
	auto context = CreateDocContext(args);

	auto location = context.index->GetModule(MakeSymbol(context.path));

	Docformat::Symbol parent_symbol;

	if (auto parent_arg = Data::GetCString(args, "parent"))
	{
		if (auto hits = FindCandidateSymbols(*context.index, context.path, SplitPath(parent_arg)))
		{
			parent_symbol = hits.GetFirst().symbol;
		}
		else
		{
			CLI::ThrowError("parent not found");
		}
	}

	auto query = Merge(SplitPath(value), ':');

	for (auto & [symbol, info] : context.index->symbols)
	{
		if (parent_symbol && info.parent_symbol != parent_symbol)
		{
			continue;
		}

		Array <CString> path = CopyOwned(BuildModulePath(context.index->GetModule(info.module)).a);

		if (PathStartsWith(path, context.path))
		{
			CString combined = Merge(Mid(path, context.path.GetSize()), ':');

			combined.Push(':');
			combined.Append(info.name);

			if (query.Empty() || Search<CaseInsensitive>(combined, query))
			{
				CString line;

				line.Append(CLI::Detail::kColours[CLI::kColourBrightBlack]);
				line.Append(kCategoryTags[info.category_ex]);
				line.Append(CLI::Detail::kColours[CLI::kColourDefault]);
				line.Push(' ');

				out.Write(line.GetData(), line.GetSize());

				PrintSymbol(out, *context.index, location, info);
			}
		}
	}
}

void Info(const Data::PropertySet & args, CString::View value, System::FileHandle & out)
{
	auto context = CreateDocContext(args);

	auto location = context.index->GetModule(MakeSymbol(context.path));

	if (value)
	{
		auto query = SplitPath(value);

		auto hits = FindBestHits(FindCandidateSymbols(*context.index, context.path, query));

		if (hits.Empty())
		{
			CLI::ThrowError("symbol not found");
		}
		else if (hits.GetSize() > 1)
		{
			CLI::Print(out, CLI::kColourBrightYellow, "multiple candidates:");

			for (auto & hit : hits)
			{
				if (auto symbolinfo = context.index->QuerySymbolInfo(hit.symbol))
				{
					CString suffix;
					
					REFLEX_IF_DEBUG(suffix = Join(' ', '[', ToCString(hit.down), ',', ToCString(hit.up), ']', ' ', Merge(hit.matched_path, kPathDelimiter));)

					PrintSymbol(out, *context.index, location , *symbolinfo, suffix);
				}
			}
		}
		else 
		{
			auto & info = *hits.GetFirst().info;
			
			PrintSymbol(out, *context.index, location, info);

			WriteLabel(out, "type: ", GetCategoryLabel(info.category_ex));

			if (auto base = Docformat::GetTypeBase(info.record))
			{
				WriteLabel(out, "inherits: ", FormatSymbolChain(*context.index, base, context.path));
			}

			if (auto source = Docformat::GetTypeTemplateSource(info.record))
			{
				WriteLabel(out, "template: ", FormatSymbolChain(*context.index, source, context.path));
			}

			auto category = info.category_ex;

			if (category == kCategoryExFunction || category == kCategoryExMethod)
			{
				WriteSignatures(out, *context.index, info.record, context.path);
			}
			else if (category == kCategoryExTypedef)
			{
				WriteLabel(out, "alias: ", FormatSymbolChain(*context.index, Docformat::GetTypedefTarget(info.record), context.path));
			}
			else if (category == kCategoryExEnum || category == kCategoryExObject || category == kCategoryExValue)
			{
				WriteTypeChildren(out, *context.index, location, info, context.path);
			}

			if (auto description = GetDescription(info))
			{
				Data::ArchiveObject output;

				auto writer = ExportFormatWriter::CreatePlainText(output);

				WriteDescription(writer, Data::EncodeUTF8(description), kPathDelimiter);

				out.Write(output.value.GetData(), output.value.GetSize());
			}
			else
			{
				CLI::Print(out, CLI::kColourBrightBlack, "[no description]");
			}
		}
	}
	else
	{
		CLI::ThrowError("symbol not found");
	}
}

REFLEX_END_INTERNAL

void ReflexCLI::DocHelp(System::FileHandle & out)
{
	CLI::Print(out, CLI::kColourBrightBlack, "<symbol> [--root <codebase>]");
	PrintCommandWithDescription(out, "list", "[query] [--parent Reflex::GLX::TextArea] [--root <codebase>]");
	PrintCommandWithDescription(out, "tree", "[base] [--root <codebase>]");
	PrintCommandWithDescription(out, "where", "");
	PrintCommandWithDescription(out, "push", "<namespace|group>");
	PrintCommandWithDescription(out, "pop", "");
	PrintCommandWithDescription(out, "info", "<symbol> [--root <codebase>]");
	PrintCommandWithDescription(out, "codebases", "");
	PrintCommandWithDescription(out, "codebase", "[name]");
}

void ReflexCLI::Doc(const Data::PropertySet & args, System::FileHandle & out)
{
	auto cmd = Data::GetCString(args, K32("value"));
	auto arg = Data::GetCString(args, K32("value") + 1);

	if (!cmd)
	{
		DocHelp(out);
		return;
	}

	switch (MakeKey32(cmd))
	{
	case K32("tree"): Tree(args, arg, out); return;
	case K32("list"): List(args, arg, out); return;
	case K32("info"): Info(args, arg, out); return;
	case K32("push"): Push(args, arg, out); return;
	case K32("codebase"): Codebase(arg, out); return;
	case K32("codebases"): Codebases(out); return;
	case K32("where"): Where(args, out); return;
	case K32("pop"): Pop(args, out); return;
	default:
		Info(args, cmd, out);
		return;
	}
}

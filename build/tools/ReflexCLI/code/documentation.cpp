#include "documentation.h"
#include "common.h"

REFLEX_BEGIN_INTERNAL(ReflexCLI::Documentation)

Reference <ModuleNode> CreateModuleTree(const Data::PropertySet & info, const Data::KeyMap & namespaces)
{
	using Index = Map <Docformat::Symbol, ModuleNode*>;

	Index index;

	Unretained <ModuleNode> root = index.Set({}, New<ModuleNode>().Adr());

	root->is_namespace = true;

	static constexpr auto AcquireGuide = [](Index & index, ModuleNode & parent, Docformat::Symbol guide_id, CString::View name, bool is_namespace)
	{
		if (auto p = index.Search(guide_id))
		{
			return *p;
		}

		auto node = REFLEX_CREATE(ModuleNode);

		node->symbol = guide_id;
		node->name = name;
		node->is_namespace = is_namespace;
		node->Attach(parent);

		return index.Set(guide_id, node);
	};

	static constexpr auto AcquireBranch = [](Index & index, ModuleNode & root, const CString::View & path, bool is_namespace)
	{
		if (path)
		{
			auto parts = SplitNamespace(path);
			auto parent = &root;
			UInt n = 0;

			while (n < parts.GetSize())
			{
				auto [ns, name] = ReverseSplice(Left(parts, ++n), 1);
				
				parent = AcquireGuide(index, *parent, { MergeNamespace(ns), name.GetFirst() }, name.GetFirst(), is_namespace);
			}
		}
	};

	for (auto & [key, ns] : namespaces.value)
	{
		AcquireBranch(index, root, ns, true);
	}

	for (auto & [adr, ref] : info.Iterate<Data::PropertySet>())
	{
		if (Data::GetBool(ref, kGroup))
		{
			if (auto ppath = ref->QueryProperty<Data::ArrayOfCStringProperty>(kPath))
			{
				auto & path = ppath->value;
				auto merged = Merge(path, Docformat::kNamespaceDelimiter);
				AcquireBranch(index, root, merged, false);
			}
		}
	}

	return root;
}

struct TextFormatWriter final : public ExportFormatWriter
{
	using ExportFormatWriter::ExportFormatWriter;

	Pair <CString::View> GetDesc() const override { return { "Text", "txt" }; }
	CString Escape(CString::View s) override { return CString(s); }
	CString EncodeLink(CString::View text, UInt64 /*id*/) override { return text; }

	void Heading(UInt level, CString::View text, UInt64 id = 0) override
	{
		Data::WriteLine(out);
		Data::WriteLine(out, (level == 1) ? Uppercase(text) : CString(text));

		CString underline;

		for (UInt i = 0; i < text.size; ++i)
		{
			underline.Push(level <= 2 ? '=' : '-');
		}

		Data::WriteLine(out, underline);
		Data::WriteLine(out);
	}

	void HeadingPath(UInt level, ArrayView <HeadingPart> parts, UInt64 id) override
	{
		CString flat;

		for (auto & i : parts)
		{
			flat.Append(i.text);
		}

		Heading(level, flat, id);
	}

	void SectionBegin(CString::View /*id*/) override {}
	void SectionEnd() override {}

	void LineBegin(CString::View text, CString::View cls) override
	{
		if (m_block == Block::kList)
		{
			CString line;

			for (UInt i = 1; i < m_list_depth; ++i)
			{
				line.Append("  ");
			}

			line.Append("* ");
			line.Append(text);
			Data::WriteLine(out, line);
		}
		else
		{
			Data::WriteLine(out, text);
		}
	}

	void LineEnd() override {}

	void HorizontalLine() override
	{
		Data::WriteLine(out, "----------------------------------------");
		Data::WriteLine(out);
	}

	void ListBegin(CString::View /*css_class*/ = {}) override
	{
		++m_list_depth;
		m_block = Block::kList;
	}

	void ListEnd() override
	{
		if (m_list_depth) --m_list_depth;

		if (!m_list_depth)
		{
			m_block = Block::kText;
			Data::WriteLine(out);
		}
	}

	void CodeBegin(CString::View lang) override
	{
		m_block = Block::kCode;
		Data::WriteLine(out, Join('[', lang, ']'));
	}

	void CodeEnd() override
	{
		m_block = Block::kText;
		Data::WriteLine(out);
	}
};

ModuleNode * AcquireModuleNode(TableIndex & out, const Map <Docformat::Symbol,UInt32> & rows, Data::Table::ConstRowCursor & rowptr, const Data::Table::ColumnInfo & parent_symbol_col, const Data::Table::ColumnInfo & subcategory_col, const Data::Table::ColumnInfo & name_col, Docformat::Symbol symbol)
{
	if (auto pnode = out.module_index.Search(symbol))
	{
		return *pnode;
	}

	auto prow = rows.Search(symbol);

	if (!prow)
	{
		return out.root_module.Adr();
	}

	rowptr.SetIndex(*prow);

	auto node = REFLEX_CREATE(ModuleNode);
	auto parent_symbol = Reinterpret<Docformat::Symbol>(rowptr.ReadValue<UInt64>(parent_symbol_col));

	node->symbol = symbol;
	node->name = rowptr.ReadArray<char>(name_col);
	node->is_namespace = rowptr.ReadValue<Key32>(subcategory_col) == K32("namespace");

	out.module_index.Set(symbol, node);

	auto parent = parent_symbol.name.value ? AcquireModuleNode(out, rows, rowptr, parent_symbol_col, subcategory_col, name_col, parent_symbol) : out.root_module.Adr();

	node->Attach(*parent);

	return node;
}

REFLEX_INSTANTIATE_DEFAULT_ALLOCATOR;

ModuleNode g_null_module;

REFLEX_END_INTERNAL

ReflexCLI::Documentation::ModuleNode & ReflexCLI::Documentation::ModuleNode::null = g_null_module;

Reflex::Unretained <Reflex::Data::Table> ReflexCLI::Documentation::CreateTable(const WString::View & folder, Data::KeyMap & keymap)
{
	auto symbols_decoded = Data::DecodePropertySet(Data::kPropertySheetFormat, File::Open(Join(folder, L"symbols.txt")));
	auto info_decoded = Data::DecodePropertySet(Data::kPropertySheetFormat, File::Open(Join(folder, L"info.txt")));

	if (auto error = Data::GetError(info_decoded))
	{
		ThrowError(Join(error.value.b, ": ", error.value.c), ToCString(error.value.a));
	}

	Data::RegisterKey(keymap, "module");
	Data::RegisterKey(keymap, "namespace");
	Data::RegisterKey(keymap, "group");
	Data::RegisterKey(keymap, "value");
	Data::RegisterKey(keymap, "object");
	Data::RegisterKey(keymap, "enum");

	for (auto & i : Docformat::kCategories) Data::RegisterKey(keymap, i.b);

	const Data::Table::ColumnInfo kColumns[] =
	{
		{ K32("UniqueID"), Data::Table::kColumnTypeUInt32 },
		{ Docformat::kSymbol, Data::Table::kColumnTypeUInt64 },
		{ Docformat::kParentID, Data::Table::kColumnTypeUInt64 },
		{ Docformat::kCategory, Data::Table::kColumnTypeKey32, UInt8(MakeBit(5)) },	//this is needed for UI
		{ kSubCategory, Data::Table::kColumnTypeKey32 },
		{ Docformat::kIndexed, Data::Table::kColumnTypeBool },
		{ Docformat::kIndex, Data::Table::kColumnTypeUInt32 },
		{ kModule, Data::Table::kColumnTypeUInt64 },
		{ Docformat::kNamespace, Data::Table::kColumnTypeKey32 },
		{ Docformat::kName, Data::Table::kColumnTypeStringASCII },
		{ kData, Data::Table::kColumnTypeBinary },
		{ kDescription, Data::Table::kColumnTypeBinary },
	};

	auto table = Data::Table::Create(ToView(kColumns));

	ArrayView <Data::Table::ColumnInfo> columns = table->GetColumns();

	auto uid_col = columns[0];
	auto symbol_col = Data::AssumeColumn(columns, Docformat::kSymbol, Data::Table::kColumnTypeUInt64);
	auto parent_col = Data::AssumeColumn(columns, Docformat::kParentID, Data::Table::kColumnTypeUInt64);
	auto category_col = Data::AssumeColumn(columns, Docformat::kCategory, Data::Table::kColumnTypeKey32);
	auto subcategory_col = Data::AssumeColumn(columns, kSubCategory, Data::Table::kColumnTypeKey32);
	auto indexed_col = Data::AssumeColumn(columns, Docformat::kIndexed, Data::Table::kColumnTypeBool);
	auto index_col = Data::AssumeColumn(columns, Docformat::kIndex, Data::Table::kColumnTypeUInt32);
	auto module_col = Data::AssumeColumn(columns, kModule, Data::Table::kColumnTypeUInt64);
	auto ns_col = Data::AssumeColumn(columns, Docformat::kNamespace, Data::Table::kColumnTypeKey32);
	auto name_col = Data::AssumeColumn(columns, Docformat::kName, Data::Table::kColumnTypeStringASCII);
	auto data_col = Data::AssumeColumn(columns, kData, Data::Table::kColumnTypeBinary);
	auto desc_col = Data::AssumeColumn(columns, kDescription, Data::Table::kColumnTypeBinary);

	Map <Docformat::Symbol, Data::PropertySet*> info_map;
	Map <Docformat::Symbol, UInt> row_index;
	Data::KeyMap namespaces;

	for (auto & [adr, ref] : info_decoded.Iterate<Data::PropertySet>())
	{
		if (auto path = Data::GetCStringArray(ref, K32("Path")))
		{
			info_map.Set(MakeSymbol(CopyUnowned(path)), ref.Adr());
		}
	}

	const auto get_module = [&info_map](Docformat::Symbol symbol)
	{
		if (auto pinfo = info_map.Search(symbol))
		{
			if (Data::GetBool(**pinfo, kGroup))
			{
				return symbol;
			}

			if (auto module_path = Data::GetCStringArray(**pinfo, kModule))
			{
				return MakeSymbol(CopyUnowned(module_path));
			}
		}

		return Docformat::Symbol();
	};

	//LATER maybe here set all parents to containing namespace, where item does not have a parent
	//this would affect how full-names are generated tho

	UInt32 uid = 0;

	for (auto & [adr, record] : symbols_decoded.Iterate<Data::PropertySet>())
	{
		auto symbol = Docformat::GetSymbol(record);
		auto ns = Docformat::GetNamespace(record);
		auto name = Docformat::GetName(record);

		REFLEX_ASSERT(name);

		Data::RegisterKey(keymap, ns);
		Data::RegisterKey(namespaces, ns);

		auto subcategory = GetSubCategory(Docformat::GetCategory(record), Docformat::GetTypeFlags(record));
		auto ns_expanded = SplitNamespace(ns);
		auto module = MakeSymbol(ns_expanded);	//default module is namespace

		auto fallback = &Data::PropertySet::null;
		auto & info = **info_map.Search(symbol, &fallback);

		if (Data::GetBool(info, kGroup))
		{
			Data::SetBool(record, kGroup, true);
		}

		// A symbol can inherit module placement from either its own info node or
		// its documented parent; its own placement takes priority.
		for (auto i : { Docformat::GetParent(record), symbol })
		{
			if (auto placement = get_module(i))
			{
				module = placement;
			}
		}

		row_index.Set(symbol, table->GetNumRow());

		auto row = table->AddRow();

		row.WriteValue<UInt32>(uid_col, ++uid);
		row.WriteValue<UInt64>(symbol_col, UInt64(symbol));
		row.WriteValue<UInt64>(parent_col, UInt64(Docformat::GetParent(record)));
		row.WriteValue<Key32>(category_col, Data::RegisterKey(keymap, Docformat::kCategories[Docformat::GetCategory(record)].b));
		row.WriteValue<Key32>(subcategory_col, subcategory ? Data::RegisterKey(keymap, subcategory) : Key32());
		row.WriteValue<bool>(indexed_col, Docformat::IsIndexed(record));
		row.WriteValue<UInt32>(index_col, Docformat::GetIndex(record));
		row.WriteValue<UInt64>(module_col, UInt64(module));
		row.WriteValue<Key32>(ns_col, ns ? Data::RegisterKey(keymap, ns) : Key32());
		row.WriteArray<char>(name_col, name);
		row.WriteArray<UInt8>(data_col, Data::EncodePropertySet(Data::kPropertySetFormat, record));
	}

	auto root = CreateModuleTree(info_decoded, namespaces);

	Function <void(const ModuleNode &)> append_module_rows = [&](const ModuleNode & module)
	{
		if (!row_index.Search(module.symbol))
		{
			auto [path, ns_depth] = BuildModulePath(module);
			auto [ns, grouping] = Splice(path, ns_depth);
			auto record_ns = module.is_namespace && ns ? Left(ns, ns.size - 1) : ns;
			auto row = table->AddRow();

			row.WriteValue<UInt32>(uid_col, ++uid);
			row.WriteValue<UInt64>(symbol_col, UInt64(module.symbol));
			row.WriteValue<Key32>(category_col, Data::RegisterKey(keymap, kModuleCategory));
			row.WriteValue<Key32>(subcategory_col, Data::RegisterKey(keymap, module.is_namespace ? kNamespaceSubCategory : kGroupSubCategory));
			row.WriteArray<char>(name_col, module.name);
			row.WriteValue<Key32>(ns_col, record_ns ? Data::RegisterKey(keymap, MergeNamespace(record_ns)) : Key32());

			if (auto parent = module.GetParent())
			{
				row.WriteValue<UInt64>(parent_col, UInt64(parent->symbol));
			}

			row.WriteValue<UInt64>(module_col, UInt64(module.symbol));
			row.WriteValue<bool>(indexed_col, true);

			auto record = Make<Data::PropertySet>();

			if (auto parent = module.GetParent())
			{
				if (parent->symbol)
				{
					Data::SetUInt64(*record, Docformat::kParentID, UInt64(parent->symbol));
				}
			}

			if (record_ns)
			{
				Data::SetCString(*record, Docformat::kNamespace, MergeNamespace(record_ns));
			}

			Data::SetCString(*record, Docformat::kName, module.name);
			Data::SetBool(*record, kGroup, !module.is_namespace);

			row.WriteArray<UInt8>(data_col, Data::EncodePropertySet(Data::kPropertySetFormat, *record));

			row_index.Set(module.symbol, table->GetNumRow() - 1);
		}

		for (auto & child : module)
		{
			append_module_rows(child);
		}
	};

	for (auto & child : *root)
	{
		append_module_rows(child);
	}

	for (auto & [adr, ref] : info_decoded.Iterate<Data::PropertySet>())
	{
		auto path = Data::GetCStringArray(ref, kPath);

		auto path_symbol = MakeSymbol(CopyUnowned(path));

		if (auto prow_idx = row_index.Search(path_symbol))
		{
			auto row = (*table)[*prow_idx];
			
			bool is_group = Data::GetBool(ref, kGroup);

			if (auto desc = Data::GetWString(ref, kDescription))
			{
				row.WriteArray<UInt8>(desc_col, Data::EncodeUTF8(desc));
			}

			if (auto module_path = Data::GetCStringArray(ref, kModule))
			{
				row.WriteValue<UInt64>(module_col, UInt64(MakeSymbol(CopyUnowned(module_path))));
			}
			else if (is_group)
			{
				row.WriteValue<UInt64>(module_col, UInt64(path_symbol));
			}

			if (is_group)
			{
				if (path.size > 1)
				{
					row.WriteValue<UInt64>(parent_col, UInt64(MakeSymbol(CopyUnowned(Left(path, path.size - 1)))));
				}
				else
				{
					row.WriteValue<UInt64>(parent_col, UInt64(Docformat::Symbol()));
				}
			}

			if (auto pindexed = ref->QueryProperty<Data::BoolProperty>(Docformat::kIndexed))
			{
				row.WriteValue<bool>(indexed_col, pindexed->value);
			}

			if (auto encoded = row.ReadArray<UInt8>(data_col))
			{
				auto record = Data::DecodePropertySet(Data::kPropertySetFormat, encoded);

				if (is_group)
				{
					Data::SetBool(record, kGroup, true);
				}

				if (auto desc = Data::GetWString(ref, kDescription))
				{
					Data::SetWString(record, kDescription, desc);
				}

				row.WriteArray<UInt8>(data_col, Data::EncodePropertySet(Data::kPropertySetFormat, record));
			}
		}
	}

	Data::SortBy(*table, Docformat::kName, true);

	return table;
}

REFLEX_NOINLINE Reflex::Array <Reflex::CString::View> ReflexCLI::Documentation::SplitNamespace(const CString::View & string)
{
	if (string)
	{
		return Split(string, Docformat::kNamespaceDelimiter);
	}

	return {};
}

REFLEX_NOINLINE Reflex::CString ReflexCLI::Documentation::MergeNamespace(const ArrayView <CString::View> & parts)
{
	return Merge(parts, Docformat::kNamespaceDelimiter);
}

REFLEX_NOINLINE Reflex::Array <Reflex::CString::View> ReflexCLI::Documentation::BuildScope(const TableIndex & index, const SymbolInfo & info, bool include_groups)
{
	Array <CString::View> scope_parts;
	Docformat::Symbol ns_anchor;

	// WORKAROUND: rebuild the namespace portion from the module tree because
	// symbol parent links are not yet normalized to consistently point at the
	// containing namespace/module. Long-term, table construction should assign
	// canonical parents for all symbols so this path can rely on parent_symbol
	// alone.

	if (info.ns)
	{
		const bool allows[2] = { include_groups, true };

		ns_anchor = MakeSymbol(SplitNamespace(info.ns));

		if (auto pmodule = index.QueryModule(ns_anchor, nullptr))
		{
			Array <CString::View> module_parts;

			for (auto itr = pmodule; itr && itr->name; itr = itr->GetParent())
			{
				if (allows[itr->is_namespace])
				{
					module_parts.Insert(0, itr->name);
				}
			}

			scope_parts = module_parts;
		}
	}

	if (auto parent = info.parent_symbol)
	{
		auto base_size = scope_parts.GetSize();

		while (auto psymbolinfo = index.QuerySymbolInfo(parent, nullptr))
		{
			if (parent == ns_anchor) break;

			bool allow = true;

			if (psymbolinfo->category_ex >= kCategoryExNamespace)
			{
				if (auto parent_module = index.QueryModule(parent))
				{
					allow = parent_module->is_namespace || include_groups;
				}
			}

			if (allow)
			{
				scope_parts.Insert(base_size, psymbolinfo->name);
			}

			parent = psymbolinfo->parent_symbol;
		}
	}

	return scope_parts;
}

REFLEX_NOINLINE Reflex::Pair <Reflex::CString> ReflexCLI::Documentation::BuildFullSymbolName(const TableIndex & index, const SymbolInfo & info)
{
	CString name = info.name;

	if (info.record)
	{
		if (auto targs = Docformat::GetTemplateArgs(*info.record))
		{
			name.Push(' ');
			name.Push('<');

			for (auto & targ_sym : targs)
			{
				if (auto ptarg_info = index.QuerySymbolInfo(targ_sym, nullptr))
				{
					auto [a, b] = BuildFullSymbolName(index, *ptarg_info);
					bool is_template = b && b.GetLast() == '>';

					if (is_template) name.Push(' ');

					name.Append(Docformat::PrependNamespace(a, b));

					if (is_template) name.Push(' ');
				}
				else
				{
					name.Append(kUndefined);
				}

				name.Push(',');
			}

			name.GetLast() = '>';
		}
	}

	return { MergeNamespace(BuildScope(index, info)), name };
}

Reflex::CString ReflexCLI::Documentation::MakeNamespacedSymbol(const TableIndex & index, const CString::View & current_ns, const SymbolInfo & info)
{
	auto [ns, name] = BuildFullSymbolName(index, info);

	auto out = Docformat::PrependNamespace(ns, name);

	StripCurrentNamespace(current_ns, out);

	return out;
}

REFLEX_NOINLINE void ReflexCLI::Documentation::StripCurrentNamespace(const CString::View & current_ns, CString & io)
{
	if (current_ns)
	{
		auto parts = SplitNamespace(current_ns);

		REFLEX_RLOOP(idx, parts.GetSize())
		{
			CString search = MergeNamespace(Left(parts, idx + 1));
			
			search.Append(Docformat::kNamespaceDelimiter);

			while (auto pos = Search(io, search))
			{
				io.Remove(pos.value, search.GetSize());
			}
		}
	}
}

Reflex::Data::Archive ReflexCLI::Documentation::ReplaceMarkupTokens(CString::View view)
{
	constexpr CString::View kQ = "$q";
	constexpr CString::View kSpc = "$spc";

	Data::Archive t = Data::Pack(view);

	t = Replace(t, ToView(Data::Pack(kQ)), ToView(UInt8(Reflex::kDoubleQuote)));
	t = Replace(t, ToView(Data::Pack(kSpc)), ToView(UInt8(' ')));

	return t;
}

REFLEX_NOINLINE Reflex::Pair <Docformat::Symbol,Reflex::CString> ReflexCLI::Documentation::DecodeLink(CString::View value, CString::View delimiter)
{
	auto properties = Data::DecodePropertySet(Data::kPropertySheetFormat, ReplaceMarkupTokens(value));
	auto path = Data::GetCStringArray(properties, "symbol");
	CString desc = Data::GetCString(properties, "text");

	if (!desc)
	{
		desc = Merge(path, delimiter);
	}

	return { MakeSymbol(CopyUnowned(path)), desc };
}

bool ReflexCLI::Documentation::WriteDescription(ExportFormatWriter & cb, const Data::Archive::View & markup, CString::View delimiter)
{
	REFLEX_LOCAL(CString,RenderInlineMarkup)(ExportFormatWriter & cb, const Data::PropertySet & markup_node, CString::View delimiter)
	{
		auto tag = Data::GetKey32(markup_node, "type");
		auto value = Data::GetCString(markup_node, "value");

		CString rendered;

		if (tag == K32("link"))
		{
			auto [symbol, text] = DecodeLink(value, delimiter);
			auto escaped = cb.Escape(text);

			if (symbol)
			{
				rendered.Append(cb.EncodeLink(escaped, UInt64(symbol)));
			}
			else
			{
				rendered.Append(escaped);
			}
		}
		else
		{
			rendered.Append(cb.Escape(Data::Unpack<CString::View>(ReplaceMarkupTokens(value))));
		}

		for (auto & child : Data::GetXmlNodes(markup_node))
		{
			rendered.Append(Call(cb, child, delimiter));
		}

		return rendered;
	}
	REFLEX_END

	bool written = false;
	auto decoded = Data::DecodePropertySet(Data::kReflexMarkupFormat, markup);

	for (auto & markup_node : Data::GetXmlNodes(decoded))
	{
		auto tag = Data::GetKey32(markup_node, "type");
		auto children = Data::GetXmlNodes(markup_node);

		if (tag == K32("list"))
		{
			cb.ListBegin();

			for (auto & i : children)
			{
				cb.LineBegin(RenderInlineMarkup::Call(cb, i, delimiter));
				cb.LineEnd();
			}

			cb.ListEnd();
		}
		else if (tag == K32("code"))
		{
			cb.CodeBegin("cpp");

			for (auto & i : children)
			{
				cb.LineBegin(cb.Escape(Data::Unpack<CString::View>(ReplaceMarkupTokens(Data::GetCString(i, K32("value"))))));
				cb.LineEnd();
			}

			cb.CodeEnd();
		}
		else if (tag == K32("br"))
		{
			cb.HorizontalLine();
		}
		else
		{
			switch (tag.value)
			{
			case K32("h1"):
				cb.Heading(2, RenderInlineMarkup::Call(cb, markup_node, delimiter));
				break;

			case K32("h2"):
				cb.Heading(3, RenderInlineMarkup::Call(cb, markup_node, delimiter));
				break;

			case K32("h3"):
				cb.Heading(4, RenderInlineMarkup::Call(cb, markup_node, delimiter));
				break;

			default:
				cb.LineBegin(RenderInlineMarkup::Call(cb, markup_node, delimiter));
				cb.LineEnd();
				break;
			}
		}

		written = true;
	}

	return written;
}

Reflex::Pair < Reflex::Array <Reflex::CString::View>, Reflex::UInt > ReflexCLI::Documentation::BuildModulePath(const ModuleNode & node)
{
	Pair < Array <CString::View>, UInt > out;
	auto & path = out.a;
	auto itr = &node;

	while (itr->name)
	{
		path.Insert(0, itr->name);

		if (itr->is_namespace)
		{
			out.b++;
		}

		itr = itr->GetParent();
	}

	return out;
}

Reflex::Array <Reflex::ConstAlreadyRetained <ReflexCLI::Documentation::ModuleNode>> ReflexCLI::Documentation::SortChildModules(const ModuleNode & parent, bool namespace_first)
{
	Array <ConstAlreadyRetained <ModuleNode>> children;

	children.Allocate(parent.GetNumItem());

	for (auto & i : parent)
	{
		children.Push<kAllocateNone>(&i);
	}

	Sort(children, [namespace_first](const ConstAlreadyRetained <ModuleNode> & a, const ConstAlreadyRetained <ModuleNode> & b)
	{
		if (a->is_namespace != b->is_namespace)
		{
			return namespace_first ? (a->is_namespace > b->is_namespace) : (a->is_namespace < b->is_namespace);
		}

		return a->name < b->name;
	});

	return children;
}

Reflex::Unretained <ReflexCLI::Documentation::ExportFormatWriter> ReflexCLI::Documentation::ExportFormatWriter::CreatePlainText(WillRetain <Data::BinaryProperty> output)
{
	return New<TextFormatWriter>(output);
}

ReflexCLI::Documentation::CategoryEx ReflexCLI::Documentation::GetCategoryEx(Key32 category, Key32 subcategory)
{
	if (category == Docformat::kCategories[Docformat::kCategoryType].a)
	{
		if (subcategory == K32("enum"))
		{
			return kCategoryExEnum;
		}
		else if (subcategory == K32("object"))
		{
			return kCategoryExObject;
		}
		else
		{
			return kCategoryExValue;
		}
	}
	else if (category == Docformat::kCategories[Docformat::kCategoryTypedef].a)
	{
		return kCategoryExTypedef;
	}
	else if (category == Docformat::kCategories[Docformat::kCategoryFunction].a)
	{
		return kCategoryExFunction;
	}
	else if (category == Docformat::kCategories[Docformat::kCategoryGlobal].a)
	{
		return kCategoryExGlobal;
	}
	else if (category == Docformat::kCategories[Docformat::kCategoryMethod].a)
	{
		return kCategoryExMethod;
	}
	else if (category == Docformat::kCategories[Docformat::kCategoryMember].a)
	{
		return kCategoryExMember;
	}
	else if (category == K32("module"))
	{
		if (subcategory == K32("namespace"))
		{
			return kCategoryExNamespace;
		}
		else if (subcategory == K32("group"))
		{
			return kCategoryExGroup;
		}
	}

	return kCategoryExUnknown;
}

Reflex::CString::View ReflexCLI::Documentation::GetCategoryLabel(CategoryEx category)
{
	constexpr CString::View kLabels[] =
	{
		"unknown",
		"enum",
		"object",
		"value",
		"typedef",
		"function",
		"global",
		"method",
		"member",
		"namespace",
		"group",
	};

	return kLabels[category];
}

Reflex::CString::View ReflexCLI::Documentation::GetSubCategory(Docformat::Category category, Docformat::TypeFlags flags)
{
	if (category == Docformat::kCategoryType)
	{
		if (flags & Docformat::kTypeFlagEnum)
		{
			return "enum";
		}
		else if (flags & Docformat::kTypeFlagObject)
		{
			return "object";
		}
		else
		{
			return "value";
		}
	}

	return {};
}

ReflexCLI::Documentation::TableIndex::TableIndex()
	: root_module(New<ModuleNode>())
{
	root_module->is_namespace = true;
	module_index.Set({}, root_module.Adr())->is_namespace = true;
	m_null_symbolinfo.ns = kUndefined;
	m_null_symbolinfo.name = kUndefined;
}

ReflexCLI::Documentation::TableIndex::TableIndex(ConstWillRetain <Data::Table> data)
	: table(data)
	, root_module(New<ModuleNode>())
{
	REFLEX_ASSERT(data);
	root_module->is_namespace = true;
	module_index.Set({}, root_module.Adr())->is_namespace = true;
	m_null_symbolinfo.ns = kUndefined;
	m_null_symbolinfo.name = kUndefined;
	symbol_col = Data::AssumeColumn(data, Docformat::kSymbol, Data::Table::kColumnTypeUInt64);
	auto parent_symbol_col = Data::AssumeColumn(data, Docformat::kParentID, Data::Table::kColumnTypeUInt64);
	auto symbol_type_col = Data::AssumeColumn(data, Docformat::kCategory, Data::Table::kColumnTypeKey32);
	auto subcategory_col = Data::AssumeColumn(data, kSubCategory, Data::Table::kColumnTypeKey32);
	name_col = Data::AssumeColumn(data, Docformat::kName, Data::Table::kColumnTypeStringASCII);
	auto module_col = Data::AssumeColumn(data, kModule, Data::Table::kColumnTypeUInt64);
	auto data_col = Data::AssumeColumn(data, kData, Data::Table::kColumnTypeBinary);
	indexed_col = Data::AssumeColumn(data, Docformat::kIndexed, Data::Table::kColumnTypeBool);
	auto index_col = Data::AssumeColumn(data, Docformat::kIndex, Data::Table::kColumnTypeUInt32);
	auto desc_col = Data::AssumeColumn(data, kDescription, Data::Table::kColumnTypeBinary);

	Map <Docformat::Symbol,UInt32> module_rows;

	for (auto row : data)
	{
		auto symbol = Reinterpret<Docformat::Symbol>(row.ReadValue<UInt64>(symbol_col));
		auto category_ex = GetCategoryEx(row.ReadValue<Key32>(symbol_type_col), row.ReadValue<Key32>(subcategory_col));

		REFLEX_ASSERT(category_ex != kCategoryExUnknown);

		auto parent_symbol = Reinterpret<Docformat::Symbol>(row.ReadValue<UInt64>(parent_symbol_col));

		if (!parent_symbol.name.value)
		{
			parent_symbol = {};
		}

		auto & item = symbols[symbol];
		item.symbol = symbol;
		item.category_ex = category_ex;
		item.module = Reinterpret<Docformat::Symbol>(row.ReadValue<UInt64>(module_col));
		item.indexed = row.ReadValue<bool>(indexed_col);
		item.index = UInt16(row.ReadValue<UInt32>(index_col));

		auto encoded = row.ReadArray<UInt8>(data_col);

		item.record = New<Data::PropertySet>(Data::DecodePropertySet(Data::kPropertySetFormat, encoded));
		item.parent_symbol = Docformat::GetParent(*item.record);
		item.ns = Docformat::GetNamespace(*item.record);
		item.name = Docformat::GetName(*item.record);
		item.index = Docformat::GetIndex(*item.record);

		if (auto desc = row.ReadArray<UInt8>(desc_col))
		{
			Data::SetWString(item.record.RemoveConst(), kDescription, Data::DecodeUTF8(desc));
		}

		if (item.category_ex == kCategoryExNamespace || Data::GetBool(*item.record, kGroup))
		{
			module_rows.Set(symbol, UInt32(row.GetIndex()));
		}
	}

	Data::Table::ConstRowCursor rowptr = { table };

	for (auto & i : module_rows)
	{
		AcquireModuleNode(*this, module_rows, rowptr, parent_symbol_col, subcategory_col, name_col, i.key);
	}

	for (auto & i : symbols)
	{
		if (i.value.category_ex == kCategoryExTypedef)
		{
			auto alias = Docformat::GetTypedefTarget(i.value.record);
			auto index = Docformat::GetIndex(i.value.record);

			if (symbols.Search(alias))
			{
				typedefs.Push({ i.value.symbol, alias, index });
			}
		}
	}

	Sort(typedefs, [](const auto & a, const auto & b)
	{
		return a.c < b.c;
	});
}

const ReflexCLI::Documentation::SymbolInfo * ReflexCLI::Documentation::TableIndex::QuerySymbolInfo(Docformat::Symbol symbol, const SymbolInfo * fallback) const
{
	return symbols.Search(symbol, fallback);
}

const ReflexCLI::Documentation::ModuleNode * ReflexCLI::Documentation::TableIndex::QueryModule(Docformat::Symbol symbol, const ModuleNode * fallback) const
{
	ModuleNode * null = RemoveConst(fallback);

	return *module_index.Search(symbol, &null);
}

Docformat::Symbol ReflexCLI::Documentation::TableIndex::ResolveTypedef(Docformat::Symbol symbol, UInt16 usage) const
{
	for (auto & i : ReverseIterate(typedefs))
	{
		if (i.b == symbol && (!usage || i.c <= usage))
		{
			return i.a;
		}
	}

	return symbol;
}

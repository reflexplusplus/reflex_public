#pragma once

//shared documentation index and rendering, also used by ReflexDocumentation

#include "../../common/docformat/include/docformat.h"
#include "reflex_ext/data/table.h"

namespace ReflexCLI::Documentation
{
	using namespace Reflex;


	REFLEX_DECLARE_KEY32(Path);
	REFLEX_DECLARE_KEY32(SubCategory);
	REFLEX_DECLARE_KEY32(Module);
	REFLEX_DECLARE_KEY32(Data);
	REFLEX_DECLARE_KEY32(Group);
	REFLEX_DECLARE_KEY32(Description);

	struct HeadingPart
	{
		CString cls;
		CString text;
		UInt64 target = 0;
		bool is_current = false;
	};

	struct ExportFormatWriter : public Object
	{
		static TRef <ExportFormatWriter> CreatePlainText(TRef <Data::BinaryProperty> output);

		ExportFormatWriter(TRef <Data::BinaryProperty> output)
			: m_output(output)
			, out(output->value)
			, m_block(Block::kText)
		{
		}

		virtual Pair <CString::View> GetDesc() const = 0;
		virtual void Heading(UInt level, CString::View text, UInt64 id = 0) = 0;
		virtual void HeadingPath(UInt level, ArrayView <HeadingPart> parts, UInt64 id = 0) = 0;
		virtual void SectionBegin(CString::View id) = 0;
		virtual void SectionEnd() = 0;
		virtual void LineBegin(CString::View text, CString::View cls = {}) = 0;
		virtual void LineEnd() = 0;
		virtual void HorizontalLine() = 0;
		virtual CString EncodeLink(CString::View text, UInt64 id) = 0;
		virtual void ListBegin(CString::View css_class = {}) = 0;
		virtual void ListEnd() = 0;
		virtual void CodeBegin(CString::View lang) = 0;
		virtual void CodeEnd() = 0;
		virtual CString Escape(CString::View s) = 0;
		Data::Archive::View GetOutput() const { return out; }


	protected:

		enum Block : UInt8 { kText, kList, kCode };

		Reference <Data::BinaryProperty> m_output;

		Data::Archive & out;

		Block m_block = Block::kText;
		UInt m_list_depth = 0;
	};

	constexpr CString::View kUndefined = "{undefined}";

	enum CategoryEx : UInt8
	{
		kCategoryExUnknown,
		kCategoryExEnum,
		kCategoryExObject,
		kCategoryExValue,
		kCategoryExTypedef,
		kCategoryExFunction,
		kCategoryExGlobal,
		kCategoryExMethod,
		kCategoryExMember,

		kCategoryExNamespace,
		kCategoryExGroup,

		kNumCategoryEx,
	};

	struct ModuleNode : public Node <ModuleNode>
	{
		static ModuleNode & null;

		using Node::Attach;

		Docformat::Symbol symbol;
		CString name;
		bool is_namespace = false;
	};

	struct SymbolInfo
	{
		Docformat::Symbol symbol;
		Docformat::Symbol parent_symbol;
		CString::View ns;
		CString::View name;
		ConstReference <Data::PropertySet,kReferenceStrictSafeFlags> record;
		CategoryEx category_ex = kCategoryExUnknown;
		Docformat::Symbol module;
		bool indexed = true;
		UInt16 index = 0;
	};

	struct TableIndex : public Object
	{
		REFLEX_OBJECT(TableIndex, Object);

		TableIndex();

		TableIndex(ConstTRef <Data::Table> table);


		ConstTRef <ModuleNode> GetRootModule() const { return root_module; }

		const ModuleNode * QueryModule(Docformat::Symbol symbol, const ModuleNode * fallback = nullptr) const;

		ConstTRef <ModuleNode> GetModule(Docformat::Symbol symbol) const { return QueryModule(symbol, GetRootModule().Adr()); }


		const SymbolInfo * QuerySymbolInfo(Docformat::Symbol symbol, const SymbolInfo * fallback = nullptr) const;

		ConstTRef <SymbolInfo> GetSymbolInfo(Docformat::Symbol symbol) const { return QuerySymbolInfo(symbol, &m_null_symbolinfo); }

		Docformat::Symbol ResolveTypedef(Docformat::Symbol symbol, UInt16 usage = 0) const;


		ConstReference <Data::Table> table;

		Data::Table::ColumnInfo symbol_col, name_col, /*data_col,*/ indexed_col;

		Map <Docformat::Symbol, SymbolInfo> symbols;
		Map <Docformat::Symbol, ModuleNode*> module_index;
		Reference <ModuleNode> root_module;
		Array <Tuple <Docformat::Symbol, Docformat::Symbol, UInt> > typedefs;

	private:

		SymbolInfo m_null_symbolinfo;
	};

	using TypedefIndex = Function <Docformat::Symbol(Docformat::Symbol type, UInt16 usage)>;

	Array <CString::View> SplitNamespace(const CString::View & string);
	CString MergeNamespace(const ArrayView <CString::View> & parts);

	inline Docformat::Symbol MakeSymbol(const ArrayView <CString::View> & ns_path)
	{
		if (ns_path)
		{
			auto [ns, name] = ReverseSplice<true>(ns_path, 1);

			return { MergeNamespace(ns), name.GetFirst() };
		}

		return {};
	}

	Array <CString::View> BuildScope(const TableIndex & index, const SymbolInfo & info, bool include_groups = false);
	Pair <CString> BuildFullSymbolName(const TableIndex & index, const SymbolInfo & info);
	CString MakeNamespacedSymbol(const TableIndex & index, const CString::View & current_ns, const SymbolInfo & info);
	void StripCurrentNamespace(const CString::View & current_ns, CString & in_out);

	inline Docformat::Symbol ResolveTypeRef(const TypedefIndex & index, Docformat::Symbol type, UInt16 usage_index)
	{
		return index(type, usage_index);
	}

	template <typename TYPE> inline Array <Array <TYPE>> CopyOwned(ArrayView <ArrayView <TYPE>> items);

	template <typename TYPE> inline Array <Array <TYPE>> CopyOwned(const Array <ArrayView <TYPE>> & items);

	template <typename TYPE> inline Array <ArrayView <TYPE>> CopyUnowned(ArrayView < Array <TYPE> > items);

	template <typename TYPE> inline Array <ArrayView <TYPE>> CopyUnowned(const Array <Array <TYPE>> & items);


	inline constexpr CString::View kModuleCategory = "module";
	inline constexpr CString::View kNamespaceSubCategory = "namespace";
	inline constexpr CString::View kGroupSubCategory = "group";

	CategoryEx GetCategoryEx(Key32 category, Key32 subcategory);
	CString::View GetCategoryLabel(CategoryEx category);

	Pair < Array <CString::View>, UInt > BuildModulePath(const ModuleNode & node);
	Array <ConstTRef <ModuleNode>> SortChildModules(const ModuleNode & parent, bool namespace_first);

	TRef <Data::Table> CreateTable(const WString::View & folder, Data::KeyMap & keymap);
	CString::View GetSubCategory(Docformat::Category category, Docformat::TypeFlags flags);
	Data::Table::ConstRowCursor FindSymbol(const Data::Table & table, Docformat::Symbol symbol);
	Data::Archive ReplaceMarkupTokens(CString::View view);
	Pair <Docformat::Symbol, CString> DecodeLink(CString::View value, CString::View delimiter);

	WString GetDescription(const SymbolInfo & info);

	bool WriteDescription(ExportFormatWriter & cb, const Data::Archive::View & markup, CString::View delimiter);
}




//
//

inline Reflex::Data::Table::ConstRowCursor ReflexCLI::Documentation::FindSymbol(const Data::Table & table, Docformat::Symbol symbol)
{
	return Data::FindFirst(table, { Data::Equals(Docformat::kSymbol, Reinterpret<UInt64>(symbol)) }, false);
}

inline Reflex::WString ReflexCLI::Documentation::GetDescription(const SymbolInfo & info)
{
	return Data::GetWString(info.record, kDescription);
}

template <typename TYPE> inline Reflex::Array <Reflex::Array <TYPE>> ReflexCLI::Documentation::CopyOwned(ArrayView <ArrayView <TYPE>> items)
{
	Array < Array <TYPE> > rtn;

	rtn.Allocate(items.size);

	for (auto & i : items) rtn.Push(i);

	return rtn;
}

template <typename TYPE> inline Reflex::Array <Reflex::Array <TYPE>> ReflexCLI::Documentation::CopyOwned(const Array <ArrayView <TYPE>> & items)
{
	return CopyOwned(ToView(items));
}

template <typename TYPE> inline Reflex::Array <Reflex::ArrayView <TYPE>> ReflexCLI::Documentation::CopyUnowned(ArrayView < Array <TYPE> > items)
{
	Array < ArrayView <TYPE> > rtn;

	rtn.Allocate(items.size);

	for (auto & i : items) rtn.Push(i);

	return rtn;
}

template <typename TYPE> inline Reflex::Array <Reflex::ArrayView <TYPE>> ReflexCLI::Documentation::CopyUnowned(const Array <Array <TYPE>> & items)
{
	return CopyUnowned(ToView(items));
}

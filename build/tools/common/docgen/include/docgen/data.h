#pragma once

#include "types.h"




//
//declarations

namespace Docgen
{

	constexpr CString::View kNamespaceDelimiter = "::";

	REFLEX_DECLARE_KEY32(Symbol);
	REFLEX_DECLARE_KEY32(Category);
	REFLEX_DECLARE_KEY32(TypeFlags);
	REFLEX_DECLARE_KEY32(ParentID);
	REFLEX_DECLARE_KEY32(Name);
	REFLEX_DECLARE_KEY32(Namespace);
	REFLEX_DECLARE_KEY32(Index);
	REFLEX_DECLARE_KEY32(Indexed);
	REFLEX_DECLARE_KEY32(BaseID);
	REFLEX_DECLARE_KEY32(IsTemplateDefinition);
	REFLEX_DECLARE_KEY32(TemplateSourceID);
	REFLEX_DECLARE_KEY32(TypeID);
	REFLEX_DECLARE_KEY32(IsConst);
	REFLEX_DECLARE_KEY32(IsRef);
	REFLEX_DECLARE_KEY32(IsPointer);
	REFLEX_DECLARE_KEY32(Overloads);
	REFLEX_DECLARE_KEY32(Return);
	REFLEX_DECLARE_KEY32(Arguments);
	REFLEX_DECLARE_KEY32(IsStatic);
	REFLEX_DECLARE_KEY32(IsVirtual);

	constexpr Pair <Key32, CString::View> kCategories[Item::kNumCategory] =
	{
		{ K32("type"), "type" },
		{ K32("typedef"), "typedef" },

		{ K32("function"), "function" },
		{ K32("global"), "global" },

		{ K32("method"), "method" },
		{ K32("member"), "member" },
	};

	constexpr Key32 GetCategoryKey(Item::Category category) { return kCategories[category].a; }



	//unpack kData

	Field RestoreField(const Data::PropertySet & node);

	decltype(Docgen::FunctionItem::overloads) UnpackFunctionSignatures(const Data::PropertySet & in);


	CString::View GetNamespace(const Data::PropertySet & data);

	CString::View GetName(const Data::PropertySet & data);

	Symbol GetSymbol(const Data::PropertySet & data, Key32 id = kSymbol);

	Item::Category GetCategory(const Data::PropertySet & data);

	TypeItem::Flags GetTypeFlags(const Data::PropertySet & data);

	Symbol GetParent(const Data::PropertySet & data);

	Symbol GetTypeBase(const Data::PropertySet & data);

	Symbol GetTypeTemplateSource(const Data::PropertySet & data);

	Symbol GetTypedefTarget(const Data::PropertySet & data);

	Data::PropertySet GetTypeConstructors(const Data::PropertySet & data);

	ArrayView <Symbol> GetTemplateArgs(const Data::PropertySet & data);

	bool IsTemplateDefinition(const Data::PropertySet & data);

	bool IsIndexed(const Data::PropertySet & data);

	UInt16 GetIndex(const Data::PropertySet & data);

}




//
//impl

inline CString::View Docgen::GetNamespace(const Data::PropertySet & data)
{
	return Data::GetCString(data, kNamespace);
}

inline CString::View Docgen::GetName(const Data::PropertySet & data)
{
	return Data::GetCString(data, kName);
}

inline Docgen::Symbol Docgen::GetSymbol(const Data::PropertySet & data, Key32 id)
{
	Symbol fallback;

	return Reinterpret<Symbol>(Data::GetUInt64(data, id, Reinterpret<UInt64>(fallback)));
}

inline Docgen::Item::Category Docgen::GetCategory(const Data::PropertySet & data)
{
	Key32 category = Data::GetCString(data, kCategory);

	auto idx = Search<KeyCompare>(ToView(kCategories), category);

	return Item::Category(idx ? idx.value : UInt(0));
}

inline Docgen::TypeItem::Flags Docgen::GetTypeFlags(const Data::PropertySet & data)
{
	return TypeItem::Flags(Data::GetUInt32(data, kTypeFlags));
}

inline Docgen::Symbol Docgen::GetParent(const Data::PropertySet & data)
{
	return GetSymbol(data, kParentID);
}

inline Docgen::Symbol Docgen::GetTypeBase(const Data::PropertySet & data)
{
	return GetSymbol(data, kBaseID);
}

inline Docgen::Symbol Docgen::GetTypeTemplateSource(const Data::PropertySet & data)
{
	return GetSymbol(data, kTemplateSourceID);
}

inline Docgen::Symbol Docgen::GetTypedefTarget(const Data::PropertySet & data)
{
	return GetSymbol(data, kTypeID);
}

inline bool Docgen::IsTemplateDefinition(const Data::PropertySet & data)
{
	return Data::GetBool(data, kIsTemplateDefinition);
}

inline UInt16 Docgen::GetIndex(const Data::PropertySet & data)
{
	return UInt16(Data::GetUInt32(data, kIndex));
}

inline bool Docgen::IsIndexed(const Data::PropertySet & data)
{
	return Data::GetBool(data, kIndexed, true);
}

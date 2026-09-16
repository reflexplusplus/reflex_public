#pragma once

#include "format.h"




//
// Docformat: documentation storage format

namespace Docformat
{

	//storage

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
	REFLEX_DECLARE_KEY32(Constructors);
	REFLEX_DECLARE_KEY32(Methods);
	REFLEX_DECLARE_KEY32(Members);
	REFLEX_DECLARE_KEY32(TemplateArgs);


	void RegisterKeys(Data::PropertySet & data);

	void StoreField(Data::PropertySet & node, const Field & field);
	Field RestoreField(const Data::PropertySet & node);

	void PackFunctionSignatures(Data::PropertySet & data, ArrayView <FunctionSignature> signatures);
	Array <FunctionSignature> UnpackFunctionSignatures(const Data::PropertySet & in);


	CString::View GetNamespace(const Data::PropertySet & data);
	void SetNamespace(Data::PropertySet & data, CString::View value);

	CString::View GetName(const Data::PropertySet & data);
	void SetName(Data::PropertySet & data, CString::View value);

	Symbol GetSymbol(const Data::PropertySet & data, Key32 id = kSymbol);
	void SetSymbol(Data::PropertySet & data, Symbol value, Key32 id = kSymbol);

	Category GetCategory(const Data::PropertySet & data);
	void SetCategory(Data::PropertySet & data, Category value);

	TypeFlags GetTypeFlags(const Data::PropertySet & data);
	void SetTypeFlags(Data::PropertySet & data, TypeFlags value);

	Symbol GetParent(const Data::PropertySet & data);
	void SetParent(Data::PropertySet & data, Symbol value);

	Symbol GetTypeBase(const Data::PropertySet & data);
	void SetTypeBase(Data::PropertySet & data, Symbol value);

	Symbol GetTypeTemplateSource(const Data::PropertySet & data);
	void SetTypeTemplateSource(Data::PropertySet & data, Symbol value);

	Symbol GetTypedefTarget(const Data::PropertySet & data);
	void SetTypedefTarget(Data::PropertySet & data, Symbol value);

	Data::PropertySet GetTypeConstructors(const Data::PropertySet & data);
	void SetTypeConstructors(Data::PropertySet & data, WillRetain <Data::PropertySet> value);

	ArrayView <Symbol> GetTemplateArgs(const Data::PropertySet & data);
	void SetTemplateArgs(Data::PropertySet & data, ArrayView <Symbol> value);

	bool IsTemplateDefinition(const Data::PropertySet & data);
	void SetTemplateDefinition(Data::PropertySet & data, bool value);

	bool IsIndexed(const Data::PropertySet & data);
	void SetIndexed(Data::PropertySet & data, bool value);

	UInt16 GetIndex(const Data::PropertySet & data);
	void SetIndex(Data::PropertySet & data, UInt16 value);



	//presentation helpers

	CString PrependNamespace(CString::View ns, CString::View symbol);


	constexpr CString::View kNamespaceDelimiter = "::";

	constexpr Pair <Key32, CString::View> kCategories[kNumCategory] =
	{
		{ K32("type"), "type" },
		{ K32("typedef"), "typedef" },
		{ K32("function"), "function" },
		{ K32("global"), "global" },
		{ K32("method"), "method" },
		{ K32("member"), "member" },
	};

}




//
//impl

inline Reflex::CString::View Docformat::GetNamespace(const Data::PropertySet & data)
{
	return Data::GetCString(data, kNamespace);
}

inline void Docformat::SetNamespace(Data::PropertySet & data, CString::View value)
{
	Data::SetCString(data, kNamespace, value);
}

inline Reflex::CString::View Docformat::GetName(const Data::PropertySet & data)
{
	return Data::GetCString(data, kName);
}

inline void Docformat::SetName(Data::PropertySet & data, CString::View value)
{
	Data::SetCString(data, kName, value);
}

inline Docformat::Symbol Docformat::GetSymbol(const Data::PropertySet & data, Key32 id)
{
	Symbol fallback;

	return Reinterpret<Symbol>(Data::GetUInt64(data, id, UInt64(fallback)));
}

inline void Docformat::SetSymbol(Data::PropertySet & data, Symbol value, Key32 id)
{
	REFLEX_ASSERT(value);

	Data::SetUInt64(data, id, UInt64(value));
}

inline Docformat::Category Docformat::GetCategory(const Data::PropertySet & data)
{
	Key32 category = Data::GetCString(data, kCategory);

	auto idx = Search<KeyCompare>(ToView(kCategories), category);

	return Category(idx ? idx.value : UInt(0));
}

inline void Docformat::SetCategory(Data::PropertySet & data, Category value)
{
	Data::SetCString(data, kCategory, kCategories[value].b);
}

inline Docformat::TypeFlags Docformat::GetTypeFlags(const Data::PropertySet & data)
{
	return TypeFlags(Data::GetUInt32(data, kTypeFlags));
}

inline void Docformat::SetTypeFlags(Data::PropertySet & data, TypeFlags value)
{
	Data::SetUInt32(data, kTypeFlags, UInt32(value));
}

inline Docformat::Symbol Docformat::GetParent(const Data::PropertySet & data)
{
	return GetSymbol(data, kParentID);
}

inline void Docformat::SetParent(Data::PropertySet & data, Symbol value)
{
	SetSymbol(data, value, kParentID);
}

inline Docformat::Symbol Docformat::GetTypeBase(const Data::PropertySet & data)
{
	return GetSymbol(data, kBaseID);
}

inline void Docformat::SetTypeBase(Data::PropertySet & data, Symbol value)
{
	SetSymbol(data, value, kBaseID);
}

inline Docformat::Symbol Docformat::GetTypeTemplateSource(const Data::PropertySet & data)
{
	return GetSymbol(data, kTemplateSourceID);
}

inline void Docformat::SetTypeTemplateSource(Data::PropertySet & data, Symbol value)
{
	SetSymbol(data, value, kTemplateSourceID);
}

inline Docformat::Symbol Docformat::GetTypedefTarget(const Data::PropertySet & data)
{
	return GetSymbol(data, kTypeID);
}

inline void Docformat::SetTypedefTarget(Data::PropertySet & data, Symbol value)
{
	SetSymbol(data, value, kTypeID);
}

inline void Docformat::SetTypeConstructors(Data::PropertySet & data, WillRetain <Data::PropertySet> value)
{
	Data::SetPropertySet(data, kConstructors, value);
}

inline void Docformat::SetTemplateArgs(Data::PropertySet & data, ArrayView <Symbol> value)
{
	Data::SetUInt64Array(data, kTemplateArgs, Reinterpret<ArrayView<UInt64>>(value));
}

inline bool Docformat::IsTemplateDefinition(const Data::PropertySet & data)
{
	return Data::GetBool(data, kIsTemplateDefinition);
}

inline void Docformat::SetTemplateDefinition(Data::PropertySet & data, bool value)
{
	Data::SetBool(data, kIsTemplateDefinition, value);
}

inline Reflex::UInt16 Docformat::GetIndex(const Data::PropertySet & data)
{
	return UInt16(Data::GetUInt32(data, kIndex));
}

inline void Docformat::SetIndex(Data::PropertySet & data, UInt16 value)
{
	Data::SetUInt32(data, kIndex, value);
}

inline bool Docformat::IsIndexed(const Data::PropertySet & data)
{
	return Data::GetBool(data, kIndexed, true);
}

inline void Docformat::SetIndexed(Data::PropertySet & data, bool value)
{
	Data::SetBool(data, kIndexed, value);
}

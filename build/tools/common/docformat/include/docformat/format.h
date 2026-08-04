#pragma once

#include "[require].h"




// 
// Docformat: documentation storage format

namespace Docformat
{
	using namespace Reflex;


	enum Category : UInt8
	{
		kCategoryType,
		kCategoryTypedef,
		kCategoryFunction,
		kCategoryGlobal,
		kCategoryMethod,
		kCategoryMember,
		kNumCategory
	};

	enum TypeFlags : UInt8
	{
		kTypeFlagEnum = MakeBit(0),
		kTypeFlagObject = MakeBit(1),
		kTypeFlagTemplatePlaceholder = MakeBit(2),
	};


	struct Symbol
	{
		Symbol(Key32 ns = kNullKey, Key32 name = kNullKey) : ns(ns), name(name) {}

		explicit operator bool() const { return IsSet(name); }
		explicit operator UInt64() const { return Reinterpret<UInt64>(*this); }

		bool operator<(const Symbol & value) const { return UInt64(*this) < UInt64(value); }
		bool operator==(const Symbol & value) const = default;

		Key32 ns, name;
	};

	template <class TYPE_ID>
	struct AbstractField
	{
		TYPE_ID type;
		bool is_const = false;
		bool is_pointer = false;
		bool is_ref = false;
		bool reserved = false;
		CString::View name;
	};

	using Field = AbstractField <Symbol>;


	struct FunctionSignature
	{
		Field rtn;
		Array <Field> targs;
		Array <Field> arguments;
		bool is_const = false;
		bool is_virtual = false;
	};

}

REFLEX_SET_TRAIT(Docformat::Symbol, IsBoolCastable);

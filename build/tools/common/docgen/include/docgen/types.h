#pragma once

#include "forward.h"




//
//declarations

namespace Docgen
{
	
	struct Symbol;

	
	template <class TYPE_ID> struct AbstractField;

	using Field = AbstractField <Symbol>;

	
	struct Item;

	struct TypeItem;
	
	struct TypedefItem;

	struct GlobalItem;

	struct FunctionItem;

	struct MethodItem;

	struct MemberItem;
}




//
//Symbol

struct Docgen::Symbol	//64 bit key for indexing
{
	Symbol(Key32 ns = kNullKey, Key32 name = kNullKey) : ns(ns), name(name) {}

	explicit operator bool() const { return IsSet(name); }
	explicit operator UInt64() const { return Reinterpret<UInt64>(*this); }

	bool operator<(const Symbol & value) const { return Reinterpret<UInt64>(*this) < Reinterpret<UInt64>(value); }
	bool operator==(const Symbol & value) const = default;

	Key32 ns, name;
};




//
//Field

template <class TYPE_ID>
struct Docgen::AbstractField
{
	TYPE_ID type;
	bool is_const = false;
	bool is_pointer = false;
	bool is_ref = false;
	bool reserved = false;
	CString::View name;
};




//
//Item

struct Docgen::Item : public Object
{
	enum Category
	{
		kCategoryType,	//enum or class
		kCategoryTypedef,

		kCategoryFunction,
		kCategoryGlobal,

		kCategoryMethod,
		kCategoryMember,

		kNumCategory//invalid
	};

	using Object::SetOnHeap;

	Symbol symbol;
	Category category;
	UInt16 index;			//needed for typedef resolution, and preserving order of members
	CString::View ns;
	CString::View name;
};

struct Docgen::TypeItem : public Item
{
	enum Flags : UInt8
	{
		kFlagEnum = MakeBit(0),
		kFlagObject = MakeBit(1),				//reflex specific
		kFlagTemplatePlaceholder = MakeBit(2),	//eg Docgen::TYPE
	};

	const TypeItem * base;

	const TypeItem * source_template;

	Key32 class_ns;

	UInt16 flags;

	UInt8 member_count;

	UInt8 method_count;

	Array <const TypeItem*> targs;	//for templates
};

struct Docgen::TypedefItem : public Item
{
	const TypeItem * source_type;
};

struct Docgen::GlobalItem : public Item
{
	Field variable;
};

struct Docgen::FunctionItem : Item
{
	struct Signature
	{
		UInt64 hash;

		Field rtn;

		Array <Field> targs;		//for templates

		Array <Field> arguments;

		bool is_const = false;		//for methods

		bool is_virtual = false;	//for methods
	};

	Array <Signature> overloads;
};

struct Docgen::MethodItem : FunctionItem
{
	const TypeItem * owner;

	UInt8 index;
};

struct Docgen::MemberItem : GlobalItem
{
	const TypeItem * owner;

	UInt8 index;
};

REFLEX_SET_TRAIT(Docgen::Symbol, IsBoolCastable);

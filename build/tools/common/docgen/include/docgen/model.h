#pragma once

#include "forward.h"


// In-memory model used while generating documentation records.

struct Docgen::Item : public Object
{
	using Object::SetOnHeap;

	Symbol symbol;
	Category category;
	UInt16 index;
	CString::View ns;
	CString::View name;
};

struct Docgen::TypeItem : public Item
{
	const TypeItem * base;
	const TypeItem * source_template;
	Key32 class_ns;
	UInt16 flags;
	UInt8 member_count;
	UInt8 method_count;
	Array <const TypeItem*> targs;
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
	Array <FunctionSignature> overloads;
	Array <UInt64> overload_hashes;
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

#pragma once

#include "program.h"





REFLEX_NS(Reflex::VM::Detail)

using TypeIndex = UInt16;	//for fast linear lookup, the type index within Bindings

enum ContainerType : UInt8
{
	kContainerTypeNull,					//not a container

	kContainerTypeValues,						//no objects, triviallyThreadSafe, non circular
	kContainerTypeThreadSafeObjects,			//objects are threadsafe and non circular
	kContainerTypeNonCircularObjects,			//objects are non circular
	kContainerTypeCircularObjects,			//objects

	kContainerTypeSize
};

REFLEX_END




//
//Type

#define VM_CTR_PARAMS VM::Context & context, VM::TypeRef type

REFLEX_SET_TRAIT(VM::Type, IsSingleThreadExclusive);

struct Reflex::VM::Type : public Reflex::Object
{
	static Object & GetContextNull(VM_CTR_PARAMS);

	enum Flags
	{
		//client defined
		kFlagDefaultConstructable,	//type can be newd if ctr is set and this is true, otherwise ctr just used for creating the null type
		kFlagAssignable,
		kFlagExplicitNullable,

		//the base object is considered circular and non-threadsafe, because we dont who derives from it
		//a derived class can set to non-circular and/or threadsafe, and then subseqent derived classes can not undo this (compile time check for inherit)

		//set automatically
		kFlagNonCircular,
		kFlagThreadsafe,

		kFlagScriptType,

		kFlagFinal,	//for objects, needed for MT copy.  only c++ implementation classes can derive from a final type

		kNumFlag,
	};

	static constexpr UInt8 kDefaultFlags = Bits<false,true,true,false,false,false,true>::value;

	using Ctr = FunctionPointer <Object&(Context&,TypeRef)>;

	using CrossContextCopyFn = FunctionPointer <Object*(Context&,Object&,TypeRef,bool)>;

	using Members = Detail::Variables;


	[[nodiscard]] static TRef <Type> CreateValue();

	[[nodiscard]] static TRef <Type> CreateObject();


	Type(Bindings & bindings, TypeID type_id, Key32 ns, StaticString name, UInt size);

	Type(Bindings & bindings, TypeID type_id, Key32 ns, StaticString name, const Data::Archive::View & null);	//VALUE

	Type(Bindings & bindings, Reflex::Detail::DynamicTypeRef rttypeinfo, Key32 ns, StaticString name, bool noncircular, bool threadsafe);	//OBJECT -> move to sub-class


	bool IsObject() const { return object_t; }


	Symbol symbol;

	TypeID type_id = 0;

	Detail::TypeIndex tidx;

	StaticString name;

	UInt8 size = 0;	

	Flags8 flags = kDefaultFlags;

	Detail::Variables members;


	//object exts

	Reflex::Detail::DynamicTypeRef object_t = 0;

	Ctr ctr = 0;				//in template case recast to TemplateCtr

	Ctr null = &GetContextNull;

	Data::Archive params;		//data for ctr (also use for default value of values?)


	CrossContextCopyFn contextcopyfn[2];	//st an mt.  st works automatically for all non-circular types, mt works automatically for all mt types



protected:

	Type();

};


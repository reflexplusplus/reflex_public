#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::Bootstrap
{

	union Value32;

	class ParameterDefinition;


	extern FunctionPointer <WString(Value32)> kFromContinuous;

}




//
//Value32

union Reflex::Bootstrap::Value32
{
	UInt32 uvalue;
	Int32 ivalue;
	Float32 fvalue;
};

REFLEX_SET_TRAIT(Reflex::Bootstrap::Value32, IsRawCopyable)




//
//ParameterDefinition

class Reflex::Bootstrap::ParameterDefinition : public Object
{
public:

	REFLEX_OBJECT(ParameterDefinition, Object);

	static ParameterDefinition & null;


	enum Type : UInt8
	{
		kTypeContinuous,
		kTypeDiscrete,
		kTypeEnumeration,
		kTypeBoolean,

		kNumType,
	};


	[[nodiscard, deprecated("use Bootstrap::DefineContinuousParameter")]] static Unretained <ParameterDefinition> CreateReal(CString && name, Float32 min, Float32 max, Float32 step, Float32 initial, UInt8 group_flags, decltype(kFromContinuous) to_string = kFromContinuous);

	[[nodiscard, deprecated("use Bootstrap::DefineDiscreteParameter")]] static Unretained <ParameterDefinition> CreateDiscrete(CString && name, Int32 min, Int32 max, Int32 initial, UInt8 group_flags);

	[[nodiscard, deprecated("use Bootstrap::DefineBoolParameter")]] static Unretained <ParameterDefinition> CreateBool(CString && name, bool initial, UInt8 group_flags);

	[[nodiscard, deprecated("use Bootstrap::DefineEnumParameter")]] static Unretained <ParameterDefinition> CreateEnum(CString && name, ArrayView <WString> values, Int32 initial, UInt8 group_flags);
	
	
	virtual Type GetType() const = 0;

	virtual Pair <Value32> GetRange() const = 0;

	virtual Value32 GetStep() const = 0;

	virtual WString ToString(Value32 value) const = 0;

	virtual Value32 FromString(WString::View value) const = 0;

	virtual WString::View GetName() const = 0;

	virtual Value32 GetDefaultValue() const = 0;

	virtual UInt8 GetGroupFlags() const = 0;

	virtual UInt8 GetFlags() const = 0;

};




//
//impl

namespace Reflex::Bootstrap
{
	using ParamDesc = ParameterDefinition;

	[[deprecated("use ParameterDefinition")]] typedef ParameterDefinition ParamInfo;

	inline WString ContinuousToWString(Value32 value) { return Reflex::ToWString(value.fvalue, 1); }
}

REFLEX_NS(Reflex::Bootstrap::Detail)
using ParamDefs = ObjectOf < Array < Pair < Key32, ConstReference <ParameterDefinition> > > >;
REFLEX_END

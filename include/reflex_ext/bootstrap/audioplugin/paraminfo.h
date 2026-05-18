#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::Bootstrap
{

	union Value32;

	struct EnumParamDesc;

	struct ParamDesc;


	Float32 Normalise(const ParamDesc & info, Value32 value);

	Value32 Expand(const ParamDesc & info, Float32 value);

}




//
//Value32

union Reflex::Bootstrap::Value32
{
	Float32 fvalue;
	Int32 ivalue;
};




//
//ParamDesc

struct Reflex::Bootstrap::ParamDesc : public Object
{
	REFLEX_OBJECT(ParamDesc, Object);

	static ParamDesc & null;

	
	enum Type : UInt8
	{
		kTypeReal,
		kTypeDiscrete,
		kTypeEnum,
		kTypeBool
	};

	[[nodiscard]] static TRef <ParamDesc> CreateReal(CString && name, Float32 min, Float32 max, Float32 origin, Float32 init);
	
	[[nodiscard]] static TRef <ParamDesc> CreateDiscrete(CString && name, Int32 min, Int32 max, Int32 init);
	
	[[nodiscard]] static TRef <ParamDesc> CreateBool(CString && name, bool init);

	[[nodiscard]] static TRef <EnumParamDesc> CreateEnum(CString && name, const ArrayView <CString> & values, Int32 init);


	Type type = kTypeReal;

	UInt8 flags = 0;	//use this to denote things like 'enabled'

	CString name;

	Value32 init_value;
	Value32 min;
	Value32 max;
	Value32 origin;
};




//
//EnumParamDesc

struct Reflex::Bootstrap::EnumParamDesc : public ParamDesc
{
	REFLEX_OBJECT(EnumParamDesc, ParamDesc);

	static EnumParamDesc & null;


	Array <CString> values;
};





//
//Legacy

namespace Reflex::Bootstrap
{
	[[deprecated("use ParamDesc")]] typedef ParamDesc ParamInfo;
}
#include "reflex_ext/bootstrap/audioplugin/paraminfo.h"




//
//ParamDesc

Reflex::TRef <Reflex::Bootstrap::ParamDesc> Reflex::Bootstrap::ParamDesc::CreateReal(CString && name, Float32 min, Float32 max, Float32 origin, Float32 init)
{
	auto info = New<ParamDesc>();
	info->type = kTypeReal;
	info->name = std::move(name);
	info->init_value.fvalue = Clip(init, min, max);
	info->min.fvalue = min;
	info->max.fvalue = max;
	info->origin.fvalue = Clip(origin, min, max);
	return info;
}

Reflex::TRef <Reflex::Bootstrap::ParamDesc> Reflex::Bootstrap::ParamDesc::CreateDiscrete(CString && name, Int32 min, Int32 max, Int32 init)
{
	auto info = New<ParamDesc>();
	info->type = kTypeDiscrete;
	info->name = std::move(name);
	info->init_value.ivalue = Clip(init, min, max);
	info->min.ivalue = min;
	info->max.ivalue = max;
	info->origin.ivalue = min;
	return info;
}

Reflex::TRef <Reflex::Bootstrap::EnumParamDesc> Reflex::Bootstrap::ParamDesc::CreateEnum(CString && name, const ArrayView <CString> & values, Int32 init)
{
	if (values)
	{
		auto info = New<EnumParamDesc>();
		info->type = kTypeEnum;
		info->name = std::move(name);
		info->min.ivalue = 0;
		info->max.ivalue = values.size - 1;
		info->init_value.ivalue = Clip<Int32>(0, info->max.ivalue, init);
		info->values = values;
		return info;
	}
	else
	{
		return Null<EnumParamDesc>();
	}
}

Reflex::TRef <Reflex::Bootstrap::ParamDesc> Reflex::Bootstrap::ParamDesc::CreateBool(CString && name, bool init)
{
	auto info = New<ParamDesc>();
	info->type = kTypeBool;
	info->name = std::move(name);
	info->min.ivalue = 0;
	info->max.ivalue = 1;
	info->init_value.ivalue = init;
	return info;
}

Reflex::Float32 Reflex::Bootstrap::Normalise(const ParamDesc & info, Value32 value)
{
	switch (info.type)
	{
	case ParamDesc::kTypeReal:
		return Reflex::Normalise(value.fvalue, info.min.fvalue, info.max.fvalue);

	default:
		return Reflex::Normalise(Float32(value.ivalue), Float32(info.min.ivalue), Float32(info.max.ivalue));
	}
}

Reflex::Bootstrap::Value32 Reflex::Bootstrap::Expand(const ParamDesc & info, Float32 normal)
{
	Value32 rtn;

	switch (info.type)
	{
	case ParamDesc::kTypeReal:
		rtn.fvalue = LinearInterpolate(normal, info.min.fvalue, info.max.fvalue);
		break;
	default:
		rtn.ivalue = ToInt32(LinearInterpolate(normal, Float32(info.min.ivalue), Float32(info.max.ivalue)));
		break;
	}

	return rtn;
}

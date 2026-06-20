#include "reflex_ext/bootstrap/audioplugin/paraminfo.h"




//
//ParamDesc

Reflex::TRef <Reflex::Bootstrap::ParamDesc> Reflex::Bootstrap::ParamDesc::CreateReal(CString && name, Float32 min, Float32 max, Float32 origin, Float32 initial)
{
	auto info = New<ParamDesc>();
	info->type = kTypeReal;
	info->name = std::move(name);
	info->to_string = [min, max](Value32 value)
	{
		return Join(ToWString(Reflex::Normalise(value.fvalue, min, max) * 100.0f, 0, false), L'%');
	};
	info->init_value.fvalue = Clip(initial, min, max);
	info->min.fvalue = min;
	info->max.fvalue = max;
	info->origin.fvalue = Clip(origin, min, max);
	return info;
}

Reflex::TRef <Reflex::Bootstrap::ParamDesc> Reflex::Bootstrap::ParamDesc::CreateDiscrete(CString && name, Int32 min, Int32 max, Int32 initial)
{
	auto info = New<ParamDesc>();
	info->type = kTypeDiscrete;
	info->name = std::move(name);
	info->to_string = [](Value32 value) 
	{
		return ToWString(value.ivalue); 
	};
	info->init_value.ivalue = Clip(initial, min, max);
	info->min.ivalue = min;
	info->max.ivalue = max;
	info->origin.ivalue = min;
	return info;
}

Reflex::TRef <Reflex::Bootstrap::ParamDesc> Reflex::Bootstrap::ParamDesc::CreateEnum(CString && name, const ArrayView <CString> & values, Int32 initial)
{
	REFLEX_ASSERT(values);

	if (values)
	{
		auto info = New<ParamDesc>();
		info->type = kTypeEnum;
		info->name = std::move(name);
		info->to_string = [labels = Join(values)](Value32 value)
		{
			return ToWString(labels[Clip<Int32>(value.ivalue, 0, labels.GetSize() - 1)]);
		};
		info->min.ivalue = 0;
		info->max.ivalue = values.size - 1;
		info->init_value.ivalue = Clip<Int32>(0, info->max.ivalue, initial);
		return info;
	}
	else
	{
		return Null<ParamDesc>();
	}
}

Reflex::TRef <Reflex::Bootstrap::ParamDesc> Reflex::Bootstrap::ParamDesc::CreateBool(CString && name, bool initial)
{
	auto info = New<ParamDesc>();
	info->type = kTypeBool;
	info->name = std::move(name);
	info->to_string = [](Value32 value) -> WString
	{
		constexpr WString::View kBooleanLabels[] = { L"Off", L"On" };

		return kBooleanLabels[True(value.ivalue)];
	};
	info->min.ivalue = 0;
	info->max.ivalue = 1;
	info->init_value.ivalue = initial;
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

#include "functions.h"




REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

Value32 MakeFloatValue(Float32 value)
{
	return { .fvalue = value };
}

Value32 MakeIntValue(Int32 value)
{
	return { .ivalue = value };
}

struct StandardParameterDefinition : public ParameterDefinition
{
	StandardParameterDefinition(Type type, Value32 min, Value32 max, Value32 step, WString && name, Value32 default_value, UInt8 group_flags, UInt8 flags)
		: m_type(type)
		, m_minmax({ min, max })
		, m_step(step)
		, m_name(std::move(name))
		, m_default_value(default_value)
		, m_group_flags(group_flags)
		, m_flags(flags)
	{
	}

	Type GetType() const override { return m_type; }
	Pair <Value32> GetRange() const override { return m_minmax; }
	Value32 GetStep() const override { return m_step; }
	WString::View GetName() const override { return m_name; }
	Value32 GetDefaultValue() const override { return m_default_value; }
	UInt8 GetGroupFlags() const override { return m_group_flags; }
	UInt8 GetFlags() const override { return m_flags; }

	Type m_type;
	Pair <Value32> m_minmax;
	Value32 m_step;
	WString m_name;
	Value32 m_default_value;
	UInt8 m_group_flags;
	UInt8 m_flags;
};

struct ContinuousParameterDefinition final : public StandardParameterDefinition
{
	ContinuousParameterDefinition(WString && name, Float32 min, Float32 max, Float32 step, Float32 initial, UInt8 group_flags, FunctionPointer<WString(Value32)> to_string)
		: StandardParameterDefinition(kTypeContinuous, MakeFloatValue(min), MakeFloatValue(max), MakeFloatValue(Max(step, 0.00001f)), std::move(name), MakeFloatValue(Clip(initial, min, max)), group_flags, 0)
		, m_to_string(to_string)
	{
	}

	WString ToString(Value32 value) const override
	{
		return m_to_string(value);
	}

	Value32 FromString(WString::View value) const override
	{
		return MakeFloatValue(ToFloat32(value));
	}

	FunctionPointer<WString(Value32)> m_to_string;
};

struct DiscreteParameterDefinition final : public StandardParameterDefinition
{
	DiscreteParameterDefinition(WString && name, Int32 min, Int32 max, Int32 initial, UInt8 group_flags)
		: StandardParameterDefinition(kTypeDiscrete, MakeIntValue(min), MakeIntValue(max), MakeIntValue(1), std::move(name), MakeIntValue(Clip(initial, min, max)), group_flags, 0)
	{
	}

	WString ToString(Value32 value) const override { return ToWString(value.ivalue); }

	Value32 FromString(WString::View value) const override { return MakeIntValue(ToInt32(value)); }
};

struct EnumParameterDefinition final : public StandardParameterDefinition
{
	EnumParameterDefinition(WString && name, ArrayView <WString> values, Int32 initial, UInt8 group_flags)
		: StandardParameterDefinition(kTypeEnumeration, MakeIntValue(0), MakeIntValue(values.size - 1), MakeIntValue(1), std::move(name), MakeIntValue(Clip<Int32>(0, values.size - 1, initial)), group_flags, 0)
	{
		m_labels.Allocate(values.size);

		for (auto & value : values) m_labels.Push<kAllocateNone>(std::move(value));
	}

	WString ToString(Value32 value) const override
	{
		return m_labels[Clip<Int32>(value.ivalue, 0, m_labels.GetSize() - 1)];
	}

	Value32 FromString(WString::View value) const override
	{
		if (auto idx = Search<CaseInsensitive>(m_labels, value))
		{
			return MakeIntValue(idx.value);
		}
		else
		{
			return MakeIntValue(ToInt32(value));
		}
	}

	Array <WString> m_labels;
};

struct BoolParameterDefinition final : public StandardParameterDefinition
{
	static constexpr WString::View kBooleanLabels[] = { L"Off", L"On" };

	BoolParameterDefinition(WString && name, bool initial, UInt8 group_flags)
		: StandardParameterDefinition(kTypeBoolean, MakeIntValue(0), MakeIntValue(1), MakeIntValue(1), std::move(name), MakeIntValue(initial), group_flags, 0)
	{
	}

	WString ToString(Value32 value) const override
	{
		return kBooleanLabels[True(value.ivalue)];
	}

	Value32 FromString(WString::View value) const override
	{
		return MakeIntValue(CaseInsensitive::eq(value, L"On"));
	}
};

struct NullParameterDefinition final : public StandardParameterDefinition
{
	NullParameterDefinition()
		: StandardParameterDefinition(kTypeContinuous, MakeFloatValue(0.0f), MakeFloatValue(1.0f), MakeFloatValue(0.0f), {}, MakeFloatValue(0.0f), 0, 0)
	{
	}

	WString ToString(Value32 value) const override { return ToWString(value.fvalue, 3); }

	Value32 FromString(WString::View value) const override { return MakeFloatValue(ToFloat32(value)); }
};

Reflex::Detail::Module::Member <NullParameterDefinition> g_null_parameter_definition(module);

REFLEX_END_INTERNAL

Reflex::Bootstrap::ParameterDefinition & Reflex::Bootstrap::ParameterDefinition::null = Reflex::Bootstrap::g_null_parameter_definition;

Reflex::TRef <Reflex::Bootstrap::ParameterDefinition> Reflex::Bootstrap::DefineContinuousParameter(WString && name, Float32 min, Float32 max, Float32 step, Float32 initial, UInt8 group_flags, FunctionPointer<WString(Value32)> to_string)
{
	return New<ContinuousParameterDefinition>(std::move(name), min, max, step, initial, group_flags, to_string);
}

Reflex::TRef <Reflex::Bootstrap::ParameterDefinition> Reflex::Bootstrap::DefineDiscreteParameter(WString && name, Int32 min, Int32 max, Int32 initial, UInt8 group_flags)
{
	return New<DiscreteParameterDefinition>(std::move(name), min, max, initial, group_flags);
}

Reflex::TRef <Reflex::Bootstrap::ParameterDefinition> Reflex::Bootstrap::DefineEnumParameter(WString && name, ArrayView <WString> values, Int32 initial, UInt8 group_flags)
{
	REFLEX_ASSERT(values);

	if (values)
	{
		return New<EnumParameterDefinition>(std::move(name), values, initial, group_flags);
	}
	else
	{
		return Null<ParameterDefinition>();
	}
}

Reflex::TRef <Reflex::Bootstrap::ParameterDefinition> Reflex::Bootstrap::DefineBoolParameter(WString && name, bool initial, UInt8 group_flags)
{
	return New<BoolParameterDefinition>(std::move(name), initial, group_flags);
}

Reflex::TRef <Reflex::Bootstrap::ParameterDefinition> Reflex::Bootstrap::ParameterDefinition::CreateReal(CString && name, Float32 min, Float32 max, Float32 step, Float32 initial, UInt8 group_flags, FunctionPointer<WString(Value32)> to_string)
{
	return DefineContinuousParameter(ToWString(name), min, max, step, initial, group_flags, to_string);
}

Reflex::TRef <Reflex::Bootstrap::ParameterDefinition> Reflex::Bootstrap::ParameterDefinition::CreateDiscrete(CString && name, Int32 min, Int32 max, Int32 initial, UInt8 group_flags)
{
	return DefineDiscreteParameter(ToWString(name), min, max, initial, group_flags);
}

Reflex::TRef <Reflex::Bootstrap::ParameterDefinition> Reflex::Bootstrap::ParameterDefinition::CreateBool(CString && name, bool initial, UInt8 group_flags)
{
	return DefineBoolParameter(ToWString(name), initial, group_flags);
}

Reflex::TRef <Reflex::Bootstrap::ParameterDefinition> Reflex::Bootstrap::ParameterDefinition::CreateEnum(CString && name, ArrayView <WString> values, Int32 initial, UInt8 group_flags)
{
	return DefineEnumParameter(ToWString(name), values, initial, group_flags);
}

Reflex::Float32 Reflex::Bootstrap::Normalise(const ParameterDefinition & definition, Value32 value)
{
	auto [min, max] = definition.GetRange();

	if (definition.GetType() == ParameterDefinition::kTypeContinuous)
	{
		return Reflex::Normalise(value.fvalue, min.fvalue, max.fvalue);
	}

	return Reflex::Normalise(Float32(value.ivalue), Float32(min.ivalue), Float32(max.ivalue));
}

Reflex::Bootstrap::Value32 Reflex::Bootstrap::Expand(const ParameterDefinition & definition, Float32 normal)
{
	auto [min, max] = definition.GetRange();

	if (definition.GetType() == ParameterDefinition::kTypeContinuous)
	{
		return MakeFloatValue(LinearInterpolate(normal, min.fvalue, max.fvalue));
	}

	return MakeIntValue(ToInt32(LinearInterpolate(normal, Float32(min.ivalue), Float32(max.ivalue))));
}

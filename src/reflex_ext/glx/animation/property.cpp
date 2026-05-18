#include "property.h"




//
//

Reflex::GLX::InterpolatedAnimationImpl::InterpolateFn Reflex::GLX::AbstractPropertyAnimation::GetInterpolateFn(UInt n)
{
	switch (n)
	{
	case 1:
		return reinterpret_cast<InterpolateFn>(&OnInterpolate<1>);
	case 2:
		return reinterpret_cast<InterpolateFn>(&OnInterpolate<2>);
	case 4:
		return reinterpret_cast<InterpolateFn>(&OnInterpolate<4>);
	default:
		return 0;
	}
}

Reflex::GLX::AbstractPropertyAnimation::AbstractPropertyAnimation(const TypeHandler & type, Key32 property_id)
	: InterpolatedAnimationImpl(GetInterpolateFn(type.n))
	, type(type)
	, property_id(property_id)
{
}

void Reflex::GLX::AbstractPropertyAnimation::OnSetTarget(GLX::Object & target)
{
	InterpolatedAnimationImpl::OnSetTarget(target);

	Address adr = { property_id, type.type_id };

	auto prop = target.QueryProperty(adr);

	if (prop)
	{
		m_prop = prop;
	}
	else
	{
		m_prop = type.create().Adr();

		auto pcurrent = GetValueAdr(type, m_prop);

		auto pfrom = m_from_to;

		REFLEX_LOOP_PTR(pcurrent, ptr, type.n)
		{
			*ptr = *pfrom++;
		}

		target.SetProperty(adr, m_prop);
	}
}

void Reflex::GLX::AbstractPropertyAnimation::OnFlip()
{
	auto pto = m_from_to + type.n;
		
	REFLEX_LOOP_PTR(m_from_to, pfrom, type.n)
	{
		Swap(*pfrom, *pto++);
	}
}

Reflex::Float32 Reflex::GLX::AbstractPropertyAnimation::OnRecomputeProgress(GLX::Object & target)
{
	auto n = type.n;

	const auto pcurrent = GetValueAdr(type, m_prop);

	const auto pfrom = m_from_to;

	const auto pto = pfrom + n;

	Float32 num = 0.0f;
	
	Float32 den = 0.0f;

	for (UInt i = 0; i < n; ++i)
	{
		const Float32 d = pto[i] - pfrom[i];
		const Float32 vc = pcurrent[i] - pfrom[i];
		num += vc * d;
		den += d * d;
	}
	
	if (den <= 0.0f) return 1.0f;
	
	return Clip(num / den, 0.0f, 1.0f);
}

template <Reflex::UInt N> void Reflex::GLX::AbstractPropertyAnimation::OnInterpolate(AbstractPropertyAnimation & self, GLX::Object & object, Float x)
{
	auto & type = self.type;

	auto pcurrent = GetValueAdr(type, self.m_prop);
	
	auto pfrom = self.m_from_to;
	
	auto pto = pfrom + type.n;

	if constexpr (N == 4)
	{
		SIMD::FloatV4 from = SIMD::LoadUnaligned(pfrom);
		SIMD::FloatV4 to = SIMD::LoadUnaligned(pto);

		auto current = SIMD::LinearInterpolate(x, from, to);
		
		auto [a,b,c,d] = current.Read();

		pcurrent[0] = a;
		pcurrent[1] = b;
		pcurrent[2] = c;
		pcurrent[3] = d;
	}
	else if constexpr (N == 2)
	{ 
		pcurrent[0] = LinearInterpolate(x, pfrom[0], pto[0]);
		pcurrent[1] = LinearInterpolate(x, pfrom[1], pto[1]);
	}
	else if constexpr (N == 1)
	{
		*pcurrent = LinearInterpolate(x, *pfrom, *pto);
	}

	object.Realign();
}

const Reflex::GLX::AbstractPropertyAnimation::TypeHandler Reflex::GLX::AbstractPropertyAnimation::kTypeHandlers[] =
{
	{
		REFLEX_TYPEID(Data::Float32Property),
		[]() -> TRef <Reflex::Object>
		{
			return REFLEX_CREATE(Data::Float32Property);
		},
		UInt8(REFLEX_OFFSETOF(Data::Float32Property,value)),
		1
	},
	{
		REFLEX_TYPEID(SizeProperty),
		[]() -> TRef <Reflex::Object>
		{
			return REFLEX_CREATE(SizeProperty);
		},
		UInt8(REFLEX_OFFSETOF(SizeProperty,value)),
		2
	},
	{
		REFLEX_TYPEID(PointProperty),
		[]() -> TRef <Reflex::Object>
		{
			return REFLEX_CREATE(PointProperty);
		},
		UInt8(REFLEX_OFFSETOF(PointProperty,value)),
		2
	},
	{
		REFLEX_TYPEID(ColourProperty),
		[]() -> TRef <Reflex::Object>
		{
			return REFLEX_CREATE(ColourProperty);
		},
		UInt8(REFLEX_OFFSETOF(ColourProperty,value)),
		4
	},
	{
		REFLEX_TYPEID(MarginProperty),
		[]() -> TRef <Reflex::Object>
		{
			return REFLEX_CREATE(MarginProperty);
		},
		UInt8(REFLEX_OFFSETOF(MarginProperty,value)),
		4
	}
};

Reflex::TRef <Reflex::GLX::InterpolatedAnimation> Reflex::GLX::Detail::CreatePropertyAnimation(Address property_adr, const ArrayView <Float32> & from, const ArrayView <Float32> & to)
{
	struct PropertyAnimationImpl : public AbstractPropertyAnimation
	{
		PropertyAnimationImpl(const TypeHandler & type, Key32 property_id)
			: AbstractPropertyAnimation(type, property_id)
		{
		}

		using AbstractPropertyAnimation::m_from_to;
	};

	if (!MemCompare(from.data, to.data, from.size * sizeof(Float32)))
	{
		auto type_id = property_adr.type_id;

		for (auto & i : AbstractPropertyAnimation::kTypeHandlers)
		{
			if (i.type_id == type_id)
			{
				REFLEX_ASSERT(from.size == i.n);
				REFLEX_ASSERT(to.size == i.n);

				auto t = Reflex::Detail::Constructor<PropertyAnimationImpl>::CreateVariableSize(g_default_allocator, i.n * sizeof(Float64), i, property_adr.id);

				auto dst = t->m_from_to;

				for (auto & i : from) *dst++ = i;

				for (auto & i : to) *dst++ = i;

				return t;
			}
		}
	}

	return InterpolatedAnimation::null;
}

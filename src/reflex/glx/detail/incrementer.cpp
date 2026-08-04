#include "reflex/glx/detail/incrementer.h"




REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

struct IncrementerImpl : public Incrementer
{
	static constexpr Float64 kSmall = 1.0 / kMaxUInt32;

	IncrementerImpl()
		: m_min(0.0f),
		m_max(1.0f),
		m_step(0.0f),
		m_sensitivity(128.0f),
		m_direction(-1.0f),
		m_init(0),
		m_drag_z(0.0f),
		m_accum(0.0f)
	{
	}

	virtual void SetRange(Float min, Float max, Float step, Float pixelrange, bool invert) override
	{
		m_min = min;

		m_max = max;

		m_step = step;

		m_direction = invert ? 1.0f : -1.0f;

		m_sensitivity = pixelrange;
	}

	virtual void Begin(Float32 init) override
	{
		m_init = init;

		m_drag_z = 0.0f;

		m_accum = 0.0;

		auto range = m_max - m_min;
		
		m_mult_step[0] = { (1.0 / (m_sensitivity / range) * m_direction), Reflex::Max<Float64>(m_step, kSmall) };

		m_mult_step[1] = { (1.0 / ((m_sensitivity * 30.0f) / range)) * m_direction, kSmall };
	}

	virtual Float32 Process(Float drag, bool fine) override
	{
		auto & mult_step = m_mult_step[fine];

		Float64 inc = (m_drag_z - drag) * mult_step.a;

		m_drag_z = drag;

		m_accum = Quantise(m_accum + inc, mult_step.b);

		return Clip(Float32(m_init + m_accum), m_min, m_max);
	}


	Float m_min, m_max, m_step;

	Float m_sensitivity;

	Float m_direction;


	Pair <Float64> m_mult_step[2];
	
	Float m_init;

	Float m_drag_z;

	Float64 m_accum;

};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::GLX::Detail::Incrementer> Reflex::GLX::Detail::Incrementer::Create()
{
	return REFLEX_CREATE(IncrementerImpl);
}

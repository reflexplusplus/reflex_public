#include "colour_filter.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

Matrix4x4 MultiplyMatrix(const Matrix4x4 & a, const Matrix4x4 & b)
{
	Matrix4x4 c = kNoValue;

	const auto a0 = a.data[0];
	const auto a1 = a.data[1];
	const auto a2 = a.data[2];
	const auto a3 = a.data[3];

	REFLEX_LOOP(col, 4)
	{
		const auto bc = b.data[col];

		const auto b0 = SIMD::Shuffle<0, 0, 0, 0>(bc, bc);
		const auto b1 = SIMD::Shuffle<1, 1, 1, 1>(bc, bc);
		const auto b2 = SIMD::Shuffle<2, 2, 2, 2>(bc, bc);
		const auto b3 = SIMD::Shuffle<3, 3, 3, 3>(bc, bc);

		SIMD::FloatV4 r = a0 * b0 + a1 * b1 + a2 * b2 + a3 * b3;

		c.data[col] = r;
	}

	return c;
}

REFLEX_INLINE void SetRowMajor(Matrix4x4 & matrix, const ArrayView <Float> & rm)
{
	REFLEX_ASSERT(rm.size == 16);

	Float32 cm[16];

	cm[0] = rm[0];
	cm[1] = rm[4];
	cm[2] = rm[8];
	cm[3] = rm[12];

	cm[4] = rm[1];
	cm[5] = rm[5];
	cm[6] = rm[9];
	cm[7] = rm[13];

	cm[8] = rm[2];
	cm[9] = rm[6];
	cm[10] = rm[10];
	cm[11] = rm[14];

	cm[12] = rm[3];
	cm[13] = rm[7];
	cm[14] = rm[11];
	cm[15] = rm[15];

	matrix.Set({ cm, 16 });
}

REFLEX_INLINE void SetColumnMajor(Matrix4x4 & matrix, const ArrayView <Float> & cm)
{
	REFLEX_ASSERT(cm.size == 16);

	matrix.Set(cm);
}

REFLEX_END_INTERNAL

void Reflex::GLX::Detail::ColourFilterImpl::Init(GenericPropertiesSchema& schema)
{
	RegisterStandardWrapper(schema);

	switch (schema.uid.value)
	{
	case K32("Tint"):
	case K32("Exposure"):
		RegisterColourProperty(schema, REFLEX_OFFSETOF(Properties, values), "rgb");
		break;

	//case K32("ColorTransform"):
	//	RegisterFloatArrayProperty(schema, REFLEX_OFFSETOF(Properties, values), "matrix");
	//	break;

	default:
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, values), "amount");
		break;
	}

	SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
	{
		auto properties = Cast<Properties>(pproperties);

		PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive, properties->layers);

		vtable.OnBind = [](const GenericLayer & layer, GenericLayer::ObjectState & data, GLX::Object & object)
		{
			auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

			AllocateLayers(object, properties->layers, scratch->content_state);
		};

		vtable.OnAccommodate = &StandardWrapperAccommodate;

		vtable.OnAlign = &ColourFilterImpl::OnAlign;

		vtable.OnRedraw = &ColourFilterImpl::OnRedraw;
	});
}

void Reflex::GLX::Detail::ColourFilterImpl::OnAlign(const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float& contenth)
{
	constexpr auto SetSaturationMatrix = [](Matrix4x4 & matrix, Float s)
	{
		constexpr Float r = 0.2126f;
		constexpr Float g = 0.7152f;
		constexpr Float b = 0.0722f;

		SetColumnMajor(matrix,
		{
			r + (1 - r) * s, r - r * s, r - r * s, 0,
			g - g * s, g + (1 - g) * s, g - g * s, 0,
			b - b * s, b - b * s, b + (1 - b) * s, 0,
			0, 0, 0, 1,
		});
	};

	//constexpr auto GetAmount = [](const char * name, const Colour & params, Float fallback)
	//{
	//	return params.r;
	//};

	//constexpr auto GetRGB = [](const char * name, const Colour & params)
	//{
	//	switch (params.GetSize())
	//	{
	//	case 1:
	//	{
	//		auto first = params[0];
	//		return MakeTuple(first, first, first);
	//	}

	//	case 3:
	//		return MakeTuple(params[0], params[1], params[2]);

	//	default:
	//		Error(output, name, "rgb requires 1 or 3 values");
	//		return MakeTuple(1.0f, 1.0f, 1.0f);
	//	}
	//};

	auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

	auto & values = properties->values;

	switch (layer.schema->uid.value)
	{
		case K32("Brightness"):
		{
			auto a = values.r;	//GetAmount("Brightness", values, 0.0f);

			SetRowMajor(scratch->colour_matrix,
			{
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				a, a, a, 1,
			});
			break;
		}

		case K32("Contrast"):
		{
			auto a = values.r;	//GetAmount("Contrast", values, 1.0f);

			auto o = 0.5f * (1.0f - a);

			SetRowMajor(scratch->colour_matrix,
			{
				a, 0, 0, 0,
				0, a, 0, 0,
				0, 0, a, 0,
				o, o, o, 1
			});
			break;
		}

		case K32("Saturate"):
			SetSaturationMatrix(scratch->colour_matrix, values.r);	//GetAmount("Saturate", values, 1.0f)
			break;

		case K32("Tint"):
		{
			auto [r, g, b, a] = values;

			SetColumnMajor(scratch->colour_matrix,
			{
				r, 0, 0, 0,
				0, g, 0, 0,
				0, 0, b, 0,
				0, 0, 0, 1,
			});
			break;
		}

		case K32("Grayscale"):
		case K32("Greyscale"):
			SetSaturationMatrix(scratch->colour_matrix, 1.0f - values.r);	//GetAmount("Grayscale", values, 1.0f)
			break;

		case K32("Invert"):
		{
			auto a = values.r;	//GetAmount("Invert", values, 1.0f);

			auto d = 1.0f - 2.0f * a;

			SetColumnMajor(scratch->colour_matrix,
			{
				d, 0, 0, 0,
				0, d, 0, 0,
				0, 0, d, 0,
				a, a, a, 1
			});
			break;
		}

		case K32("Sepia"):
		{
			auto a = values.r;	//GetAmount("Sepia", values, 1.0f);

			constexpr Triad <Float32> r = { 0.393f, 0.769f, 0.189f };
			constexpr Triad <Float32> g = { 0.349f, 0.686f, 0.168f };
			constexpr Triad <Float32> b = { 0.272f, 0.534f, 0.131f };

			SetRowMajor(scratch->colour_matrix,
			{
				1.0f + (r.a - 1.0f) * a, r.b * a, r.c * a, 0.0f,
				g.a * a, 1.0f + (g.b - 1.0f)*a, g.c * a, 0.0f,
				b.a * a, b.b * a, 1.0f + (b.c - 1.0f)*a, 0.0f,
				0.0f, 0.0f, 0.0f,1.0f
			});
			break;
		}

		case K32("Exposure"):
		{
			auto [r, g, b, a] = values;

			SetColumnMajor(scratch->colour_matrix,
			{
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				r, g, b, 1,
			});
			break;
		}

		//case K32("ColorTransform"):
		//case K32("ColourTransform"):
		//	if (values.GetSize() == 16)
		//	{
		//		SetRowMajor(scratch->colour_matrix, values);
		//	}
		//	else
		//	{
		//		Error(output, "ColorTransform requires 16 values");
		//	}
		//	break;

		default:
			scratch->colour_matrix.Identity();
			break;
	}

	AlignLayers(scratch->content_state, size, contenth);
};

Reflex::GLX::Detail::ColourFilterImpl::Renderer::Renderer(Scratch& scratch, Size pixelsize)
	: StandardLayersRenderer(scratch.content_state, pixelsize)
	, colour_matrix(scratch.colour_matrix),
	m_stack_entry(++st_uid_counter),
	m_previous_parent_uid_z(0)
{
}

void Reflex::GLX::Detail::ColourFilterImpl::Renderer::Render(const System::Renderer::Transform& transform, const System::Colour& colour) const
{
	auto & previous_stack_entry = *st_current;

	if (SetFiltered(m_previous_parent_uid_z, previous_stack_entry.uid))
	{
		//previous parent changed, recompute..

		m_stack_entry.matrix = MultiplyMatrix(previous_stack_entry.matrix, colour_matrix);
	}

	Core::g_renderer->SetColourTransform({ m_stack_entry.matrix.data->GetData(), 16 });

	st_current = &m_stack_entry;

	StandardLayersRenderer::Render(transform, colour);

	st_current = &previous_stack_entry;

	Core::g_renderer->SetColourTransform({ previous_stack_entry.matrix.data->GetData(), 16 });
}

Reflex::TRef <Reflex::System::Renderer::Graphic> Reflex::GLX::Detail::ColourFilterImpl::OnRedraw(const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags)
{
	auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

	return New<Renderer>(scratch, pixelsize);
}

const Reflex::GLX::Detail::Layer::Class Reflex::GLX::Detail::g_colour_fx_layers[]
{
	{ "Saturate", &GenericLayer::CreateSchema<ColourFilterImpl>, &GenericLayer::Create<ColourFilterImpl> },
	{ "Grayscale", &GenericLayer::CreateSchema<ColourFilterImpl>, &GenericLayer::Create<ColourFilterImpl> },
	{ "Greyscale", &GenericLayer::CreateSchema<ColourFilterImpl>, &GenericLayer::Create<ColourFilterImpl> },
	{ "Tint", &GenericLayer::CreateSchema<ColourFilterImpl>, &GenericLayer::Create<ColourFilterImpl> },
	{ "Exposure", &GenericLayer::CreateSchema<ColourFilterImpl>, &GenericLayer::Create<ColourFilterImpl> },
	{ "Invert", &GenericLayer::CreateSchema<ColourFilterImpl>, &GenericLayer::Create<ColourFilterImpl> },
	{ "Brightness", &GenericLayer::CreateSchema<ColourFilterImpl>, &GenericLayer::Create<ColourFilterImpl> },
	{ "Contrast", &GenericLayer::CreateSchema<ColourFilterImpl>, &GenericLayer::Create<ColourFilterImpl> },
	{ "Sepia", &GenericLayer::CreateSchema<ColourFilterImpl>, &GenericLayer::Create<ColourFilterImpl> },
	//{ "ColorTransform", &GenericLayer::CreateSchema<ColourFilterImpl>, &GenericLayer::Create<ColourFilterImpl> },
};

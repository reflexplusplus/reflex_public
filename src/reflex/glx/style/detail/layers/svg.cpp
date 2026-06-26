#include "types.h"
#include "reflex/glx/detail/svg.h"




//
//svg

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

struct SvgWrapperImpl
{
	struct Properties : public StandardPropertiesWithColour
	{
		ConstReference <Data::PropertySet> xml;
	};

	struct Scratch : public StandardScratch
	{
		Size size;

		Reference <Graphic> graphic;
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterStandardPropertiesWithColour(schema);

		auto pdefs = schema.RegisterProperties(1);

		pdefs[0] = { UInt16(REFLEX_OFFSETOF(Properties, xml)), 0, kBindStageBuildLayout, "path", MakeAddress<Data::PropertySet>(K32("xml")), 0, [](const Reflex::Object & object, void * adr, UInt64 data)
		{
			auto & xml_ref = *Cast<ConstReference <Data::PropertySet>>(adr);
			
			xml_ref = Cast<Data::PropertySet>(object);
		} };

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & height)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				if (SetFiltered(scratch->size, size))
				{
					scratch->graphic = CreateGraphic(DecodeSVG(properties->xml, {}, size));
				}
			};

			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				return scratch->graphic;
			};
		});
	}
};

REFLEX_END_INTERNAL

const Reflex::GLX::Detail::Layer::Class Reflex::GLX::Detail::g_svg_wrapper = { "{svg}", &GenericLayer::CreateSchema<SvgWrapperImpl>, &GenericLayer::Create<SvgWrapperImpl> };

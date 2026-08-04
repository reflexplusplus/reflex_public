#include "types.h"
#include "reflex/glx/style/canvas.h"
#include "reflex/glx/detail/functions/misc.h"
#include "../../../vm.h"



//
//canvas

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

struct CanvasImpl
{
	enum Mode : UInt8
	{
		kModeMonochrome,
		kModeColour,
		kModeGraphic,
		kNumMode
	};

	using UnderlyingCallback = Function <void(CanvasProperties & ctx)>;

	struct CallbackProperty : public AbstractProperty
	{
		CallbackProperty(const Function<void(CanvasContext&)> & ondraw)
			: CallbackProperty(kModeMonochrome, *Reinterpret<UnderlyingCallback>(&ondraw))
		{
		}

		CallbackProperty(const Function<void(ColourCanvasContext&)> & ondraw)
			: CallbackProperty(kModeColour, *Reinterpret<UnderlyingCallback>(&ondraw))
		{
		}

		CallbackProperty(const Function<void(GraphicCanvasContext&)> & ondraw)
			: CallbackProperty(kModeGraphic, *Reinterpret<UnderlyingCallback>(&ondraw))
		{
		}

		REFLEX_NOINLINE CallbackProperty(Mode mode, const Function<void(CanvasProperties&)> & ondraw)
			: mode(mode)
			, callback(ondraw)
		{
		}

		Mode mode;
		UnderlyingCallback callback;
	};

	struct Properties : public StandardPropertiesWithColour
	{
		Key32 id;
	};

	struct Scratch : public StandardScratch
	{
		Mode mode = kNumMode;
		UnderlyingCallback callback;
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);

		RegisterKey32Property(schema, REFLEX_OFFSETOF(Properties, id), "id", kPropertyGroupCustom0, kBindStageBuildLayout);
		
		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive;

			auto scratch = Cast<Scratch>(pscratch);

			switch (self.schema->uid.value)
			{
			case K32("Canvas"):
				scratch->mode = kModeMonochrome;
				break;

			case K32("ColourCanvas"):
			case K32("ColorCanvas"):
				scratch->mode = kModeColour;
				break;

			case K32("GraphicCanvas"):
				scratch->mode = kModeGraphic;
				break;
			}

			vtable.OnAlign = BindStandardAlign(self);

			vtable.OnBind = [](const GenericLayer & layer, GenericLayer::ObjectState & data, GLX::Object & object)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				CString::View callback_type = "{missing callback}";

				if (auto ptr = object.QueryProperty<CallbackProperty>(properties->id))
				{
					if (scratch->mode == ptr->mode)
					{
						scratch->callback = ptr->callback;

						return;
					}
					else
					{
						callback_type = g_canvas_layers[ptr->mode].id;
					}
				}

				scratch->callback.Clear();

				output.LogEx(kLogError, {}, Layer::Class::Query(layer.schema->uid)->id, "(id: ", GLX::GetKey(properties->id), ") mismatch ", callback_type);
			};

			switch (scratch->mode)
			{
			case kModeMonochrome:
				if (self.propertyflags & kPropertyGroupIndent)
				{
					vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixel_size, UInt8 flags) -> TRef <Graphic>
					{
						auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

						Points points;

						CanvasContext ctx = { scratch->inner.size, pixel_size, points };

						scratch->callback(ctx);

						return New<GraphicRendererWithOffsetAndColour>(CreateGraphic(points), scratch->inner.origin, properties->colour);
					};
				}
				else
				{
					RemoveConst(self.flags) |= Layer::kOptimisationFlagVector;

					vtable.OnVectoriseMonochrome = [](const GenericLayer & layer, const GenericLayer::ObjectState & data, Size pixel_size, Colour & colour, Points & points)
					{
						auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

						CanvasContext ctx = { scratch->inner.size, pixel_size, points };

						scratch->callback(ctx);

						colour = properties->colour;
					};
				}
				break;

			case kModeColour:
				if (self.propertyflags & (kPropertyGroupIndent|kPropertyGroupColour))
				{
					vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixel_size, UInt8 flags) -> TRef <Graphic>
					{
						auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

						ColourPoints points;

						ColourCanvasContext ctx = { scratch->inner.size, pixel_size, points };

						scratch->callback(ctx);

						return New<GraphicRendererWithOffsetAndColour>(CreateGraphic(points), scratch->inner.origin, properties->colour);
					};
				}
				else
				{
					RemoveConst(self.flags) |= Layer::kOptimisationFlagVector;

					vtable.OnVectoriseColour = [](const GenericLayer & layer, const GenericLayer::ObjectState & data, Size pixel_size, ColourPoints & points)
					{
						auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

						ColourCanvasContext ctx = { scratch->inner.size, pixel_size, points };

						scratch->callback(ctx);
					};
				}
				break;

			case kModeGraphic:
				if (self.propertyflags & (kPropertyGroupIndent | kPropertyGroupColour))
				{
					vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixel_size, UInt8 flags) -> TRef <Graphic>
					{
						auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

						GraphicCanvasContext ctx = { scratch->inner.size, pixel_size };

						scratch->callback(ctx);

						return New<GraphicRendererWithOffsetAndColour>(ctx.output, scratch->inner.origin, properties->colour);
					};
					break;
				}
				else
				{
					vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixel_size, UInt8 flags)
					{
						auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

						GraphicCanvasContext ctx = { scratch->inner.size, pixel_size };

						scratch->callback(ctx);

						return ctx.output;
					};
					break;
				}
			}

		});
	}
};

constexpr auto CreateCanvasSchema = &GenericLayer::CreateSchema<CanvasImpl>;
constexpr auto CreateCanvas = &GenericLayer::Create<CanvasImpl>;

REFLEX_END_INTERNAL

const Reflex::GLX::Detail::Layer::Class Reflex::GLX::Detail::g_canvas_layers[]
{
	{ "Canvas", CreateCanvasSchema, CreateCanvas },
	{ "ColourCanvas", CreateCanvasSchema, CreateCanvas },
	{ "ColorCanvas", CreateCanvasSchema, CreateCanvas },
	{ "GraphicCanvas", CreateCanvasSchema, CreateCanvas },
};

void Reflex::GLX::SetCanvas(Object & object, Key32 id, const Function<void(CanvasContext&)> & ondraw)
{
	object.SetProperty(id, REFLEX_CREATE(Detail::CanvasImpl::CallbackProperty, ondraw));

	object.RebuildLayout();
}

void Reflex::GLX::SetColourCanvas(Object & object, Key32 id, const Function<void(ColourCanvasContext&)> & ondraw)
{
	object.SetProperty(id, REFLEX_CREATE(Detail::CanvasImpl::CallbackProperty, ondraw));

	object.RebuildLayout();
}

void Reflex::GLX::SetGraphicCanvas(Object & object, Key32 id, const Function<void(GraphicCanvasContext&)> & ondraw)
{
	object.SetProperty(id, REFLEX_CREATE(Detail::CanvasImpl::CallbackProperty, ondraw));

	object.RebuildLayout();
}

void Reflex::GLX::UnsetCanvas(Object & object, Key32 id)
{
	object.UnsetProperty<Detail::CanvasImpl::CallbackProperty>(id);

	object.RebuildLayout();
}

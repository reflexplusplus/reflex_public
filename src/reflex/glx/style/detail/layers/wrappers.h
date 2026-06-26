#pragma once

#include "layerproperties.h"
#include "../../../detail/transform_scope.h"




//
//helpers for wrappers

REFLEX_NS(Reflex::GLX::Detail)

inline UInt8 PropogateOptimisationFlags(GenericLayer & self, UInt8 flags, const ArrayOfLayer & layers)
{
	for (auto & i : layers) flags &= i->flags;

	RemoveConst(self.flags) = flags;

	return flags;
}

struct StandardWrapperProperties
{
	const ArrayOfLayer layers;
};

struct StandardWrapperScratch
{
	REFLEX_INLINE void Build(const ArrayOfLayer & layers, GLX::Object & object)
	{
		layerflags = AllocateLayers(object, layers, content_state);
	}

	REFLEX_INLINE void Accommodate(GLX::Object & object, Size & contentsize)
	{
		AccommodateLayers(content_state, contentsize);

		flowflags = object.GetLayoutFlags();
	}

	REFLEX_INLINE void Align(Size size, Float & contenth)
	{
		AlignLayers(content_state, size, contenth);
	}

	Array <LayerWithState> content_state;
	UInt8 layerflags;
	UInt8 flowflags;
};

struct StandardWrapperWithIndentProperties : public StandardWrapperProperties
{
	Margin indent;
};

struct StandardWrapperWithIndentScratch : public StandardWrapperScratch
{
	Rect inner;
};

inline void RegisterStandardWrapper(GenericPropertiesSchema & schema)
{
	RegisterLayersProperty(schema, REFLEX_OFFSETOF(StandardWrapperProperties,layers), "content");
}

inline void RegisterStandardWrapperWithIndent(GenericPropertiesSchema & schema)
{
	RegisterStandardWrapper(schema);

	RegisterMarginProperty(schema, REFLEX_OFFSETOF(StandardWrapperWithIndentProperties,indent), "indent", kPropertyGroupIndent, kBindStageAccommodate);
}

inline void StandardWrapperBind(const GenericLayer & layer, GenericLayer::ObjectState & data, GLX::Object & object)
{
	auto properties = data.GetProperties<StandardWrapperProperties>();

	auto scratch = data.GetScratch<StandardWrapperScratch>(layer);

	scratch->Build(properties->layers, object);
}

inline void StandardWrapperAccommodate(const GenericLayer & layer, GenericLayer::ObjectState & data, Size & contentsize)
{
	auto scratch = data.GetScratch<StandardWrapperScratch>(layer);

	scratch->Accommodate(data.owner, contentsize);
}

inline void StandardWrapperAlign(const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
{
	auto scratch = data.GetScratch<StandardWrapperScratch>(layer);

	scratch->Align(size, contenth);
}

struct StandardLayersRenderer : public System::Renderer::Graphic
{
	StandardLayersRenderer(Array <LayerWithState> & layers, Size pixelsize) 
		: layers(layers)
	{
		RedrawLayers(layers, pixelsize);
	}

	virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
	{
		DrawLayers(*Core::RenderContext::st_current, colour, layers);
	}

	Array <LayerWithState> & layers;
};

struct StandardLayersRendererWithOffset : public StandardLayersRenderer
{
	StandardLayersRendererWithOffset(Array <LayerWithState> & layers, Size pixelsize, Point offset)
		: StandardLayersRenderer(layers, pixelsize),
		offset(offset)
	{
	}

	virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
	{
		auto & ctx = *Core::RenderContext::st_current;

		TransformScope scope = { ctx, offset };

		DrawLayers(ctx, colour, layers);
	}

	const Point offset;
};

struct StandardLayersRendererWithOffsetAndColour : public StandardLayersRendererWithOffset
{
	StandardLayersRendererWithOffsetAndColour(Array <LayerWithState> & state, Size pixelsize, Point offset, const Colour & colour)
		: StandardLayersRendererWithOffset(state, pixelsize, offset),
		colour(colour)
	{
	}

	virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
	{
		DrawLayers(*Core::RenderContext::st_current, colour * StandardLayersRendererWithOffsetAndColour::colour, layers);
	}

	const Colour colour;
};

inline void FindAndSetLayer(ArrayOfLayer & list, const Data::Key32Property & object)
{
	auto id = object.value;

	if (auto p = FindResource(st_current_style, MakeAddress<ArrayOfLayerDesc>(id), 0))
	{
		Detail::CreateLayers(st_current_style, *Cast<ArrayOfLayerDesc>(p), list);
	}
	else if (auto p = FindResource(st_current_style, MakeAddress<LayerDesc>(id), 0))
	{
		auto precompiled = Cast<LayerDesc>(p);

		auto layer = Detail::Layer::Create(precompiled->cls, st_current_style, *precompiled);

		list.Push(layer);
	}
}

REFLEX_END
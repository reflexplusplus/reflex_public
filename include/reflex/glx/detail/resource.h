#pragma once

#include "layer.h"




//
//Detail

namespace Reflex::GLX::Detail
{

	struct ResourceDesc;

	struct LayerDesc;

	using ArrayOfLayerDesc = Data::ObjectArray <LayerDesc>;

}




//
//ResourceDesc

struct Reflex::GLX::Detail::ResourceDesc : public Reflex::Data::PropertySet
{
	ResourceDesc(Key32 type) : type(type) {}

	const Key32 type;
};

REFLEX_SET_TRAIT(Reflex::GLX::Detail::LayerDesc, IsSingleThreadExclusive);




//
//LayerDesc

struct Reflex::GLX::Detail::LayerDesc : public ResourceDesc
{
	REFLEX_OBJECT(LayerDesc, ResourceDesc);

	LayerDesc(const Layer::Class & cls) : ResourceDesc("Layer"), cls(cls) {}

#if (REFLEX_DEBUG)
	void OnSetProperty(Address adr, Object & object) override;
#endif

	const ConstTRef <Layer::Class> cls;
};

REFLEX_SET_TRAIT(Reflex::GLX::Detail::ResourceDesc, IsSingleThreadExclusive);

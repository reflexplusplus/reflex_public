#pragma once

#include "../[require].h"




//
//Internal

REFLEX_NS(Reflex::System::Common)

struct RendererFactory : public Reflex::Detail::StaticItem <RendererFactory>
{
	RendererFactory(CString::View name, FunctionPointer <TRef<Renderer>(const Renderer::Config & config)> ctr) : name(name), ctr(ctr) {}

	const CString::View name;
	
	FunctionPointer <TRef<Renderer>(const Renderer::Config & config)> ctr;
};

UInt GetValue(const Renderer::Config & config, Key32 key, UInt32 fallback);

bool VerifyBitmap(const BitmapInfo & info, const ArrayView <UInt8> & data);

REFLEX_END


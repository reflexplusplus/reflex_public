#pragma once

#include "../globals.h"
#include "../../common/graphics/functions.h"




//
//Internal

REFLEX_NS(Reflex::System::OSX)

template <class TYPE> inline TRef <Renderer> CreateRenderer(const Renderer::Config & config)
{
	bool hd = Common::GetValue(config, Renderer::kHD, true);
	bool tx = Common::GetValue(config, Renderer::kTX, true);

	g_enableretina = hd && (globals->m_pixeldensity > 1);

	return REFLEX_CREATE(TYPE, g_enableretina, false, tx);
}

REFLEX_END

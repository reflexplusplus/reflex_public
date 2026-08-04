
#include "glx/core.cpp"

#include "glx/library.cpp"

REFLEX_DISABLE_WARNINGS
#include "glx/freetype.c"
REFLEX_ENABLE_WARNINGS

#include "glx/defines.cpp"

#include "glx/standard_layout.cpp"
#include "glx/object.cpp"
#include "glx/window.cpp"
#include "glx/event.cpp"
#include "glx/style.cpp"

#include "glx/animation/animation.cpp"
#include "glx/animation/scope.cpp"
#include "glx/animation/container.cpp"
#include "glx/animation/interpolated.cpp"

#include "glx/functions.cpp"
#include "glx/functions/dialogs.cpp"
#include "glx/functions/mods.cpp"
#include "glx/functions/lookup.cpp"
#include "glx/functions/clock.cpp"
#include "glx/functions/drawing.cpp"
#include "glx/functions/drawing_path.cpp"
#include "glx/functions/drawing_triangle.cpp"
#include "glx/functions/properties.cpp"

#include "glx/data.cpp"

#include "glx/behaviours/textedit.cpp"
#include "glx/behaviours/resizer.cpp"
#include "glx/behaviours/gestures.cpp"

#include "glx/widgets/viewport.cpp"
#include "glx/widgets/rangebar.cpp"

#include "glx/detail/event_log.cpp"
#include "glx/detail/functions.cpp"
#include "glx/detail/incrementer.cpp"
#include "glx/detail/snapshot.cpp"
#include "glx/detail/vector.cpp"
#include "glx/detail/svg.cpp"

Reflex::UIntNative Reflex::GLX::Detail::WakeLayers()
{
	const ArrayView <Reflex::GLX::Detail::Layer::Class> groups[] =
	{
		ToView(g_vector_layers),
		ToView(g_gradient_layers),
		ToView(g_image_layers),
		ToView(g_text_layers),
		ToView(g_wrapper_layers),
		ToView(g_texture_fx_layers),
		ToView(g_colour_fx_layers),
		ToView(g_special_layers),
		ToView(g_viewport_layers),
	};

	UIntNative a = 0;

	for (auto & group : groups)
	{
		a |= ToUIntNative(&group);
	}

	return a;
}



//TODO CSTYLE/LAYERS
//unify Renderer and Layer, CStyle will just hold one root Layer (1 x Render layer, 1 x Mask Layer, 1 x Children layer)
//unify Layer adn PreCompiledLayer, just needs a Compile call when used by CStyle
//Text: txt y pos when line_height > 1.0x needs to be adjusted so text is vertically positioned on the line according to justify y property
//ZSort: this needs refactor so that setting zsort calls RebuildLayout, so that it can be known by parent when creating its renderer, not one frame later as now.  
// this can then optimise Renderer impls, with the sorting being pre-calculated in on accommodate


//TODO INPUT MODEL
//OnSetLayout return OnAccomodate (or Accommodator)
//OnAccomodate return OnAlign
//OnAlign return Renderer

//TODO DropExternal should be Event, but will break freestyle etc

//TODO Menu: Open add orientation param, and remove from Popup");
//TODO accommodate filtering semantic needed");

//TODO("GLX", "Generic Show function");
//TODO("GLX", "Unify show/hide data with EnterExit Data");

//1) renderer/bitmap add CreateTextures
//define texture stretch mode
//extend shader with blur, invert

//TODO ("GLX", "tidy and extract JPG class");
//TODO "GLX", "global engine &, member of library, get rid of desktop, give context default param");

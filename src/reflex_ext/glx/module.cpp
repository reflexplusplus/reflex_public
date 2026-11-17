#include "module.h"
#include "../../../include/reflex_ext/glx/detail/functions.h"
#include "../../../include/reflex_ext/glx/animation/functions.h"
#include "widgets/menu.h"




REFLEX_NS(Reflex::GLX)
extern FunctionPointer <Unretained <Animation>(Float, Float, const Function <void(Object&, Float)> &, Float)> g_create_logarithmic_animation;
extern FunctionPointer <Unretained <Animation>(const Function <void(Object &, Float)> &)> g_create_interpolated_animation;
REFLEX_END

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct Globals
{
	template <class TYPE> using NullImpl = Reflex::Detail::StaticObject <TYPE>;

	Globals()
	{
		g_create_logarithmic_animation = &CreateLogarithmicAnimation;
		g_create_interpolated_animation = [](const Function <void(Object &, Float)> & callback) -> Unretained <Animation>
		{
			return CreateInterpolatedAnimation(callback);
		};

		Detail::g_set_content_impl = Detail::g_render_to_texture ? &Detail::SetContentAnimated : &Detail::SetContentNotAnimated;

		RegisterKey("MenuOpen");
		RegisterKey("MenuSelect");	//item: Object;
		RegisterKey("RequestClose");

		RegisterKey("ListSelect");
		RegisterKey("ListOpen");
		RegisterKey("ListStartDrag");
		RegisterKey("ListRequestRemove");
		RegisterKey("ListRequestRemove");
		RegisterKey("ListReorder");

		RegisterKey("SelectPanel");
	}

	NullImpl <MenuImpl> null_menu;
};

Reflex::Detail::Module::Member <Globals> g_glx_ext_globals(g_glx_ext_module);

REFLEX_END_INTERNAL

Reflex::Detail::Module Reflex::GLX::g_glx_ext_module("GLX ext", Reflex::GLX::module);

Reflex::GLX::Menu & Reflex::GLX::Menu::null = g_glx_ext_globals->null_menu;

decltype(&Reflex::GLX::Detail::SetContentNotAnimated) Reflex::GLX::Detail::g_set_content_impl = nullptr;

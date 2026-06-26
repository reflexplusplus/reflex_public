#pragma once

#include "core/impl.h"

#include "reflex/glx/library.h"
#include "reflex/glx/warnings.h"
#include "reflex/glx/window.h"
#include "reflex/glx/text.h"
#include "reflex/glx/detail/image_set.h"

#include "widgets/viewport/coordinates.h"

#include "style/detail/cstyle.h"
#include "style/stylesheet.h"

#include "behaviours/texteditimpl.h"
#include "animation/player.h"

#include "object.h"




//
//declarations

REFLEX_NS(Reflex::GLX)

class Library;

typedef The <Library> TheLibrary;

extern TRef <Library> g_library;

extern UInt8 kCreateRendererFlags;


extern TypeID g_array_cstring_property_typeid, g_cstring_property_typeid, g_key32_property_typeid;

extern TypeID g_resource_desc_typeid;

extern TypeID g_font_typeid, g_imageset_typeid;

extern TypeID g_layer_desc_typeid, g_array_of_layer_desc_typeid;


extern const Data::DecompressionAlgorithm * gLZO;

REFLEX_END




//
//library

class Reflex::GLX::Library : public Core::DesktopImpl
{
public:

	template <class TYPE> using NullImpl = Reflex::Detail::StaticObject <TYPE>;

	struct NullFont : public Detail::Font
	{
		Pair <Float32> GetHeightAndTail() const override { return {}; }
		Float32 GetTextWidth(const WString::View & text) const override { return 0.0f; }
		Pair < TRef <System::Renderer::Graphic>, Float32 > CreateText(const WString::View & text, Point position, Size scale) const override { return {}; }
	};

	struct NullRenderer : public Core::Renderer
	{
		REFLEX_OBJECT(NullRenderer, Core::Renderer);

		NullRenderer() : Renderer(false) {}

		void Render(const System::Renderer::Transform & transform, const Colour & colour) const override {}
	};

	struct NullImageSet : public Detail::ImageSet
	{
		NullImageSet()
			: Detail::ImageSet(System::Renderer::Canvas::null)
		{
			AddFrame(kNullKey, { {}, kNormal });
		}
	};

	struct NullCStyle : public Detail::ComputedStyle
	{
		using ComputedStyle::ComputedStyle;

		TRef <Core::Renderer> CreateRenderer(GLX::Object & object) const override { return GLX::Core::Renderer::null; }
		TRef <ComputedStyle> Mutate(const ComputedStyle & b) const override { return ComputedStyle::null; }
	};

	struct NullAnimation : public InterpolatedAnimation
	{
		using InterpolatedAnimation::InterpolatedAnimation;

		void OnSetEasing(Easing) override {}
		void OnFlip() override {}
		void OnBegin() override {}
		bool OnClock(Float delta) override { return false; }
		void OnSkip() override {}
	};

	struct NullContainerAnimation : public ContainerAnimation
	{
		bool OnClock(Float delta) override { return false; }
		void OnSkip() override {}
	};



	//lifetime

	Library(File::ResourcePool & resourcepool, const System::Renderer::Config & config);

	~Library();


	Reference <Reflex::Allocator> m_allocator;

	Reference <Reflex::Object> m_onchangedisplays, m_onclock;

	File::ResourcePool::Monitor m_resourcepool_monitor;

	AnimationScope m_animationscope;

	const UInt m_pixeldensity;

	const Float m_fdpifactor;

	const Float m_pixelsize;

	const Pair <bool,Float32> m_system_theme;

	const bool m_mobile;

	bool m_canrendertotexture;

	bool m_supportsimageformat[System::kNumImageFormat];

	UInt8 m_onclock_filter;


	Map < Address, Pair <decltype(&Detail::ComputedStyleImpl::NullPropertySetter), UInt16 > > m_cstylesetters;

	Detail::Layer::Factory m_layerfactory;



	//nulls

	NullImpl <NullRenderer> null_renderer;

	NullImpl <NullImageSet> null_image_set;

	NullImpl <NullCStyle> null_cstyle;

	NullImpl <NullFont> null_font;

	NullImpl <NullViewPortCoordinates> null_viewport_coordinates;

	NullImpl <Text> null_text;


	Reference <Reflex::Object> null_layerstate;

	NullImpl <States> null_states;

	NullImpl <GLX::Object::Mods> null_mods;

	ConstReference <Detail::ComputedStyle> null_cstyle_ref;

	
	//null objects

	Reflex::Detail::Initialiser < NullImpl <StyleSheet> > null_stylesheet;

	Reflex::Detail::Initialiser < NullImpl <NullObject> > null_object;

	struct WindowClient : public GLX::WindowClient {};

	Reflex::Detail::Initialiser < NullImpl <WindowClient> > null_window;


	Reflex::Detail::Initialiser < NullImpl <TextEditBehaviourWithState> > null_delegate;

	NullImpl <GLX::Event> null_event;


	NullImpl <NullContainerAnimation> null_container_animation;

	NullImpl <NullAnimation> null_animation;

	Reflex::Detail::Initialiser < NullImpl <AbstractViewBar> > null_viewbar;

	Reflex::Detail::Initialiser < NullImpl <ScrollArea> > null_scroller;

	Reflex::Detail::Initialiser < NullImpl <ZoomArea> > null_zoomable;

	Detail::ImageSet::Frame null_frame;


	//propertysheets

	Data::KeyMap keymap;

	ResourceParser iresource;

	StyleSheetParser istylesheet;

	const Data::Format & kStyleSheetFormat;


	//activity

	Array <Pair <GLX::Object*, Core::WeakReference>> m_mouseover, m_mouseover_z;

	GlobalPlayer m_globalplayer;


	//cache

	struct Cache
	{
		Array <Point> point_workspaces[4];

		Array <System::ColourPoint> colourpoint_workspaces[4];

		Array <Int> int_workspace;

		Reference <Object> vectorcompiler;

		WString tempstring, maskstring;

		Array <WString> lines;

		Array <Detail::Font::FaceDesc> faces;

		Map < UInt64, Reference <Detail::Font> > fonts;
	} 
	
	cache;


	Reflex::Object * m_restart_clock;

};
